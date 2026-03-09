#include <kcli.hpp>

#include <functional>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

namespace {

std::vector<std::string> CopyTokens(const std::vector<std::string_view>& tokens) {
    std::vector<std::string> copied;
    copied.reserve(tokens.size());
    for (std::string_view token : tokens) {
        copied.emplace_back(token);
    }
    return copied;
}

std::string DescribeValue(bool value) {
    return value ? "true" : "false";
}

std::string DescribeValue(int value) {
    return std::to_string(value);
}

std::string DescribeValue(const std::string& value) {
    return "\"" + value + "\"";
}

std::string DescribeValue(std::string_view value) {
    return "\"" + std::string(value) + "\"";
}

std::string DescribeValue(const std::vector<std::string>& values) {
    std::ostringstream stream;
    stream << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            stream << ", ";
        }
        stream << DescribeValue(values[i]);
    }
    stream << "]";
    return stream.str();
}

template <typename T>
std::string DescribeValue(const T& value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

class TestContext {
public:
    void Expect(bool condition, std::string_view message) {
        if (condition) {
            return;
        }

        ++failures_;
        std::cerr << "FAIL: " << message << "\n";
    }

    template <typename T, typename U>
    void ExpectEq(const T& actual, const U& expected, std::string_view message) {
        if (actual == expected) {
            return;
        }

        ++failures_;
        std::cerr << "FAIL: " << message << "\n"
                  << "  expected: " << DescribeValue(expected) << "\n"
                  << "  actual:   " << DescribeValue(actual) << "\n";
    }

    void ExpectContains(std::string_view actual,
                        std::string_view needle,
                        std::string_view message) {
        if (actual.find(needle) != std::string_view::npos) {
            return;
        }

        ++failures_;
        std::cerr << "FAIL: " << message << "\n"
                  << "  missing:  " << DescribeValue(needle) << "\n"
                  << "  actual:   " << DescribeValue(actual) << "\n";
    }

    template <typename ExceptionT, typename Fn>
    void ExpectThrowContains(Fn&& fn,
                             std::string_view needle,
                             std::string_view message) {
        try {
            fn();
        } catch (const ExceptionT& ex) {
            const std::string_view actual(ex.what());
            if (actual.find(needle) != std::string_view::npos) {
                return;
            }

            ++failures_;
            std::cerr << "FAIL: " << message << "\n"
                      << "  expected exception containing: " << DescribeValue(needle) << "\n"
                      << "  actual exception:             " << DescribeValue(actual) << "\n";
            return;
        } catch (const std::exception& ex) {
            ++failures_;
            std::cerr << "FAIL: " << message << "\n"
                      << "  expected exception type: " << typeid(ExceptionT).name() << "\n"
                      << "  actual exception:       " << DescribeValue(std::string_view(ex.what()))
                      << "\n";
            return;
        } catch (...) {
            ++failures_;
            std::cerr << "FAIL: " << message << "\n"
                      << "  actual exception: non-std::exception\n";
            return;
        }

        ++failures_;
        std::cerr << "FAIL: " << message << "\n"
                  << "  expected exception containing: " << DescribeValue(needle) << "\n"
                  << "  actual exception: none\n";
    }

    int failures() const {
        return failures_;
    }

private:
    int failures_ = 0;
};

struct ArgvFixture {
    std::vector<std::string> storage;
    std::vector<char*> argv;
    int argc = 0;

    ArgvFixture(std::initializer_list<std::string_view> tokens) {
        storage.reserve(tokens.size());
        for (std::string_view token : tokens) {
            storage.emplace_back(token);
        }
        Rebuild();
    }

    char** data() {
        return argv.data();
    }

    std::vector<std::string> CurrentTokens() const {
        std::vector<std::string> tokens;
        tokens.reserve(static_cast<std::size_t>(argc));
        for (int i = 0; i < argc; ++i) {
            tokens.emplace_back(argv[static_cast<std::size_t>(i)]);
        }
        return tokens;
    }

private:
    void Rebuild() {
        argv.clear();
        argv.reserve(storage.size() + 1);
        for (std::string& token : storage) {
            argv.push_back(token.data());
        }
        argv.push_back(nullptr);
        argc = static_cast<int>(storage.size());
    }
};

class CaptureStdout {
public:
    CaptureStdout() : previous_(std::cout.rdbuf(stream_.rdbuf())) {
    }

    ~CaptureStdout() {
        std::cout.rdbuf(previous_);
    }

    std::string str() const {
        return stream_.str();
    }

private:
    std::ostringstream stream_;
    std::streambuf* previous_ = nullptr;
};

void CaseProcessRequiresInitialize(TestContext& t) {
    t.ExpectThrowContains<std::logic_error>(
        [] {
            (void)kcli::Process();
        },
        "Initialize",
        "Process() should require Initialize()");

    t.ExpectThrowContains<std::logic_error>(
        [] {
            (void)kcli::FailOnUnknown();
        },
        "Initialize",
        "FailOnUnknown() should require Initialize()");
}

void CaseInitializeRejectsInvalidRoot(TestContext& t) {
    ArgvFixture args{"prog"};

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            kcli::Initialize(args.argc, args.data(), {.root = "-build"});
        },
        "root must use '--root' or 'root'",
        "Initialize() should reject single-dash roots");
}

