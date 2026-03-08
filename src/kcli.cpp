#include <kcli.hpp>

#include "kcli/backend.hpp"
#include "kcli/util.hpp"

#include <algorithm>
#include <deque>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kcli {
namespace {

struct SessionState {
    backend::ParseState parse{};
    FailureMode failure_mode = FailureMode::Return;
    PositionalHandler positional_handler{};
    std::deque<std::string> owned_tokens{};
    bool initialized = false;
};

thread_local SessionState g_session{};

ProcessResult MakeError(std::string_view option, std::string_view message) {
    ProcessResult result{};
    result.ok = false;
    result.error_option = std::string(option);
    result.error_message = std::string(message);
    return result;
}

SessionState& RequireSession() {
    if (!g_session.initialized || g_session.parse.argc_ptr == nullptr) {
        throw std::logic_error("kcli session is not initialized; call kcli::Initialize(argc, argv) first");
    }
    return g_session;
}

std::string NormalizeRootOptionOrThrow(std::string_view raw_root) {
    std::string root = detail::TrimWhitespace(raw_root);
    if (root.empty()) {
        return {};
    }

    if (detail::StartsWith(root, "--")) {
        root.erase(0, 2);
    } else if (root.front() == '-') {
        throw std::invalid_argument("kcli root must use '--root' or 'root'");
    }

    return detail::NormalizeRootNameOrThrow(root);
}

std::string NormalizeHandlerOptionOrThrow(std::string_view raw_option,
                                          bool inline_mode,
                                          std::string_view root) {
    std::string option = detail::TrimWhitespace(raw_option);
    if (option.empty()) {
        throw std::invalid_argument("kcli handler option must not be empty");
    }

    if (inline_mode) {
        if (detail::StartsWith(option, "--")) {
            const std::string full_prefix = "--" + std::string(root) + "-";
            if (!detail::StartsWith(option, full_prefix)) {
                throw std::invalid_argument(
                    "kcli inline handler option must use '-name' or '" + full_prefix + "name'");
            }
            option.erase(0, full_prefix.size());
        } else if (option.front() == '-') {
            option.erase(0, 1);
        }
    } else {
        if (detail::StartsWith(option, "--")) {
            option.erase(0, 2);
        } else if (option.front() == '-') {
            throw std::invalid_argument("kcli end-user handler option must use '--name' or 'name'");
        }
    }

    return detail::NormalizeCommandOrThrow(option);
}

std::string NormalizeAliasOrThrow(std::string_view raw_alias) {
    const std::string alias = detail::TrimWhitespace(raw_alias);
    if (alias.size() < 2 ||
        alias.front() != '-' ||
        detail::StartsWith(alias, "--") ||
        detail::ContainsWhitespace(alias)) {
        throw std::invalid_argument("kcli alias must use single-dash form, e.g. '-v'");
    }
    return alias;
}

std::string NormalizeAliasTargetOrThrow(std::string_view raw_target) {
    const std::string target = detail::TrimWhitespace(raw_target);
    if (target.empty() || detail::ContainsWhitespace(target)) {
        throw std::invalid_argument("kcli alias target must be a single CLI token");
    }
    return target;
}

void ApplyFailureMode(const SessionState& state, const ProcessResult& result) {
    if (result.ok || state.failure_mode != FailureMode::Throw) {
        return;
    }

    if (result.error_message.empty()) {
        throw std::runtime_error("kcli parse failed");
    }
    throw std::runtime_error(result.error_message);
}

void CompactArgv(const std::vector<bool>& consumed, ProcessResult& result, int& argc, char** argv) {
    if (argc <= 0 || argv == nullptr) {
        result.stats.remaining_argc = std::max(argc, 0);
        return;
    }

    int write_index = 1;
    for (int read_index = 1; read_index < argc; ++read_index) {
        if (read_index >= 0 &&
            static_cast<std::size_t>(read_index) < consumed.size() &&
            !consumed[static_cast<std::size_t>(read_index)]) {
            argv[write_index++] = argv[read_index];
        }
    }

    if (write_index < argc) {
        argv[write_index] = nullptr;
    }

    argc = write_index;
    result.stats.remaining_argc = write_index;
}

void ConsumePositionals(SessionState& state, ProcessResult& result) {
    if (!state.positional_handler || state.parse.argc_ptr == nullptr || state.parse.argv == nullptr) {
        return;
    }
    if (backend::IsInlineMode(state.parse)) {
        return;
    }

    int& argc = *state.parse.argc_ptr;
    if (argc <= 1) {
        result.stats.remaining_argc = std::max(argc, 0);
        return;
    }

    std::vector<bool> consumed(static_cast<std::size_t>(argc), false);
    std::vector<std::string_view> positional_tokens;
    positional_tokens.reserve(static_cast<std::size_t>(argc));
    bool after_separator = false;
    int first_index = -1;

    for (int i = 1; i < argc; ++i) {
        if (state.parse.argv[i] == nullptr) {
            continue;
        }

        const std::string_view token(state.parse.argv[i]);
        if (!after_separator && token == "--") {
            consumed[static_cast<std::size_t>(i)] = true;
            after_separator = true;
            continue;
        }

        if (after_separator || (!token.empty() && token.front() != '-')) {
            consumed[static_cast<std::size_t>(i)] = true;
            positional_tokens.push_back(token);
            if (first_index < 0) {
                first_index = i;
            }
        }
    }

    if (positional_tokens.empty() &&
        std::none_of(consumed.begin(), consumed.end(), [](bool value) { return value; })) {
        result.stats.remaining_argc = argc;
        return;
    }

    HandlerContext context{};
    context.mode = Mode::EndUser;
    context.value_tokens = positional_tokens;
    context.from_alias = false;
    context.option_index = first_index;

    try {
        state.positional_handler(context);
    } catch (const std::exception& ex) {
        result = MakeError("", ex.what());
        return;
    } catch (...) {
        result = MakeError("", "unknown exception while handling positional arguments");
        return;
    }

    CompactArgv(consumed, result, argc, state.parse.argv);
}

} // namespace

