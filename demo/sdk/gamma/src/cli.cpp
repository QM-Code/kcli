#include <gamma/sdk.hpp>

#include <kcli.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

bool g_strict = false;
std::string g_tag = "none";

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

void handleStrict(const kcli::HandlerContext& context, std::string_view value) {
    g_strict = true;
    PrintProcessingLine(context, value);
}

void handleTag(const kcli::HandlerContext& context, std::string_view value) {
    if (!value.empty()) {
        g_tag = std::string(value);
    }
    PrintProcessingLine(context, value);
}

} // namespace

namespace kcli::demo::gamma {

kcli::InlineParser GetInlineParser() {
    kcli::InlineParser parser("--gamma");
    parser.setHandler("-strict",
                      handleStrict,
                      "Enable strict gamma mode.",
                      kcli::ValueMode::Optional);
    parser.setHandler("-tag",
                      handleTag,
                      "Set a gamma tag label.",
                      kcli::ValueMode::Required);
    return parser;
}

void EmitDemoOutput() {
    std::cout << "[gamma] strict=" << std::boolalpha << g_strict
              << ", tag='" << g_tag << "'\n";
}

} // namespace kcli::demo::gamma
