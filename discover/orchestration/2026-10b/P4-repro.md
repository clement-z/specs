# P4 — F-01 reproducible runs (branch `ws/repro`)

Follow `discover/orchestration/2026-10b/COMMON.md` (heartbeat first). Phase id: `P4`. Opus 5.5 xhigh; subagents
allowed. P1–P3 are merged (read their reports). Design brief: `discover/scout/F-01-reproducible-runs.md`.

Decided (`backlog/DECISIONS-2026-10-21.md`, F-01):
- `--seed N` = one master seed; per-device RNG streams derived from (master seed, hierarchical instance name),
  robust to netlist edits; the same seed later feeds `gauss()`/Monte-Carlo draws (leave a hook, F-02 is not in scope).
  Default without `--seed`: a fixed documented seed (runs are reproducible by default) — state it in the report.
- The detector dark-field injection stays, as a documented option.
- Name-ordered iteration everywhere objects are collected or printed today in pointer order
  (`sc_get_all_*_by_type` returning `std::set<T*>`, `init()` order, auto-probe and VCD declaration order): fixes B-016.
- Run manifest: version (`spx::version::full`), seed, command line, netlist file hashes, written next to the
  trace (and printed with `--version`-style info on request); `--version` already exists.

Expected golden impact: a one-time re-record of ordering only (stdout listings, VCD declaration order) plus
detector-noise outputs that now depend on the seed. Prove it: for every re-recorded case, show that the multiset of
lines / the set of (signal, waveform) pairs is unchanged except for seeded noise. Then remove every per-case
`unordered_streams` exception P1 listed, and show `check --strict` passes twice in a row and under a clang
ASan/UBSan build (`CC=clang CXX=clang++ meson setup ... -Db_sanitize=address,undefined -Db_lundef=false`, with
`ASAN_OPTIONS=detect_leaks=0` since leak suppressions come in P7): ordering must no longer depend on the allocator.

Note from P2 (see `backlog/sprint-2026-10b/P2-REPORT.md`, "Found, not done"): `upgrade_signal` asserts
`has_writer ^ has_reader`, but a unidirectional net can already have a writer and readers when a bidirectional
device upgrades it; today's pointer-ordered creation avoids it, name-ordered iteration may not. If it fires, relax it
to `assert(has_writer || has_reader)` (the slot convention approved for B-144/B-145 already handles that case) in its
own commit `fix(B-145): …`, with `regress/b145-upgrade-reader-case2` / `b145-upgrade-roles` as the tests.
