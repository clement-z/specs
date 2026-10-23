# F-32: Schematic inferred from a netlist (rules + LLM in the loop), KiCad output

value 4 | effort L | risk med (layout quality, external model dependency) | prerequisites: F-13 (KiCad symbols and the
symbol ↔ instance-name mapping), F-31 (activity map, which would draw on it) | horizon 2
Idea of the maintainer, 2026-10-23 (not scoped or evaluated yet).

## Idea
Generate a readable KiCad schematic from a SPECS netlist: automatic placement and routing rules (signal flow left to
right, rings and couplers in recognisable shapes, subcircuits as hierarchical sheets), with an **LLM in the loop**
to propose or correct placements where the rules are not enough. Constraints stated by the maintainer:
- a model of about **27B parameters** class is enough;
- reached through an **API key**;
- **model-family agnostic** (no dependency on one provider's API or model family).

## Why
- The activity map (F-31) then colours a real schematic instead of a Graphviz graph (its step 3, "back-annotation").
- Netlists written by hand or generated (CROW, PUF, OCTANE) get a schematic for papers, teaching and review.

## Open points (to scope later)
- Output: KiCad 8/9 schematic files with the symbols of `utils/specs.kicad_sym` (B-081/B-082 symbol fixes first).
- Which parts are rules and which are the model's; how the result is checked (the schematic's netlist must be
  identical to the input netlist).
- How the model is configured (endpoint, key, model name) without tying the tool to one provider; offline use.