void Initialize(int& argc, char** argv, const SessionOptions& options) {
    if (g_session.parse.argv != argv) {
        g_session.owned_tokens.clear();
    }

    backend::Reset(g_session.parse);
    g_session.failure_mode = options.failure_mode;
    g_session.positional_handler = PositionalHandler{};
    g_session.initialized = true;

    const std::string normalized_root = NormalizeRootOptionOrThrow(options.root);
    if (normalized_root.empty()) {
        backend::Initialize(g_session.parse, argc, argv);
    } else {
        backend::Initialize(g_session.parse, argc, argv, normalized_root);
    }
    backend::SetPolicy(g_session.parse, options.policy);
}

bool ExpandAlias(std::string_view alias, std::string_view target) {
    SessionState& state = RequireSession();
    if (backend::IsInlineMode(state.parse)) {
        throw std::logic_error("kcli::ExpandAlias is only valid in end-user mode");
    }

    const std::string alias_token = NormalizeAliasOrThrow(alias);
    const std::string target_token = NormalizeAliasTargetOrThrow(target);

    if (state.parse.argc_ptr == nullptr || state.parse.argv == nullptr || *state.parse.argc_ptr <= 1) {
        return false;
    }

    bool replaced = false;
    for (int i = 1; i < *state.parse.argc_ptr; ++i) {
        if (state.parse.argv[i] == nullptr) {
            continue;
        }

        const std::string_view token(state.parse.argv[i]);
        if (token == "--") {
            break;
        }

        if (token == alias_token) {
            state.owned_tokens.push_back(target_token);
            state.parse.argv[i] = const_cast<char*>(state.owned_tokens.back().c_str());
            replaced = true;
        }
    }

    return replaced;
}

void SetHandler(std::string_view option,
                FlagHandler handler,
                std::string_view description) {
    SessionState& state = RequireSession();
    const std::string command =
        NormalizeHandlerOptionOrThrow(option,
                                      backend::IsInlineMode(state.parse),
                                      backend::GetRoot(state.parse));
    backend::Implement(state.parse, command, std::move(handler), description);
}

void SetHandler(std::string_view option,
                ValueHandler handler,
                std::string_view description,
                ValueMode mode) {
    SessionState& state = RequireSession();
    const std::string command =
        NormalizeHandlerOptionOrThrow(option,
                                      backend::IsInlineMode(state.parse),
                                      backend::GetRoot(state.parse));
    backend::Implement(state.parse, command, std::move(handler), description, mode);
}

void SetPositionalHandler(PositionalHandler handler) {
    SessionState& state = RequireSession();
    if (backend::IsInlineMode(state.parse)) {
        throw std::logic_error("kcli::SetPositionalHandler is not allowed in inline mode");
    }
    if (!handler) {
        throw std::invalid_argument("kcli positional handler must not be empty");
    }
    state.positional_handler = std::move(handler);
}

void ClearPositionalHandler() {
    SessionState& state = RequireSession();
    state.positional_handler = PositionalHandler{};
}

ProcessResult Process() {
    SessionState& state = RequireSession();
    ProcessResult result = backend::Process(state.parse);
    if (result.ok) {
        ConsumePositionals(state, result);
    }
    ApplyFailureMode(state, result);
    return result;
}

ProcessResult FailOnUnknown() {
    SessionState& state = RequireSession();

    ProcessResult result{};
    if (state.parse.argc_ptr == nullptr) {
        result = MakeError("", "kcli session is not initialized");
        ApplyFailureMode(state, result);
        return result;
    }

    const int argc = *state.parse.argc_ptr;
    if (argc <= 0 || state.parse.argv == nullptr) {
        result.stats.remaining_argc = std::max(argc, 0);
        return result;
    }

    for (int i = 1; i < argc; ++i) {
        if (state.parse.argv[i] == nullptr) {
            continue;
        }

        const std::string token = detail::TrimWhitespace(std::string_view(state.parse.argv[i]));
        if (token.empty()) {
            continue;
        }
        if (token == "--") {
            break;
        }
        if (token.front() == '-') {
            result = MakeError(token, "unknown option " + token);
            break;
        }
    }

    result.stats.remaining_argc = argc;
    ApplyFailureMode(state, result);
    return result;
}

} // namespace kcli
