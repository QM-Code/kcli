#include <kcli.hpp>

#include <functional>
#include <initializer_list>
#include <iostream>
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

template <typename ConfigureFn>
void AddInlineParser(kcli::PrimaryParser& parser,
                     std::string_view root,
                     ConfigureFn&& configure) {
    kcli::InlineParser inline_parser(root);
    configure(inline_parser);
    parser.addInlineParser(inline_parser);
}

struct CapturedCliError {
    std::string option{};
    std::string message{};
};

template <typename Fn>
CapturedCliError ExpectCliError(TestContext& t,
                                Fn&& fn,
                                std::string_view message) {
    try {
        (void)fn();
    } catch (const kcli::CliError& ex) {
        CapturedCliError captured{};
        captured.option = std::string(ex.option());
        captured.message = ex.what();
        return captured;
    } catch (const std::exception& ex) {
        t.Expect(false, message);
        std::cerr << "  actual exception: " << ex.what() << "\n";
        return {};
    } catch (...) {
        t.Expect(false, message);
        std::cerr << "  actual exception: non-std::exception\n";
        return {};
    }

    t.Expect(false, message);
    return {};
}

void CasePrimaryParserEmptyParseSucceeds(TestContext& t) {
    ArgvFixture args{"prog"};
    kcli::PrimaryParser parser;

    parser.parse(args.argc, args.data());
    t.ExpectEq(args.CurrentTokens(), std::vector<std::string>{"prog"}, "argv should remain unchanged");
}

void CaseInlineParserRejectsInvalidRoot(TestContext& t) {
    t.ExpectThrowContains<std::invalid_argument>(
        [] {
            kcli::InlineParser parser("-build");
            (void)parser;
        },
        "must use '--root' or 'root'",
        "InlineParser should reject single-dash roots");
}

void CaseEndUserKnownOptionsWithUnknownOptionError(TestContext& t) {
    ArgvFixture args{"prog", "--verbose", "pos1", "--output", "stdout", "--bogus", "pos2"};

    bool verbose = false;
    std::string output;
    std::vector<std::string> positionals;

    kcli::PrimaryParser parser;
    parser.setHandler("verbose",
                      [&](const kcli::HandlerContext&) {
                          verbose = true;
                      },
                      "Enable verbose logging.");
    parser.setHandler("output",
                      [&](const kcli::HandlerContext&, std::string_view value) {
                          output = std::string(value);
                      },
                      "Set output target.",
                      kcli::ValueMode::Required);
    parser.setPositionalHandler(
        [&](const kcli::HandlerContext& context) {
            positionals = CopyTokens(context.value_tokens);
        });

    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "unknown end-user options should fail the parse");

    t.Expect(!verbose, "handlers should not run before the full command line validates");
    t.ExpectEq(output, std::string(), "value handlers should not run on invalid command lines");
    t.ExpectEq(positionals,
               std::vector<std::string>{},
               "positional handlers should not run on invalid command lines");
    t.ExpectEq(error.option, std::string("--bogus"), "the unknown option should be reported");
    t.ExpectContains(error.message,
                     "unknown option --bogus",
                     "unknown end-user options should surface the standard error");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--bogus"},
               "argv should be compacted after parsing");
}

void CaseAddAliasRewritesTokens(TestContext& t) {
    ArgvFixture args{"prog", "-v", "tail"};

    bool verbose = false;

    kcli::PrimaryParser parser;
    parser.addAlias("-v", "--verbose");
    parser.setHandler("--verbose",
                      [&](const kcli::HandlerContext& context) {
                          verbose = true;
                          t.Expect(context.from_alias, "alias-expanded handlers should report from_alias=true");
                      },
                      "Enable verbose logging.");

    (void)parser.parse(args.argc, args.data());

    t.Expect(verbose, "alias-expanded token should trigger the registered handler");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "tail"},
               "processed alias token should be removed from argv");
}

void CaseAddAliasRewritesAfterDoubleDash(TestContext& t) {
    ArgvFixture args{"prog", "--", "-v"};

    bool verbose = false;

    kcli::PrimaryParser parser;
    parser.addAlias("-v", "--verbose");
    parser.setHandler("--verbose",
                      [&](const kcli::HandlerContext&) {
                          verbose = true;
                      },
                      "Enable verbose logging.");

    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "literal '--' should make the overall parse fail");

    t.Expect(!verbose, "handlers should not run before the full command line validates");
    t.ExpectEq(error.option, std::string("--"), "literal '--' should be reported as the error token");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--"},
               "only the literal '--' token should remain");
}

