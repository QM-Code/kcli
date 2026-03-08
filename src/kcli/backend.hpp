#pragma once

#include <kcli.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kcli::backend {

struct CommandBinding {
    bool expects_value = false;
    FlagHandler flag_handler{};
    ValueHandler value_handler{};
    ValueMode value_mode = ValueMode::None;
    std::string description{};
};

struct ParseState {
    int* argc_ptr = nullptr;
    char** argv = nullptr;

    Mode mode = Mode::EndUser;
    std::string root_name{};
    ParsePolicy policy{};

    std::unordered_map<std::string, CommandBinding> commands;
    std::vector<std::string> command_order;
};

void Reset(ParseState& state);
void Initialize(ParseState& state, int& argc, char** argv);
void Initialize(ParseState& state, int& argc, char** argv, std::string_view root);
bool IsInlineMode(const ParseState& state);
std::string GetRoot(const ParseState& state);
void SetPolicy(ParseState& state, const ParsePolicy& policy);
void Implement(ParseState& state,
               std::string_view command,
               FlagHandler handler,
               std::string_view description);
void Implement(ParseState& state,
               std::string_view command,
               ValueHandler handler,
               std::string_view description,
               ValueMode mode);
ProcessResult Process(ParseState& state);

} // namespace kcli::backend
