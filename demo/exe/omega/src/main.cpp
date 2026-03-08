#include <alpha/sdk.hpp>
#include <beta/sdk.hpp>
#include <gamma/sdk.hpp>
#include <kcli.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::string ExecutableName(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return "app";
    }
    return std::string(path);
}

bool IsRootTokenOrSubcommand(std::string_view arg, std::string_view root) {
    const std::string root_token = std::string("--") + std::string(root);
    if (arg == root_token) {
        return true;
    }
    const std::string root_prefix = root_token + "-";
    return arg.rfind(root_prefix, 0) == 0;
}

bool HasInlineRootArgs(int argc, char** argv) {
    if (argc <= 1 || argv == nullptr) {
        return false;
    }
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }
        const std::string_view arg(argv[i]);
        if (IsRootTokenOrSubcommand(arg, "alpha") ||
            IsRootTokenOrSubcommand(arg, "beta") ||
            IsRootTokenOrSubcommand(arg, "renamed")) {
            return true;
        }
    }
    return false;
}

void handleBuildProfile(const kcli::HandlerContext&, std::string_view) {
}

void handleBuildClean(const kcli::HandlerContext&) {
}

void handleVerbose(const kcli::HandlerContext&) {
}

void handleOutput(const kcli::HandlerContext&, std::string_view) {
}

} // namespace

int main(int argc, char** argv) {
    spdlog::set_pattern("[%^%l%$] %v");

    const std::string exe_name = ExecutableName((argc > 0) ? argv[0] : nullptr);
    const bool requested_inline_roots = HasInlineRootArgs(argc, argv);

    try {
        // Imported libraries consume their own inline namespaces.
        kcli::demo::alpha::ProcessCLI(argc, argv);
        kcli::demo::beta::ProcessCLI(argc, argv);
        // Explicit root override: gamma's default root ("gamma") is replaced with "renamed".
        kcli::demo::gamma::ProcessCLI(argc, argv, "renamed");

        // App-defined inline namespace group (for example --build-*).
        kcli::Parser build;
        build.Initialize(argc, argv, "build");
        build.Implement("profile",
                        handleBuildProfile,
                        "Set build profile.",
                        kcli::ValueMode::Required);
        build.Implement("clean", handleBuildClean, "Enable clean build.");
        const kcli::ProcessResult build_result = build.Process();
        if (!build_result) {
            throw std::runtime_error(build_result.error_message.empty()
                                         ? "build parser failed"
                                         : build_result.error_message);
        }

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
        const kcli::ProcessResult app_result = cli.Process();
        if (!app_result) {
            throw std::runtime_error(app_result.error_message.empty()
                                         ? "application parser failed"
                                         : app_result.error_message);
        }
    } catch (const std::exception& ex) {
        spdlog::error("CLI error: {}", ex.what());
        return 2;
    }

    if (requested_inline_roots) {
        return 0;
    }

    std::cout << "\nKCLI demo omega compile/link/integration check passed\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << exe_name << " --<root>\n\n";
    std::cout << "Enabled --<root> prefixes:\n";
    std::cout << "  --alpha\n";
    std::cout << "  --beta\n";
    std::cout << "  --renamed (gamma override)\n\n";
    return 0;
}
