#pragma once

#include <kcli.hpp>

namespace kcli::demo::alpha {

ProcessResult ProcessCLI(int& argc, char** argv, const SessionOptions& options = {});
void EmitDemoOutput();

} // namespace kcli::demo::alpha
