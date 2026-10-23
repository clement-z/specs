# B-022: OpticalSignal value semantics: id churn, `operator=` by value, vptr, inverted `operator<`
severity: low | category: perf | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: partly addressed in a935b7e (sprint 2026-10c D2): no virtual destructor, ids only with -Dsignal_ids, defaulted copy; the inverted operator< stays open
location: src/optical_signal.h:23-28,52-63,58,120-123,147-153,231
## Problem
The copy constructor copies `m_id` and then overwrites it with `getNewId()`. `operator=` returns **by value**, an
extra copy per assignment, each drawing a new id. The user-declared copy operations and virtual destructor suppress
moves and add a vptr (32 B instead of 24; no subclass exists). `operator<` is inverted (|a|>|b|) and is only used
implicitly by `pair<>` ordering in PQueue (B-001). The debug `m_id` is printed in every signal dump (`[0269]`), so
golden stdout changes whenever the number of copies changes. The 32-bit `nextId` wrap itself is harmless
(well-defined unsigned arithmetic, display only).
## Evidence
Code reading. The golden harness compares the `[NNNN]` integers exactly (run_golden.py:202-205).
## Expected
A cheap trivially-copyable value type. Ids, if kept, not in default output.
## Suggested fix
Return `OpticalSignal&` from `operator=`. Drop the virtual destructor. Either drop `m_id` or stop printing it.
Replace the implicit `operator<` with an explicit time-only and FIFO comparator in PQueue.
## Golden impact
Stdout `[NNNN]` ids in most cir/*, opt/* and tb/* cases (re-record).
## Related
B-001, B-015.

- Discovery seeds: S1.22, S5.18 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-22.
## Verification
Code reading. Checked the golden comparator.