void CaseAddAliasRejectsInvalidAlias(TestContext& t) {
    kcli::PrimaryParser parser;

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            parser.addAlias("--verbose", "--output");
        },
        "single-dash form",
        "addAlias() should reject invalid alias syntax");
}

void CaseAddAliasRejectsInvalidTarget(TestContext& t) {
    kcli::PrimaryParser parser;

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            parser.addAlias("-v", "--bad target");
        },
        "single CLI token",
        "addAlias() should reject alias targets with whitespace");
}

void CasePositionalHandlerRequiresNonEmpty(TestContext& t) {
    kcli::PrimaryParser parser;
    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            parser.setPositionalHandler(kcli::PositionalHandler{});
        },
        "must not be empty",
        "setPositionalHandler() should reject empty handlers");
}

void CaseEndUserHandlerNormalizationRejectsSingleDash(TestContext& t) {
    kcli::PrimaryParser parser;

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            parser.setHandler("-verbose",
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

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "build",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setHandler("-flag",
                                     [&](const kcli::HandlerContext&) {
                                         flag = true;
                                     },
                                     "Enable build flag.");
            inline_parser.setHandler("--build-value",
                                     [&](const kcli::HandlerContext&, std::string_view raw_value) {
                                         value = std::string(raw_value);
                                     },
                                     "Set build value.",
                                     kcli::ValueMode::Required);
        });

    parser.parse(args.argc, args.data());
    t.Expect(flag, "short-form inline handler should dispatch");
    t.ExpectEq(value, std::string("data"), "fully-qualified inline handler should dispatch");
    t.ExpectEq(args.CurrentTokens(), std::vector<std::string>{"prog"}, "all known inline tokens should be consumed");
}

void CaseInlineHandlerNormalizationRejectsWrongRoot(TestContext& t) {
    kcli::InlineParser inline_parser("--build");

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            inline_parser.setHandler("--other-flag",
                                     [&](const kcli::HandlerContext&) {
                                     },
                                     "Enable other flag.");
        },
        "inline handler option must use '-name' or '--build-name'",
        "inline handlers should reject mismatched fully-qualified roots");
}

void CaseInlineBareRootPrintsHelp(TestContext& t) {
    ArgvFixture args{"prog", "--build"};

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "build",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setHandler("-flag",
                                     [&](const kcli::HandlerContext&) {
                                     },
                                     "Enable build flag.");
            inline_parser.setHandler("-value",
                                     [&](const kcli::HandlerContext&, std::string_view) {
                                     },
                                     "Set build value.",
                                     kcli::ValueMode::Required);
        });

    CaptureStdout capture;
    parser.parse(args.argc, args.data());
    const std::string output = capture.str();

    t.ExpectEq(args.CurrentTokens(), std::vector<std::string>{"prog"}, "bare inline root should be removed from argv");
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

void CaseInlineRootValueHandlerHelpRowPrints(TestContext& t) {
    ArgvFixture args{"prog", "--build"};

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "build",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setRootValueHandler(
                [&](const kcli::HandlerContext&, std::string_view) {
                },
                "<selector>",
                "Select build targets.");
            inline_parser.setHandler("-flag",
                                     [&](const kcli::HandlerContext&) {
                                     },
                                     "Enable build flag.");
        });

    CaptureStdout capture;
    parser.parse(args.argc, args.data());
    const std::string output = capture.str();

    t.ExpectContains(output,
                     "--build <selector>",
                     "bare inline root help should include the root value form");
    t.ExpectContains(output,
                     "Select build targets.",
                     "bare inline root help should include the root value description");
}

