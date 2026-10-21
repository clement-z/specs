# B-037: COUPLER `k` is field coupling while the paper's κ is power: document it
severity: low | category: docs | effort: XS | fix-class: needs-human | verdict: CONFIRMED | status: fixed in 8e30077 (sprint 2026-10b P2): documented in doc/netlist.md, warning removed
location: src/parser/parse_element.cpp:281, src/parser/parse_element.cpp:303-322, doc/resources/cheatsheet.pdf, src/devices/crow.h:30
## Problem
These are the facts. The only defect is that the same symbol and value mean different things in the paper and in the
netlist, plus a misleading warning.
- Netlist: positional arg0 and `K|KF|KFIELD` set `Pthr = 1 - k²` (:305, :316). `k` is therefore the **field**
  cross-coupling amplitude, and P_cross = k².
  - `KP|KPOW|KPOWER` set `Pthr = 1 - kp` (power, :319).
  - `T` sets `Pthr = t²` (through field, :322).
  - The cheatsheet says "k is the cross coupling coefficient for field", which is consistent.
- Every COUPLER instance prints `Warning: coupling power for DC currently set to field.` (:281). The comments at
  :318/:321 say "field coef" for the power and through keywords.
- Paper:
  - Fig. 1b shows a netlist with `k=0.15`.
  - In the Results section, "κ = 0.15 power cross-coupling ratio" describes the CROW benchmark (Fig. 3). That
    benchmark is built in C++ with `m_coupling_through = 0.85` (power), not from a netlist.
- Fig. 1c was produced with **field** semantics:
  - The through-port spike at pulse start peaks at ≈0.98 W (page 3 rendered at 600 dpi). This equals 1 − 0.15²
    = 0.9775. Power semantics would give 0.85.
  - The drop rise time (≈0.2 ns field time constant) also matches P_cross = 2.25% with the default ng and 600 µm ring.
  - Running the repo's `circuits/add_drop_pyspecs.cir` (Fig. 1 circuit, `k=0.15`) gives max P(out) = 0.9775 at
    t = 500 ps, with round-trip events every 4 ps.
  - Nothing suggests the semantics changed.
- The Fig. 1b netlist does not run as printed: `.save`, `.dc WL(cwsrc1) ...` and `values="pulse.txt"` are not
  accepted by the current grammar (see A4/A6).
## Evidence
- Code lines above.
- `specs -f circuits/add_drop_pyspecs.cir`, then VCD scan of PROBE1 (out): `max out power 0.9775 at 500 ps`. The
  following events are at 504, 508, 512 ps.
- Fig. 1c rendered with `pdftoppm -r 600`: the spike top sits about 6 px below the 1.0 gridline at 345 px/W.
## Expected
Unambiguous documentation of which quantity each keyword sets. There should be no per-instance warning that suggests
something is misconfigured.
## Suggested fix
(needs-human: naming decision) Keep `k` = field, because changing it alters every golden cir case and the paper
figure. Then:
- Say "k = field amplitude, P_cross = k²; use kp for power" in the cheatsheet/README.
- Fix the comments at :318/:321.
- Replace the :281 warning with nothing, or with a one-time note.
- Optionally deprecate bare `k` in favour of `kf`/`kp`.
## Golden impact
If the :281 warning line is removed, these 13 cases lose one stdout line per coupler (debug noise only):
- cir/add_drop, cir/add_drop_pyspecs, cir/subckt_tests, cir/paul_3ring_puf;
- cir-json/add_drop, cir-json/add_drop_pyspecs, cir-json/subckt_tests;
- opt/multi-file, opt/mode-fd, opt/mode-sampled-time, opt/tol-resolution-overrides, opt/tracefile-o, opt/vci.

This list comes from `grep -l "^Warning: coupling power for DC"` over the baseline. Changing the semantics of `k`
would change the VCDs of every coupler case; not recommended.
## Related
A4 S3.28 (copy-pasted "coupling power for DC" warning on PSHIFT :378 and MZI :475). A6 S3.05 (cheatsheet).

- Discovery seeds: S2.05 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-07.
## Verification
Code reading of the cited lines, the run of add_drop_pyspecs.cir with VCD inspection, and visual inspection of the
paper figure.
