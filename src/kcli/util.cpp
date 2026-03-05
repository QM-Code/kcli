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

    if (root.empty() || ContainsWhitespace(root)) {
        throw std::invalid_argument("kcli root is invalid");
    }

    return root;
}

std::string NormalizeCommandOrThrow(std::string_view raw_command) {
    std::string command = TrimWhitespace(raw_command);
    if (command.empty()) {
        throw std::invalid_argument("kcli command must not be empty");
    }
    if (command.front() == '-') {
        throw std::invalid_argument("kcli command must not start with '-'");
    }
    if (ContainsWhitespace(command)) {
        throw std::invalid_argument("kcli command must not contain whitespace");
    }
    return command;
}

std::string NormalizeDescriptionOrThrow(std::string_view raw_description) {
    std::string description = TrimWhitespace(raw_description);
    if (description.empty()) {
        throw std::invalid_argument("kcli command description must not be empty");
    }
    return description;
}

} // namespace kcli::detail
