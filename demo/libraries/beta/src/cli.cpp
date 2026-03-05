#include <beta/sdk.hpp>

#include <kcli.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string g_profile = "safe";
int g_workers = 2;

void PrintProcessingLine(const kcli::HandlerContext& context, std::string_view value) {
    if (context.value_tokens.empty()) {
        std::cout << "Processing " << context.option << "\n";
        return;
    }

    if (context.value_tokens.size() == 1) {
        std::cout << "Processing " << context.option
                  << " with value \"" << value << "\"\n";
        return;
    }

    std::cout << "Processing " << context.option << " with values [";
    for (std::size_t i = 0; i < context.value_tokens.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "\"" << context.value_tokens[i] << "\"";
    }
    std::cout << "]\n";
}

void handleProfile(const kcli::HandlerContext& context, std::string_view value) {
    if (!value.empty()) {
        g_profile = std::string(value);
    }
    PrintProcessingLine(context, value);
}

void handleWorkers(const kcli::HandlerContext& context, std::string_view value) {
    if (!value.empty()) {
        g_workers = std::stoi(std::string(value));
    }
    PrintProcessingLine(context, value);
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
    const kcli::ProcessResult result = cli.Process();
    if (!result) {
        const std::string message = result.error_message.empty()
                                        ? "beta CLI parse failed"
                                        : ("beta CLI parse error: " + result.error_message);
        throw std::runtime_error(message);
    }
}

void EmitDemoOutput() {
    std::cout << "[beta] profile='" << g_profile << "', workers=" << g_workers << "\n";
}

} // namespace kcli::demo::beta
