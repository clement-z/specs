# B-016: Output ordering depends on heap addresses (pointer-ordered `std::set`)
severity: low | category: determinism | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/utils/sysc_utils.h:26-66, src/specs.cpp:359-361,282-298,348-353
## Problem
`sc_get_all_{module,object}_by_type` return `std::set<T*>`, ordered by pointer. This order drives `init()`
order (and with it the order of sub-module creation and `sc_spawn` in MZI/GTD `init()`), auto-probe creation
(VCD variable ids), and the OP printout order. The seed's claim of differences "across identical runs" is wrong:
6 identical runs gave identical stdout and VCD, because glibc malloc is deterministic. Any change in heap layout
reorders the output.
## Evidence
`GLIBC_TUNABLES=glibc.malloc.tcache_count=0 specs -f circuits/subckt_tests.cir`: the OP printout starts with
`ROOT/X1/ring_tl` instead of `ROOT/X1/ring_bl`, and the VCD differs in 2427 diff lines (identifier assignment).
## Expected
Order is independent of the allocator, the compiler and unrelated code changes.
## Suggested fix
Use a name comparator (`strcmp(a->name(), b->name())`) or return a vector in hierarchy (creation) order. Choosing
between those two is the design decision. I built the name-order variant: all numeric results are unchanged.
## Golden impact
Measured for name order: stdout is reordered in cir/add_drop_pyspecs, cir/subckt_tests, opt/tracefile-o,
opt/mode-sampled-time and opt/vci (accepted as "reordered" by the harness). `tb/mesh` FAILs because its W505
warning order changes (re-record).
## Related
none.

- Discovery seeds: S1.13, S4.08 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-16.
## Verification
Ran the malloc-tunable runs, rebuilt specs.cpp, parse_analysis.cpp and time_monitor.cpp with the name comparator, and ran the golden check.
