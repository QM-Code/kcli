#include "impl.hpp"

#include "util.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace kcli {

void Parser::Impl::Reset() {
    argc_ptr = nullptr;
    argv = nullptr;
    mode = Mode::EndUser;
    root_name.clear();
    policy = ParsePolicy{};
    commands.clear();
    command_order.clear();
    aliases.clear();
    root_value_handler = ValueHandler{};
    root_value_usage.clear();
    root_value_description.clear();
    unknown_option_handler = UnknownOptionHandler{};
    error_handler = ErrorHandler{};
    warning_handler = WarningHandler{};
}

void Parser::Impl::Initialize(int& argc, char** argv_value) {
    argc_ptr = &argc;
    argv = argv_value;
    mode = Mode::EndUser;
    root_name.clear();
}

void Parser::Impl::Initialize(int& argc, char** argv_value, std::string_view root) {
    argc_ptr = &argc;
    argv = argv_value;
    mode = Mode::Inline;
    root_name = detail::NormalizeRootNameOrThrow(root);
}

Mode Parser::Impl::GetMode() const {
    return mode;
}

bool Parser::Impl::IsInlineMode() const {
    return mode == Mode::Inline;
}

bool Parser::Impl::IsEndUserMode() const {
    return mode == Mode::EndUser;
}

std::string Parser::Impl::GetRoot() const {
    return root_name;
}

void Parser::Impl::SetPolicy(const ParsePolicy& next_policy) {
    policy = next_policy;
}

ParsePolicy Parser::Impl::GetPolicy() const {
    return policy;
}

void Parser::Impl::Implement(std::string_view command,
                             FlagHandler handler,
                             std::string_view description) {
    if (!handler) {
        throw std::invalid_argument("kcli flag handler must not be empty");
    }

    const std::string command_id = detail::NormalizeCommandOrThrow(command);
    const std::string command_description = detail::NormalizeDescriptionOrThrow(description);

    const bool is_new = commands.find(command_id) == commands.end();
    CommandBinding binding{};
    binding.expects_value = false;
    binding.flag_handler = std::move(handler);
    binding.value_mode = ValueMode::None;
    binding.description = command_description;

    commands[command_id] = std::move(binding);
    if (is_new) {
        command_order.push_back(command_id);
    }
}

void Parser::Impl::Implement(std::string_view command,
                             ValueHandler handler,
                             std::string_view description,
                             ValueMode mode_value) {
    if (!handler) {
        throw std::invalid_argument("kcli value handler must not be empty");
    }

    const std::string command_id = detail::NormalizeCommandOrThrow(command);
    const std::string command_description = detail::NormalizeDescriptionOrThrow(description);

    const bool is_new = commands.find(command_id) == commands.end();
    CommandBinding binding{};
    binding.expects_value = true;
    binding.value_handler = std::move(handler);
    binding.value_mode = mode_value;
    binding.description = command_description;

    commands[command_id] = std::move(binding);
    if (is_new) {
        command_order.push_back(command_id);
    }
}

void Parser::Impl::Remove(std::string_view command) {
    const std::string command_id = detail::NormalizeCommandOrThrow(command);
    if (commands.erase(command_id) == 0) {
        return;
    }

    command_order.erase(
        std::remove(command_order.begin(), command_order.end(), command_id),
        command_order.end());

    for (auto it = aliases.begin(); it != aliases.end();) {
        if (it->second == command_id) {
            it = aliases.erase(it);
        } else {
            ++it;
        }
    }
}

bool Parser::Impl::HasCommand(std::string_view command) const {
    std::string command_id = detail::TrimWhitespace(command);
    if (command_id.empty() || command_id.front() == '-' || detail::ContainsWhitespace(command_id)) {
        return false;
    }
    return commands.find(command_id) != commands.end();
}

void Parser::Impl::SetRootValueHandler(ValueHandler handler) {
    if (!handler) {
        throw std::invalid_argument("kcli root value handler must not be empty");
    }
    root_value_handler = std::move(handler);
    root_value_usage.clear();
    root_value_description.clear();
}

void Parser::Impl::SetRootValueHandler(ValueHandler handler,
                                       std::string_view value_usage,
                                       std::string_view description) {
    if (!handler) {
        throw std::invalid_argument("kcli root value handler must not be empty");
    }

    const std::string usage = detail::TrimWhitespace(value_usage);
    if (usage.empty()) {
        throw std::invalid_argument("kcli root value usage must not be empty");
    }

    const std::string desc = detail::NormalizeDescriptionOrThrow(description);

    root_value_handler = std::move(handler);
    root_value_usage = usage;
    root_value_description = desc;
}

void Parser::Impl::ClearRootValueHandler() {
    root_value_handler = ValueHandler{};
    root_value_usage.clear();
    root_value_description.clear();
}

bool Parser::Impl::HasRootValueHandler() const {
    return static_cast<bool>(root_value_handler);
}

bool Parser::Impl::AddAlias(std::string_view alias,
                            std::string_view command) {
    if (mode == Mode::Inline) {
        throw std::logic_error("kcli: AddAlias is not allowed in inline mode");
    }

    const std::string alias_token = detail::TrimWhitespace(alias);
    if (alias_token.size() < 2 ||
        alias_token.front() != '-' ||
        detail::StartsWith(alias_token, "--") ||
        detail::ContainsWhitespace(alias_token)) {
        emitWarning("kcli: rejected alias (must be single-dash form, e.g. '-v' or '-out')");
        return false;
    }

    const std::string command_id = detail::TrimWhitespace(command);
    if (command_id.empty() || command_id.front() == '-' || detail::ContainsWhitespace(command_id)) {
        emitWarning("kcli: rejected alias target (must be a command id without dashes)");
        return false;
    }

    if (commands.find(command_id) == commands.end()) {
        emitWarning("kcli: rejected alias target (command is not registered)");
        return false;
    }

    aliases[alias_token] = command_id;
    return true;
}

bool Parser::Impl::RemoveAlias(std::string_view alias) {
    const std::string alias_token = detail::TrimWhitespace(alias);
    if (alias_token.empty()) {
        return false;
    }
    return aliases.erase(alias_token) > 0;
}

void Parser::Impl::SetUnknownOptionHandler(UnknownOptionHandler handler) {
    unknown_option_handler = std::move(handler);
}

void Parser::Impl::SetErrorHandler(ErrorHandler handler) {
    error_handler = std::move(handler);
}

void Parser::Impl::SetWarningHandler(WarningHandler handler) {
    warning_handler = std::move(handler);
}

void Parser::Impl::emitWarning(std::string_view message) const {
    if (warning_handler) {
        warning_handler(message);
    }
}

void Parser::Impl::emitUnknownOption(std::string_view option) const {
    if (unknown_option_handler) {
        unknown_option_handler(option);
    }
}

void Parser::Impl::reportError(ProcessResult& result,
                               std::string_view option,
                               std::string_view message) const {
    if (error_handler) {
        error_handler(message);
    }

    if (result.ok) {
        result.ok = false;
        result.error_option = std::string(option);
        result.error_message = std::string(message);
    }
}

ProcessResult Parser::Impl::Process() {
    if (mode == Mode::Inline) {
        return processInline();
    }
    return processEndUser();
}

} // namespace kcli
