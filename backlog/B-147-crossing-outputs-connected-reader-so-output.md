# B-147: Crossing outputs are connected AS_READER, so output light is dropped on bidir nets
severity: high | category: bug | effort: XS | fix-class: needs-human | verdict: CONFIRMED | status: fixed in 6d6bf9c (sprint 2026-10b P2): next-free-slot convention
location: src/parser/parse_element.cpp:539-540
## Problem
`CrossingElement::instantiate_and_connect_uni` binds `p_out1`/`p_out2` with `AS_READER`. On a unidirectional net
this only corrupts the connect counters. If the output net is bidirectional, `connect_uni` picks the *read* slot, so
the crossing writes the same signal as the bidir device on the other side, and the light never propagates.
## Evidence
`backlog/repro/a4-crossing-out-bidir.cir` (crossing output `c` feeds `wg1 c e`; `e` has 2 writers, so wg1 is
bidirectional): rc 0, `ROOT/c:0 = 1 mW`, `ROOT/c:1 = 0 W`, `ROOT/e:0 = ROOT/e:1 = 0 W`. The light is lost. The same
circuit with `coupler1 a b c d k=0` instead of the crossing gives `ROOT/e:0 = 1 mW`.
## Expected
`ROOT/e:0 = 1 mW`, as with the coupler.
## Suggested fix
`parse_element.cpp:539-540`: `nets[2], AS_READER` becomes `nets[2], AS_WRITER`, and `nets[3], AS_READER` becomes
`nets[3], AS_WRITER`.

Classified `needs-human` only because the fix changes the results of netlists that run today (with wrong results). The change itself is mechanical; a maintainer sign-off is enough.

**Dependency (from the second check): apply only together with or after B-144.** Applied alone, AS_WRITER breaks circuits that work today, where a crossing output `c&` is read by a uni device (`backlog/repro/v7-xing-v4.cir`, `v7-xing-v5.cir`: 0.5 mW becomes 0), and it does not fix `v7-xing-v3.cir`. With B-144 and this fix together, every variant is correct.
## Golden impact
None (no golden case has a crossing; verified with the harness). For crossings whose output is on a bidirectional
net, the result changes from "output silently lost" to the coupler-equivalent value. This is wiring, not modelling.
The orchestrator may downgrade it to needs-human if it applies the "valid results change" rule strictly.
## Related
B-159 (CrossingBi never used), B-144.

- Discovery seeds: S2.07, S3.13 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-04.
## Verification
Patched a scratch build: `e:0 = 1 mW, -2.1890 rad`, identical to the coupler version. Golden unchanged.

**Independent second check (V7, of F-A4-04): ADJUST.** Method: applied AS_WRITER alone and together with B-144; ran the repro, a coupler reference and 4 crossing variants of my own; golden check 44/44 for both

Base repro: e:0 = 0. Coupler (k=0) reference: e:0 = 1 mW at -2.1890 rad. With AS_WRITER alone, the repro gives e:0 = 1 mW at -2.1890 rad, as claimed.
**Applied alone, it breaks circuits that work today.** In `v7-xing-v4.cir` and `v7-xing-v5.cir`, the crossing output `c&` is read by a uni splitter. Base gives z1 = 0.5 mW (correct). With AS_WRITER alone, z1 = 0. The reason: a uni writer at writer count 0 takes slot 1 while readers read slot 0 (the B-144 convention).
`v7-xing-v3.cir` (crossing output `c&` read by wg1) is broken in base and stays broken with AS_WRITER alone.
With B-144 + F-A4-04 every variant is correct: repro e:0 = 1 mW; v3 e:0 = 1 mW and z1 = 0.5 mW; v4 and v5 z1 = 0.5 mW. `v7-xing-v2.cir` (crossing created first, its output upgraded later) is 1 mW in every build.
Changes needed:
- Keep needs-human, and add: "depends on B-144; must not be applied alone."
- Golden impact None is confirmed.
- AS_WRITER is the only correct binding for the uni variant. Instantiating CrossingBi when a net is bidirectional (as DC does, B-159) is a separate design option.
Location lines 539-540 are correct.

**Orchestrator:** Dependency on B-144 recorded; kept needs-human.
