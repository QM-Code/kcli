#include "util.hpp"

#include <cctype>
#include <stdexcept>

namespace kcli::detail {

bool StartsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

std::string TrimWhitespace(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return std::string(value);
}

bool ContainsWhitespace(std::string_view value) {
    for (const char ch : value) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            return true;
        }
    }
    return false;
}

std::string NormalizeRootNameOrThrow(std::string_view raw_root) {
    std::string root = TrimWhitespace(raw_root);
    if (root.empty()) {
        throw std::invalid_argument("kcli root must not be empty");
    }
    if (root.front() == '-') {
        throw std::invalid_argument("kcli root must not begin with '-'");
    }
    if (ContainsWhitespace(root)) {
        throw std::invalid_argument("kcli root is invalid");
    }
    return root;
}

std::string NormalizeInlineRootOptionOrThrow(std::string_view raw_root) {
    std::string root = TrimWhitespace(raw_root);
    if (root.empty()) {
        throw std::invalid_argument("kcli root must not be empty");
    }
    if (StartsWith(root, "--")) {
        root.erase(0, 2);
    } else if (root.front() == '-') {
        throw std::invalid_argument("kcli root must use '--root' or 'root'");
    }
    return NormalizeRootNameOrThrow(root);
}

std::string NormalizeInlineHandlerOptionOrThrow(std::string_view raw_option,
                                                std::string_view root_name) {
    std::string option = TrimWhitespace(raw_option);
    if (option.empty()) {
        throw std::invalid_argument("kcli inline handler option must not be empty");
    }

    if (StartsWith(option, "--")) {
        const std::string full_prefix = "--" + std::string(root_name) + "-";
        if (!StartsWith(option, full_prefix)) {
            throw std::invalid_argument(
                "kcli inline handler option must use '-name' or '" + full_prefix + "name'");
        }
        option.erase(0, full_prefix.size());
    } else if (option.front() == '-') {
        option.erase(0, 1);
    } else {
        throw std::invalid_argument(
            "kcli inline handler option must use '-name' or '--" +
            std::string(root_name) + "-name'");
    }

    if (option.empty()) {
        throw std::invalid_argument("kcli command must not be empty");
    }
    if (option.front() == '-') {
        throw std::invalid_argument("kcli command must not start with '-'");
    }
    if (ContainsWhitespace(option)) {
        throw std::invalid_argument("kcli command must not contain whitespace");
    }
    return option;
}

std::string NormalizePrimaryHandlerOptionOrThrow(std::string_view raw_option) {
    std::string option = TrimWhitespace(raw_option);
    if (option.empty()) {
        throw std::invalid_argument("kcli end-user handler option must not be empty");
    }

    if (StartsWith(option, "--")) {
        option.erase(0, 2);
    } else if (option.front() == '-') {
        throw std::invalid_argument("kcli end-user handler option must use '--name' or 'name'");
    }

    if (option.empty()) {
        throw std::invalid_argument("kcli command must not be empty");
    }
    if (option.front() == '-') {
        throw std::invalid_argument("kcli command must not start with '-'");
    }
    if (ContainsWhitespace(option)) {
        throw std::invalid_argument("kcli command must not contain whitespace");
    }
    return option;
}

std::string NormalizeAliasOrThrow(std::string_view raw_alias) {
    const std::string alias = TrimWhitespace(raw_alias);
    if (alias.size() < 2 ||
        alias.front() != '-' ||
        StartsWith(alias, "--") ||
        ContainsWhitespace(alias)) {
        throw std::invalid_argument("kcli alias must use single-dash form, e.g. '-v'");
    }
    return alias;
}

std::string NormalizeAliasTargetOrThrow(std::string_view raw_target) {
    const std::string target = TrimWhitespace(raw_target);
    if (target.empty() || ContainsWhitespace(target)) {
        throw std::invalid_argument("kcli alias target must be a single CLI token");
    }
    return target;
}

std::string NormalizeDescriptionOrThrow(std::string_view raw_description) {
    std::string description = TrimWhitespace(raw_description);
    if (description.empty()) {
        throw std::invalid_argument("kcli command description must not be empty");
    }
    return description;
}

ProcessResult MakeError(std::string_view option, std::string_view message) {
    ProcessResult result{};
    result.ok = false;
    result.error_option = std::string(option);
    result.error_message = std::string(message);
    return result;
}

void ApplyFailureMode(FailureMode failure_mode, const ProcessResult& result) {
    if (result.ok || failure_mode != FailureMode::Throw) {
        return;
    }

    if (result.error_message.empty()) {
        throw std::runtime_error("kcli parse failed");
    }
    throw std::runtime_error(result.error_message);
}

}  // namespace kcli::detail
