# B-098: `analyze_detector_trace.py` is orphaned and partly broken
severity: low | category: style/maintainability | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: utils/analyze_detector_trace.py:1-444 (esp. :406-413, :294-315, :416-424)
## Problem
The script analyses a CSV (`P (W), tau (s), phi (rad), t (s), lambda (m), id`, default `detector_trace.txt`) that nothing in `src/` writes (grep for the column names finds only this file; the only other mention is Makefile:234 `rm -f ... detector_trace.txt`). Dead/broken parts: `compare_Pout_vecs` opens a pickle in text mode and uses undefined names (`Pout_nosort`, `t_nosort`, `Pout`); `plot_waveform` saves `Pout.png` before `grid()`/`legend()`; `__main__` prints unrelated `waveguide_length_for_phase_shift` demo values at start; unused imports `PriorityQueue`, `c`, `h` (shadowed in a comprehension), unused locals `i`, `tmax`, `dt`. It writes `NonIntersectingPulses.obj`, `Pout.obj`, `Pout.png` into the cwd as side effects.
## Evidence
```
utils/analyze_detector_trace.py:411:34: undefined name 'Pout_nosort'   (:412 't_nosort', 'Pout')
utils/analyze_detector_trace.py:6:1: 'queue.PriorityQueue' imported but unused
grep -rIn "tau (s)\|P (W)" src circuits  -> nothing
```
## Expected
Either a documented tool with a producer, or removed.
## Suggested fix
Decide: delete (history keeps it; also drop `detector_trace.txt`/`Pout.*` from `.gitignore` and the Makefile clean rule), or keep and add the missing writer. If kept: remove `compare_Pout_vecs`, the demo prints, the unused imports/locals, and move the `savefig` after `grid/legend`. B-099 is a separate real bug in the same file.
## Golden impact
none.
## Related
B-099, B-089.

- Discovery seeds: S3.33 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-23.
## Verification
pyflakes output; grep across the tree.