void CaseEndUserKnownOptionsAndArgcCompaction(TestContext& t) {
    ArgvFixture args{"prog", "--verbose", "pos1", "--output", "stdout", "--bogus", "pos2"};

    bool verbose = false;
    std::string output;
    std::vector<std::string> positionals;

    kcli::Initialize(args.argc, args.data());
    kcli::SetHandler("verbose",
                     [&](const kcli::HandlerContext&) {
                         verbose = true;
                     },
                     "Enable verbose logging.");
    kcli::SetHandler("output",
                     [&](const kcli::HandlerContext&, std::string_view value) {
                         output = std::string(value);
                     },
                     "Set output target.",
                     kcli::ValueMode::Required);
    kcli::SetPositionalHandler(
        [&](const kcli::HandlerContext& context) {
            positionals = CopyTokens(context.value_tokens);
        });

    const kcli::ProcessResult process_result = kcli::Process();

    t.Expect(process_result.ok, "Process() should succeed for known end-user options");
    t.Expect(verbose, "flag handler should run");
    t.ExpectEq(output, std::string("stdout"), "value handler should receive the joined value");
    t.ExpectEq(positionals,
               std::vector<std::string>{"pos1", "pos2"},
               "positional handler should receive remaining positional tokens");
    t.ExpectEq(process_result.stats.consumed_options,
               2,
               "Process() should count consumed end-user options");
    t.ExpectEq(process_result.stats.consumed_values,
               1,
               "Process() should count consumed option value tokens");
    t.ExpectEq(process_result.stats.remaining_argc,
               2,
               "Process() should leave only the unknown option token");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--bogus"},
               "argv should be compacted after processing");

    const kcli::ProcessResult unknown_result = kcli::FailOnUnknown();
    t.Expect(!unknown_result.ok, "FailOnUnknown() should reject leftover option-like tokens");
    t.ExpectEq(unknown_result.error_option,
               std::string("--bogus"),
               "FailOnUnknown() should identify the leftover option");
}

void CaseInitializeClearsPositionalHandlerBetweenPhases(TestContext& t) {
    ArgvFixture args{"prog", "tail"};

    bool called = false;

    kcli::Initialize(args.argc, args.data());
    kcli::SetPositionalHandler(
        [&](const kcli::HandlerContext&) {
            called = true;
        });

    kcli::Initialize(args.argc, args.data());
    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "re-initialized session should still process cleanly");
    t.Expect(!called, "Initialize() should clear the old positional handler");
    t.ExpectEq(result.stats.remaining_argc,
               2,
               "without a positional handler, positional tokens should remain");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "tail"},
               "argv should remain unchanged when no handler consumes positionals");
}

void CaseExpandAliasRewritesTokens(TestContext& t) {
    ArgvFixture args{"prog", "-v", "tail"};

    bool verbose = false;

    kcli::Initialize(args.argc, args.data());
    const bool changed = kcli::ExpandAlias("-v", "--verbose");

    t.Expect(changed, "ExpandAlias() should report a replacement");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--verbose", "tail"},
               "ExpandAlias() should rewrite matching tokens");

    kcli::SetHandler("--verbose",
                     [&](const kcli::HandlerContext&) {
                         verbose = true;
                     },
                     "Enable verbose logging.");

    const kcli::ProcessResult result = kcli::Process();
    t.Expect(result.ok, "rewritten alias token should be parsed normally");
    t.Expect(verbose, "rewritten alias should trigger the registered handler");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "tail"},
               "processed alias token should be removed from argv");
}

