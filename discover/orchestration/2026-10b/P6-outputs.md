# P6 — F-05 outputs and tracing (branch `ws/outputs`)

Follow `discover/orchestration/2026-10b/COMMON.md` (heartbeat first). Phase id: `P6`. Opus 5.5 high; subagents
allowed. Brief: `discover/scout/F-05-outputs-and-tracing.md`.

Decided (F-05): when the netlist has PROBEs or `.save` targets, trace only those; trace everything only as a fallback
when it has neither (backward compatible); `--no-trace`; CSV and `.npz` export of probed signals (pick a
dependency-free writer; document column layout: time, per-λ power and complex field). Keep VCD as the default
format. Expected golden impact: VCDs of cases that contain PROBEs lose the auto-traced nets (re-record, and show the
probed signals are unchanged); cases without PROBEs unchanged. Add golden cases for `.save`, `--no-trace`, CSV and
`.npz` (store `.npz` as a hash plus a decoded text summary if binary comparison is awkward). Measure and report the
speed-up on the CROW benchmark from `discover/scout/bench/` (full tracing vs probes only).
