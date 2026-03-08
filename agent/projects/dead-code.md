# Dead Code Follow-up

This note tracks dead or likely-dead code found after the procedural API refactor and backend de-OO work.

## Confirmed dead

- `EmitDemoOutput()` is dead in all three demo SDKs.
  - Declared in:
    - `demo/sdk/alpha/include/alpha/sdk.hpp`
    - `demo/sdk/beta/include/beta/sdk.hpp`
    - `demo/sdk/gamma/include/gamma/sdk.hpp`
  - Defined in:
    - `demo/sdk/alpha/src/cli.cpp`
    - `demo/sdk/beta/src/cli.cpp`
    - `demo/sdk/gamma/src/cli.cpp`
  - No callers were found in the repo.

- Dead public type aliases remain in `include/kcli.hpp`:
  - `UnknownOptionHandler`
  - `ErrorHandler`
  - `WarningHandler`
  - No public API currently accepts them, and no repo references were found after the refactor.

## Likely dead / verify before removal

- `ClearPositionalHandler()`
  - Declared in `include/kcli.hpp`
  - Defined in `src/kcli.cpp`
  - No current callers were found.
  - It may still be a coherent public operation, but `Initialize(...)` already clears the positional handler for each phase, so its practical value looks low.
