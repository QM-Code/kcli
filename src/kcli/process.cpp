#include "impl.hpp"

#include "util.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

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

spdlog::logger& GetKcliLogger() {
    static std::shared_ptr<spdlog::logger> logger = []() {
        if (auto existing = spdlog::get("kcli")) {
            existing->set_pattern("[%^%l%$] %v");
            return existing;
        }

        auto base = spdlog::default_logger();
        std::shared_ptr<spdlog::logger> created;
        if (base) {
            const auto& sinks = base->sinks();
            created = std::make_shared<spdlog::logger>("kcli", sinks.begin(), sinks.end());
            created->set_level(base->level());
            created->flush_on(base->flush_level());
        } else {
            created = std::make_shared<spdlog::logger>("kcli");
            created->set_level(spdlog::level::info);
        }

        created->set_pattern("[%^%l%$] %v");
        try {
            spdlog::register_logger(created);
        } catch (const spdlog::spdlog_ex&) {
            if (auto existing = spdlog::get("kcli")) {
                existing->set_pattern("[%^%l%$] %v");
                return existing;
            }
        }
        return created;
    }();
    return *logger;
}

struct CollectedValues {
    bool has_value = false;
    std::vector<std::string> parts{};
    int last_index = -1;
};

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

    // Default behavior: consume additional contiguous non-option
    // tokens as part of the same value.
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

} // namespace