void CaseExpandAliasRewritesAfterDoubleDash(TestContext& t) {
    ArgvFixture args{"prog", "--", "-v"};

    bool verbose = false;

    kcli::Initialize(args.argc, args.data());
    const bool changed = kcli::ExpandAlias("-v", "--verbose");

    t.Expect(changed, "ExpandAlias() should no longer stop at '--'");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--", "--verbose"},
               "ExpandAlias() should rewrite tokens after '--'");

    kcli::SetHandler("--verbose",
                     [&](const kcli::HandlerContext&) {
                         verbose = true;
                     },
                     "Enable verbose logging.");

    const kcli::ProcessResult result = kcli::Process();
    t.Expect(result.ok, "known options after '--' should still be parsed");
    t.Expect(verbose, "rewritten token after '--' should still dispatch");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--"},
               "only the literal '--' token should remain");

    const kcli::ProcessResult unknown_result = kcli::FailOnUnknown();
    t.Expect(!unknown_result.ok, "FailOnUnknown() should treat '--' as an unknown option token");
    t.ExpectEq(unknown_result.error_option,
               std::string("--"),
               "FailOnUnknown() should report the literal '--' token");
}

void CaseExpandAliasRejectsInvalidAlias(TestContext& t) {
    ArgvFixture args{"prog"};
    kcli::Initialize(args.argc, args.data());

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            (void)kcli::ExpandAlias("--verbose", "--output");
        },
        "single-dash form",
        "ExpandAlias() should reject invalid alias syntax");
}

void CaseExpandAliasRejectsInvalidTarget(TestContext& t) {
    ArgvFixture args{"prog"};
    kcli::Initialize(args.argc, args.data());

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            (void)kcli::ExpandAlias("-v", "--bad target");
        },
        "single CLI token",
        "ExpandAlias() should reject alias targets with whitespace");
}

void CaseExpandAliasNotAllowedInline(TestContext& t) {
    ArgvFixture args{"prog"};
    kcli::Initialize(args.argc, args.data(), {.root = "build"});

    t.ExpectThrowContains<std::logic_error>(
        [&] {
            (void)kcli::ExpandAlias("-v", "--verbose");
        },
        "only valid in end-user mode",
        "ExpandAlias() should be rejected in inline mode");
}

void CaseSetRootValueHandlerRequiresInlineAndNonEmpty(TestContext& t) {
    ArgvFixture args{"prog"};

    kcli::Initialize(args.argc, args.data());
    t.ExpectThrowContains<std::logic_error>(
        [&] {
            kcli::SetRootValueHandler(
                [&](const kcli::HandlerContext&, std::string_view) {
                });
        },
        "only valid in inline mode",
        "SetRootValueHandler() should be rejected in end-user mode");

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            kcli::SetRootValueHandler(kcli::ValueHandler{});
        },
        "must not be empty",
        "SetRootValueHandler() should reject empty handlers");
}

void CaseSetPositionalHandlerRequiresEndUserAndNonEmpty(TestContext& t) {
    ArgvFixture args{"prog"};

    kcli::Initialize(args.argc, args.data());
    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            kcli::SetPositionalHandler(kcli::PositionalHandler{});
        },
        "must not be empty",
        "SetPositionalHandler() should reject empty handlers");

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    t.ExpectThrowContains<std::logic_error>(
        [&] {
            kcli::SetPositionalHandler(
                [&](const kcli::HandlerContext&) {
                });
        },
        "not allowed in inline mode",
        "SetPositionalHandler() should be rejected in inline mode");
}

void CaseEndUserHandlerNormalizationRejectsSingleDash(TestContext& t) {
    ArgvFixture args{"prog"};
    kcli::Initialize(args.argc, args.data());

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            kcli::SetHandler("-verbose",
                             [&](const kcli::HandlerContext&) {
                             },
                             "Enable verbose logging.");
        },
        "end-user handler option must use '--name' or 'name'",
        "end-user handlers should reject single-dash option names");
}

void CaseInlineHandlerNormalizationAcceptsShortAndFullForms(TestContext& t) {
    ArgvFixture args{"prog", "--build-flag", "--build-value", "data"};

    bool flag = false;
    std::string value;

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    kcli::SetHandler("-flag",
                     [&](const kcli::HandlerContext&) {
                         flag = true;
                     },
                     "Enable build flag.");
    kcli::SetHandler("--build-value",
                     [&](const kcli::HandlerContext&, std::string_view raw_value) {
                         value = std::string(raw_value);
                     },
                     "Set build value.",
                     kcli::ValueMode::Required);

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "inline handlers should accept short and fully-qualified registration forms");
    t.Expect(flag, "short-form inline handler should dispatch");
    t.ExpectEq(value,
               std::string("data"),
               "fully-qualified inline handler should dispatch");
    t.ExpectEq(result.stats.remaining_argc, 1, "all known inline tokens should be consumed");
}