void CaseInlineRootValueHandlerJoinsTokens(TestContext& t) {
    ArgvFixture args{"prog", "--build", "fast", "mode"};

    std::string received_value;
    std::vector<std::string> received_tokens;
    std::string received_option;
    int option_index = -1;

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "build",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setRootValueHandler(
                [&](const kcli::HandlerContext& context, std::string_view value) {
                    received_value = std::string(value);
                    received_tokens = CopyTokens(context.value_tokens);
                    received_option = std::string(context.option);
                    option_index = context.option_index;
                });
        });

    parser.parse(args.argc, args.data());
    t.ExpectEq(received_value, std::string("fast mode"), "root value handler should receive joined text");
    t.ExpectEq(received_tokens,
               std::vector<std::string>{"fast", "mode"},
               "root value handler should receive tokenized parts");
    t.ExpectEq(received_option, std::string("--build"), "root value handler should report the root token");
    t.ExpectEq(option_index, 1, "root value handler should report the option index");
    t.ExpectEq(args.CurrentTokens(), std::vector<std::string>{"prog"}, "root value processing should remove consumed tokens");
}

void CaseInlineMissingRootValueHandlerErrors(TestContext& t) {
    ArgvFixture args{"prog", "--build", "fast"};

    kcli::PrimaryParser parser;
    parser.addInlineParser(kcli::InlineParser("--build"));
    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "root values should error when no root value handler is registered");

    t.ExpectEq(error.option, std::string("--build"), "root value errors should identify the root option");
    t.ExpectContains(error.message,
                     "unknown value for option '--build'",
                     "root value errors should explain the missing handler");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "the failing root/value pair should still be consumed");
}

void CaseOptionalValueModeAllowsMissingValue(TestContext& t) {
    ArgvFixture args{"prog", "--build-enable"};

    bool called = false;
    std::string received_value;
    std::vector<std::string> received_tokens;

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "build",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setHandler("-enable",
                                     [&](const kcli::HandlerContext& context, std::string_view value) {
                                         called = true;
                                         received_value = std::string(value);
                                         received_tokens = CopyTokens(context.value_tokens);
                                     },
                                     "Enable build mode.",
                                     kcli::ValueMode::Optional);
        });

    parser.parse(args.argc, args.data());
    t.Expect(called, "optional value handler should still run");
    t.ExpectEq(received_value, std::string(), "optional value handler should receive an empty value");
    t.ExpectEq(received_tokens,
               std::vector<std::string>{},
               "optional value handler should receive no token parts when omitted");
    t.ExpectEq(args.CurrentTokens(), std::vector<std::string>{"prog"}, "optional value option should still be consumed");
}

void CaseValueModeNoneDoesNotConsumeFollowingTokens(TestContext& t) {
    ArgvFixture args{"prog", "--build-meta", "data"};

    bool called = false;
    std::string received_value;

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "build",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setHandler("-meta",
                                     [&](const kcli::HandlerContext&, std::string_view value) {
                                         called = true;
                                         received_value = std::string(value);
                                     },
                                     "Record metadata.",
                                     kcli::ValueMode::None);
        });

    parser.parse(args.argc, args.data());
    t.Expect(called, "ValueMode::None handler should run");
    t.ExpectEq(received_value, std::string(), "ValueMode::None handler should receive an empty value");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "data"},
               "ValueMode::None should leave the following token untouched");
}

void CaseRequiredValueModeRejectsMissingValue(TestContext& t) {
    ArgvFixture args{"prog", "--build-value"};

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "build",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setHandler("-value",
                                     [&](const kcli::HandlerContext&, std::string_view) {
                                     },
                                     "Set build value.",
                                     kcli::ValueMode::Required);
        });

    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "required value handlers should reject missing values");

    t.ExpectEq(error.option,
               std::string("--build-value"),
               "required value errors should identify the failing option");
    t.ExpectContains(error.message,
                     "requires a value",
                     "required value errors should explain the missing value");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "failing required options should still be removed from argv");
}

void CaseRequiredValueModeAcceptsDashPrefixedFirstValue(TestContext& t) {
    ArgvFixture args{"prog", "--build-value", "-debug"};

    std::string value;

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "build",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setHandler(
                "-value",
                [&](const kcli::HandlerContext&, std::string_view raw_value) {
                    value = std::string(raw_value);
                },
                "Set build value.",
                kcli::ValueMode::Required);
        });

    parser.parse(args.argc, args.data());
    t.ExpectEq(value, std::string("-debug"), "handler should receive the dash-prefixed value");
    t.ExpectEq(args.CurrentTokens(), std::vector<std::string>{"prog"}, "accepted dash-prefixed value should be consumed");
}

