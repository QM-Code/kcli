#include <gamma/sdk.hpp>

#include <kcli.hpp>

#include <iostream>
#include <string>

namespace {

bool g_strict = false;
std::string g_tag = "none";

void handleStrict(const kcli::HandlerContext&) {
    g_strict = true;
}

void handleTag(const kcli::HandlerContext&, std::string_view value) {
    if (!value.empty()) {
        g_tag = std::string(value);
    }
}

} // namespace

namespace kcli::demo::gamma {

void ProcessCLI(int& argc, char** argv, std::string_view root) {
    kcli::Parser cli;
    cli.Initialize(argc, argv, root);
    cli.Implement("strict", handleStrict, "Enable strict gamma mode.");
    cli.Implement("tag",
                  handleTag,
                  "Set a gamma tag label.",
                  kcli::ValueMode::Required);
    cli.Process();
}

void EmitDemoOutput() {
    std::cout << "[gamma] strict=" << std::boolalpha << g_strict
              << ", tag='" << g_tag << "'\n";
}

} // namespace kcli::demo::gamma
