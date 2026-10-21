# B-004: `.op`/`.dc`/`.tran` silently override the `-m` CLI mode
severity: med | category: cli | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in c6414ce (sprint 2026-10b P2): explicit -m overrides the analysis default
location: src/parser/parse_analysis.cpp:26,32,85, src/main.cpp:290-312
## Problem
`-m` writes `specsGlobalConfig.simulation_mode`. Every analysis `create()` then overwrites it unconditionally
(FD for OP/DC, TD for TRAN). `-m` prints "Optical ports working in event-driven mode by default" and is then
ignored. It only has an effect on netlists without an analysis directive and on testbenches (crow_tb.cpp reads
the mode). Golden `opt/mode-*` outputs are identical to the default runs.
## Evidence
```
specs -m td -f backlog/repro/a1-op-no-wavelength.cir
Optical ports working in event-driven mode by default
- simulation mode: Event-driven frequency-domain
```
## Expected
The documented precedence applies. Commit 1b0780d says "cmdline args priority". A conflicting `-m` should
produce an error or a warning.
## Suggested fix
Design choice. One option: TRAN keeps a CLI-given mode (this allows FD transients); OP/DC reject `-m td` or
`-m sampled-time` with an error. This needs a "mode was set on the CLI" flag, because the analyses cannot tell a
default from an explicit choice. The part of seed S1.18 saying "extra analyses are silently ignored" is wrong: a
second analysis is a hard error (parse_tree.cpp:379-384, verified).
## Golden impact
`opt/mode-fd` and `opt/mode-sampled-time` may change, depending on the chosen rule.
## Related
B-002.

- Discovery seeds: S1.18, S4.06 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-04.
## Verification
Ran the command above. Code reading of parse_analysis.cpp:26,32,85. A two-analysis netlist exits with "Attempted to register a new analysis".
