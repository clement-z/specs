# B-100: Seven `@sa DelayedWriter` comments point to a class that no longer exists
severity: low | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/ring.h:22, src/devices/phaseshifter.h:47,114, src/devices/splitter.h:29,34, src/devices/waveguide.h:68,118
## Problem
`DelayedWriter` was replaced by `OpticalOutputPort` (the members documented as "Timed writer to the output port" are `OpticalOutputPort m_out_writer`). The cross reference is dead in Doxygen and in the editor.
## Evidence
```
src/devices/waveguide.h:62-70:  /** Timed writer to the output port. * @sa DelayedWriter */ OpticalOutputPort m_out_writer;
git grep "class DelayedWriter" -> none in the current tree
```
## Expected
`@sa OpticalOutputPort`.
## Suggested fix
`sed -i 's/@sa DelayedWriter/@sa OpticalOutputPort/'` on the five files (7 occurrences). Comment-only change.
## Golden impact
none (comments only).
## Related
B-101.

- Discovery seeds: S5.23 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-25.
## Verification
grep of `DelayedWriter` over src/ (only these 7 comment lines); doxygen resolves `OpticalOutputPort` (class in src/optical_output_port.h:47).
