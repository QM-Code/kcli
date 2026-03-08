#pragma once

#include <string_view>

namespace kcli::demo::gamma {

void ProcessCLI(int& argc, char** argv, std::string_view root = "gamma");
void EmitDemoOutput();

} // namespace kcli::demo::gamma
