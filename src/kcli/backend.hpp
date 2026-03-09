#pragma once

#include <kcli.hpp>

#include <deque>
#include <string>
#include <vector>

namespace kcli::detail {

struct CommandBinding {
    bool expects_value = false;
    FlagHandler flag_handler{};
    ValueHandler value_handler{};
    ValueMode value_mode = ValueMode::None;
    std::string description{};
};

struct AliasBinding {
    std::string alias{};
    std::string target{};
};

struct InlineParserData {
    std::string root_name{};
    ValueHandler root_value_handler{};
    std::vector<std::pair<std::string, CommandBinding>> commands{};
};

struct PrimaryParserData {
    FailureMode failure_mode = FailureMode::Return;
    PositionalHandler positional_handler{};
    std::deque<std::string> owned_tokens{};
    std::vector<AliasBinding> aliases{};
    std::vector<std::pair<std::string, CommandBinding>> commands{};
    std::vector<InlineParserData> inline_parsers{};
};

InlineParserData CloneInlineParserData(const InlineParserData& data);

void SetInlineRoot(InlineParserData& data, std::string_view root);
void SetRootValueHandler(InlineParserData& data, ValueHandler handler);
void SetInlineHandler(InlineParserData& data,
                      std::string_view option,
                      FlagHandler handler,
                      std::string_view description);
void SetInlineHandler(InlineParserData& data,
                      std::string_view option,
                      ValueHandler handler,
                      std::string_view description,
                      ValueMode mode);

void SetFailureMode(PrimaryParserData& data, FailureMode failure_mode);
void SetAlias(PrimaryParserData& data, std::string_view alias, std::string_view target);
void SetPrimaryHandler(PrimaryParserData& data,
                       std::string_view option,
                       FlagHandler handler,
                       std::string_view description);
void SetPrimaryHandler(PrimaryParserData& data,
                       std::string_view option,
                       ValueHandler handler,
                       std::string_view description,
                       ValueMode mode);
void SetPositionalHandler(PrimaryParserData& data, PositionalHandler handler);
void AddInlineParser(PrimaryParserData& data, InlineParserData parser);
ProcessResult Parse(PrimaryParserData& data, int& argc, char** argv);

}  // namespace kcli::detail
