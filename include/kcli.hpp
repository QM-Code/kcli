#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kcli {

enum class ValueMode {
    None,
    Required,
    Optional,
};

struct HandlerContext {
    std::string_view root{};
    std::string_view option{};
    std::string_view command{};
    // Raw shell tokens consumed for this handler, preserved verbatim.
    // For optional-value handlers, an omitted value leaves this empty;
    // an explicit empty-string argument contributes one empty token.
    std::vector<std::string_view> value_tokens{};
    bool from_alias = false;
    int option_index = -1;
};

using FlagHandler = std::function<void(const HandlerContext&)>;
using ValueHandler = std::function<void(const HandlerContext&, std::string_view)>;
// Positional handlers receive remaining positional tokens in
// HandlerContext::value_tokens.
using PositionalHandler = std::function<void(const HandlerContext&)>;

class CliError : public std::runtime_error {
public:
    CliError(std::string option, std::string message);
    ~CliError() override;

    std::string_view option() const noexcept;

private:
    std::string option_;
};

namespace detail {
struct InlineParserData;
struct PrimaryParserData;
}  // namespace detail

class InlineParser {
public:
    explicit InlineParser(std::string_view root);
    InlineParser(const InlineParser& other);
    InlineParser& operator=(const InlineParser& other);
    InlineParser(InlineParser&& other) noexcept;
    InlineParser& operator=(InlineParser&& other) noexcept;
    ~InlineParser();

    void setRoot(std::string_view root);

    void setRootValueHandler(ValueHandler handler);
    void setRootValueHandler(ValueHandler handler,
                             std::string_view value_placeholder,
                             std::string_view description);

    void setHandler(std::string_view option,
                    FlagHandler handler,
                    std::string_view description);
    void setHandler(std::string_view option,
                    ValueHandler handler,
                    std::string_view description,
                    ValueMode mode = ValueMode::Required);

private:
    std::unique_ptr<detail::InlineParserData> data_;

    friend class PrimaryParser;
};

class PrimaryParser {
public:
    PrimaryParser();
    PrimaryParser(const PrimaryParser&) = delete;
    PrimaryParser& operator=(const PrimaryParser&) = delete;
    PrimaryParser(PrimaryParser&& other) noexcept;
    PrimaryParser& operator=(PrimaryParser&& other) noexcept;
    ~PrimaryParser();

    void addAlias(std::string_view alias, std::string_view target);

    void setHandler(std::string_view option,
                    FlagHandler handler,
                    std::string_view description);
    void setHandler(std::string_view option,
                    ValueHandler handler,
                    std::string_view description,
                    ValueMode mode = ValueMode::Required);

    void setPositionalHandler(PositionalHandler handler);

    void addInlineParser(InlineParser parser);

    void parse(int argc, char* const* argv);
    void parseOrThrow(int argc, char* const* argv);

private:
    std::unique_ptr<detail::PrimaryParserData> data_;
};

}  // namespace kcli
