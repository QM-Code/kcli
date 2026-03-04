#pragma once

#include <string>
#include <string_view>

namespace kcli::detail {

bool StartsWith(std::string_view value, std::string_view prefix);
std::string TrimWhitespace(std::string_view value);
bool ContainsWhitespace(std::string_view value);

std::string NormalizeRootNameOrThrow(std::string_view raw_root);
std::string NormalizeCommandOrThrow(std::string_view raw_command);
std::string NormalizeDescriptionOrThrow(std::string_view raw_description);

} // namespace kcli::detail
