# B-128: golden numeric "print slack" lets low-precision numbers change by 100 % or more (tolerances, config printout)
severity: med | category: test-gap | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 0474575, 1adc540 (sprint 2026-10b P1)
location: tests/golden/run_golden.py:175-182 (`printed_ulp`), :207-209 (slack), :1091-1094 (`--print-slack` default 1)
## Problem
The "+-1 unit in the last printed digit" slack is meant for 6-digit floats printed by SystemC. For a token with few digits, one unit of the last digit is as large as the value itself, so the check accepts any change of up to that size. Every case prints the config block (`abstol: 1e-08`, `default reltol: 0.0001`), the exact quantities CLAUDE.md says need human sign-off, and these lines are not otherwise protected unless the case traces them in a VCD.
## Evidence
`python3 -I backlog/repro/a5-golden-slack-probe.py <repo>` (uses the real `Tol`/`compare_text` with the default settings):
```
PASS '- abstol: 1e-08'            -> '- abstol: 2e-08' / '1e-07' / '0'
PASS '- default reltol: 0.0001'   -> '0.0002' / '0.001' / '0.0000'
PASS 'pulse 2.0 W'                -> 'pulse 2.1 W'
fail '- engine timescale (log10 of s): -13' -> '-14' (integers are exact)
```
The current build passes with `--print-slack 0 --atol 0 --rtol 0` (44/44), so the slack is only needed for cross-compiler differences in the last digit of long tokens.
## Expected
Slack only for tokens with enough significant digits (5 or more) so the permitted change is at most 1e-4 relative; shorter tokens compare exactly.
## Suggested fix
In `printed_ulp()` return 0.0 when the token has fewer than 5 significant digits (count digits of the mantissa without leading zeros). Choice of threshold is a design decision, hence needs-design. Related caveat already in README: `atol=1e-12` makes values below about 1e-3 effectively absolute.
## Golden impact
none for the Release build (passes with zero slack).
## Related
B-126.

- Review ids: F-A5-08.
## Verification
Ran the probe script and the zero-slack check (above).
