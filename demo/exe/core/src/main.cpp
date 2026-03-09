#include <alpha/sdk.hpp>
#include <kcli.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

std::string ExecutableName(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return "app";
    }
    return std::string(path);
}

void handleVerbose(const kcli::HandlerContext&) {
}

void handleOutput(const kcli::HandlerContext&, std::string_view) {
}

} // namespace

int main(int argc, char** argv) {
    spdlog::set_pattern("[%^%l%$] %v");

    const std::string exe_name = ExecutableName((argc > 0) ? argv[0] : nullptr);
    kcli::PrimaryParser parser;
    kcli::InlineParser alphaParser = kcli::demo::alpha::GetInlineParser();

    try {
        parser.setFailureMode(kcli::FailureMode::Throw);
        parser.addInlineParser(alphaParser);

        parser.addAlias("-v", "--verbose");
        parser.addAlias("-out", "--output");
        parser.addAlias("-a", "--alpha-enable");

        parser.setHandler("--verbose", handleVerbose, "Enable verbose app logging.");
        parser.setHandler("--output",
                          handleOutput,
                          "Set app output target.",
                          kcli::ValueMode::Required);
        parser.parse(argc, argv);
    } catch (const std::exception& ex) {
        spdlog::error("CLI error: {}", ex.what());
        return 2;
    }

    std::cout << "\nKCLI demo core compile/link/integration check passed\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << exe_name << " --alpha\n";
    std::cout << "  " << exe_name << " --output stdout\n\n";
    std::cout << "Enabled inline roots:\n";
    std::cout << "  --alpha\n\n";
    return 0;
}
