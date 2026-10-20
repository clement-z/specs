# B-067: CWSource `first_run` is never cleared (dead condition)
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 39e7734 (ws-cleanup C-02)
location: src/devices/cw_source.cpp:18,36
## Problem
`bool first_run = true;` is never modified, so `!first_run || ...` is always `false || ...`. The flag misleads readers about the reset handshake.
## Evidence
cw_source.cpp:18 declares it and :36 is its only use.
## Expected
No dead state.
## Suggested fix
```
-    bool first_run = true;
-
...
-        if ( !first_run || !reset.read().to_bool())
+        if (!reset.read().to_bool())
```
## Golden impact
None.
## Related
B-066 (the print at :33).

- Discovery seeds: S1.30 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-21.
## Verification
Code reading. Behaviour is identical because `!first_run` is the constant `false`.

**Independent second check (V1, of F-A3-21): CONFIRMED.** Method: applied the fix on the scratch copy and ran the golden check: 44/44 pass, also with --atol 0

`first_run` is declared at cw_source.cpp:18 and used only at :36, never written. The rewrite is identical in behaviour.
