#include <alpha/sdk.hpp>
#include <beta/sdk.hpp>
#include <gamma/sdk.hpp>
#include <kcli.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>
#include <string_view>

namespace {

void handleBuildProfile(const kcli::HandlerContext&, std::string_view) {
}

void handleBuildClean(const kcli::HandlerContext&) {
}

void handleVerbose(const kcli::HandlerContext&) {
}

void handleOutput(const kcli::HandlerContext&, std::string_view) {
}

void handleArgs(const kcli::HandlerContext&) {
}

} // namespace

int main(int argc, char** argv) {
    spdlog::set_pattern("[%^%l%$] %v");

    try {
        // With no root specified, this is end-user mode.
        kcli::Initialize(argc, argv, {.failure_mode = kcli::FailureMode::Throw});

        // Expand aliases before any library consumes its own root.
        kcli::ExpandAlias("-v", "--verbose");
        kcli::ExpandAlias("-out", "--output");
        kcli::ExpandAlias("-a", "--alpha-enable");
        kcli::ExpandAlias("-b", "--build-profile");

        // Imported libraries consume their own inline namespaces.
        kcli::demo::alpha::ProcessCLI(argc, argv, {.failure_mode = kcli::FailureMode::Throw});
        kcli::demo::beta::ProcessCLI(argc, argv, {.failure_mode = kcli::FailureMode::Throw});
        kcli::demo::gamma::ProcessCLI(
            argc,
            argv,
            {.root = "--renamed", .failure_mode = kcli::FailureMode::Throw});

        // App-defined inline namespace group (for example --build-*).
        kcli::Initialize(
            argc,
            argv,
            {.root = "--build", .failure_mode = kcli::FailureMode::Throw});
        kcli::SetHandler("-profile",
                         handleBuildProfile,
                         "Set build profile.",
                         kcli::ValueMode::Required);
        kcli::SetHandler("-clean", handleBuildClean, "Enable clean build.");
        kcli::Process();

        // App-defined end-user options.
        kcli::Initialize(argc, argv, {.failure_mode = kcli::FailureMode::Throw});
        kcli::SetHandler("--verbose", handleVerbose, "Enable verbose app logging.");
        kcli::SetHandler("--output",
                         handleOutput,
                         "Set app output target.",
                         kcli::ValueMode::Required);
        kcli::SetPositionalHandler(handleArgs);
        kcli::Process();

        // No option-like leftovers are allowed after all phases complete.
        kcli::FailOnUnknown();
    } catch (const std::exception& ex) {
        spdlog::error("CLI error: {}", ex.what());
        return 2;
    }

    std::cout << "\nKCLI demo omega compile/link/integration check passed\n\n";
    std::cout << "Usage:\n";
    std::cout << "  kcli_demo_omega --<root>\n\n";
    std::cout << "Enabled --<root> prefixes:\n";
    std::cout << "  --alpha\n";
    std::cout << "  --beta\n";
    std::cout << "  --renamed (gamma override)\n\n";
    return 0;
}
