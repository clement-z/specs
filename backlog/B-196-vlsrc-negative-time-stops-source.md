# B-196: A VLSRC value with a negative time stops the source silently
severity: low | category: bug | effort: XS | fix-class: needs-design | verdict: CONFIRMED | status: open
location: src/devices/value_list_source.cpp (comparison of the value times), src/parser/parse_element.cpp (VLSRC)

## Problem
Found in phase PJ (`backlog/sprint-2026-10c/PJ-REPORT.md`, "Found, not done"). `sc_time(double)` truncates
`v*scale + 0.5` to `int64` and stores it as `uint64`, so a value time of -1e-10 s becomes a huge time: the source
waits for it forever and emits nothing after it, with no message. The initial value at 0 is still emitted. EVLSRC
compares times in seconds and skips such a value with a warning.

## Expected
A negative time is reported: an error at parse time, or skipped with the EVLSRC warning.

## Suggested fix (needs a decision: error or warning)
Reject negative times when the values are read (error, exit 1), or use EVLSource's test (skip with a warning).
Add an `err/` or `regress/` golden case.

## Golden impact
One new case; no existing case uses a negative time.
