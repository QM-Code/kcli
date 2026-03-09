# Karma CLI Parsing SDK

Standalone CLI parsing SDK used by `ktrace` and `kconfig`.

## Build SDK

```bash
./kbuild.py --build-latest
```

SDK output:
- `build/latest/sdk/include`
- `build/latest/sdk/lib`
- `build/latest/sdk/lib/cmake/KcliSDK`

## Build and Test Demos

```bash
# Builds SDK plus kbuild.json "build.defaults.demos".
./kbuild.py --build-latest

# Explicit demo-only run (uses build.demos when no args are provided).
./kbuild.py --build-demos

./demo/exe/core/build/latest/test
```

Demos:
- Bootstrap compile/link check: `demo/bootstrap/`
- SDKs: `demo/sdk/{alpha,beta,gamma}`
- Executables: `demo/exe/{core,omega}`

Demo builds are orchestrated by the root `kbuild.py`.

Useful demo commands:

```bash
./demo/exe/core/build/latest/test
./demo/exe/core/build/latest/test --alpha
./demo/exe/core/build/latest/test --alpha-message "hello"
./demo/exe/core/build/latest/test --output stdout
./demo/exe/omega/build/latest/test --beta-workers 8
./demo/exe/omega/build/latest/test --newgamma-tag "prod"
./demo/exe/omega/build/latest/test --alpha-d
```

## Parser Objects

- `PrimaryParser`
  Owns aliases, end-user handlers, inline parser registrations, and the single `parse(argc, argv)` pass.
- `InlineParser`
  Defines one inline root namespace such as `--alpha` or `--build` plus its `--<root>-*` handlers.

Typical flow:

```cpp
kcli::PrimaryParser parser;
kcli::InlineParser build("--build");

build.setHandler("-profile", handleProfile, "Set build profile.", kcli::ValueMode::Required);
parser.addInlineParser(build);

parser.addAlias("-v", "--verbose");
parser.setHandler("--verbose", handleVerbose, "Enable verbose logging.");

try {
    parser.parse(argc, argv);
} catch (const kcli::CliError& ex) {
    std::cerr << "CLI error: " << ex.what() << "\n";
    return 2;
}
```

Inline mode behavior:
- `--<root>` always prints available `--<root>-*` options.
- `--<root> value [value...]` is accepted only when a root value handler is registered.
- Root value handlers can also advertise a help row such as `--build <selector>`.
- Required option values consume the next CLI token, even when it starts with `-`.
- Optional values only start consuming when the next token looks like a value.
- Unknown option-like tokens fail the parse.
- Literal `--` is rejected as an unknown option; it is not treated as an option terminator.
- Aliases are defined on the primary parser and expanded before dispatch.
- Handlers run only after the full command line validates.
- `parse()` throws `kcli::CliError` on failure.

Root token rules:
- Roots can be configured as `"trace"` or `"--trace"`.
- Inline roots only match `--<root>` and `--<root>-*`.
- Roots beginning with `-` are rejected.

## Install

Consumer CMake:

```cmake
find_package(KcliSDK CONFIG REQUIRED)
target_link_libraries(main PRIVATE kcli::sdk)
```

## Coding Agents

If you are using a coding agent, paste the following prompt:

```bash
Follow the instructions in agent/BOOTSTRAP.md
```
