# B-082: KiCad MZI template maps 4 of its 5 pins and the EVLSRC list template has three columns instead of two
severity: med | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: utils/specs.kicad_sym:1120 (MZI Sim.Pins), :799 (ELECTRIC_V_SOURCE_LIST Sim.Params)
## Problem
1. `MZI` has 5 pins (pin 5 at the top, `utils/specs.kicad_sym:1461-1463`) and `MZIElement` requires 5 nets (`DECLARE_BIDIR_ELEMENT(MZIElement, ..., 5)`, parse_element.h:85) but `Sim.Pins` is `1=1 2=2 3=3 4=4`; KiCad exports only 4 nets and the line fails to parse.
2. `ELECTRIC_V_SOURCE_LIST` carries the VLSRC template `values=[[t0,p0,1550e-9],[t1,p1,1550e-9]]`; EVLSRC takes Nx2 rows `[t, V]` (parse_element.cpp:850-960 "Nx2 list [[t0, V0], ...]"), so the template is rejected as soon as the placeholders are replaced with numbers.
(The seed's claim that the `*_SOURCE_FILE` templates use `values=\` is wrong: the file contains `values=\"file.txt\"`, a properly escaped quote that KiCad unescapes to `values="file.txt"`, which the parser accepts.)
## Evidence
```
$ specs -n -f mzi4.cir   (mzi1 a b c d length=1e-6 ... )     -> Parsing error ... offending token: "length"
$ specs -n -f mzi5.cir   (mzi1 a b c d e length=1e-6 ... )   -> DONE, rc=0
evlsrc1 a values=[[0,1,1550e-9],[1e-9,0,1550e-9]]            -> Variable passed to keyword 'values' should be either: - an Nx2 list [[t0, V0], ...]
```
`backlog/repro/a6-kicad-templates.sh` (cases mzi4, evl3).
## Expected
MZI exports 5 nets; the EVLSRC list template is Nx2.
## Suggested fix
```
utils/specs.kicad_sym:1120  (property "Sim.Pins" "1=1 2=2 3=3 4=4" ...  ->  "1=1 2=2 3=3 4=4 5=5"
utils/specs.kicad_sym:799   "values=[[t0,p0,1550e-9],[t1,p1,1550e-9]]"   ->   "values=[[t0,v0],[t1,v1]]"
```
(only the MZI symbol block at :1104-1466 and the ELECTRIC_V_SOURCE_LIST block at :783-956; the V_SOURCE_LIST template at :1843 stays three-column.)
## Golden impact
none.
## Related
B-081.

- Discovery seeds: S3.18 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-07.
## Verification
Reproduced both failures with the prebuilt binary; s-expression parenthesis balance checked with a script (depth 0 at EOF). KiCad itself is not installed, so the library was not loaded in KiCad.
