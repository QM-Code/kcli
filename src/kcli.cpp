#include <kcli.hpp>

#include "kcli/backend.hpp"

#include <utility>

namespace kcli {

CliError::CliError(std::string option, std::string message, ProcessStats stats)
    : std::runtime_error(message.empty() ? "kcli parse failed" : message),
      option_(std::move(option)),
      stats_(stats) {
}

CliError::~CliError() = default;

std::string_view CliError::option() const noexcept {
    return option_;
}

const ProcessStats& CliError::stats() const noexcept {
    return stats_;
}

InlineParser::InlineParser(std::string_view root)
    : data_(std::make_unique<detail::InlineParserData>()) {
    detail::SetInlineRoot(*data_, root);
}

InlineParser::InlineParser(const InlineParser& other)
    : data_(std::make_unique<detail::InlineParserData>(
          detail::CloneInlineParserData(*other.data_))) {
}

InlineParser& InlineParser::operator=(const InlineParser& other) {
    if (this == &other) {
        return *this;
    }

    *data_ = detail::CloneInlineParserData(*other.data_);
    return *this;
}

InlineParser::InlineParser(InlineParser&& other) noexcept = default;

InlineParser& InlineParser::operator=(InlineParser&& other) noexcept = default;

InlineParser::~InlineParser() = default;

void InlineParser::setRoot(std::string_view root) {
    detail::SetInlineRoot(*data_, root);
}

void InlineParser::setRootValueHandler(ValueHandler handler) {
    detail::SetRootValueHandler(*data_, std::move(handler));
}

void InlineParser::setHandler(std::string_view option,
                              FlagHandler handler,
                              std::string_view description) {
    detail::SetInlineHandler(*data_, option, std::move(handler), description);
}

void InlineParser::setHandler(std::string_view option,
                              ValueHandler handler,
                              std::string_view description,
                              ValueMode mode) {
    detail::SetInlineHandler(*data_, option, std::move(handler), description, mode);
}

PrimaryParser::PrimaryParser()
    : data_(std::make_unique<detail::PrimaryParserData>()) {
}

PrimaryParser::PrimaryParser(PrimaryParser&& other) noexcept = default;

PrimaryParser& PrimaryParser::operator=(PrimaryParser&& other) noexcept = default;

PrimaryParser::~PrimaryParser() = default;

void PrimaryParser::addAlias(std::string_view alias, std::string_view target) {
    detail::SetAlias(*data_, alias, target);
}

void PrimaryParser::setHandler(std::string_view option,
                               FlagHandler handler,
                               std::string_view description) {
    detail::SetPrimaryHandler(*data_, option, std::move(handler), description);
}

void PrimaryParser::setHandler(std::string_view option,
                               ValueHandler handler,
                               std::string_view description,
                               ValueMode mode) {
    detail::SetPrimaryHandler(*data_, option, std::move(handler), description, mode);
}

void PrimaryParser::setPositionalHandler(PositionalHandler handler) {
    detail::SetPositionalHandler(*data_, std::move(handler));
}

void PrimaryParser::addInlineParser(InlineParser parser) {
    detail::AddInlineParser(*data_, detail::CloneInlineParserData(*parser.data_));
}

ProcessStats PrimaryParser::parse(int& argc, char** argv) {
    return detail::Parse(*data_, argc, argv);
}

}  // namespace kcli
