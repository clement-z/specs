# P1 report — golden harness hardening (branch `ws/harness`)

Sprint 2026-10b, phase P1. Items B-129, B-126, B-127, B-128 (in that order), all **done**. Only
`tests/golden/` (harness, tests, cases, baseline stored form), docs and backlog files changed; no C++ change.

## Items

| item | outcome | commits | tests | golden impact |
|---|---|---|---|---|
| B-129 | printed OpticalSignal IDs `[0042]  @ ...` → `[#k]`, k = first-appearance rank per stream/file (`canonical_ids`, applied in `normalise_text`). Streams compared as a multiset of lines compare the lines with IDs masked plus an order-independent ID structure (`id_profiles`) | 23797a8, 1747633 | `SignalIds`, `LineOrder.test_reordered_*` | **stored form only**: 30 stdout baselines rewritten in place (see below) |
| B-126 | line order checked by default; reorders accepted only for a case's `unordered_streams` or with `--lenient-order`; `--strict-order` = no-op | b43e7cd | `LineOrder` | none; `unordered_streams: ["stdout"]` on 8 cases |
| B-127 | NEW fails unless `--allow-new`; MISSING always fails; a selected gtest without result fails unless `--no-unit-tests` (`check` and `compare`); `--only` matching nothing exits 2; gtest selected only when `--only` is absent or matches `gtest/unit_tests`; `--strict` = no-op; meson passes `--no-unit-tests` when `unit_tests` is not built | bfd6a15, c6a9026 | `Coverage` | none |
| B-128 | `PRINT_SLACK_MIN_DIGITS = 5`: a text float pair whose longer token has < 5 significant digits compares with rtol only (no print slack, no atol); otherwise the slack is one unit of the last digit of the **longer** token | 0474575, 1adc540 | `PrintSlack` | none |

Status/progress commits: 17e74ae, 406dea9, 45bcb44, 4c07030, 4288b22, 91f5629, d390dc4 (+ this report).
Harness unit tests: `tests/golden/test_run_golden.py` (new, stdlib `unittest`, 30 tests), registered as
`meson test --suite golden` → `golden-harness`.