void CaseInlineHandlerNormalizationRejectsWrongRoot(TestContext& t) {
    ArgvFixture args{"prog"};
    kcli::Initialize(args.argc, args.data(), {.root = "--build"});

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            kcli::SetHandler("--other-flag",
                             [&](const kcli::HandlerContext&) {
                             },
                             "Enable other flag.");
        },
        "inline handler option must use '-name' or '--build-name'",
        "inline handlers should reject mismatched fully-qualified roots");
}

void CaseInlineBareRootPrintsHelp(TestContext& t) {
    ArgvFixture args{"prog", "--build"};

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    kcli::SetHandler("-flag",
                     [&](const kcli::HandlerContext&) {
                     },
                     "Enable build flag.");
    kcli::SetHandler("-value",
                     [&](const kcli::HandlerContext&, std::string_view) {
                     },
                     "Set build value.",
                     kcli::ValueMode::Required);

    CaptureStdout capture;
    const kcli::ProcessResult result = kcli::Process();
    const std::string output = capture.str();

    t.Expect(result.ok, "bare inline root should print help instead of failing");
    t.ExpectEq(result.stats.consumed_options,
               1,
               "bare inline root should count as a consumed option");
    t.ExpectEq(result.stats.remaining_argc,
               1,
               "bare inline root should be removed from argv");
    t.ExpectContains(output,
                     "Available --build-* options:",
                     "bare inline root should print the option listing header");
    t.ExpectContains(output,
                     "--build-flag",
                     "bare inline root help should include registered flag options");
    t.ExpectContains(output,
                     "--build-value <value>",
                     "bare inline root help should include registered value options");
}

void CaseInlineRootValueHandlerJoinsTokens(TestContext& t) {
    ArgvFixture args{"prog", "--build", "fast", "mode"};

    std::string received_value;
    std::vector<std::string> received_tokens;
    std::string received_option;
    int option_index = -1;

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    kcli::SetRootValueHandler(
        [&](const kcli::HandlerContext& context, std::string_view value) {
            received_value = std::string(value);
            received_tokens = CopyTokens(context.value_tokens);
            received_option = std::string(context.option);
            option_index = context.option_index;
        });

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "root value handler should accept bare root values");
    t.ExpectEq(received_value,
               std::string("fast mode"),
               "root value handler should receive joined value text");
    t.ExpectEq(received_tokens,
               std::vector<std::string>{"fast", "mode"},
               "root value handler should receive tokenized parts");
    t.ExpectEq(received_option,
               std::string("--build"),
               "root value handler should report the root option token");
    t.ExpectEq(option_index,
               1,
               "root value handler should report the option index");
    t.ExpectEq(result.stats.consumed_options,
               1,
               "root value processing should count the root option");
    t.ExpectEq(result.stats.consumed_values,
               2,
               "root value processing should count consumed value tokens");
    t.ExpectEq(result.stats.remaining_argc,
               1,
               "root value processing should remove consumed tokens");
}

void CaseInlineMissingRootValueHandlerErrors(TestContext& t) {
    ArgvFixture args{"prog", "--build", "fast"};

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    const kcli::ProcessResult result = kcli::Process();

    t.Expect(!result.ok, "root values should error when no root value handler is registered");
    t.ExpectEq(result.error_option,
               std::string("--build"),
               "root value errors should identify the root option");
    t.ExpectContains(result.error_message,
                     "unknown value for option '--build'",
                     "root value errors should explain the missing handler");
    t.ExpectEq(result.stats.remaining_argc,
               1,
               "the failing root/value pair should still be consumed");
}

void CaseOptionalValueModeAllowsMissingValue(TestContext& t) {
    ArgvFixture args{"prog", "--build-enable"};

    bool called = false;
    std::string received_value;
    std::vector<std::string> received_tokens;

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    kcli::SetHandler("-enable",
                     [&](const kcli::HandlerContext& context, std::string_view value) {
                         called = true;
                         received_value = std::string(value);
                         received_tokens = CopyTokens(context.value_tokens);
                     },
                     "Enable build mode.",
                     kcli::ValueMode::Optional);

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "optional value handlers should accept missing values");
    t.Expect(called, "optional value handler should still run");
    t.ExpectEq(received_value,
               std::string(),
               "optional value handler should receive an empty joined value");
    t.ExpectEq(received_tokens,
               std::vector<std::string>{},
               "optional value handler should receive no token parts when omitted");
    t.ExpectEq(result.stats.remaining_argc,
               1,
               "optional value option should still be consumed");
}

