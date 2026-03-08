#include "backend.hpp"

#include "util.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using kcli::backend::CommandBinding;
using kcli::backend::ParseState;

bool IsUsableValueToken(std::string_view value, bool reject_dash_prefixed) {
    if (value.empty()) {
        return false;
    }
    if (reject_dash_prefixed && value.front() == '-') {
        return false;
    }
    return true;
}

std::string JoinWithSpaces(const std::vector<std::string>& parts) {
    if (parts.empty()) {
        return {};
    }

    std::size_t total = 0;
    for (const std::string& part : parts) {
        total += part.size();
    }
    total += parts.size() - 1;

    std::string joined;
    joined.reserve(total);
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            joined.push_back(' ');
        }
        joined.append(parts[i]);
    }
    return joined;
}

struct CollectedValues {
    bool has_value = false;
    std::vector<std::string> parts{};
    int last_index = -1;
};

void ReportError(kcli::ProcessResult& result,
                 std::string_view option,
                 std::string_view message) {
    if (result.ok) {
        result.ok = false;
        result.error_option = std::string(option);
        result.error_message = std::string(message);
    }
}

CollectedValues CollectValueTokens(int option_index,
                                   int argc,
                                   char** argv,
                                   std::vector<bool>& consumed,
                                   bool reject_dash_prefixed_values) {
    CollectedValues collected{};
    collected.last_index = option_index;

    const int first_value_index = option_index + 1;
    const bool has_next =
        first_value_index < argc &&
        first_value_index >= 0 &&
        argv != nullptr &&
        argv[first_value_index] != nullptr &&
        !consumed[static_cast<std::size_t>(first_value_index)];

    if (!has_next) {
        return collected;
    }

    const std::string first = kcli::detail::TrimWhitespace(std::string_view(argv[first_value_index]));
    if (!IsUsableValueToken(first, reject_dash_prefixed_values)) {
        return collected;
    }

    collected.has_value = true;
    collected.parts.push_back(first);
    consumed[static_cast<std::size_t>(first_value_index)] = true;
    collected.last_index = first_value_index;

    for (int scan = first_value_index + 1; scan < argc; ++scan) {
        if (argv[scan] == nullptr) {
            break;
        }
        if (consumed[static_cast<std::size_t>(scan)]) {
            continue;
        }

        const std::string next = kcli::detail::TrimWhitespace(std::string_view(argv[scan]));
        if (next.empty()) {
            break;
        }
        if (next.front() == '-') {
            break;
        }

        collected.parts.push_back(next);
        consumed[static_cast<std::size_t>(scan)] = true;
        collected.last_index = scan;
    }

    return collected;
}

bool HasBoundArgv(const ParseState& state, kcli::ProcessResult& result) {
    if (state.argc_ptr == nullptr) {
        ReportError(result,
                    "",
                    "kcli is not initialized; call kcli::Initialize(argc, argv) first");
        return false;
    }

    if (*state.argc_ptr > 0 && state.argv == nullptr) {
        ReportError(result,
                    "",
                    "kcli received invalid argv (argc > 0 but argv is null)");
        return false;
    }

    return true;
}

void PrintInlineRootHelp(const ParseState& state) {
    const std::string prefix = "--" + state.root_name + "-";
    std::cout << "\nAvailable --" << state.root_name << "-* options:\n";

    std::vector<std::pair<std::string, std::string>> rows;
    rows.reserve(state.command_order.size());

    std::size_t max_lhs = 0;
    for (const std::string& command : state.command_order) {
        const auto it = state.commands.find(command);
        if (it == state.commands.end()) {
            continue;
        }
        const CommandBinding& binding = it->second;

        std::string lhs = prefix + command;
        if (binding.expects_value) {
            if (binding.value_mode == kcli::ValueMode::Optional) {
                lhs.append(" [value]");
            } else if (binding.value_mode == kcli::ValueMode::Required) {
                lhs.append(" <value>");
            }
        }
        max_lhs = std::max(max_lhs, lhs.size());
        rows.emplace_back(std::move(lhs), binding.description);
    }

    if (rows.empty()) {
        std::cout << "  (no options registered)\n";
    } else {
        for (const auto& row : rows) {
            std::cout << "  " << row.first;
            const std::size_t padding = max_lhs > row.first.size() ? (max_lhs - row.first.size()) : 0;
            for (std::size_t i = 0; i < padding + 2; ++i) {
                std::cout.put(' ');
            }
            std::cout << row.second << "\n";
        }
    }
    std::cout << "\n";
}