void CaseUnknownInlineOptionErrors(TestContext& t) {
    ArgvFixture args{"prog", "--build-unknown"};

    kcli::PrimaryParser parser;
    parser.addInlineParser(kcli::InlineParser("--build"));

    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "unknown inline options should fail the parse");

    t.ExpectEq(error.option,
               std::string("--build-unknown"),
               "unknown inline options should report the token");
    t.ExpectContains(error.message,
                     "unknown option --build-unknown",
                     "unknown inline options should surface the standard error");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog", "--build-unknown"},
               "unknown inline options should remain in argv");
}

void CaseUnknownOptionReportsDoubleDash(TestContext& t) {
    ArgvFixture args{"prog", "--"};

    kcli::PrimaryParser parser;
    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "parse() should reject literal '--'");

    t.ExpectEq(error.option, std::string("--"), "parse() should report literal '--' as unknown");
}

void CaseUnknownOptionThrowsCliError(TestContext& t) {
    ArgvFixture args{"prog", "--bogus"};

    kcli::PrimaryParser parser;

    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "parse() should throw CliError for unknown options");

    t.ExpectEq(error.option, std::string("--bogus"), "unknown options should identify the token");
    t.ExpectContains(error.message,
                     "unknown option --bogus",
                     "unknown options should preserve the human-facing message");
}

void CaseOptionHandlerExceptionThrowsCliError(TestContext& t) {
    ArgvFixture args{"prog", "--verbose"};

    kcli::PrimaryParser parser;
    parser.setHandler("--verbose",
                      [&](const kcli::HandlerContext&) {
                          throw std::runtime_error("option boom");
                      },
                      "Enable verbose logging.");

    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "option handler exceptions should throw CliError");

    t.ExpectEq(error.option, std::string("--verbose"), "option handler failures should report the option");
    t.ExpectContains(error.message, "option boom", "thrown option messages should be preserved");
    t.ExpectContains(error.message, "--verbose", "handler failures should include the option token");
}

void CasePositionalHandlerExceptionThrowsCliError(TestContext& t) {
    ArgvFixture args{"prog", "tail"};

    kcli::PrimaryParser parser;
    parser.setPositionalHandler(
        [&](const kcli::HandlerContext&) {
            throw std::runtime_error("positional boom");
        });

    const CapturedCliError error = ExpectCliError(
        t,
        [&] {
            return parser.parse(args.argc, args.data());
        },
        "positional handler exceptions should throw CliError");

    t.ExpectEq(error.option, std::string(), "positional failures should not report an option token");
    t.ExpectContains(error.message, "positional boom", "positional messages should be preserved");
}

void CaseSinglePassProcessingConsumesInlineEndUserAndPositionals(TestContext& t) {
    ArgvFixture args{"prog", "tail", "--alpha-message", "hello", "--output", "stdout"};

    std::string alpha_message;
    std::string output;
    std::vector<std::string> positionals;

    kcli::PrimaryParser parser;
    AddInlineParser(
        parser,
        "alpha",
        [&](kcli::InlineParser& inline_parser) {
            inline_parser.setHandler("-message",
                                     [&](const kcli::HandlerContext&, std::string_view value) {
                                         alpha_message = std::string(value);
                                     },
                                     "Set alpha message.",
                                     kcli::ValueMode::Required);
        });
    parser.setHandler("--output",
                      [&](const kcli::HandlerContext&, std::string_view value) {
                          output = std::string(value);
                      },
                      "Set output target.",
                      kcli::ValueMode::Required);
    parser.setPositionalHandler(
        [&](const kcli::HandlerContext& context) {
            positionals = CopyTokens(context.value_tokens);
        });
    (void)parser.parse(args.argc, args.data());

    t.ExpectEq(alpha_message, std::string("hello"), "inline option should be consumed in the same pass");
    t.ExpectEq(output, std::string("stdout"), "end-user option should be consumed in the same pass");
    t.ExpectEq(positionals, std::vector<std::string>{"tail"}, "remaining positionals should be consumed");
    t.ExpectEq(args.CurrentTokens(), std::vector<std::string>{"prog"}, "all consumed tokens should leave only argv[0]");
}

