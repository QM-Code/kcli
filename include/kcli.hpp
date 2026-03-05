#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace kcli {

enum class Mode {
    EndUser,
    Inline,
};

enum class RootMatchMode {
    ExactOrDash,
    Prefix,
};

enum class UnknownOptionPolicy {
    Ignore,
    Consume,
    Error,
};

enum class ValueMode {
    None,
    Required,
    Optional,
};

struct ParsePolicy {
    // Inline-mode matching behavior for --<root> and --<root>-* parsing.
    RootMatchMode root_match = RootMatchMode::ExactOrDash;

    // Behavior for unknown options that look like --<root>-<something>.
    UnknownOptionPolicy unknown_dash_option = UnknownOptionPolicy::Error;

    // Behavior for unknown options that begin with root when root_match=Prefix.
    UnknownOptionPolicy unknown_prefixed_option = UnknownOptionPolicy::Ignore;

    // Whether option values reject tokens starting with '-'.
    bool reject_dash_prefixed_values = true;
};

struct HandlerContext {
    Mode mode = Mode::EndUser;
    std::string_view root{};
    std::string_view option{};
    std::string_view command{};
    // Shell-tokenized value parts consumed for this option.
    // Examples:
    // - --alpha-message "hey bud"  => {"hey bud"}
    // - --alpha-message hey bud    => {"hey", "bud"}
    std::vector<std::string_view> value_tokens{};
    bool from_alias = false;
    int option_index = -1;
};

using FlagHandler = std::function<void(const HandlerContext&)>;
using ValueHandler = std::function<void(const HandlerContext&, std::string_view)>;
using UnknownOptionHandler = std::function<void(std::string_view option)>;
using ErrorHandler = std::function<void(std::string_view message)>;
using WarningHandler = std::function<void(std::string_view message)>;

struct ProcessStats {
    int consumed_options = 0;
    int consumed_values = 0;
    int remaining_argc = 0;
};

struct ProcessResult {
    bool ok = true;
    ProcessStats stats{};
    std::string error_option{};
    std::string error_message{};

    explicit operator bool() const noexcept { return ok; }
};

class Parser {
public:
    Parser();
    ~Parser();
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&) noexcept;
    Parser& operator=(Parser&&) noexcept;

    // End-user mode (default): handles non-namespaced options.
    void Initialize(int& argc, char** argv);

    // Inline mode: handles only --<root> and --<root>-* options.
    // Root accepts either "trace" or "--trace" forms.
    void Initialize(int& argc, char** argv, std::string_view root);

    void Reset();

    Mode GetMode() const;
    bool IsInlineMode() const;
    bool IsEndUserMode() const;
    std::string GetRoot() const;

    void SetPolicy(const ParsePolicy& policy);
    ParsePolicy GetPolicy() const;

    // In EndUser mode, command maps to --<command>.
    // In Inline mode, command maps to --<root>-<command>.
    // Description is required and is used when printing inline root help.
    void Implement(std::string_view command,
                   FlagHandler handler,
                   std::string_view description);
    void Implement(std::string_view command,
                   ValueHandler handler,
                   std::string_view description,
                   ValueMode mode = ValueMode::Required);
    void Remove(std::string_view command);
    bool HasCommand(std::string_view command) const;

    // Inline mode: plain --<root> is reserved and always prints the registered
    // --<root>-<command> options with their descriptions.
    // EndUser mode: this behavior does not apply.

    // Register a short alias (for example "-p" -> "output").
    // Hard rules:
    // - alias must be single-dash form (for example "-p"), never "--...".
    // - command must already be registered via Implement() (without dashes).
    // - inline mode is not allowed to register aliases and must throw
    //   std::logic_error when called.
    bool AddAlias(std::string_view alias, std::string_view command);
    bool RemoveAlias(std::string_view alias);

    void SetUnknownOptionHandler(UnknownOptionHandler handler);
    void SetErrorHandler(ErrorHandler handler);
    void SetWarningHandler(WarningHandler handler);

    ProcessResult Process();

private:
    struct Impl;
    Impl* impl_ = nullptr;
};

}  // namespace kcli
