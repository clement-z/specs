# B-082: KiCad MZI template maps 4 of its 5 pins and the EVLSRC list template has three columns instead of two
severity: med | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in f9ea7f7
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

**Independent second check (V5, of F-A6-07): CONFIRMED.** Method: reproduced both failures with /tmp/ws-review-build/specs; applied both edits on a scratch copy; paren-balance script

`mzi1 a b c d length=...` gives "Parsing error ... offending token: length", and with 5 nets it runs to DONE. `evlsrc1 a values=[[0,1,1550e-9],[1e-9,0,1550e-9]]` gives "Nx2 list" error, and `[[0,1],[1e-9,0]]` runs to DONE. Edits: line 1120 (inside symbol MZI, lines 1104-1466; pins 1-4 are the left/right optical pins and pin 5 is the top "Electric" pin) `"1=1 2=2 3=3 4=4"` -> `"1=1 2=2 3=3 4=4 5=5"`; line 799 (inside ELECTRIC_V_SOURCE_LIST, 783-956) -> `values=[[t0,v0],[t1,v1]]`. S-expression depth is 0 at EOF afterwards. The identical template text at line 1843 (V_SOURCE_LIST) must stay, so use line-targeted edits. TRAP for the fixer: `utils/specs.kicad_sym` has CRLF line endings (`git ls-files --eol`: i/crlf; 1986 CRs). Use `sed -i '1120s/.../.../; 799s/.../.../'`, which preserves CR. Do not rewrite the file via Python text mode, `read().replace()` with universal newlines, or an editor that normalises EOLs. Check afterwards that `grep -c $'\r'` still returns 1986 and `git diff --stat` shows 2 lines changed.