void ConsumeIndex(std::vector<bool>& consumed,
                  kcli::ProcessResult& result,
                  int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= consumed.size()) {
        return;
    }

    if (!consumed[static_cast<std::size_t>(index)]) {
        consumed[static_cast<std::size_t>(index)] = true;
        result.stats.consumed_options++;
    }
}

bool DispatchCommand(ParseState& state,
                     const std::string& command,
                     const std::string& option_token,
                     int& i,
                     int argc,
                     std::vector<bool>& consumed,
                     kcli::ProcessResult& result) {
    const auto it = state.commands.find(command);
    if (it == state.commands.end()) {
        return false;
    }

    const CommandBinding& binding = it->second;
    ConsumeIndex(consumed, result, i);

    kcli::HandlerContext context{};
    context.mode = state.mode;
    context.root = state.root_name;
    context.option = option_token;
    context.command = command;
    context.from_alias = false;
    context.option_index = i;

    try {
        if (!binding.expects_value) {
            binding.flag_handler(context);
            return true;
        }

        if (binding.value_mode == kcli::ValueMode::None) {
            binding.value_handler(context, std::string_view{});
            return true;
        }

        const CollectedValues collected =
            CollectValueTokens(i, argc, state.argv, consumed, state.policy.reject_dash_prefixed_values);
        const bool has_value = collected.has_value;
        std::string value = JoinWithSpaces(collected.parts);
        result.stats.consumed_values += static_cast<int>(collected.parts.size());

        if (!has_value && binding.value_mode == kcli::ValueMode::Required) {
            ReportError(result,
                        option_token,
                        "option '" + option_token + "' requires a value");
            return true;
        }

        if (has_value) {
            i = collected.last_index;
        } else {
            value.clear();
        }

        context.value_tokens.reserve(collected.parts.size());
        for (const std::string& token : collected.parts) {
            context.value_tokens.push_back(token);
        }
        binding.value_handler(context, value);
        return true;
    } catch (const std::exception& ex) {
        ReportError(result, option_token, ex.what());
        return true;
    } catch (...) {
        ReportError(result, option_token, "unknown exception while handling option");
        return true;
    }
}

void ApplyUnknownPolicy(const ParseState& state,
                        kcli::UnknownOptionPolicy policy_value,
                        const std::string& option_token,
                        int index,
                        std::vector<bool>& consumed,
                        kcli::ProcessResult& result) {
    switch (policy_value) {
    case kcli::UnknownOptionPolicy::Ignore:
        return;
    case kcli::UnknownOptionPolicy::Consume:
        ConsumeIndex(consumed, result, index);
        return;
    case kcli::UnknownOptionPolicy::Error:
        ConsumeIndex(consumed, result, index);
        if (state.mode == kcli::Mode::Inline && !state.root_name.empty()) {
            ReportError(result,
                        option_token,
                        "unknown option " + option_token +
                            " (use --" + state.root_name + " to list options)");
        } else {
            ReportError(result, option_token, "unknown option " + option_token);
        }
        return;
    }
}

void CompactArgv(ParseState& state,
                 std::vector<bool>& consumed,
                 kcli::ProcessResult& result) {
    if (state.argc_ptr == nullptr) {
        return;
    }

    const int argc = *state.argc_ptr;
    if (argc <= 0 || state.argv == nullptr) {
        result.stats.remaining_argc = std::max(argc, 0);
        return;
    }

    int write_index = 1;
    for (int read_index = 1; read_index < argc; ++read_index) {
        if (!consumed[static_cast<std::size_t>(read_index)]) {
            state.argv[write_index++] = state.argv[read_index];
        }
    }
    if (write_index < argc) {
        state.argv[write_index] = nullptr;
    }

    *state.argc_ptr = write_index;
    result.stats.remaining_argc = write_index;
}

