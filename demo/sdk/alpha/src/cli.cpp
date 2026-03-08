#include <alpha/sdk.hpp>

#include <kcli.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string g_message = "alpha-default";
bool g_enabled = false;

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

void handleMessage(const kcli::HandlerContext& context, std::string_view value) {
    if (!value.empty()) {
        g_message = std::string(value);
    }
    PrintProcessingLine(context, value);
}

void handleEnable(const kcli::HandlerContext& context, std::string_view value) {
    g_enabled = true;
    PrintProcessingLine(context, value);
}

} // namespace

namespace kcli::demo::alpha {

ProcessResult ProcessCLI(int& argc, char** argv, const SessionOptions& options) {
    SessionOptions effective_options = options;
    if (effective_options.root.empty()) {
        effective_options.root = "--alpha";
    }

    kcli::Initialize(argc, argv, effective_options);
    kcli::SetHandler("-message",
                     handleMessage,
                     "Set the alpha message label.",
                     kcli::ValueMode::Required);
    kcli::SetHandler("-enable",
                     handleEnable,
                     "Enable alpha processing.",
                     kcli::ValueMode::Optional);
    return kcli::Process();
}

void EmitDemoOutput() {
    std::cout << "[alpha] enabled=" << std::boolalpha << g_enabled
              << ", message='" << g_message << "'\n";
}

} // namespace kcli::demo::alpha
