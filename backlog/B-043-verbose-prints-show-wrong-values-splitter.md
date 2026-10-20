# B-043: Verbose prints show wrong values (splitter ratio "0.5", PS "@1.55)")
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
