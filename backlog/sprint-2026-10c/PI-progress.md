# PI progress (pixi builds and the host's gtest)

Heartbeat: `PI-heartbeat`, id `fcbfa1b7` (delete after the report is committed).

| Step | Status | Commit | Notes |
|---|---|---|---|
| 1. Reproduce (podman archlinux + gtest) | done | | before (0e0993d): `gtest/gtest.h` not found; gtest found by the env's pkg-config in /usr/lib/pkgconfig |
| 2. Fix | done | 78ca588 | `utils/pixi-setup.sh` passes `-Dforce_fallback_for=systemc,args,gtest`; README + CHANGELOG |
| 3. Verify | done | | local gcc/clang/asan-clang 12/12; strict golden gcc 240/240; Arch container gcc+clang 12/12; plain route keeps the host gtest |
| 4. Docs + CHANGELOG | done | 78ca588 | |
| 5. Report | done | | PI-REPORT.md |
