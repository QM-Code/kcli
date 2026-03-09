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

using kcli::detail::CommandBinding;
using kcli::detail::InlineParserData;
using kcli::detail::ParseOutcome;
using kcli::detail::PrimaryParserData;

enum class InvocationKind {
    Flag,
    Value,
    Positional,
    PrintHelp,
};

struct Invocation {
    InvocationKind kind = InvocationKind::Flag;
    std::string root{};
    std::string option{};
    std::string command{};
    std::vector<std::string> value_tokens{};
    bool from_alias = false;
    int option_index = -1;
    kcli::FlagHandler flag_handler{};
    kcli::ValueHandler value_handler{};
    kcli::PositionalHandler positional_handler{};
    std::vector<std::pair<std::string, std::string>> help_rows{};
};

struct CollectedValues {
    bool has_value = false;
    std::vector<std::string> parts{};
    int last_index = -1;
};

struct InlineTokenMatch {
    enum class Kind {
        None,
        BareRoot,
        DashOption,
    };

    Kind kind = Kind::None;
    const InlineParserData* parser = nullptr;
    std::string suffix{};
};

bool IsCollectableFollowOnValueToken(std::string_view value) {
    if (value.empty()) {
        return false;
    }
    if (value.front() == '-') {
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

std::string FormatOptionErrorMessage(std::string_view option, std::string_view message) {
    if (option.empty()) {
        return std::string(message);
    }
    return "option '" + std::string(option) + "': " + std::string(message);
}

void ReportError(ParseOutcome& result,
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
                                   bool allow_option_like_first_value) {
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
    if (first.empty()) {
        return collected;
    }
    if (!allow_option_like_first_value && first.front() == '-') {
        return collected;
    }

    collected.has_value = true;
    collected.parts.push_back(first);
    consumed[static_cast<std::size_t>(first_value_index)] = true;
    collected.last_index = first_value_index;

    if (allow_option_like_first_value && first.front() == '-') {
        return collected;
    }

    for (int scan = first_value_index + 1; scan < argc; ++scan) {
        if (argv[scan] == nullptr) {
            break;
        }
        if (consumed[static_cast<std::size_t>(scan)]) {
            continue;
        }

        const std::string next = kcli::detail::TrimWhitespace(std::string_view(argv[scan]));
        if (!IsCollectableFollowOnValueToken(next)) {
            break;
        }

        collected.parts.push_back(next);
        consumed[static_cast<std::size_t>(scan)] = true;
        collected.last_index = scan;
    }

    return collected;
}

void PrintHelp(const Invocation& invocation) {
    std::cout << "\nAvailable --" << invocation.root << "-* options:\n";

    std::size_t max_lhs = 0;
    for (const auto& row : invocation.help_rows) {
        max_lhs = std::max(max_lhs, row.first.size());
    }

    if (invocation.help_rows.empty()) {
        std::cout << "  (no options registered)\n";
    } else {
        for (const auto& row : invocation.help_rows) {
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
                  int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= consumed.size()) {
        return;
    }

    if (!consumed[static_cast<std::size_t>(index)]) {
        consumed[static_cast<std::size_t>(index)] = true;
    }
}

const CommandBinding* FindCommand(const std::vector<std::pair<std::string, CommandBinding>>& commands,
                                  std::string_view command) {
    for (const auto& entry : commands) {
        if (entry.first == command) {
            return &entry.second;
        }
    }
    return nullptr;
}

std::vector<std::pair<std::string, std::string>> BuildHelpRows(const InlineParserData& parser) {
    const std::string prefix = "--" + parser.root_name + "-";
    std::vector<std::pair<std::string, std::string>> rows;
    rows.reserve(parser.commands.size());

    for (const auto& [command, binding] : parser.commands) {
        std::string lhs = prefix + command;
        if (binding.expects_value) {
            if (binding.value_mode == kcli::ValueMode::Optional) {
                lhs.append(" [value]");
            } else if (binding.value_mode == kcli::ValueMode::Required) {
                lhs.append(" <value>");
            }
        }
        rows.emplace_back(std::move(lhs), binding.description);
    }

    return rows;
}

InlineTokenMatch MatchInlineToken(const PrimaryParserData& data, std::string_view arg) {
    for (const InlineParserData& parser : data.inline_parsers) {
        const std::string root_option = "--" + parser.root_name;
        if (arg == root_option) {
            return {.kind = InlineTokenMatch::Kind::BareRoot, .parser = &parser};
        }

        const std::string root_dash_prefix = root_option + "-";
        if (kcli::detail::StartsWith(arg, root_dash_prefix)) {
            return {
                .kind = InlineTokenMatch::Kind::DashOption,
                .parser = &parser,
                .suffix = std::string(arg.substr(root_dash_prefix.size())),
            };
        }
    }

    return {};
}

bool ScheduleInvocation(const CommandBinding& binding,
                        std::string_view root,
                        std::string_view command,
                        std::string_view option_token,
                        bool from_alias,
                        int& i,
                        int argc,
                        char** argv,
                        std::vector<bool>& consumed,
                        std::vector<Invocation>& invocations,
                        ParseOutcome& result) {
    ConsumeIndex(consumed, i);

    Invocation invocation{};
    invocation.root = std::string(root);
    invocation.option = std::string(option_token);
    invocation.command = std::string(command);
    invocation.from_alias = from_alias;
    invocation.option_index = i;

    if (!binding.expects_value) {
        invocation.kind = InvocationKind::Flag;
        invocation.flag_handler = binding.flag_handler;
        invocations.push_back(std::move(invocation));
        return true;
    }

    if (binding.value_mode == kcli::ValueMode::None) {
        invocation.kind = InvocationKind::Value;
        invocation.value_handler = binding.value_handler;
        invocations.push_back(std::move(invocation));
        return true;
    }

    const CollectedValues collected = CollectValueTokens(i,
                                                         argc,
                                                         argv,
                                                         consumed,
                                                         binding.value_mode == kcli::ValueMode::Required);

    if (!collected.has_value && binding.value_mode == kcli::ValueMode::Required) {
        ReportError(result, option_token, "option '" + std::string(option_token) + "' requires a value");
        return true;
    }

    if (collected.has_value) {
        i = collected.last_index;
    }

    invocation.kind = InvocationKind::Value;
    invocation.value_handler = binding.value_handler;
    invocation.value_tokens = collected.parts;
    invocations.push_back(std::move(invocation));
    return true;
}

void SchedulePositionals(const PrimaryParserData& data,
                        int argc,
                        char** argv,
                        std::vector<bool>& consumed,
                        std::vector<Invocation>& invocations) {
    if (!data.positional_handler || argc <= 1 || argv == nullptr) {
        return;
    }

    Invocation invocation{};
    invocation.kind = InvocationKind::Positional;
    invocation.positional_handler = data.positional_handler;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr || consumed[static_cast<std::size_t>(i)]) {
            continue;
        }

        const std::string_view token(argv[i]);
        if (!token.empty() && token.front() != '-') {
            if (invocation.option_index < 0) {
                invocation.option_index = i;
            }
            consumed[static_cast<std::size_t>(i)] = true;
            invocation.value_tokens.emplace_back(token);
        }
    }

    if (!invocation.value_tokens.empty()) {
        invocations.push_back(std::move(invocation));
    }
}

void CompactArgv(int& argc,
                 char** argv,
                 const std::vector<bool>& consumed) {
    if (argc <= 0 || argv == nullptr) {
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

    argc = write_index;
}

void ExecuteInvocations(const std::vector<Invocation>& invocations,
                       ParseOutcome& result) {
    for (const Invocation& invocation : invocations) {
        if (!result.ok) {
            return;
        }

        if (invocation.kind == InvocationKind::PrintHelp) {
            PrintHelp(invocation);
            continue;
        }

        kcli::HandlerContext context{};
        context.root = invocation.root;
        context.option = invocation.option;
        context.command = invocation.command;
        context.from_alias = invocation.from_alias;
        context.option_index = invocation.option_index;
        context.value_tokens.reserve(invocation.value_tokens.size());
        for (const std::string& token : invocation.value_tokens) {
            context.value_tokens.push_back(token);
        }

        try {
            switch (invocation.kind) {
            case InvocationKind::Flag:
                invocation.flag_handler(context);
                break;
            case InvocationKind::Value:
                invocation.value_handler(context, JoinWithSpaces(invocation.value_tokens));
                break;
            case InvocationKind::Positional:
                invocation.positional_handler(context);
                break;
            case InvocationKind::PrintHelp:
                break;
            }
        } catch (const std::exception& ex) {
            ReportError(result, invocation.option, FormatOptionErrorMessage(invocation.option, ex.what()));
        } catch (...) {
            ReportError(result,
                        invocation.option,
                        FormatOptionErrorMessage(invocation.option,
                                                 "unknown exception while handling option"));
        }
    }
}

}  // namespace

namespace kcli::detail {

void Parse(PrimaryParserData& data, int& argc, char** argv) {
    ParseOutcome result{};
    if (argc > 0 && argv == nullptr) {
        result = MakeError("", "kcli received invalid argv (argc > 0 but argv is null)");
        ThrowCliError(result);
    }

    if (argc <= 0 || argv == nullptr) {
        return;
    }

    std::vector<bool> consumed(static_cast<std::size_t>(argc), false);
    std::vector<bool> from_alias(static_cast<std::size_t>(argc), false);
    std::vector<Invocation> invocations;

    for (const AliasBinding& alias : data.aliases) {
        for (int i = 1; i < argc; ++i) {
            if (argv[i] == nullptr) {
                continue;
            }

            const std::string_view token(argv[i]);
            if (token != alias.alias) {
                continue;
            }

            data.owned_tokens.push_back(alias.target);
            argv[i] = data.owned_tokens.back().data();
            from_alias[static_cast<std::size_t>(i)] = true;
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr || consumed[static_cast<std::size_t>(i)]) {
            continue;
        }

        const std::string arg = TrimWhitespace(std::string_view(argv[i]));
        if (arg.empty()) {
            continue;
        }

        if (arg.front() != '-') {
            continue;
        }

        if (arg == "--") {
            continue;
        }

        if (StartsWith(arg, "--")) {
            const InlineTokenMatch inline_match = MatchInlineToken(data, arg);
            switch (inline_match.kind) {
            case InlineTokenMatch::Kind::BareRoot: {
                ConsumeIndex(consumed, i);
                const CollectedValues collected =
                    CollectValueTokens(i, argc, argv, consumed, false);

                if (!collected.has_value) {
                    Invocation help{};
                    help.kind = InvocationKind::PrintHelp;
                    help.root = inline_match.parser->root_name;
                    help.help_rows = BuildHelpRows(*inline_match.parser);
                    invocations.push_back(std::move(help));
                    break;
                }

                if (!inline_match.parser->root_value_handler) {
                    ReportError(result,
                                arg,
                                "unknown value for option '" + arg + "'");
                    break;
                }

                Invocation invocation{};
                invocation.kind = InvocationKind::Value;
                invocation.root = inline_match.parser->root_name;
                invocation.option = arg;
                invocation.from_alias = from_alias[static_cast<std::size_t>(i)];
                invocation.option_index = i;
                invocation.value_handler = inline_match.parser->root_value_handler;
                invocation.value_tokens = collected.parts;
                invocations.push_back(std::move(invocation));
                i = collected.last_index;
                break;
            }
            case InlineTokenMatch::Kind::DashOption: {
                const CommandBinding* binding =
                    FindCommand(inline_match.parser->commands, inline_match.suffix);
                if (inline_match.suffix.empty() || binding == nullptr) {
                    break;
                }

                (void)ScheduleInvocation(*binding,
                                         inline_match.parser->root_name,
                                         inline_match.suffix,
                                         arg,
                                         from_alias[static_cast<std::size_t>(i)],
                                         i,
                                         argc,
                                         argv,
                                         consumed,
                                         invocations,
                                         result);
                break;
            }
            case InlineTokenMatch::Kind::None: {
                const std::string command = arg.substr(2);
                const CommandBinding* binding = FindCommand(data.commands, command);
                if (binding != nullptr) {
                    (void)ScheduleInvocation(*binding,
                                             "",
                                             command,
                                             arg,
                                             from_alias[static_cast<std::size_t>(i)],
                                             i,
                                             argc,
                                             argv,
                                             consumed,
                                             invocations,
                                             result);
                }
                break;
            }
            }
        }

        if (!result.ok) {
            break;
        }
    }

    if (result.ok) {
        SchedulePositionals(data, argc, argv, consumed, invocations);
    }

    CompactArgv(argc, argv, consumed);

    if (result.ok) {
        for (int i = 1; i < argc; ++i) {
            if (argv[i] == nullptr) {
                continue;
            }

            const std::string token = TrimWhitespace(std::string_view(argv[i]));
            if (token.empty()) {
                continue;
            }
            if (token.front() == '-') {
                ReportError(result, token, "unknown option " + token);
                break;
            }
        }
    }

    if (result.ok) {
        ExecuteInvocations(invocations, result);
    }

    if (!result.ok) {
        ThrowCliError(result);
    }
}

}  // namespace kcli::detail
