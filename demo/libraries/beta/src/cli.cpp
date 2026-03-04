#include <beta/sdk.hpp>

#include <kcli.hpp>

#include <iostream>
#include <string>

namespace {

std::string g_profile = "safe";
int g_workers = 2;

void handleProfile(const kcli::HandlerContext&, std::string_view value) {
    if (!value.empty()) {
        g_profile = std::string(value);
    }
}

void handleWorkers(const kcli::HandlerContext&, std::string_view value) {
    if (!value.empty()) {
        g_workers = std::stoi(std::string(value));
    }
}

} // namespace

namespace kcli::demo::beta {

void ProcessCLI(int& argc, char** argv, std::string_view root) {
    kcli::Parser cli;
    cli.Initialize(argc, argv, root);
    cli.Implement("profile",
                  handleProfile,
                  "Select beta runtime profile.",
                  kcli::ValueMode::Required);
    cli.Implement("workers",
                  handleWorkers,
                  "Set beta worker count.",
                  kcli::ValueMode::Required);
    cli.Process();
}

void EmitDemoOutput() {
    std::cout << "[beta] profile='" << g_profile << "', workers=" << g_workers << "\n";
}

} // namespace kcli::demo::beta
