#pragma once

#include <kcli.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kcli {

struct Parser::Impl {
    struct CommandBinding {
        bool expects_value = false;
        FlagHandler flag_handler{};
        ValueHandler value_handler{};
        ValueMode value_mode = ValueMode::None;
        std::string description{};
    };

    int* argc_ptr = nullptr;
    char** argv = nullptr;

    Mode mode = Mode::EndUser;
    std::string root_name{};
    ParsePolicy policy{};

    std::unordered_map<std::string, CommandBinding> commands;
    std::vector<std::string> command_order;
    std::unordered_map<std::string, std::string> aliases;

    UnknownOptionHandler unknown_option_handler{};
    ErrorHandler error_handler{};
    WarningHandler warning_handler{};

    void Reset();

    void Initialize(int& argc, char** argv);
    void Initialize(int& argc, char** argv, std::string_view root);

    Mode GetMode() const;
    bool IsInlineMode() const;
    bool IsEndUserMode() const;
    std::string GetRoot() const;

    void SetPolicy(const ParsePolicy& policy);
    ParsePolicy GetPolicy() const;

    void Implement(std::string_view command,
                   FlagHandler handler,
                   std::string_view description);
    void Implement(std::string_view command,
                   ValueHandler handler,
                   std::string_view description,
                   ValueMode mode);
    void Remove(std::string_view command);
    bool HasCommand(std::string_view command) const;

    bool AddAlias(std::string_view alias, std::string_view command);
    bool RemoveAlias(std::string_view alias);

    void SetUnknownOptionHandler(UnknownOptionHandler handler);
    void SetErrorHandler(ErrorHandler handler);
    void SetWarningHandler(WarningHandler handler);

    ProcessResult Process();

private:
    void emitWarning(std::string_view message) const;
    void emitUnknownOption(std::string_view option) const;
    void reportError(ProcessResult& result,
                     std::string_view option,
                     std::string_view message) const;

    bool hasBoundArgv(ProcessResult& result) const;
    void printInlineRootHelp() const;

    void consumeIndex(std::vector<bool>& consumed,
                      ProcessResult& result,
                      int index) const;
    bool dispatchCommand(const std::string& command,
                         const std::string& option_token,
                         bool from_alias,
                         int& i,
                         int argc,
                         std::vector<bool>& consumed,
                         ProcessResult& result);

    void applyUnknownPolicy(UnknownOptionPolicy policy_value,
                            const std::string& option_token,
                            int index,
                            std::vector<bool>& consumed,
                            ProcessResult& result);

    void compactArgv(std::vector<bool>& consumed, ProcessResult& result);

    ProcessResult processInline();
    ProcessResult processEndUser();
};

} // namespace kcli
