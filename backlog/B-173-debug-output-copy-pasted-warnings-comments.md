# B-173: Debug output, copy-pasted warnings and comments, duplicate TRACEALL branch
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 8b8a88a, 4a95b53, baseline 09d5304 (ws-cleanup C-04, C-08)
location: src/parser/parse_element.cpp:378, :475, :1058, :1247, :304, :315, :318, :321, :495, :505, :647, :660, :663, src/parser/parse_tree.cpp:245, src/parser/parse_directive.cpp:38-39, src/parser/parse_analysis.h:65-66
## Problem
- PSHIFT (:378) and MZI (:475) print `default coupling power for DC currently set to field.`, copy-pasted from the
  coupler and meaningless there.
- Debug prints: `created <name>` (Probe :1058), `-----` (PCM :1247), `\t<sig0>,<sig1>` (parse_tree.cpp:245 on every
  bidir upgrade).
- The comment "convert from cross-coupling field coef" is pasted on the `kp`/`t` branches and on MZI/CWSRC lines
  (:304-321, :495, :505, :647, :660, :663). `k` is field coupling, `kp` power, `t` through field.
- The second `TRACEALL` branch (parse_directive.cpp:38-39) is unreachable (:28 matches first).
- `(" << n << "points)"` lacks a space.
## Evidence
Code at the cited lines. Golden stdout contains `created ROOT/PROBE1..3` (3 lines each) in cir/add_drop_pyspecs,
cir-json/add_drop_pyspecs, opt/mode-sampled-time and opt/tracefile-o.
## Expected
No debug noise. Comments match the code.
## Suggested fix
- Delete :378, :475, :1058, :1247 and parse_tree.cpp:245.
- Delete parse_directive.cpp:38-39.
- Fix the comments.
- Optional: `" points)"`.

Keep the coupler warning at :281: it documents `k` semantics (rewording it is a separate decision).

Corrections from the second check:
- The `" points)"` text is at `parse_analysis.h:68`, not :65-66.
- On the coupler, **do not touch the comments at :304 and :315** (they are correct). Only reword :318 (`kp`: power cross-coupling coefficient) and :321 (`t`: through field coefficient). Delete the comments at :495, :505, :647, :660 and :663.
## Golden impact
The `created ROOT/PROBEn` lines (3 per case) disappear from stdout of cir/add_drop_pyspecs,
cir-json/add_drop_pyspecs, opt/mode-sampled-time and opt/tracefile-o. The `points)` fix changes one line in
cir/add_drop, cir-json/add_drop, opt/mode-fd, opt/multi-file and opt/tol-resolution-overrides; skip it to keep the
commit noise-only. Nothing else.
## Related
B-174.

- Discovery seeds: S3.28, S5.16, S1.27 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-36.
## Verification
Grep of the golden stdout files. Removing the duplicate TRACEALL branch was verified neutral in the scratch build
(golden unchanged).

**Independent second check (V8, of F-A4-36): ADJUST.** Method: deleted :378, :475, :1058, :1247, parse_tree.cpp:245 and parse_directive.cpp:38-39, fixed the copy-pasted comments, left " points)" alone, rebuilt, golden --atol 0 and a full diff of the failing cases

The golden impact is exactly as claimed. Only cir/add_drop_pyspecs, cir-json/add_drop_pyspecs, opt/tracefile-o and opt/mode-sampled-time fail, each only because the 3 `created ROOT/PROBEn` lines are gone; no other file differs. The `\t<sig0>,<sig1>` upgrade print, the PSHIFT/MZI warnings and `-----` do not appear in any golden output. The duplicate TRACEALL branch is unreachable, and removing it is neutral. The `(30points)` line is in exactly the 5 cases listed. Location corrections: (1) the "points)" text is at **parse_analysis.h:68**, not :65-66. (2) On the coupler, the comments at **:304 and :315 are correct** (`k` = cross-coupling field coefficient, 1-k^2). Only :318 (`kp`: should say power coefficient) and :321 (`t`: through field coefficient) are wrong. The comments at :495, :505, :647, :660 and :663 should just be deleted. Reword "Fix the comments" accordingly so an unattended fixer does not touch :304/:315. The other locations are correct.

**Orchestrator:** Location corrections recorded (comment lines to keep vs reword).
