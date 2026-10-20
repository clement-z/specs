# B-070: Clements mesh: no output phase screen, bypass rows lack the MZI's coupler phase, misleading "zero neff" comments
severity: low | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/devices/mesh_col.cpp:81,113-116,123-126,147-150,171,192-195,215,221-224, src/devices/clements.cpp:37-110
## Problem
- The comments say "Zero neff so it doesn't add any phase" and "TODO: snap to 2pi phase instead of setting neff = 0", but the bypass waveguides are built with `m_neff`. Their phase therefore matches the MZI arm propagation phase, which is reasonable but not what the comment claims.
- Bypass rows lack the global phase that each MZI picks up from its two 50/50 couplers (j·e^{jθ/2}).
- `Clements` has no final diagonal phase screen D, so not every unitary of Clements et al. 2016 can be programmed without external phase shifters.
## Evidence
Code at the cited lines. MZI = DC·diag(e^{jθ},1)·DC·diag(e^{jφ},1), with DC = (1/√2)[[1, j],[j, 1]] (mzi.cpp:29-54, directional_coupler.cpp).
## Expected
A documented phase convention and programming recipe, optionally an output phase screen, and comments that match the code.
## Suggested fix
A human confirms the intended convention. Then fix the comments (trivial) and optionally add N output phase shifters to `Clements`.
## Golden impact
tb/mesh only (after B-047), and only if the topology changes.
## Related
B-047, A2 (MZI/DC conventions).

- Discovery seeds: S2.21 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-24.
## Verification
Code reading.
