#include <kcli.hpp>

#include "kcli/impl.hpp"

#include <stdexcept>
#include <utility>

namespace kcli {

Parser::Parser()
    : impl_(new Impl()) {
}

Parser::~Parser() {
    delete impl_;
}

Parser::Parser(Parser&& other) noexcept
    : impl_(other.impl_) {
    other.impl_ = nullptr;
}

Parser& Parser::operator=(Parser&& other) noexcept {
    if (this != &other) {
        delete impl_;
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

void Parser::Initialize(int& argc, char** argv) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->Initialize(argc, argv);
}

void Parser::Initialize(int& argc, char** argv, std::string_view root) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->Initialize(argc, argv, root);
}

void Parser::Reset() {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->Reset();
}

Mode Parser::GetMode() const {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->GetMode();
}

bool Parser::IsInlineMode() const {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->IsInlineMode();
}

bool Parser::IsEndUserMode() const {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->IsEndUserMode();
}

std::string Parser::GetRoot() const {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->GetRoot();
}

void Parser::SetPolicy(const ParsePolicy& policy) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->SetPolicy(policy);
}

ParsePolicy Parser::GetPolicy() const {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->GetPolicy();
}

void Parser::Implement(std::string_view command,
                       FlagHandler handler,
                       std::string_view description) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->Implement(command, std::move(handler), description);
}

void Parser::Implement(std::string_view command,
                       ValueHandler handler,
                       std::string_view description,
                       ValueMode mode) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->Implement(command, std::move(handler), description, mode);
}

void Parser::Remove(std::string_view command) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->Remove(command);
}

bool Parser::HasCommand(std::string_view command) const {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->HasCommand(command);
}

void Parser::SetRootValueHandler(ValueHandler handler) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->SetRootValueHandler(std::move(handler));
}

void Parser::SetRootValueHandler(ValueHandler handler,
                                 std::string_view value_usage,
                                 std::string_view description) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->SetRootValueHandler(std::move(handler), value_usage, description);
}

void Parser::ClearRootValueHandler() {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->ClearRootValueHandler();
}

bool Parser::HasRootValueHandler() const {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->HasRootValueHandler();
}

bool Parser::AddAlias(std::string_view alias, std::string_view command) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->AddAlias(alias, command);
}

bool Parser::RemoveAlias(std::string_view alias) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->RemoveAlias(alias);
}

void Parser::SetUnknownOptionHandler(UnknownOptionHandler handler) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->SetUnknownOptionHandler(std::move(handler));
}

void Parser::SetErrorHandler(ErrorHandler handler) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->SetErrorHandler(std::move(handler));
}

void Parser::SetWarningHandler(WarningHandler handler) {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    impl_->SetWarningHandler(std::move(handler));
}

ProcessResult Parser::Process() {
    if (impl_ == nullptr) {
        throw std::logic_error("kcli::Parser is in a moved-from state");
    }
    return impl_->Process();
}

} // namespace kcli