void CaseValueModeNoneDoesNotConsumeFollowingTokens(TestContext& t) {
    ArgvFixture args{"prog", "--build-meta", "data"};

    bool called = false;
    std::string received_value;

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    kcli::SetHandler("-meta",
                     [&](const kcli::HandlerContext&, std::string_view value) {
                         called = true;
                         received_value = std::string(value);
                     },
                     "Record metadata.",
                     kcli::ValueMode::None);

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "ValueMode::None handlers should parse successfully");
    t.Expect(called, "ValueMode::None handler should run");
    t.ExpectEq(received_value,
               std::string(),
               "ValueMode::None handler should receive an empty value");
    t.ExpectEq(result.stats.consumed_values,
               0,
               "ValueMode::None should not consume following tokens");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "data"},
               "ValueMode::None should leave the following token untouched");
}

void CaseRequiredValueModeRejectsMissingValue(TestContext& t) {
    ArgvFixture args{"prog", "--build-value"};

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    kcli::SetHandler("-value",
                     [&](const kcli::HandlerContext&, std::string_view) {
                     },
                     "Set build value.",
                     kcli::ValueMode::Required);

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(!result.ok, "required value handlers should reject missing values");
    t.ExpectEq(result.error_option,
               std::string("--build-value"),
               "required value errors should identify the failing option");
    t.ExpectContains(result.error_message,
                     "requires a value",
                     "required value errors should explain the missing value");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "failing required options should still be removed from argv");
}

void CaseRejectDashPrefixedValuesRejectsFirstValue(TestContext& t) {
    ArgvFixture args{"prog", "--build-value", "-debug"};

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    kcli::SetHandler("-value",
                     [&](const kcli::HandlerContext&, std::string_view) {
                     },
                     "Set build value.",
                     kcli::ValueMode::Required);

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(!result.ok, "dash-prefixed values should be rejected by default");
    t.ExpectContains(result.error_message,
                     "requires a value",
                     "rejected dash-prefixed values should surface as missing values");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "-debug"},
               "rejected dash-prefixed values should remain in argv");
}

void CaseRejectDashPrefixedValuesFalseAcceptsFirstValue(TestContext& t) {
    ArgvFixture args{"prog", "--build-value", "-debug"};

    std::string value;

    kcli::Initialize(args.argc,
                     args.data(),
                     {.root = "build",
                      .policy = {.reject_dash_prefixed_values = false}});
    kcli::SetHandler("-value",
                     [&](const kcli::HandlerContext&, std::string_view raw_value) {
                         value = std::string(raw_value);
                     },
                     "Set build value.",
                     kcli::ValueMode::Required);

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "policy should allow dash-prefixed values when disabled");
    t.ExpectEq(value,
               std::string("-debug"),
               "handler should receive the dash-prefixed value");
    t.ExpectEq(result.stats.remaining_argc,
               1,
               "accepted dash-prefixed value should be consumed");
}

void CaseUnknownDashOptionPolicyIgnorePreservesToken(TestContext& t) {
    ArgvFixture args{"prog", "--build-unknown"};

    kcli::Initialize(args.argc,
                     args.data(),
                     {.root = "build",
                      .policy = {.unknown_dash_option = kcli::UnknownOptionPolicy::Ignore}});
    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "UnknownOptionPolicy::Ignore should not fail");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--build-unknown"},
               "UnknownOptionPolicy::Ignore should preserve the unknown token");
}

void CaseUnknownDashOptionPolicyConsumeRemovesToken(TestContext& t) {
    ArgvFixture args{"prog", "--build-unknown"};

    kcli::Initialize(args.argc,
                     args.data(),
                     {.root = "build",
                      .policy = {.unknown_dash_option = kcli::UnknownOptionPolicy::Consume}});
    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "UnknownOptionPolicy::Consume should not fail");
    t.ExpectEq(result.stats.consumed_options,
               1,
               "UnknownOptionPolicy::Consume should count the consumed token");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "UnknownOptionPolicy::Consume should remove the unknown token");
}

void CaseUnknownDashOptionPolicyErrorReportsHelpHint(TestContext& t) {
    ArgvFixture args{"prog", "--build-unknown"};

    kcli::Initialize(args.argc, args.data(), {.root = "build"});
    const kcli::ProcessResult result = kcli::Process();

    t.Expect(!result.ok, "UnknownOptionPolicy::Error should fail");
    t.ExpectEq(result.error_option,
               std::string("--build-unknown"),
               "UnknownOptionPolicy::Error should identify the token");
    t.ExpectContains(result.error_message,
                     "use --build to list options",
                     "UnknownOptionPolicy::Error should include the inline help hint");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "UnknownOptionPolicy::Error should still remove the unknown token");
}

