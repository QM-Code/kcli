#include <alpha/sdk.hpp>

#include <kcli.hpp>

#include <iostream>
#include <string>

namespace {

std::string g_message = "alpha-default";
bool g_enabled = false;

void handleMessage(const kcli::HandlerContext&, std::string_view value) {
    if (!value.empty()) {
        g_message = std::string(value);
    }
}

void handleEnable(const kcli::HandlerContext&) {
    g_enabled = true;
}

} // namespace

namespace kcli::demo::alpha {

void ProcessCLI(int& argc, char** argv, std::string_view root) {
    kcli::Parser cli;
    cli.Initialize(argc, argv, root);
    cli.Implement("message",
                  handleMessage,
                  "Set the alpha message label.",
                  kcli::ValueMode::Required);
    cli.Implement("enable", handleEnable, "Enable alpha processing.");
    cli.Process();
}

void EmitDemoOutput() {
    std::cout << "[alpha] enabled=" << std::boolalpha << g_enabled
              << ", message='" << g_message << "'\n";
}

} // namespace kcli::demo::alpha
