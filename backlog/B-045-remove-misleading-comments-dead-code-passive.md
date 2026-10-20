# B-045: Remove misleading comments and dead code in passive-device files
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/mzi.h:14, src/devices/mzi.h:43, src/devices/mzi.cpp:3-4, src/devices/mzi_active.h:35, src/devices/mzi_active.h:44, src/devices/mzi_active.cpp:119, src/devices/crossing.h:30, src/devices/splitter.h:46-50, src/devices/waveguide.cpp:39, src/devices/generic_2x2_coupler.h:35, src/parser/parse_element.cpp:295, src/parser/parse_element.cpp:495
## Problem
The following changes are all comment edits or dead-code removal. They change no output.
- mzi.h:14 says "A unidirectional waveguide module." for the MZI class.
- mzi.h:43 declares `WG3`, which is never created or used.
- mzi.cpp:3-4 defines `__modname`, which is unused in that file.
- mzi_active.h:35/83/146 say "Constructor for Waveguide".
- mzi_active.h:44/92/155 say "insertion loss of the y-junctions", but they are directional couplers.
- mzi_active.cpp:119 says "cast as unidirectional devices" in the Bi variant.
- crossing.h:30/58/98 say "Crosstalk relates to out2/in1 when there's nothing in in1"; it should be in2.
- splitter.h:46-50 is an orphan doc comment over a commented-out `// static double m_delay_ns;`. Doxygen attaches it
  to `m_attenuation_dB`.
- waveguide.cpp:39 and :137 are commented-out `cout`s.
- generic_2x2_coupler.h:35 has an unused local `double lambda0 = 1550e-9;`. It is the only compiler warning in this
  area (`-Wunused-variable`, seen in the scratch build log).
- generic_2x2_coupler.h:45-47 and :79-81, and generic_waveguide.h:105-110, are commented-out `cout`s.
- parse_element.cpp:
  - :295 says "for waveguide" in DCElement;
  - :495 and :505 say "convert from cross-coupling field coef" on MZI `sensitivity`;
  - :318 and :321 have the same comment on KP and T (see also B-037).
## Evidence
`grep -n` of each cited line. The unused-variable warning appeared in `/tmp/ws-review-A2/build.log`.
## Expected
Comments that match the code, and no dead declarations.
## Suggested fix
- Fix the comment texts as indicated.
- Delete mzi.h:43 `WG3` (change `unique_ptr<Waveguide> WG1, WG2, WG3;` to `WG1, WG2;`).
- Delete mzi.cpp:3-4, splitter.h:46-50, waveguide.cpp:39/137, generic_2x2_coupler.h:35/45-47/79-81 and
  generic_waveguide.h:105-110.

Do not touch print strings; those belong to B-043.

Overlap: the comment at `parse_element.cpp:495` is deleted by B-173 (which also rewords the coupler comments at :318/:321); skip it here. The same misleading comment at :647/:660/:663 (CW source) is handled by B-173 / B-052.
## Golden impact
None. Comments, an unused member, an unused macro and an unused local only.
## Related
B-031 removes the other dead members (`m_dc2_p1_in/out` become used). B-040 covers the GenericWaveguide comments
at :80-81 and :117.

- Overlap: `generic_2x2_coupler.h:35` (unused `lambda0`) is also deleted by B-111 (compiler-warning cleanup). Whichever lands first, the other skips that line.

- Review ids: F-A2-15.
## Verification
Code reading. `WG3` and `__modname` have no use in mzi.*, and the `lambda0` local is never read.

**Independent second check (V4, of F-A2-15): CONFIRMED.** Method: Applied all listed comment edits and dead-code deletions, rebuilt. Golden check (--atol 0 --print-slack 0 --strict-order): 44/44 PASS. `--vci -t mzi` and `-t mesh` give identical output.

- Every cited line matches the description:
  - mzi.h:14 and :43;
  - mzi.cpp:3-4 (`__modname` unused there; the clements and octane copies are separate);
  - mzi_active.h:35/83/146 and :44/92/155;
  - mzi_active.cpp:119;
  - crossing.h:30/58/98;
  - splitter.h:46-50;
  - waveguide.cpp:39/137;
  - generic_2x2_coupler.h:35, 45-47 and 79-81;
  - generic_waveguide.h:105-110;
  - parse_element.cpp:295, 318, 321, 495 and 505.
- `WG3` in mzi.h has no use anywhere in src/.
- The `-Wunused-variable lambda0` warning disappears. generic_2x2_coupler.h is only compiled via src/tb/directional_coupler_tb.cpp, which was rebuilt.
- Nit: "Fix the comment texts as indicated" gives no replacement text for :295/:318/:321/:495/:505. I used:
  - "for coupler";
  - "cross-coupling power coef";
  - "through field coef";
  - "phase-shifter sensitivity (rad/V)".
- Deleting the comment blocks leaves double blank lines in generic_2x2_coupler.h. Tidy those by hand.
- The same misleading "convert from cross-coupling field coef" comment also appears in CWSourceElement (parse_element.cpp:647/660/663). It is out of scope here, but might be worth adding.