void CasePrefixRootMatchIgnorePreservesToken(TestContext& t) {
    ArgvFixture args{"prog", "--builder"};

    kcli::Initialize(
        args.argc,
        args.data(),
        {.root = "build",
         .policy = {.root_match = kcli::RootMatchMode::Prefix,
                    .unknown_prefixed_option = kcli::UnknownOptionPolicy::Ignore}});
    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "prefix-mode ignore policy should not fail");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--builder"},
               "prefix-mode ignore policy should preserve the token");
}

void CasePrefixRootMatchConsumeRemovesToken(TestContext& t) {
    ArgvFixture args{"prog", "--builder"};

    kcli::Initialize(
        args.argc,
        args.data(),
        {.root = "build",
         .policy = {.root_match = kcli::RootMatchMode::Prefix,
                    .unknown_prefixed_option = kcli::UnknownOptionPolicy::Consume}});
    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "prefix-mode consume policy should not fail");
    t.ExpectEq(result.stats.consumed_options,
               1,
               "prefix-mode consume policy should count the token as consumed");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "prefix-mode consume policy should remove the token");
}

void CasePrefixRootMatchErrorReportsToken(TestContext& t) {
    ArgvFixture args{"prog", "--builder"};

    kcli::Initialize(
        args.argc,
        args.data(),
        {.root = "build",
         .policy = {.root_match = kcli::RootMatchMode::Prefix,
                    .unknown_prefixed_option = kcli::UnknownOptionPolicy::Error}});
    const kcli::ProcessResult result = kcli::Process();

    t.Expect(!result.ok, "prefix-mode error policy should fail");
    t.ExpectEq(result.error_option,
               std::string("--builder"),
               "prefix-mode error policy should identify the token");
    t.ExpectContains(result.error_message,
                     "use --build to list options",
                     "prefix-mode error policy should include the inline help hint");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "prefix-mode error policy should remove the token");
}

void CaseFailOnUnknownReturnModeReportsDoubleDash(TestContext& t) {
    ArgvFixture args{"prog", "--"};

    kcli::Initialize(args.argc, args.data());
    const kcli::ProcessResult process_result = kcli::Process();
    const kcli::ProcessResult unknown_result = kcli::FailOnUnknown();

    t.Expect(process_result.ok, "Process() should leave unknown tokens for FailOnUnknown()");
    t.Expect(!unknown_result.ok, "FailOnUnknown() should reject literal '--'");
    t.ExpectEq(unknown_result.error_option,
               std::string("--"),
               "FailOnUnknown() should report literal '--' as the unknown token");
}

void CaseFailOnUnknownThrowModeThrows(TestContext& t) {
    ArgvFixture args{"prog", "--bogus"};

    kcli::Initialize(args.argc, args.data(), {.failure_mode = kcli::FailureMode::Throw});
    const kcli::ProcessResult result = kcli::Process();
    t.Expect(result.ok, "Process() should still ignore unknown end-user options");

    t.ExpectThrowContains<std::runtime_error>(
        [] {
            (void)kcli::FailOnUnknown();
        },
        "unknown option --bogus",
        "FailOnUnknown() should throw in FailureMode::Throw");
}

void CaseInlineProcessThrowModeThrows(TestContext& t) {
    ArgvFixture args{"prog", "--build-bad"};

    kcli::Initialize(args.argc,
                     args.data(),
                     {.root = "build", .failure_mode = kcli::FailureMode::Throw});

    t.ExpectThrowContains<std::runtime_error>(
        [] {
            (void)kcli::Process();
        },
        "unknown option --build-bad",
        "inline Process() should throw for unknown options in FailureMode::Throw");
}

void CaseOptionHandlerExceptionReturnMode(TestContext& t) {
    ArgvFixture args{"prog", "--verbose"};

    kcli::Initialize(args.argc, args.data());
    kcli::SetHandler("--verbose",
                     [&](const kcli::HandlerContext&) {
                         throw std::runtime_error("option boom");
                     },
                     "Enable verbose logging.");

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(!result.ok, "option handler exceptions should be captured in return mode");
    t.ExpectEq(result.error_option,
               std::string("--verbose"),
               "option handler failures should report the option token");
    t.ExpectContains(result.error_message,
                     "option boom",
                     "option handler failures should preserve the thrown message");
}

