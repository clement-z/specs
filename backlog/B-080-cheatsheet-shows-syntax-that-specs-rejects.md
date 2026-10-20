# B-080: Cheatsheet shows syntax that SPECS rejects or contradicts, and omits most elements
severity: med | category: docs | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: doc/resources/cheatsheet.pdf (single page, Inkscape export of 2023-06-13; no source in the repo)
## Problem
The only syntax reference in the repo is wrong in the places below and incomplete. The external manual (https://demo.zrounba.fr/specs/doc/index.html, reachable today, 2.5 MB single page "(c) Raphael Cardoso & Clement Zrounba, 2023", repeats the phi/pshift/merger errors and already uses the correct `.dc /cwsrc1/WL` form and has "PDET: TODO"); its source is not in the repository, so it cannot be fixed or reviewed here. Each row was checked with the prebuilt binary (`backlog/repro/a6-cheatsheet-examples.sh`).
## Evidence
| Cheatsheet says | Implementation | Check |
|---|---|---|
| `.dc WL(cwsrc1) wl0 wl1 dwl` | `.dc /cwsrc1/WL wl0 wl1 dwl` (parser.y:400, :798-806); attributes WL/WAVELENGTH/LAMBDA, P/POW/POWER, F/FREQ/FREQUENCY (parse_analysis.cpp:48-62) | syntax error at `WL` |
| CWSRC `phi` "in degrees" | radians: `polar(modulus, phase)` (cw_source.h:43-47) | code reading |
| PROBE "1 is the output terminal" | the probed net is read: `connect_uni(obj->p_in, nets[0], AS_READER)` (parse_element.cpp:983-986) | code reading (the external manual says input) |
| `pshift1 1 2 3 att=val, gain=val` | kwargs are whitespace separated | syntax error at `,` |
| MERGER "loss is the insertion loss" (second bullet) | only `IL`/`INSERTION_LOSS` (parse_element.cpp:168) | `Unknown keyword: loss`, exit 1 |
| "Photodiode (PD)" `pd 1 2 r=val ts=val` (+ typo "devicei") | element prefix is `PDET` (parser.l:441) | `pd1` syntax error; `pdet1` OK |
| `.options ...` then indented `+ abstol=val ...` | continuation is EOL followed by `+` in column 0 (parser.l:533-534) | indented `+` syntax error |
| `.op` "ignoring all VLSRC" | `OPAnalysis::create` only switches to FD mode (parse_analysis.cpp:23-28); output goes to stdout, no VCD is written | not verifiable; external manual words it "before any value-source is activated" |
Not documented at all: MZI (5 nets), CROSSING, PCMCELL, PWR, `X`/`.subckt`/`.ends`, `.param`/`.assign`, `.include`, `{expr}` expressions, `.options resolution`/`ts` (parse_directive.cpp:34-37), COUPLER `kp`/`t`, PROBE kwargs `p/mag/phase/wl`, VLSRC `values="file"` (supported; `file=`/`scale=` kwargs exit with "not implemented", parse_element.cpp:722-731), `.tran` without duration, CLI flags, default tolerances (1e-8 / 1e-4) and default `timescale` (-13, specs.cpp:26).
## Expected
A syntax reference that the parser accepts verbatim.
## Suggested fix
Replace the PDF by a maintained text reference, e.g. `doc/netlist-reference.md` (Doxygen picks up `.md` in INPUT), written from `parser.l`/`parse_element.cpp` and checked by a golden case that parses each documented line; keep the PDF only if the SVG/Inkscape source is added. Decide the phi unit in the docs as radians (SI, matches the code; changing the code to degrees would be a units change = needs-human). Open the same corrections upstream for the external manual. This is a rewrite, not a patch; CLAUDE.md's "adding a netlist element" step "docs (cheatsheet)" should then point to the new file.
## Golden impact
none (a documentation-check case would be new).
## Related
B-084 (docs gap), B-081/07 (KiCad symbols share the same kwargs), B-083.

- Discovery seeds: S3.05, S5.24 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-05.
## Verification
Every table row run with `specs -n -f` (rc and message in the table), `.op` run separately (stdout only, no `traces/op.vcd`). Seed S3.05 confirmed; the Doxyfile/mainpage half of the seed is B-083.
