# B-180: Integer literals beyond the `int` range are clamped to 2147483647 without an error
severity: med | category: parser | effort: S | fix-class: needs-human | verdict: CONFIRMED | status: fixed in bd2465c (sprint 2026-10c phase A)
location: src/parser/parser.l:386-390, src/parser/parse_tree.h:44, src/parser/parser.y:54

## Problem
The lexer reads every digit-only literal with `stringstream >> int`, which clamps a value beyond the `int` range to
2147483647 and sets no error that anyone checks. Different netlist values silently become the same one:
`.options seed=3000000000` and `seed=99999999999999999999` are the same seed, `.assign n=3000000000` is 2147483647,
and a net named `3000000000` is `N2147483647`. Found in sprint 2026-10b P4 (F-01), where `.options seed` worked
around it by rejecting the integer literal 2147483647 with a hint (exponent form or `specs --seed`).

## Evidence
`err/f01-options-seed-int-max` (`.options seed=3000000000`) and `err/f01-options-seed-huge-integer`
(`seed=99999999999999999999`) both print `SEED=2147483647` (P4 guard). `backlog/sprint-2026-10b/P4-REPORT.md`,
"Held back, needs sign-off".

## Decision
Maintainer, 2026-10-21 (second round, supersedes the follow-up round's "parse error beyond the `int` range"): integer
literals are lexed as 64-bit signed integers; a literal beyond 9223372036854775807 is a clean parse error with
file:line and the limit; consumers that store a narrower type check their own range with a clean error. Not the
double-lexing approach of `P4-ready/lexer-int-overflow` (dropped).

## Golden impact (predicted)
`err/f01-options-seed-huge-integer`: the new parse-error message. `err/f01-options-seed-int-max` becomes a passing
case (`regress/`, VCD identical to `--seed 2147483647`). New cases: `.options seed=3000000000` (VCD identical to
`--seed 3000000000`), a literal beyond 64 bits in a keyword argument, a positional argument and a net name, and a
narrow consumer out of its range.

## Related
F-01 (`.options seed`), F-28 (`.options maxiter`), B-153 (`.options` validation), B-181 (`Variable` initialisers).

## Fix (sprint 2026-10c phase A, bd2465c)
- Lexer: `std::from_chars` into `int64_t`; beyond 9223372036854775807: `Parsing error (<file>:<line>): integer
  literal ... is out of range (the limit is 9223372036854775807)` plus the line, exit 1.
- `%union i_val`, `Variable::inum`, `Variable(int64_t)`, `as_integer()` are 64-bit; `as_integer()` of a DOUBLE outside
  the 64-bit range (or NaN) is a clean error instead of an undefined cast.
- Narrow consumers: PCMCELL `n` and `k` (stored as `int`) reject values beyond the int range with
  `Error (<name>): ... must be between ... and 2147483647 (got ...)`. Numeric net names keep the full range
  (`N3000000000`). `.options timescale` and `resolution` were already range-checked as doubles.
- `.options seed` and `maxiter`: an integer literal is exact, 0 to 2^63 − 1; exponent form keeps 0 to 2^53. P4's
  guard against 2147483647 is removed.
- Golden: `err/f01-options-seed-huge-integer` (new message), `err/f01-options-seed-int-max` →
  `regress/f01-options-seed-int-max` (`seed=2147483647`, VCD identical to `--seed 2147483647`), new
  `regress/f01-options-seed-int-literal` (VCD identical to `--seed 3000000000`), `regress/f01-options-seed-int64-max`,
  `err/b180-int-literal-{kwarg,positional,net-name}`, `regress/b180-numeric-net-names`,
  `err/b180-pcmcell-{n,k}-beyond-int`; gtest `IntegerLiterals*`.