void CaseOptionHandlerExceptionThrowMode(TestContext& t) {
    ArgvFixture args{"prog", "--verbose"};

    kcli::Initialize(args.argc, args.data(), {.failure_mode = kcli::FailureMode::Throw});
    kcli::SetHandler("--verbose",
                     [&](const kcli::HandlerContext&) {
                         throw std::runtime_error("option boom");
                     },
                     "Enable verbose logging.");

    t.ExpectThrowContains<std::runtime_error>(
        [] {
            (void)kcli::Process();
        },
        "option boom",
        "option handler exceptions should throw in FailureMode::Throw");
}

void CasePositionalHandlerExceptionReturnMode(TestContext& t) {
    ArgvFixture args{"prog", "tail"};

    kcli::Initialize(args.argc, args.data());
    kcli::SetPositionalHandler(
        [&](const kcli::HandlerContext&) {
            throw std::runtime_error("positional boom");
        });

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(!result.ok, "positional handler exceptions should be captured in return mode");
    t.ExpectEq(result.error_option,
               std::string(),
               "positional handler failures should not report an option token");
    t.ExpectContains(result.error_message,
                     "positional boom",
                     "positional handler failures should preserve the thrown message");
}

void CasePositionalHandlerExceptionThrowMode(TestContext& t) {
    ArgvFixture args{"prog", "tail"};

    kcli::Initialize(args.argc, args.data(), {.failure_mode = kcli::FailureMode::Throw});
    kcli::SetPositionalHandler(
        [&](const kcli::HandlerContext&) {
            throw std::runtime_error("positional boom");
        });

    t.ExpectThrowContains<std::runtime_error>(
        [] {
            (void)kcli::Process();
        },
        "positional boom",
        "positional handler exceptions should throw in FailureMode::Throw");
}

void CaseMultiPhaseProcessingConsumesInlineEndUserAndPositionals(TestContext& t) {
    ArgvFixture args{"prog", "tail", "--alpha-message", "hello", "--output", "stdout"};

    std::string alpha_message;
    std::string output;
    std::vector<std::string> positionals;

    kcli::Initialize(args.argc, args.data(), {.root = "alpha"});
    kcli::SetHandler("-message",
                     [&](const kcli::HandlerContext&, std::string_view value) {
                         alpha_message = std::string(value);
                     },
                     "Set alpha message.",
                     kcli::ValueMode::Required);
    const kcli::ProcessResult inline_result = kcli::Process();

    t.Expect(inline_result.ok, "first inline phase should succeed");
    t.ExpectEq(alpha_message,
               std::string("hello"),
               "first inline phase should consume its namespaced option");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "tail", "--output", "stdout"},
               "inline phase should compact argv for the next phase");

    kcli::Initialize(args.argc, args.data());
    kcli::SetHandler("--output",
                     [&](const kcli::HandlerContext&, std::string_view value) {
                         output = std::string(value);
                     },
                     "Set output target.",
                     kcli::ValueMode::Required);
    kcli::SetPositionalHandler(
        [&](const kcli::HandlerContext& context) {
            positionals = CopyTokens(context.value_tokens);
        });
    const kcli::ProcessResult end_user_result = kcli::Process();

    t.Expect(end_user_result.ok, "second end-user phase should succeed");
    t.ExpectEq(output,
               std::string("stdout"),
               "second phase should consume the end-user value option");
    t.ExpectEq(positionals,
               std::vector<std::string>{"tail"},
               "second phase should consume remaining positionals");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "all consumed phases should leave only argv[0]");

    const kcli::ProcessResult unknown_result = kcli::FailOnUnknown();
    t.Expect(unknown_result.ok, "after both phases, no unknown tokens should remain");
}

void CaseClearPositionalHandlerStopsConsumingPositionals(TestContext& t) {
    ArgvFixture args{"prog", "tail"};

    bool called = false;

    kcli::Initialize(args.argc, args.data());
    kcli::SetPositionalHandler(
        [&](const kcli::HandlerContext&) {
            called = true;
        });
    kcli::ClearPositionalHandler();

    const kcli::ProcessResult result = kcli::Process();

    t.Expect(result.ok, "Process() should succeed after clearing the positional handler");
    t.Expect(!called, "cleared positional handler should not run");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "tail"},
               "clearing the positional handler should leave positional tokens intact");
}

using CaseFunction = void (*)(TestContext&);