void CaseInlineParserRootOverrideApplies(TestContext& t) {
    ArgvFixture args{"prog", "--newgamma-tag", "prod"};

    std::string tag;

    kcli::PrimaryParser parser;
    kcli::InlineParser gamma("--gamma");
    gamma.setHandler("-tag",
                     [&](const kcli::HandlerContext&, std::string_view value) {
                         tag = std::string(value);
                     },
                     "Set gamma tag.",
                     kcli::ValueMode::Required);
    gamma.setRoot("--newgamma");
    parser.addInlineParser(gamma);

    (void)parser.parse(args.argc, args.data());

    t.ExpectEq(tag, std::string("prod"), "overridden root should dispatch registered handlers");
    t.ExpectEq(args.CurrentTokens(),
               std::vector<std::string>{"prog"},
               "the overridden root should be the active registration");
}

void CaseDuplicateInlineRootRejected(TestContext& t) {
    kcli::PrimaryParser parser;
    parser.addInlineParser(kcli::InlineParser("--build"));

    t.ExpectThrowContains<std::invalid_argument>(
        [&] {
            parser.addInlineParser(kcli::InlineParser("build"));
        },
        "already registered",
        "duplicate inline roots should be rejected");
}

using CaseFunction = void (*)(TestContext&);

const std::pair<std::string_view, CaseFunction> kCases[] = {
    {"primary_parser_empty_parse_succeeds", CasePrimaryParserEmptyParseSucceeds},
    {"inline_parser_rejects_invalid_root", CaseInlineParserRejectsInvalidRoot},
    {"end_user_known_options_with_unknown_option_error",
     CaseEndUserKnownOptionsWithUnknownOptionError},
    {"add_alias_rewrites_tokens", CaseAddAliasRewritesTokens},
    {"add_alias_rewrites_after_double_dash", CaseAddAliasRewritesAfterDoubleDash},
    {"add_alias_rejects_invalid_alias", CaseAddAliasRejectsInvalidAlias},
    {"add_alias_rejects_invalid_target", CaseAddAliasRejectsInvalidTarget},
    {"positional_handler_requires_nonempty", CasePositionalHandlerRequiresNonEmpty},
    {"end_user_handler_normalization_rejects_single_dash",
     CaseEndUserHandlerNormalizationRejectsSingleDash},
    {"inline_handler_normalization_accepts_short_and_full_forms",
     CaseInlineHandlerNormalizationAcceptsShortAndFullForms},
    {"inline_handler_normalization_rejects_wrong_root",
     CaseInlineHandlerNormalizationRejectsWrongRoot},
    {"inline_bare_root_prints_help", CaseInlineBareRootPrintsHelp},
    {"inline_root_value_handler_help_row_prints", CaseInlineRootValueHandlerHelpRowPrints},
    {"inline_root_value_handler_joins_tokens", CaseInlineRootValueHandlerJoinsTokens},
    {"inline_missing_root_value_handler_errors", CaseInlineMissingRootValueHandlerErrors},
    {"optional_value_mode_allows_missing_value", CaseOptionalValueModeAllowsMissingValue},
    {"value_mode_none_does_not_consume_following_tokens",
     CaseValueModeNoneDoesNotConsumeFollowingTokens},
    {"required_value_mode_rejects_missing_value", CaseRequiredValueModeRejectsMissingValue},
    {"required_value_mode_accepts_dash_prefixed_first_value",
     CaseRequiredValueModeAcceptsDashPrefixedFirstValue},
    {"unknown_inline_option_errors", CaseUnknownInlineOptionErrors},
    {"unknown_option_reports_double_dash", CaseUnknownOptionReportsDoubleDash},
    {"unknown_option_throws_cli_error", CaseUnknownOptionThrowsCliError},
    {"option_handler_exception_throws_cli_error", CaseOptionHandlerExceptionThrowsCliError},
    {"positional_handler_exception_throws_cli_error", CasePositionalHandlerExceptionThrowsCliError},
    {"single_pass_processing_consumes_inline_end_user_and_positionals",
     CaseSinglePassProcessingConsumesInlineEndUserAndPositionals},
    {"inline_parser_root_override_applies", CaseInlineParserRootOverrideApplies},
    {"duplicate_inline_root_rejected", CaseDuplicateInlineRootRejected},
};

}  // namespace

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
