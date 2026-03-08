#include <alpha/sdk.hpp>
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

bool HasAlphaArgs(int argc, char** argv) {
    if (argc <= 1 || argv == nullptr) {
        return false;
    }
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }
        if (IsRootTokenOrSubcommand(argv[i], "alpha")) {
            return true;
        }
    }
    return false;
}

void handleVerbose(const kcli::HandlerContext&) {
}

void handleOutput(const kcli::HandlerContext&, std::string_view) {
}

} // namespace

int main(int argc, char** argv) {
    spdlog::set_pattern("[%^%l%$] %v");

    const std::string exe_name = ExecutableName((argc > 0) ? argv[0] : nullptr);
    const bool requested_alpha_args = HasAlphaArgs(argc, argv);

    try {
        kcli::demo::alpha::ProcessCLI(argc, argv);

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

    if (requested_alpha_args) {
        return 0;
    }

    std::cout << "\nKCLI demo core compile/link/integration check passed\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << exe_name << " --alpha\n";
    std::cout << "  " << exe_name << " --output stdout\n\n";
    std::cout << "Enabled inline roots:\n";
    std::cout << "  --alpha\n\n";
    return 0;
}
