# B-071: The paper's CROW benchmark cannot be reproduced from the repo
severity: low | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/devices/crow.cpp:24-48, src/tb/crow_tb.cpp:33,179-191
## Problem
The ECOC paper benchmarks a CROW with 2N waveguides whose lengths are L_i ~ N(33.2, 5.0) µm, κ = 0.15 (power), N up to 512, and 10 pulses of 1 mW / 1 ns at 500 MHz.
- `CROW::connect_submodules()` gives every waveguide the same length `m_ring_length/2` (crow.cpp:28). It has no per-waveguide lengths and no random variation, and the variation code is commented out (:20-21, :27, :30).
- crow_tb uses different values: L ≈ 776 µm per ring, through power 0.8, neff = 1, ng = 2, N = 3 (crow_tb.cpp:179-190, overriding values set two lines earlier).
## Evidence
Code at the cited lines. The paper is discover/paper-zrounba2023-specs-ecoc.txt (Results §).
## Expected
A way to reproduce Fig. 3, for example per-waveguide lengths plus a seeded normal sampler, and a tb or netlist with the paper's parameters.
## Suggested fix
A human decides whether reproducing the paper is a goal. If so, add `setWaveguideLengths(vector<double>)` or a seeded variation option, and a `crow_paper` tb.
## Golden impact
None, unless tb/ac_crow parameters are changed.
## Related
B-072, B-055 (seeding).

- Discovery seeds: S2.22 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-25.
## Verification
Code reading against the paper text.
