#pragma once

#include <string_view>

namespace kcli::demo::beta {

void ProcessCLI(int& argc, char** argv, std::string_view root = "beta");
void EmitDemoOutput();

} // namespace kcli::demo::beta
