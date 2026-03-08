#pragma once

#include <kcli.hpp>

namespace kcli::demo::gamma {

ProcessResult ProcessCLI(int& argc, char** argv, const SessionOptions& options = {});
void EmitDemoOutput();

} // namespace kcli::demo::gamma
