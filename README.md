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
./demo/exe/omega/build/latest/test --renamed-tag "prod"
./demo/exe/omega/build/latest/test --alpha-d
```

## Parser Modes

- End-user mode (default): `Initialize(argc, argv)`  
  Handles top-level options and aliases (for example `-v -> verbose`).
- Inline mode: `Initialize(argc, argv, "root")`  
  Handles `--<root>` and `--<root>-*` only.

Inline mode behavior:
- `--<root>` always prints available `--<root>-*` options.
- `--<root> value [value...]` is accepted only when a root value handler is registered.
- Unknown `--<root>-*` options hard-error.
- Aliases are not allowed.

Root token rules:
- Root must be bare (for example `"trace"`, not `"--trace"`).
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
