# Executable Demo

Compile/link/load baseline for an app that consumes `KcliSDK` plus three
demo libraries.

`src/main.cpp` intentionally stays minimal:
1. Call each library `ProcessCLI(argc, argv, <root>)`.
2. Construct one end-user parser and call `Process()`.
3. Print a single integration-check success line.
