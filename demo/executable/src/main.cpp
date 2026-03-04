#include <alpha/sdk.hpp>
#include <beta/sdk.hpp>
#include <gamma/sdk.hpp>
#include <kcli.hpp>

#include <iostream>
#include <string>

namespace {

std::string g_build_profile = "dev";
bool g_build_clean = false;
bool g_verbose = false;
std::string g_output = "stdout";

void handleBuildProfile(const kcli::HandlerContext&, std::string_view value) {
    g_build_profile = std::string(value);
}

void handleBuildClean(const kcli::HandlerContext&) {
    g_build_clean = true;
}

void handleVerbose(const kcli::HandlerContext&) {
    g_verbose = true;
}

void handleOutput(const kcli::HandlerContext&, std::string_view value) {
    g_output = std::string(value);
}

} // namespace

int main(int argc, char** argv) {
    // Imported libraries consume their own inline namespaces.
    kcli::demo::alpha::ProcessCLI(argc, argv);
    kcli::demo::beta::ProcessCLI(argc, argv);
    kcli::demo::gamma::ProcessCLI(argc, argv, "runtime");

    // App-defined inline namespace group (for example --build-*).
    kcli::Parser build;
    build.Initialize(argc, argv, "build");
    build.Implement("profile",
                    handleBuildProfile,
                    "Set build profile.",
                    kcli::ValueMode::Required);
    build.Implement("clean", handleBuildClean, "Enable clean build.");
    build.Process();

    // App-defined end-user options.
    kcli::Parser cli;
    cli.Initialize(argc, argv);
    cli.Implement("verbose", handleVerbose, "Enable verbose app logging.");
    cli.Implement("output",
                  handleOutput,
                  "Set app output target.",
                  kcli::ValueMode::Required);
    cli.AddAlias("-v", "verbose");
    cli.AddAlias("-out", "output");
    cli.Process();

    // Library symbols are reachable from the app.
    kcli::demo::alpha::EmitDemoOutput();
    kcli::demo::beta::EmitDemoOutput();
    kcli::demo::gamma::EmitDemoOutput();

    std::cout << "[app] build_profile='" << g_build_profile
              << "', build_clean=" << std::boolalpha << g_build_clean
              << ", verbose=" << g_verbose
              << ", output='" << g_output << "'\n";
    std::cout << "KCLI demo executable compile/link/integration check passed\n";
    return 0;
}
