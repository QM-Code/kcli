#include "impl.hpp"

#include "util.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
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

    bool printed_any = false;
    for (const std::string& command : command_order) {
        const auto it = commands.find(command);
        if (it == commands.end()) {
            continue;
        }
        const CommandBinding& binding = it->second;

        std::cout << "  " << prefix << command;
        if (binding.expects_value) {
            if (binding.value_mode == ValueMode::Optional) {
                std::cout << " [value]";
            } else if (binding.value_mode == ValueMode::Required) {
                std::cout << " <value>";
            }
        }
        std::cout << "  " << binding.description << "\n";
        printed_any = true;
    }

    if (!printed_any) {
        std::cout << "  (no options registered)\n";
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

        std::string value{};
        const int value_index = i + 1;
        const bool has_next =
            value_index < argc &&
            value_index >= 0 &&
            argv != nullptr &&
            argv[value_index] != nullptr &&
            !consumed[static_cast<std::size_t>(value_index)];

        bool has_value = false;
        if (has_next) {
            value = detail::TrimWhitespace(std::string_view(argv[value_index]));
            has_value = IsUsableValueToken(value, policy.reject_dash_prefixed_values);
        }

        if (!has_value && binding.value_mode == ValueMode::Required) {
            reportError(result,
                        option_token,
                        "option '" + option_token + "' requires a value");
            return true;
        }

        if (has_value) {
            consumed[static_cast<std::size_t>(value_index)] = true;
            result.stats.consumed_values++;
            ++i;
        } else {
            value.clear();
        }

        if (binding.value_handler) {
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
        reportError(result, option_token, "unknown option '" + option_token + "'");
        return;
    }
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
            consumeIndex(consumed, result, i);
            printInlineRootHelp();
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
    return result;
}

} // namespace kcli
