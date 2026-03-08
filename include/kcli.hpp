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
// Positional handlers receive remaining positional tokens in
// HandlerContext::value_tokens.
using PositionalHandler = std::function<void(const HandlerContext&)>;
using UnknownOptionHandler = std::function<void(std::string_view option)>;
using ErrorHandler = std::function<void(std::string_view message)>;
using WarningHandler = std::function<void(std::string_view message)>;

enum class FailureMode {
    Return,
    Throw,
};

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

struct SessionOptions {
    // Empty root selects end-user mode. Accepts either "build" or "--build".
    std::string_view root{};
    FailureMode failure_mode = FailureMode::Return;
    ParsePolicy policy{};
};

// Procedural parse-session API.
void Initialize(int& argc, char** argv, const SessionOptions& options = {});
bool ExpandAlias(std::string_view alias, std::string_view target);
void SetHandler(std::string_view option,
                FlagHandler handler,
                std::string_view description);
void SetHandler(std::string_view option,
                ValueHandler handler,
                std::string_view description,
                ValueMode mode = ValueMode::Required);
void SetPositionalHandler(PositionalHandler handler);
void ClearPositionalHandler();
ProcessResult Process();
ProcessResult FailOnUnknown();

}  // namespace kcli
