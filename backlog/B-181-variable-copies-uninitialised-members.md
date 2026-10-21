# B-181: `Variable` copies uninitialised members (undefined behaviour on every netlist parse)
severity: med | category: memory/UB | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed (see commit)
location: src/parser/parse_tree.h:41-47, :277

## Problem
`Variable` stores one value per type (`num`, `cnum`, `inum`, `bnum`, …) and each constructor initialises only the
member of its own type. The defaulted copy assignment (`:277`, also used by the copy constructor) copies every
member, so it reads the others while they are indeterminate. Loading an indeterminate `bool` is undefined
behaviour: gcc's UBSan (`-fsanitize=bool`) reports `load of value 190, which is not a valid value for type 'bool'`
(190 = 0xbe, ASan's fill byte for fresh heap memory) on every netlist parse, which made the gcc ASan/UBSan CI job
fail in 146 golden cases (stderr differs). clang's UBSan did not report it.

## Fix
Default member initialisers for `type` (`NONE`), `num`, `cnum`, `inum` (0) and `bnum` (`false`). Constructors that
set a member still override it; no value that is ever read changes.

## Verification
gcc 11 ASan+UBSan (debug, as in CI): `meson test` 12/12, no sanitizer message in any test log (was: golden 44/190).
gcc release: golden 190/190 at `--rtol 0 --atol 0 --print-slack 0` (bit-identical). clang release and clang
ASan+UBSan: `meson test` 12/12.

## Golden impact
None.
