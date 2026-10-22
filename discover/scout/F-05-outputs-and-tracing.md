# F-05: Outputs and tracing control (`.save`, table export, no-trace mode, pyspecs reader)

value 5 | effort S-M | risk low | prerequisites: none | horizon 1 (parts 1-2), horizon 2 (FST)

## Problem / user story
*As a user I want to record the two signals I care about, in a format pandas opens, without a gigabyte VCD.*
Measured on the Release build (`bench/RESULTS.md` §2): with the default `trace_all_optical_nets = 1`
(`src/specs.cpp:38`) a 30-ring CROW run is 4.4x slower than the engine alone and writes 1 GB; even two explicit
probes write 15-32 MB because every probe traces four reals per event (`src/devices/probe.h:74-77`: power, abs,
phase, wavelength) and every wavelength of every event is a VCD record. Other defects: the VCD is never closed in
the CLI path, `-o ""` dereferences a null trace file (`src/specs.cpp:366-369`), sources print per event
(`src/devices/cw_source.cpp:33`, `value_list_source.cpp`), `.save` is lexed (`src/parser/parser.l:461`) but has
no grammar rule, so the paper's Fig. 1b netlist does not parse; `pyspecs` depends on `pyDigitalWaveTools`
(last release 2022, `pyspecs/specsparse.py:1`), misdetects sweeps (`:258-266`) and uses the removed
`fillna(method=)` (`:104, 146`).

## Prior art
ngspice binary rawfile / `wrdata`; Xyce `.PRINT FORMAT=CSV|RAW|...`; INTERCONNECT datasets and `exportsweep`;
Photontorch returns tensors; FST (GTKWave, 10-50x smaller than VCD) with `pylibfst`/`pywellen` readers; Surfer
and GTKWave for viewing; gplugins `.npz`; Parquet/Arrow for columnar tables (SOURCES C §5).

## Proposed design
1. **Selection.** `.save P(out) P(drop) E(out) PHI(out) I(pd1)` grammar (new `directive.save` rule next to
   `src/parser/parser.y:420-446`) creates probes with only the requested quantities and sets `traceall=0`;
   CLI `--save ...` and `--no-trace`; when a netlist has explicit probes and no `.save`, default `traceall`
   to 0 (results unchanged, only tracing).
2. **Table sink.** A small `TraceSink` interface with two backends: the existing VCD (`sc_trace`) and a
   **table** writer selected by `-o out.csv` / `--format csv|npz` (NumPy `.npz` is a zip of `.npy` arrays;
   a 60-line writer, no dependency). Probes write to the sink directly in `on_port_in_changed()`
   (`src/devices/probe.cpp`) instead of through `sc_signal<double>` members, so VCD can be disabled entirely.
   Long format `time_s, signal, re, im` (event list, exact) or wide with `--sample dt` decimation. Parquet is
   produced by pyspecs from `.npz`, not by the binary (Arrow is too heavy a dependency).
3. **Hygiene.** Close the VCD at the end of `runAnalysis` (`src/specs.cpp:41-60`); guard the null trace
   file; gate per-event prints behind `--vci`/verbosity (F-27).
4. **pyspecs.** `pyspecs.load(path)` dispatching on extension; VCD through `pywellen`; sweep detection from the
   manifest (F-01). Data model: long DataFrame plus `to_wide()`; detectors and probes in one table with a
   `kind` column.
5. **Later.** FST output through `libfst` (MIT) or Minres SCC's tracer for 10-50x smaller files; GTKWave
   `.gtkw` save-file generation (analog step rendering) from the probe list.

## Touch points
`src/specs.cpp:36-38, 260-314, 365-370` · `src/devices/probe.{h,cpp}` · `src/parser/parser.l:461`,
`src/parser/parser.y:420-446` · `src/main.cpp:194-285` (`-o`, `--format`, `--save`, `--no-trace`) ·
`pyspecs/specsparse.py` (rewrite on pywellen), `pyspecs/specs.py:18-22` (defaults) · `tests/golden/run_golden.py`
(compare tables as text; already numeric-aware).

## Validation plan
- `bench/bench_crow.py` case B (N=30, traceall) before/after: target within 10 % of case C (no tracing) when
  `.save` lists two signals; VCD size drops from 1 GB to the probe subset.
- CSV/npz values equal the VCD-derived piecewise-constant waveform at all event times (the golden harness's
  VCD parser gives the oracle).
- The paper's Fig. 1b netlist with `.save P(out) P(drop)` parses and runs.
- pyspecs: `load()` on VCD, CSV and npz returns identical DataFrames; notebook runs in CI.

## Effort breakdown
`.save` + flags 2 d · table sink + npz 3 d · hygiene 0.5 d · pyspecs loader 2 d · FST later 3-5 d.

## Open questions for the maintainer
- When no probe and no `.save` are present: keep tracing everything (today) or trace nothing and warn?
- Phase column wrapped or unwrapped? Keep `abs` and `wavelength` columns at all (power + complex field suffice)?
- Default extension/format for sweeps and MC (proposed: `.npz` per point, index CSV).
