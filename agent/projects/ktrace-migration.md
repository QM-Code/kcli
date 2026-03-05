# KTrace Migration Pre-Checklist

This checklist captures agreed migration policy before swapping `ktrace` CLI parsing to `kcli`.

## Locked Decisions

- [x] Unknown inline options are hard errors.
- [x] Duplicate options trigger duplicate handler calls (application decides what that means).
- [x] Keep current inline execution model (preconfiguration/help-style work at inline layer; no transactional rollback needed).
- [x] Use `spdlog` for user-facing parse errors (colorized `[error]` output).
- [x] If single-token value behavior is needed by consumers, handle that after conversion.
- [x] README/docs cleanup can happen after migration.

## Decisions (Closed)

- [x] Exit code for CLI parse failure in `ktrace` demo/executable: `2`.
- [x] Root literal style in source code: bare root (`"trace"`, `"config"`).
  - `kcli::Parser::Initialize(..., root)` normalization remains compatible with either form.

## Open Decisions To Close Before/During Migration
- [ ] Remaining `argc/argv` contract after inline parsing.
  - Minimum migration goal: preserve current practical behavior for `ktrace`/`kconfig`.
  - Post-migration follow-up: define strict contract for leftovers/order/compaction policy.

## Already Proven In `kcli` Demo

- [x] Unknown `--<root>-<command>` options fail and surface clear errors.
- [x] Automated CLI tests exist for unknown/known option behavior (`demo_cli_` tests).
- [x] `spdlog` is wired through local `vcpkg` and build/export dependency metadata.

## Migration Gate

Do not start `ktrace` code replacement until the open remaining-argv contract is confirmed.
