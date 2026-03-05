# Executable Demo

Compile/link/load baseline for an app that consumes `KcliSDK` plus three
demo libraries.

`src/main.cpp` intentionally stays minimal:
1. Call each library `ProcessCLI(argc, argv, <root>)`.
2. Construct one end-user parser and call `Process()`.
3. Print a single integration-check success line.

## CLI Behavior Tests

The executable demo defines CLI behavior checks in:

- `tests/demo_cli_cases.sh` (repo root)

When `BUILD_TESTING=ON`, `CMakeLists.txt` registers these CTest tests:

1. `demo_cli_unknown_alpha_option_errors`
2. `demo_cli_unknown_beta_option_errors`
3. `demo_cli_unknown_renamed_option_errors`
4. `demo_cli_known_alpha_option_ok`

Run only these tests:

```bash
ctest --test-dir demo/executable/build/latest --output-on-failure -R demo_cli_
```
