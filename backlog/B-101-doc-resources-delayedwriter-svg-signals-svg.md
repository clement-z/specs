# B-101: doc/resources/delayedwriter.svg and signals.svg document classes that were removed
severity: low | category: docs | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: doc/resources/delayedwriter.svg, doc/resources/signals.svg
## Problem
Both are PlantUML class diagrams of `DelayedWriter<T>` and of `OpticalSignal` with `Type {NONE, WRITE, ERASE, READOUT}`, `id/type/parameter union/wavelength`, `attenuate()`. The current `OpticalSignal` has `m_field`, `m_wavelength_id`, `m_id`; `DelayedWriter` is gone. Nothing references the files; the Makefile `doc` target only copies `doc/resources/` to `doc/html/`.
## Evidence
Text extracted from the SVGs vs `src/optical_signal.h:15-60`; `grep -rn "delayedwriter\|signals.svg" src README.md doc Makefile` finds nothing.
## Expected
Diagrams reflect the code or are absent.
## Suggested fix
Either delete both (`git rm`) or regenerate for `OpticalOutputPort`/`OpticalSignal`. Regeneration needs the PlantUML source (not in repo).
## Golden impact
none.
## Related
B-100.

- Discovery seeds: S5.23 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-26.
## Verification
Extracted `<text>` nodes from the SVGs; compared with the headers.
