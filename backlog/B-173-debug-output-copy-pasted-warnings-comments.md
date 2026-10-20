# B-173: Debug output, copy-pasted warnings and comments, duplicate TRACEALL branch
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
