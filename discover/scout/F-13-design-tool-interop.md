# F-13: Design-tool interop (KiCad generator and symbols, gdsfactory netlist import, SAX-compatible export)

value 4 | effort M | risk low | prerequisites: F-03 for S-parameter round trips, F-26 helps | horizon 2

## Problem / user story
*As a layout engineer I draw in KiCad or gdsfactory and want to simulate in SPECS without hand-writing nets; as a
SAX user I want SPECS as a time-domain backend.* The shipped KiCad library is in the KiCad 7 format
(`utils/specs.kicad_sym`, `version 20220914`) with `Sim.Device=SPICE` templates (`:15-23`), three symbols whose
reference prefixes have no lexer rule (ADROP, APASS, PCM), wrong templates, and KiCad's default net names
(`Net-(WG1-Pad2)`) do not lex (discovery report 03 §5). There is no gdsfactory path and no way to export a
SPECS-characterised block for SAX/Simphony users; gplugins no longer lists any time-domain simulator.

## Prior art (SOURCES A)
KiCad 9 "custom netlist generators": any command run on the intermediate XML (`%I` input, `%O` output, `%B`
base name, `%P` project path); `Sim.Device`, `Sim.Type`, `Sim.Params`, `Sim.Pins`, `Sim.Library` fields;
gdsfactory pydantic `Netlist {instances, placements, connections, ports, nets}` with `"instance,port"`
references and recursive netlists; SAX `SDict = {(in, out): array}` with `wl` in um and `port@mode` keys; gplugins
`.npz` with `o1@0,o2@0` keys; INTERCONNECT `exportsweep`; Simphony `save_sparams`.

## Proposed design
1. **KiCad.** `utils/kicad2specs.py` registered as a custom generator: reads the intermediate XML, maps symbol
   reference prefixes and `Sim.*` fields to SPECS elements (table shared with the device registry, F-26), orders
   pins by `Sim.Pins`, sanitises net names (`Net-(WG1-Pad2)` -> `n_WG1_2`, `/in` -> `in`), emits `.include`s for
   `Sim.Library`, writes a `.cir` next to the project. Refresh `specs.kicad_sym` to the KiCad 8/9 format with
   one symbol per registry entry (add `X`, `PWR`, `EVLSRC`, `SPAR`, `TIA`, `ADC`) and an example project
   `examples/kicad/add_drop/` that round-trips to `circuits/add_drop.cir`.
2. **gdsfactory import.** `pyspecs.from_gdsfactory(netlist_yaml_or_dict, model_map)`: instances become
   elements through a mapping table (`straight(length um) -> WG L=`, `coupler(gap, length) -> COUPLER kp=`,
   `mzi -> .subckt`), `connections` become nets, `ports` become top-level nets, recursive netlists become
   `.subckt`s, and unknown components become `SPAR` elements pointing at gplugins `.npz`/`.sparam` S-parameters
   (F-03). Units converted um -> m.
3. **Export for SAX/Simphony.** `pyspecs.export_sdict(result)` turns a `.dc` sweep of an N-port netlist into
   `{('in','out'): complex array}` plus `wl` (um); `to_npz()` with `o1@0,o2@0` keys; Touchstone through
   scikit-rf when installed; the SPECS CLI gains `--export-sparam file.sparam` for the same (F-03 writer).
4. **Netlist leniency flag.** `--lenient-names` lets `( ) -` appear inside net names for netlists produced by
   other tools, as an alternative to sanitising.

## Touch points
`utils/specs.kicad_sym` (regenerate) · new `utils/kicad2specs.py`, `examples/kicad/` · `pyspecs/` (importer,
exporter) · `src/parser/parser.l` (net name lexing, lenient mode) · F-03 writer · docs: "From KiCad",
"From gdsfactory", "To SAX" pages.

## Validation plan
- KiCad project -> generator -> SPECS run equals `circuits/add_drop.cir` outputs (golden-level comparison).
- gdsfactory `mzi()` YAML -> SPECS `.dc` transmission vs `sax` on the same neff/ng within 1e-3; the exported
  S-dict loaded into `sax.circuit` reproduces the SPECS sweep.
- Every symbol in the library instantiates a parsable element (script that emits one netlist per symbol).

## Effort breakdown
KiCad generator 3 d · library refresh 2 d · gdsfactory importer 4 d · export 2 d · examples/tests 3 d.

## Open questions for the maintainer
- Who owns the KiCad library long term (submit to KiCad's official libraries once stable)?
- Parameter-name policy with gdsfactory (keep gdsfactory names in um, translate at import)?
- Should the exporter emit Lumerical `.sparam` (de facto photonics) or Touchstone by default? Proposed: both,
  `.sparam` first.