### B-129 baseline rewrite (the only baseline change)
The 30 files were rewritten by applying `canonical_ids()` to the stored text (one-off script, not a re-run), so
line order and every value are byte-identical apart from the IDs. Verified for each file: old text with IDs masked
== new text with IDs masked, and the old→new ID mapping is a bijection. `manifest.json` `rerecords` entry cites
B-129 and lists the 30 cases (cir/add_drop, cir/add_drop_pyspecs, cir/subckt_test, cir/subckt_tests, opt/mode-fd,
opt/mode-sampled-time, opt/multi-file, opt/tol-resolution-overrides, opt/tracefile-o, opt/vci, regress/b006,
b031, b150, and 17 tb/* cases). A fresh gcc record matches the rewritten baseline with `--rtol 0 --atol 0
--print-slack 0` (apart from the line order of the 8 cases below).

## Cases with `unordered_streams` (for P4 / F-01 to remove)
Order differs between the baseline and **all** of today's builds (gcc, clang, gcc `-O0`; run-to-run identical
within a build), pointer-ordered OP listing / `--vci` dump (B-016):
`cir/add_drop_pyspecs`, `cir/subckt_tests`, `opt/tracefile-o`, `opt/mode-sampled-time`, `opt/multi-file`,
`opt/vci`, `regress/b031-mzi-bidir`, `regress/b006-empty-output`. Each case's `note` says to remove it after F-01.

Reordered **only** under clang ASan+UBSan `-O0` (documented in the README, *not* exempted; use `--lenient-order`):
`cir/add_drop`, `cir/subckt_test`, `cir-json/add_drop`, `opt/mode-fd`, `opt/tol-resolution-overrides`,
`regress/b150-include-in-subckt`, `err/b017-json-unwritable` — element creation order ("Creating ROOT/X1/WG2
(reason: unidir backlog)" moves), i.e. the parser/elaboration also iterates in pointer order. Under ASan,
`cir/paul_3ring_puf` even stops at a different element (4 fewer stdout lines) → real failure, also for P4.

## Decisions taken (conservative, not covered by the decision record)
1. **B-129 rank scheme for multiset streams.** First-appearance ranks follow line order, so a reorder permutes
   them (6 of the 8 unordered cases would fail). The stored form uses first-appearance ranks exactly as decided;
   multiset comparisons mask IDs and compare `id_profiles` (per ID: the lines it appears in, IDs and numbers
   masked). A first attempt (re-rank in ID-masked sorted order, 23797a8) broke on lines differing only by ID; fixed
   in 1747633. Limitation (review): numbers are masked in profiles, so in unordered streams a swap of which signal
   carries which value between lines of the same shape is not detected. Only affects the 8 exempted cases until P4.
2. **B-129 re-record = in-place rewrite**, not a run of `record`, so stored line order/values cannot change.
3. **B-126 exemption set** = cases that reorder in the acceptance builds (gcc, clang, -O0) only; ASan-only
   reorders documented, not exempted (stricter). The sanitizer job (non-blocking) will report them until P4/P7.
4. **B-127 gtest selection**: gtest is selected iff `--only` is absent or matches `gtest/unit_tests`
   (`record --only '^gtest'` keeps working; `record --only '^cir/x' --unit-tests` no longer re-records gtest).
   Skipping it is explicit via the new `--no-unit-tests`. `record` without `--unit-tests` only warns (full record).
   `--strict` and `--strict-order` kept as accepted no-ops so older command lines (COMMON.md) still work.
5. **B-128 threshold 5** (the backlog value). Data: gcc, clang, gcc -O0 pass 81/81 with `--print-slack 0
   --atol 0`; clang ASan too (except `paul_3ring_puf`, ordering); every tolerance/config printout in the baseline
   has 1 significant digit, so any threshold ≥ 2 satisfies the acceptance. 5 keeps the slack for long computed
   tokens (6-digit `%g`, 7-digit fixed, 4-decimal phases ≥ 1 rad). **Short tokens also drop `atol`**: needed for
   the acceptance, because the default `atol=1e-12` let `abstol: 1e-12` → `2e-12` (or `0`) pass; this matches
   the backlog item's "shorter tokens compare exactly". `--rtol` still applies to them.
6. Review follow-up (1adc540): the slack of a mixed pair is the ulp of the longer token (`"1"` vs `"1.99999"`
   used to pass with a slack of 1; pre-existing, same defect class as B-128).

## Perturbation demonstrations
`bash backlog/repro/p1-golden-perturbations.sh <repo> /tmp/P1-fgcc` (scratch baseline copy, repo untouched),
fresh gcc build, all 14 checks as expected:

| perturbation | command | exit |
|---|---|---|
| tb/wg: first two time blocks swapped | `check --only '^tb/wg$'` | **1** (`ONLY THE LINE ORDER DIFFERS`) |
| same | `... --lenient-order` | 0 |
| baseline dir of tb/wg deleted (NEW) | `check --only '^tb/wg$'` | **1** |
| same | `... --allow-new` | 0 |
| gtest selected, no `--unit-tests` | `check --only '^(tb/wg\|gtest)'` | **1** (`MISSING gtest/unit_tests`) |
| same | `... --no-unit-tests` / `... --unit-tests <b>` | 0 / 0 |
| candidate dir without gtest | `compare` | **1** |
| empty selection | `check --only no_such_case`, `compare --only no_such_case` | **2** / **2** |
| `- abstol: 1e-12` → `2e-12` (cir/add_drop) | `check --only '^cir/add_drop$'` | **1** |
| `- default reltol: 1e-06` → `2e-06` | same | **1** |
| control: `1.000000` → `1.000001` in tb/wg | `check` / `check --print-slack 0` | 0 / 1 |

Older repros: `sh backlog/repro/a5-golden-holes.sh <repo>` part 1 now FAILs (exit 1);
`python3 -I backlog/repro/a5-golden-slack-probe.py <repo>` reports `fail` for all 7 edits.

## Final results (HEAD before this report: c6a9026 + status commit)
Fresh builds `/tmp/P1-fgcc` (gcc 11.5, Meson default) and `/tmp/P1-fclang` (clang 19.1.7), prebuilt deps
`specs-deps-next`:
- `meson test`: gcc 12/12 OK, clang 12/12 OK (unit, 9 tb, golden-harness, golden).
- `check --specs <b>/specs --unit-tests <b>/src/test/unit_tests --baseline tests/golden/baseline -j 8`
  (new defaults): gcc and clang **81 passed (8 with notes = the unordered cases), 0 failed, 0 new, 0 missing,
  0 skipped**; the gtest case runs and passes. Same with `--atol 0 --strict --strict-order`.
- gcc `-O0` (`/tmp/P1-debug`): 81/81 with the defaults and `--atol 0`.
- `-Dtests=disabled` build: `meson test --suite golden` 2/2 (gtest SKIPPED via `--no-unit-tests`).
- clang ASan+UBSan `-O0` (`ASAN_OPTIONS=detect_leaks=0`, `--lenient-order`): 80/81, `cir/paul_3ring_puf` fails
  (see above). gcc ASan could not be built here (`libasan.so.6` missing).
- `record --only '^tb/wg$' --repeat 2`: identical across runs.

## Docs updated
`tests/golden/README.md` (normalisation, options, comparison rules, gtest rules, adding a case, determinism
findings), `tests/golden/cases.json` description, `CLAUDE.md` testing section, `CHANGELOG.md` [unreleased],
`discover/04-build-test-ci.md` (reorder rule), backlog item status lines and `backlog.md` rows. CI files call
`meson test` only (no direct `run_golden.py` call), so they needed no change; the meson registration gained
`--no-unit-tests` for builds without unit tests.

## Found, not done
- **Heartbeat**: the coordinator's `paseo heartbeat create ... P1-heartbeat` was denied by the Claude Code
  permission classifier ("Unauthorized Persistence"); not created, so nothing to delete. Resumption relied on the
  coordinator.
- **P4**: besides the OP listing / `--vci` dump, element creation ("unidir backlog") is pointer-ordered (ASan
  reorders 7 more cases and changes where `cir/paul_3ring_puf` stops).
- **P7**: the sanitizer job will fail on the ASan-only reorders and on `paul_3ring_puf` until P4; `--lenient-order`
  is available if needed in the meantime (decision for the coordinator).
- `id_profiles` masks numbers (limitation in decision 1); could keep rounded values once the exemptions are gone.
- Short fixed-point tokens (`0.000317 W`, phases < 1 rad such as `-0.8107`) now compare exactly: a last-digit
  rounding flip on another compiler would fail. None observed in gcc/clang/-O0/ASan.
- STALE cases (in the baseline manifest, removed from `cases.json`) are still only printed, not failing.
- A partial `record --only '^gtest'` does not update `manifest["gtest"]` (unused by `check`, cosmetic).
- B-027's status says `drop_all_events` was kept because removing it shifts printed signal IDs in
  `regress/b031-mzi-bidir`: no longer a blocker since B-129.
- `tests/golden/__pycache__/` gets created by the imports in `test_run_golden.py` (ignored by `.gitignore`).
