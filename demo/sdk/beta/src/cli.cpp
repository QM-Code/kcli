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

ProcessResult ProcessCLI(int& argc, char** argv, const SessionOptions& options) {
    SessionOptions effective_options = options;
    if (effective_options.root.empty()) {
        effective_options.root = "--beta";
    }

    kcli::Initialize(argc, argv, effective_options);
    kcli::SetHandler("-profile",
                     handleProfile,
                     "Select beta runtime profile.",
                     kcli::ValueMode::Required);
    kcli::SetHandler("-workers",
                     handleWorkers,
                     "Set beta worker count.",
                     kcli::ValueMode::Required);
    return kcli::Process();
}

void EmitDemoOutput() {
    std::cout << "[beta] profile='" << g_profile << "', workers=" << g_workers << "\n";
}

} // namespace kcli::demo::beta
