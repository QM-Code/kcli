#pragma once

#include <string_view>

namespace kcli::demo::alpha {

void ProcessCLI(int& argc, char** argv, std::string_view root = "alpha");
void EmitDemoOutput();

} // namespace kcli::demo::alpha
