#include <kcli.hpp>

#include "kcli/backend.hpp"

#include <utility>

namespace kcli {

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

void PrimaryParser::setFailureMode(FailureMode failure_mode) {
    detail::SetFailureMode(*data_, failure_mode);
}

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

ProcessResult PrimaryParser::parse(int& argc, char** argv) {
    return detail::Parse(*data_, argc, argv);
}

}  // namespace kcli
