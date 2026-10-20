# B-031: Fix MZIActiveBi: p_vin bound twice (elaboration abort) and arms miswired
severity: high | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/mzi_active.cpp:165,192 (double bind), src/devices/mzi_active.cpp:172,175,199,202 (arm wiring), src/devices/mzi_active.cpp:100 (wire name)
## Problem
An `MZI*` netlist element instantiates `MZIActiveBi` as soon as its first or second optical net is bidirectional
(`&`). That variant cannot run, for two reasons:
1. `ps->p_vin(p_vin)` is bound twice, once in the forward block (:165) and once in the backward block (:192).
   SystemC rejects the second bind at elaboration, so every bidirectional MZI netlist aborts with exit 1.
2. In the lower (reference) arm, the forward `wg3->p1_out` and `dc2->p1_in` are bound to the phase-shifter output net
   `m_ps_p1_out` (:172, :175) instead of `m_dc2_p1_in`. The backward `wg3->p1_in` and `dc2->p1_out` are bound to
   `m_ps_p1_in` (:199, :202) instead of `m_dc2_p1_out`. As a result `m_ps_p1_out` has two writers (the PS and WG3),
   WG2 (upper arm) also reads the lower arm, and `m_dc2_p1_in`/`m_dc2_p1_out` are created (:92, :100) but never
   bound. If only bug 1 is fixed, the MZI gives wrong results.

Also, the wire created at :100 for DC2 is named `"/DC1_in2_1"`; it should be `"/DC2_in2_1"`.
## Evidence
Repro: `backlog/repro/a2-mzi-bidir.cir`. The `d&` dangling net forces the Bi variant; plain `in&` triggers the same
error. Run with `specs -f a2-mzi-bidir.cir`.

Stock binary output:
```
Error: (E107) bind interface to port failed: interface already bound to port: port 'ROOT/MZI1/PS1.port_0' (sc_port)
In file: .../thirdparty/systemc/src/sysc/communication/sc_port.cpp:235
exit=1
```
I then built a scratch copy with only line :192 removed and ran `.op` (V=0, length=10e-6, length_ref=10.2e-6, att=1).
The result was `out1:0 = 0.5 W (0.7854 rad)` and `out2:0 = 0.5 W (0.7854 rad)`. The same netlist without `&`
(MZIActiveUni) gives `out1 = 0.605467 W (0.5371 rad)` and `out2 = 0.394301 W (-2.6045 rad)`.
## Expected
MZIActiveBi should give, in the forward direction, the same transfer as MZIActiveUni. It should be reciprocal in the
backward direction.
## Suggested fix
In src/devices/mzi_active.cpp:
```
:100  m_dc2_p1_out = make_shared<spx::oa_signal_type>((string(name()) + "/DC1_in2_1").c_str());
   -> m_dc2_p1_out = make_shared<spx::oa_signal_type>((string(name()) + "/DC2_in2_1").c_str());
:172  wg3->p1_out(*m_ps_p1_out);   ->  wg3->p1_out(*m_dc2_p1_in);
:175  dc2->p1_in(*m_ps_p1_out);    ->  dc2->p1_in(*m_dc2_p1_in);
:192  ps->p_vin(p_vin);            ->  (delete this line; p_vin is already bound at :165)
:199  wg3->p1_in(*m_ps_p1_in);     ->  wg3->p1_in(*m_dc2_p1_out);
:202  dc2->p1_out(*m_ps_p1_in);    ->  dc2->p1_out(*m_dc2_p1_out);
```
Optionally, add `backlog/repro/a2-mzi-bidir.cir` as a new golden case in the same commit.
## Golden impact
None. No golden case instantiates MZIActive: there is no `MZI` netlist element in circuits/, and the tbs use the
2-PS `MZI` class. I ran the full golden check with this fix applied and all 44 cases passed.
## Related
- B-038 covers which MZI nets trigger the Bi variant.
- Cross-area A4: a uni source that is created before a bidirectional device on a `&` net never reaches it (see the
  final report). For that reason the repro puts the `&` on a dangling net.
- discover/02-devices-physics.md §4 "MZI".

- Discovery seeds: S2.03, S2.04 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-01.
## Verification
- Ran the repro on `/tmp/ws-review-build/specs` and got E107, exit 1.
- In a scratch build (`/tmp/ws-review-A2/build`) I applied the 5 binding changes:
  - Forward `.op` gave `out1:0 = 0.605467 W / 0.5371 rad` and `out2:0 = 0.394301 W / -2.6045 rad`. This is
    identical to MZIActiveUni.
  - With the source on `out1`, the backward run gave `in:0 = 0.605467 W` and `d:0 = 0.394301 W` (reciprocal).
  - Golden check: 44/44 PASS.

The fix is safe because it only changes bindings inside MZIActiveBi, which currently cannot elaborate. The rename
only changes an internal signal name.
