# B-147: Crossing outputs are connected AS_READER, so output light is dropped on bidir nets
severity: high | category: bug | effort: XS | fix-class: needs-human | verdict: CONFIRMED
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
