# B-100: Seven `@sa DelayedWriter` comments point to a class that no longer exists
severity: low | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in a4cece1
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

Correction from the second check: the 7 occurrences are in **4** files (`ring.h:22`, `phaseshifter.h:47,114`, `splitter.h:29,34`, `waveguide.h:68,118`), not five. Doxygen prints no warning for them, so the warning count stays at 77; the effect is a working link in the HTML. Fixer check: `git diff --stat` shows 7 comment lines in 4 files.
## Golden impact
none (comments only).
## Related
B-101.

- Discovery seeds: S5.23 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-25.
## Verification
grep of `DelayedWriter` over src/ (only these 7 comment lines); doxygen resolves `OpticalOutputPort` (class in src/optical_output_port.h:47).

**Independent second check (V5, of F-A6-25): ADJUST.** Method: grep; applied sed on a scratch copy; doxygen out of tree before/after

The change is right and all 7 occurrences are on members of type `OpticalOutputPort` (ring.h:22, phaseshifter.h:47,114, splitter.h:29,34, waveguide.h:68,118). Text fix: the finding says "five files" but it is four files (ring.h, phaseshifter.h, splitter.h, waveguide.h), 7 occurrences. Doxygen emits no warning for the dead reference, so the warning count is unchanged at 77. The visible effect is in the HTML: before, `<dd>DelayedWriter </dd>` (plain text); after, a link to class_optical_output_port.html. Impact statement "dead in Doxygen" is accurate in that sense. Fixer check: `git diff --stat` shows 7 +/- lines in 4 files, all comment lines. No rebuild or golden run is needed, although `ninja specs` is cheap if wanted.

**Orchestrator:** File count corrected (4 files, 7 occurrences).