kcli::ProcessResult ProcessInline(ParseState& state) {
    kcli::ProcessResult result{};
    if (!HasBoundArgv(state, result)) {
        return result;
    }

    const int argc = *state.argc_ptr;
    if (argc <= 0 || state.argv == nullptr) {
        result.stats.remaining_argc = std::max(argc, 0);
        return result;
    }

    std::vector<bool> consumed(static_cast<std::size_t>(argc), false);
    const std::string root_option = "--" + state.root_name;
    const std::string root_dash_prefix = root_option + "-";

    for (int i = 1; i < argc; ++i) {
        if (state.argv[i] == nullptr) {
            continue;
        }

        const std::string arg = kcli::detail::TrimWhitespace(std::string_view(state.argv[i]));
        if (arg.empty()) {
            continue;
        }

        if (arg == root_option) {
            const int option_index = i;
            ConsumeIndex(consumed, result, i);

            const CollectedValues collected = CollectValueTokens(
                option_index,
                argc,
                state.argv,
                consumed,
                state.policy.reject_dash_prefixed_values);
            result.stats.consumed_values += static_cast<int>(collected.parts.size());
            if (collected.has_value) {
                i = collected.last_index;
            }

            if (!collected.has_value) {
                PrintInlineRootHelp(state);
                continue;
            }

            if (!state.root_value_handler) {
                ReportError(result,
                            root_option,
                            "unknown value for option '" + root_option + "'");
                continue;
            }

            kcli::HandlerContext context{};
            context.mode = state.mode;
            context.root = state.root_name;
            context.option = root_option;
            context.command = "";
            context.from_alias = false;
            context.option_index = option_index;
            context.value_tokens.reserve(collected.parts.size());
            for (const std::string& token : collected.parts) {
                context.value_tokens.push_back(token);
            }

            try {
                state.root_value_handler(context, JoinWithSpaces(collected.parts));
            } catch (const std::exception& ex) {
                ReportError(result, root_option, ex.what());
            } catch (...) {
                ReportError(result,
                            root_option,
                            "unknown exception while handling option");
            }
            continue;
        }

        if (kcli::detail::StartsWith(arg, root_dash_prefix)) {
            const std::string command = arg.substr(root_dash_prefix.size());
            if (command.empty() ||
                !DispatchCommand(state, command, arg, i, argc, consumed, result)) {
                ApplyUnknownPolicy(
                    state,
                    state.policy.unknown_dash_option,
                    arg,
                    i,
                    consumed,
                    result);
            }
            continue;
        }

        if (state.policy.root_match == kcli::RootMatchMode::Prefix &&
            kcli::detail::StartsWith(arg, root_option)) {
            ApplyUnknownPolicy(
                state,
                state.policy.unknown_prefixed_option,
                arg,
                i,
                consumed,
                result);
            continue;
        }
    }

    CompactArgv(state, consumed, result);
    return result;
}

kcli::ProcessResult ProcessEndUser(ParseState& state) {
    kcli::ProcessResult result{};
    if (!HasBoundArgv(state, result)) {
        return result;
    }

    const int argc = *state.argc_ptr;
    if (argc <= 0 || state.argv == nullptr) {
        result.stats.remaining_argc = std::max(argc, 0);
        return result;
    }

    std::vector<bool> consumed(static_cast<std::size_t>(argc), false);

    for (int i = 1; i < argc; ++i) {
        if (state.argv[i] == nullptr) {
            continue;
        }

        const std::string arg = kcli::detail::TrimWhitespace(std::string_view(state.argv[i]));
        if (arg.empty()) {
            continue;
        }

        if (kcli::detail::StartsWith(arg, "--") && arg.size() > 2) {
            const std::string command = arg.substr(2);
            (void)DispatchCommand(state, command, arg, i, argc, consumed, result);
        }
    }

    CompactArgv(state, consumed, result);
    return result;
}

} // namespace

namespace kcli::backend {

ProcessResult Process(ParseState& state) {
    if (state.mode == Mode::Inline) {
        return ProcessInline(state);
    }
    return ProcessEndUser(state);
}

} // namespace kcli::backend