const std::pair<std::string_view, CaseFunction> kCases[] = {
    {"process_requires_initialize", CaseProcessRequiresInitialize},
    {"initialize_rejects_invalid_root", CaseInitializeRejectsInvalidRoot},
    {"end_user_known_options_and_argc_compaction", CaseEndUserKnownOptionsAndArgcCompaction},
    {"initialize_clears_positional_handler_between_phases",
     CaseInitializeClearsPositionalHandlerBetweenPhases},
    {"expand_alias_rewrites_tokens", CaseExpandAliasRewritesTokens},
    {"expand_alias_rewrites_after_double_dash", CaseExpandAliasRewritesAfterDoubleDash},
    {"expand_alias_rejects_invalid_alias", CaseExpandAliasRejectsInvalidAlias},
    {"expand_alias_rejects_invalid_target", CaseExpandAliasRejectsInvalidTarget},
    {"expand_alias_not_allowed_inline", CaseExpandAliasNotAllowedInline},
    {"set_root_value_handler_requires_inline_and_nonempty",
     CaseSetRootValueHandlerRequiresInlineAndNonEmpty},
    {"set_positional_handler_requires_end_user_and_nonempty",
     CaseSetPositionalHandlerRequiresEndUserAndNonEmpty},
    {"end_user_handler_normalization_rejects_single_dash",
     CaseEndUserHandlerNormalizationRejectsSingleDash},
    {"inline_handler_normalization_accepts_short_and_full_forms",
     CaseInlineHandlerNormalizationAcceptsShortAndFullForms},
    {"inline_handler_normalization_rejects_wrong_root",
     CaseInlineHandlerNormalizationRejectsWrongRoot},
    {"inline_bare_root_prints_help", CaseInlineBareRootPrintsHelp},
    {"inline_root_value_handler_joins_tokens", CaseInlineRootValueHandlerJoinsTokens},
    {"inline_missing_root_value_handler_errors", CaseInlineMissingRootValueHandlerErrors},
    {"optional_value_mode_allows_missing_value", CaseOptionalValueModeAllowsMissingValue},
    {"value_mode_none_does_not_consume_following_tokens",
     CaseValueModeNoneDoesNotConsumeFollowingTokens},
    {"required_value_mode_rejects_missing_value", CaseRequiredValueModeRejectsMissingValue},
    {"reject_dash_prefixed_values_rejects_first_value",
     CaseRejectDashPrefixedValuesRejectsFirstValue},
    {"reject_dash_prefixed_values_false_accepts_first_value",
     CaseRejectDashPrefixedValuesFalseAcceptsFirstValue},
    {"unknown_dash_option_policy_ignore_preserves_token",
     CaseUnknownDashOptionPolicyIgnorePreservesToken},
    {"unknown_dash_option_policy_consume_removes_token",
     CaseUnknownDashOptionPolicyConsumeRemovesToken},
    {"unknown_dash_option_policy_error_reports_help_hint",
     CaseUnknownDashOptionPolicyErrorReportsHelpHint},
    {"prefix_root_match_ignore_preserves_token", CasePrefixRootMatchIgnorePreservesToken},
    {"prefix_root_match_consume_removes_token", CasePrefixRootMatchConsumeRemovesToken},
    {"prefix_root_match_error_reports_token", CasePrefixRootMatchErrorReportsToken},
    {"fail_on_unknown_return_mode_reports_double_dash",
     CaseFailOnUnknownReturnModeReportsDoubleDash},
    {"fail_on_unknown_throw_mode_throws", CaseFailOnUnknownThrowModeThrows},
    {"inline_process_throw_mode_throws", CaseInlineProcessThrowModeThrows},
    {"option_handler_exception_return_mode", CaseOptionHandlerExceptionReturnMode},
    {"option_handler_exception_throw_mode", CaseOptionHandlerExceptionThrowMode},
    {"positional_handler_exception_return_mode", CasePositionalHandlerExceptionReturnMode},
    {"positional_handler_exception_throw_mode", CasePositionalHandlerExceptionThrowMode},
    {"multi_phase_processing_consumes_inline_end_user_and_positionals",
     CaseMultiPhaseProcessingConsumesInlineEndUserAndPositionals},
    {"clear_positional_handler_stops_consuming_positionals",
     CaseClearPositionalHandlerStopsConsumingPositionals},
};

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <case>\n";
        std::cerr << "Available cases:\n";
        for (const auto& [name, _] : kCases) {
            std::cerr << "  " << name << "\n";
        }
        return 2;
    }

    const std::string_view requested_case(argv[1]);
    for (const auto& [name, fn] : kCases) {
        if (name != requested_case) {
            continue;
        }

        TestContext context;
        fn(context);
        return context.failures() == 0 ? 0 : 1;
    }

    std::cerr << "Unknown case: " << requested_case << "\n";
    return 2;
}
