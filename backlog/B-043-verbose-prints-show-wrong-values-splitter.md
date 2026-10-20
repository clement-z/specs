# B-043: Verbose prints show wrong values (splitter ratio "0.5", PS "@1.55)")
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 3c2b444 (ws-cleanup C-02)
location: src/devices/splitter.cpp:14, src/devices/phaseshifter.cpp:16, src/devices/phaseshifter.cpp:91
## Problem
With `--vci`:
- the splitter always prints `splitting ratio_power = 0.5 W/W`, whatever `m_split_ratio` is;
- the phase shifter prints `initial phase delay = 0 rad @1.55)`, with a stray "@1.55)" copied from the waveguide.
## Evidence
- splitter.cpp:14: `cout << "splitting ratio_power = 0.5" << " W/W" << endl;`
- phaseshifter.cpp:16 and :91: `... << m_phaseshift_rad << " rad @1.55)" << endl;`
## Expected
The splitter should print the actual ratio, and the PS label should be clean.
## Suggested fix
- splitter.cpp:14 becomes `cout << "splitting ratio_power = " << m_split_ratio << " W/W" << endl;`
- phaseshifter.cpp:16 and :91: change `" rad @1.55)"` to `" rad"`.
## Golden impact
None. Only opt/vci uses `--vci`. Its netlist (subckt_tests.cir plus the included add_drop_sub.cir) has only VLSRC,
WG and COUPLER, and its baseline stdout contains neither string.
## Related
The bitstream part of S2.26 is B-044.

- Discovery seeds: S2.26 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-13.
## Verification
Code reading. I checked the element lists of circuits/subckt_tests.cir and add_drop_sub.cir, and grepped
tests/golden/baseline/cases/opt/vci/stdout.txt (0 matches).

**Independent second check (V4, of F-A2-13): CONFIRMED.** Method: Applied the 3 print edits, rebuilt. Golden check (--print-slack 0, --strict-order): 44/44 PASS. Diffed `--vci` output.

- A netlist with `splitter ratio=0.3` plus `pshift` under `--vci` now prints `splitting ratio_power = 0.3 W/W` and `initial phase delay = 0 rad`.
- tb ps and tb phaseshifter with `--vci` differ only in that line.
- tb spli and tb splitter are unchanged, because they use ratio 0.5.
- verbose_component_initialization is only set by the CLI flag (main.cpp:338), never by a tb.
- The opt/vci baseline does not contain "splitting ratio_power" or "initial phase delay". Its 5 "@1.55)" hits are waveguide "phase delay" lines, which this fix does not touch.
