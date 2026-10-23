# B-189: A CW source with a negative power aborts instead of reporting an error
severity: low | category: bug | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: open
location: src/devices/cw_source.h (`setPower`), CWSRC creation in src/parser/parse_element.cpp

## Problem
Found in phase PF02 (`backlog/sprint-2026-10c/PF02-REPORT.md`, "Found, not done"). `cwsrc1 a wl=1.55u power=-1`
reaches `polar(sqrt(power), phase)` (`src/devices/cw_source.h:50`): `sqrt(-1)` is NaN, and with
`_GLIBCXX_ASSERTIONS` (the pixi builds) `std::polar` aborts (`Assertion '__rho >= 0' failed`, SIGABRT, exit 134).
Builds without the assertion would carry a NaN field instead. In a `.step`/`.mc` plan, such a run is recorded
`crashed` rather than `failed` with a message.

## Expected
A clean error at netlist creation (`Error: ... power must be >= 0`, exit 1), like other parameter checks.

## Suggested fix
Validate `power >= 0` (finite) when the CWSRC element is created; check the other sources' power/amplitude
parameters (VLSRC values) the same way. Add an `err/` golden case.

## Golden impact
One new error case; no existing case changes.

## Related
F-02 (sweeps and Monte-Carlo: drawn powers can be negative, e.g. `gauss()` around a small mean).
