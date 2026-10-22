# B-179: PUF circuit: ring coupling must be aligned with the reference design before benchmark use
severity: med | category: physics | effort: XS | fix-class: needs-human | verdict: CONFIRMED | status: open (decision pending)
location: circuits/paul_3ring_puf.cir:5 (used at :39-43 and below)

## Problem
`circuits/paul_3ring_puf.cir` sets the coupling of every add-drop ring to `k=0.22360679774997896`, i.e. √0.05:
with the netlist's field convention (B-037, `doc/netlist.md`), this is a 5 % cross-coupling power ratio. The value
determines the rings' loaded Q, so it changes the circuit's spectral response, its event activity and its
convergence behaviour by large factors. It has not been checked against the reference design of this PUF, and the
maintainer asked that it be aligned before the circuit is used as a benchmark.

## Evidence
`circuits/paul_3ring_puf.cir:5`: `.assign R=10e-6 dR=10e-9 k=0.22360679774997896`, passed as `k1={k} k2={k}` to every
`add_drop_filter` instance (`:39-43` and the bottom chain).

## Expected
The netlist's `k` equals the reference design's field coupling (if the design specifies a power ratio κ, then
`k = sqrt(κ)`, or the netlist uses `kp=κ`), with a comment stating which convention and source value it follows.

## Suggested fix
Maintainer decision: pick the reference value, set it in `circuits/paul_3ring_puf.cir` with a comment, and
re-record its golden case (today it is an expected-error case, because its values file is not in the repository).

## Golden impact
`cir/paul_3ring_puf` (expected error before the coupling matters: missing values file) is unaffected until the
values file is added; any benchmark built on this netlist changes.

## Related
B-037 (field vs power coupling), F-28 (`.op`/`.dc` convergence on high-Q rings).
