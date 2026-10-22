# B-051: PCMCELL/PCMElement is fully opaque by default (constructor Tc = Ta = 0)
severity: high | category: physics | effort: XS | fix-class: needs-human | verdict: CONFIRMED | status: fixed in 344d60b (sprint 2026-10b P2)
location: src/devices/pcm_device.h:26-27,53-54,59-60, src/devices/octane_cell.cpp:32,77, src/parser/parse_element.cpp:1231-1234
## Problem
The member defaults say `m_Tc = 0`, `m_Ta = 1` (pcm_device.h:26-27). The constructor defaults `Tc = 0, Ta = 0` (pcm_device.h:53-54) override them, so T(k) = Tc + (Ta−Tc)·tanh(...) ≡ 0.
- A `PCMCELL` without `tc=`/`ta=` blocks all light.
- `OctaneCell` builds its PCM without Tc/Ta (octane_cell.cpp:32,77), so every OCTANE cell is opaque and its photodetector reads 0.

Even with the header defaults (Tc = 0), a cell in its initial state 0 is opaque. Nothing in the code or docs says which values are intended.
## Evidence
`backlog/repro/a3-pcm-opaque.cir` (1 mW -> `pcmcell1 in out emelt=1e-12 n=10`, `.op`):
- OP `ROOT/out` = 0.000000 W.
- With `tc=0.85 ta=0.95` added: 0.000850 W.
## Expected
Physical defaults, for example Tc ≈ 0.85 and Ta ≈ 0.95 as in tb/pcm_device_tb.cpp:92, or an error when Tc/Ta are missing. The header comment and the constructor must agree.
## Suggested fix
A human must choose the default values, or decide to make `tc`/`ta` required in the parser (as Emelt/N already are, parse_element.cpp:1242-1246). Then make the constructor defaults match the member defaults and pass explicit values from OctaneCell.
## Golden impact
None: no golden netlist uses PCMCELL, and tb/pcm passes Tc/Ta explicitly.
## Related
B-060 (PCM model), B-069 (OCTANE composites are unreachable today), B-052 (positional Tc/Ta).

- Discovery seeds: S2.01 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-05.
## Verification
I ran the repro with and without tc/ta using the release binary.

**Independent second check (V1, of F-A3-05): CONFIRMED.** Method: ran a3-pcm-opaque.cir with and without tc/ta using the release binary, and read pcm_device.h

- **Without tc/ta:** OP `ROOT/out` = 0.000000 W.
- **With `tc=0.85 ta=0.95`:** 0.000850 W.
- **Code:**
  - pcm_device.h:26-27 has member defaults Tc=0 and Ta=1.
  - The constructor defaults at :53-54 are Tc=0 and Ta=0 and override them.
  - octane_cell.cpp:32 and :77 pass no Tc/Ta.
- **Class:** needs-human is correct, because this is a physics default.
- **Severity:** high is defensible (silent all-zero output). PCMCELL is undocumented and not used by any golden case, so med would also be reasonable.
