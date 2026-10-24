# Phase F03d — SPAR decisions of the second discussion (branch `ws/spar4`)

Follow `COMMON.md`. Phase id `PF03d`. Decisions: `backlog/DECISIONS-2026-10-21.md`, row "SPAR second discussion"
(2026-10-23). **Read first `backlog/sprint-2026-10c/SPAR-discussion-2.md`**, in particular its "Notes for the
follow-up" (order, formulas, expected values, predicted golden impact per item), then `PF03c-REPORT.md`,
`doc/sparam.md`, `src/io/sparam_file.{h,cpp}`, `src/devices/sparam_device.{h,cpp}`.

The maintainer signed these numerics changes off. Write your predictions (golden cases, counters, gtest numbers)
in `PF03d-progress.md` before each re-record.

## Scope (in the summary's order, one commit per item)
1. **Remove `delay_per`**: device, header, parser, `--vci` line, the negative-delay warning's per-output text;
   delete the golden cases `regress/spar-ring-tran-output` and `err/spar-bad-delay-per` (cases.json, baseline,
   netlists, manifest bookkeeping as the harness requires); remove the three `test_sparam.py` cases; docs
   (`doc/sparam.md`, `doc/netlist.md`), `CHANGELOG.md`, `tests/golden/README.md` counts. `delay="auto"|"none"|<s>`
   unchanged ("auto" documented as one delay per path). Nothing else may move.
2. **Grid check on the actual intervals** (`leaveOneOutErrors()`: the re/im decision on the two intervals around
   the point), comments and `doc/sparam.md` "Grid check". No golden change; re-run the `SParamFile` gtests.
3. **PCHIP slopes per run** between re/im intervals, in the device and in the leave-one-out check, as the summary
   describes. Expected: MZI ~9.2e-4 between points, ring through ~2.1e-4; dispersive line and BDC bit-identical.
   No golden case uses `pchip`; update the gtest number and keep the `test_sparam.py` PCHIP bound.
4. **Pulse-pair tau0** in `removeDelay()` (closed form with the mean step, one or two Newton steps, intervals with a
   step above ~3x the median skipped, 1e-3 rad cut and f_ref unchanged). Validate against the summary's expected
   values (ring through 0.74919 ps, drop 1.1236 ps, MZI ~5.018 ps, 1 nm file 0.6235 / 1.1406 ps; `wg` fixtures
   +1.40097 ps). Predicted golden: `out` of `cir/spar-add-drop-ring`, `regress/spar-ring-tran`,
   `regress/spar-grid-warning` by ~1.7e-14 (strict re-record), warning text unchanged. Update the docs, the
   comments named in the summary and the "Fix" paragraph of `backlog/B-190-*.md`.
5. Docs and report.

B-194 (Lumerical group-delay header) is **not** in scope (backlog only).

## Validation
Rebuild the reference prototype from the summary (stdlib Python, outside the repository) and check it bit for bit
against the C++ before the changes, as PF03c did; use it to predict items 3 and 4.

## Acceptance
- `pixi run test gcc`, `pixi run test asan-clang`: all OK, no sanitizer message.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: only the predicted cases change (and the two deleted
  cases are gone), each as predicted.
- clang and asan-gcc: before/after `compare` (strict) shows the same set.
- `stats-compare` (gcc) before/after: identical except the two deleted cases.
- Report `backlog/sprint-2026-10c/PF03d-REPORT.md` committed on `ws/spar4`: the completion marker.