namespace kcli {

bool Parser::Impl::hasBoundArgv(ProcessResult& result) const {
    if (argc_ptr == nullptr) {
        reportError(result,
                    "",
                    "kcli::Parser is not initialized; call Initialize(argc, argv) first");
        return false;
    }

    if (*argc_ptr > 0 && argv == nullptr) {
        reportError(result,
                    "",
                    "kcli::Parser received invalid argv (argc > 0 but argv is null)");
        return false;
    }

    return true;
}

void Parser::Impl::printInlineRootHelp() const {
    const std::string prefix = "--" + root_name + "-";
    std::cout << "\nAvailable --" << root_name << "-* options:\n";

    std::vector<std::pair<std::string, std::string>> rows;
    rows.reserve(command_order.size() + 1);

    if (root_value_handler && !root_value_usage.empty() && !root_value_description.empty()) {
        rows.emplace_back("--" + root_name + " " + root_value_usage, root_value_description);
    }

    std::size_t max_lhs = 0;
    for (const std::string& command : command_order) {
        const auto it = commands.find(command);
        if (it == commands.end()) {
            continue;
        }
        const CommandBinding& binding = it->second;

        std::string lhs = prefix + command;
        if (binding.expects_value) {
            if (binding.value_mode == ValueMode::Optional) {
                lhs.append(" [value]");
            } else if (binding.value_mode == ValueMode::Required) {
                lhs.append(" <value>");
            }
        }
        max_lhs = std::max(max_lhs, lhs.size());
        rows.emplace_back(std::move(lhs), binding.description);
    }

    for (const auto& row : rows) {
        max_lhs = std::max(max_lhs, row.first.size());
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

void Parser::Impl::consumeIndex(std::vector<bool>& consumed,
                                ProcessResult& result,
                                int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= consumed.size()) {
        return;
    }

    if (!consumed[static_cast<std::size_t>(index)]) {
        consumed[static_cast<std::size_t>(index)] = true;
        result.stats.consumed_options++;
    }
}

bool Parser::Impl::dispatchCommand(const std::string& command,
                                   const std::string& option_token,
                                   bool from_alias,
                                   int& i,
                                   int argc,
                                   std::vector<bool>& consumed,
                                   ProcessResult& result) {
    const auto it = commands.find(command);
    if (it == commands.end()) {
        return false;
    }

    const CommandBinding& binding = it->second;
    consumeIndex(consumed, result, i);

    HandlerContext context{};
    context.mode = mode;
    context.root = root_name;
    context.option = option_token;
    context.command = command;
    context.value_tokens.clear();
    context.from_alias = from_alias;
    context.option_index = i;

    try {
        if (!binding.expects_value) {
            if (binding.flag_handler) {
                binding.flag_handler(context);
            }
            return true;
        }

        if (binding.value_mode == ValueMode::None) {
            if (binding.value_handler) {
                binding.value_handler(context, std::string_view{});
            }
            return true;
        }

        const CollectedValues collected =
            CollectValueTokens(i, argc, argv, consumed, policy.reject_dash_prefixed_values);
        const bool has_value = collected.has_value;
        std::string value = JoinWithSpaces(collected.parts);
        result.stats.consumed_values += static_cast<int>(collected.parts.size());

        if (!has_value && binding.value_mode == ValueMode::Required) {
            reportError(result,
                        option_token,
                        "option '" + option_token + "' requires a value");
            return true;
        }

        if (has_value) {
            i = collected.last_index;
        } else {
            value.clear();
        }

        if (binding.value_handler) {
            context.value_tokens.reserve(collected.parts.size());
            for (const std::string& token : collected.parts) {
                context.value_tokens.push_back(token);
            }
            binding.value_handler(context, value);
        }
        return true;
    } catch (const std::exception& ex) {
        reportError(result, option_token, ex.what());
        return true;
    } catch (...) {
        reportError(result, option_token, "unknown exception while handling option");
        return true;
    }
}

void Parser::Impl::applyUnknownPolicy(UnknownOptionPolicy policy_value,
                                      const std::string& option_token,
                                      int index,
                                      std::vector<bool>& consumed,
                                      ProcessResult& result) {
    emitUnknownOption(option_token);

    switch (policy_value) {
    case UnknownOptionPolicy::Ignore:
        return;
    case UnknownOptionPolicy::Consume:
        consumeIndex(consumed, result, index);
        return;
    case UnknownOptionPolicy::Error:
        consumeIndex(consumed, result, index);
        if (mode == Mode::Inline && !root_name.empty()) {
            reportError(result,
                        option_token,
                        "unknown option " + option_token +
                            " (use --" + root_name + " to list options)");
        } else {
            reportError(result, option_token, "unknown option " + option_token);
        }
        return;
    }
}

void Parser::Impl::emitDefaultError(const ProcessResult& result) const {
    if (result.ok || error_handler) {
        return;
    }
    GetKcliLogger().error("{}", result.error_message);
}

void Parser::Impl::compactArgv(std::vector<bool>& consumed, ProcessResult& result) {
    if (argc_ptr == nullptr) {
        return;
    }

    const int argc = *argc_ptr;
    if (argc <= 0 || argv == nullptr) {
        result.stats.remaining_argc = std::max(argc, 0);
        return;
    }

    int write_index = 1;
    for (int read_index = 1; read_index < argc; ++read_index) {
        if (!consumed[static_cast<std::size_t>(read_index)]) {
            argv[write_index++] = argv[read_index];
        }
    }
    if (write_index < argc) {
        argv[write_index] = nullptr;
    }

    *argc_ptr = write_index;
    result.stats.remaining_argc = write_index;
}

ProcessResult Parser::Impl::processInline() {
    ProcessResult result{};
    if (!hasBoundArgv(result)) {
        return result;
    }

    const int argc = *argc_ptr;
    if (argc <= 0 || argv == nullptr) {
        result.stats.remaining_argc = std::max(argc, 0);
        return result;
    }

    std::vector<bool> consumed(static_cast<std::size_t>(argc), false);
    const std::string root_option = "--" + root_name;
    const std::string root_dash_prefix = root_option + "-";

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }

        const std::string arg = detail::TrimWhitespace(std::string_view(argv[i]));
        if (arg.empty()) {
            continue;
        }

        if (arg == root_option) {
            const int option_index = i;
            consumeIndex(consumed, result, i);

            const CollectedValues collected =
                CollectValueTokens(option_index, argc, argv, consumed, policy.reject_dash_prefixed_values);
            result.stats.consumed_values += static_cast<int>(collected.parts.size());
            if (collected.has_value) {
                i = collected.last_index;
            }

            if (!collected.has_value) {
                printInlineRootHelp();
                continue;
            }

            if (!root_value_handler) {
                reportError(result,
                            root_option,
                            "unknown value for option '" + root_option + "'");
                continue;
            }

            HandlerContext context{};
            context.mode = mode;
            context.root = root_name;
            context.option = root_option;
            context.command = std::string_view{};
            context.value_tokens.reserve(collected.parts.size());
            for (const std::string& token : collected.parts) {
                context.value_tokens.push_back(token);
            }
            context.from_alias = false;
            context.option_index = option_index;

            try {
                root_value_handler(context, JoinWithSpaces(collected.parts));
            } catch (const std::exception& ex) {
                reportError(result, root_option, ex.what());
            } catch (...) {
                reportError(result, root_option, "unknown exception while handling option");
            }
            continue;
        }

        if (detail::StartsWith(arg, root_dash_prefix)) {
            const std::string command = arg.substr(root_dash_prefix.size());
            if (command.empty() || !dispatchCommand(command, arg, false, i, argc, consumed, result)) {
                applyUnknownPolicy(policy.unknown_dash_option, arg, i, consumed, result);
            }
            continue;
        }

        if (policy.root_match == RootMatchMode::Prefix && detail::StartsWith(arg, root_option)) {
            applyUnknownPolicy(policy.unknown_prefixed_option, arg, i, consumed, result);
            continue;
        }
    }

    compactArgv(consumed, result);
    emitDefaultError(result);
    return result;
}

ProcessResult Parser::Impl::processEndUser() {
    ProcessResult result{};
    if (!hasBoundArgv(result)) {
        return result;
    }

    const int argc = *argc_ptr;
    if (argc <= 0 || argv == nullptr) {
        result.stats.remaining_argc = std::max(argc, 0);
        return result;
    }

    std::vector<bool> consumed(static_cast<std::size_t>(argc), false);

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }

        const std::string arg = detail::TrimWhitespace(std::string_view(argv[i]));
        if (arg.empty()) {
            continue;
        }

        if (detail::StartsWith(arg, "--") && arg.size() > 2) {
            const std::string command = arg.substr(2);
            if (!dispatchCommand(command, arg, false, i, argc, consumed, result)) {
                emitUnknownOption(arg);
            }
            continue;
        }

        if (detail::StartsWith(arg, "-") && !detail::StartsWith(arg, "--") && arg.size() > 1) {
            const auto alias_it = aliases.find(arg);
            if (alias_it != aliases.end()) {
                (void)dispatchCommand(alias_it->second, arg, true, i, argc, consumed, result);
            } else {
                emitUnknownOption(arg);
            }
        }
    }

    compactArgv(consumed, result);
    emitDefaultError(result);
    return result;
}

} // namespace kcli
