# B-158: Elaboration is super-linear in circuit size (linear scan of all elements per net)
severity: med | category: perf | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.cpp:64-145, src/parser/parse_tree.cpp:702-804
## Problem
- `next_fresh_bidir_net`/`next_fresh_net` restart from `nets.begin()` on every call.
- For each net, they call `next_fresh_element_bound_to`, which scans every element and string-compares its nets.

Elaboration is at least O(N_nets × N_elements), which defeats the "scalable" claim for large meshes.
## Evidence
`backlog/repro/a4-elab-perf.sh <specs>` (chain of N waveguides, `-n`), Release, loaded machine:
N=1000 0.09 s, N=4000 1.28 s, N=8000 8.16 s.
`perf record` on N=4000: 42% in `next_fresh_element_bound_to`, 24% `memcmp`, 6% `std::string ==`.
Replacing the `find_if` over `circuit_signals`/`circuit_modules` with `map::count` did not help (measured 1.57 s /
9.16 s).
## Expected
Close to linear.
## Suggested fix
Build a net→elements adjacency (in element order) once, keep a cursor over nets, and keep the per-net elements in
vector order. Element creation order must stay identical, because B-144/02 make results order-dependent until
they are fixed. Check with the golden harness.
## Golden impact
None if the creation order is preserved.
## Related
B-144, B-176.

- Review ids: F-A4-18.
## Verification
Timings and perf profile on the Release binary and the scratch build.
