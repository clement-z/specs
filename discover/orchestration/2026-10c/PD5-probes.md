# Phase D5 — probes on multi-wavelength nets: PROBE wavelength latch + B-184 (branch `ws/probes`)

Follow `COMMON.md` and `PD-common-f15.md` (builds, strict golden, clang/asan before/after, counters, report).
Phase id: `PD5`. Scope: maintainer decisions of 2026-10-23 ("D3 plain probes" row in
`backlog/DECISIONS-2026-10-21.md`, and the go for this follow-up), backlog `B-184`. Last item before v0.3.0.

## Background
A net (`spx::oa_signal_type`, `src/specs.h`) holds one `OpticalSignal`: the last value written, of any
wavelength. The output port emits one entry per delta cycle, so a probe sensitive to its net sees every
wavelength written to it as a separate event. `MLPROBE` (`src/devices/probe.cpp`) already filters events by
wavelength. Since D3 a plain `PROBE` on a multi-wavelength net shows one of the wavelengths of each time step in
an unspecified order (`doc/netlist.md`, "PROBE is for single-wavelength nets").

## To do
1. **B-184 (MLPROBE at the start of a `.tran`).** Read the issue and its two fix options. The expected result:
   at t = 0 an MLPROBE reports the operating-point value of every listed wavelength present on its net. Prefer
   the option local to the probe (it keeps listening while disabled and stores the last value per wavelength
   without tracing, then traces them when enabled), unless you find it wrong; justify the choice. No new globals,
   no printing per event. Add the issue's repro as a golden case.
2. **PROBE wavelength latch.** A plain `PROBE` (also the automatic `PROBE{<net>}` probes and `.save` targets)
   traces **one** wavelength:
   - an optional keyword `wl=<value>` selects it (exact match, like MLPROBE); without it the probe latches;
   - the latch is **per analysis point**: in `.op` and at each `.dc` point, the smallest wavelength present at
     that point; in `.tran`, the smallest wavelength present when the probe is enabled (with item 1's
     per-wavelength memory: the operating-point values), or, if none, the smallest one arriving in the first
     time step that brings any (the VCD records one value per time step, so the choice inside that step is
     invisible). Then it ignores the other wavelengths;
   - a `.dc WL(...)` wavelength sweep must keep working exactly as today: each point has its own wavelength
     (`circuits/add_drop.cir`, `cir/add_drop*` cases bit-identical);
   - warnings: when a probe ignores another wavelength for the first time, say so once (which probe, which
     wavelength it traces, "use MLPROBE"); for automatic probes, one summary line for all of them rather than one
     per net. stderr, never per event.
   - Single-wavelength nets: behaviour and outputs unchanged (bit-identical).
3. **Docs:** `doc/netlist.md` (PROBE syntax and the "single-wavelength nets" paragraph: describe the latch and
   `wl=`), the cheatsheet/element table where PROBE arguments are listed, `CHANGELOG.md` `[unreleased]`, B-184
   status and `backlog.md` row.

## Predicted golden impact (state the list before changing outputs; re-record only those)
- Cases whose plain probes sit on multi-wavelength nets: their traced values now follow one wavelength.
- Cases with an MLPROBE on a multi-wavelength net in `.tran` that starts from an operating point: values at
  t = 0 for wavelengths that read 0 before.
- `cli/help` only if the CLI changes (it should not); gtest list.
Before touching code, find the candidate cases (netlists with several wavelengths and probes) and write the
predicted list into `PD5-progress.md`. Every other case must stay strictly bit-identical, and counters identical.
For each re-recorded case, prove the change is only the predicted one (e.g. for a PROBE: the new trace equals the
MLPROBE trace of the latched wavelength from the same run; for B-184: only t = 0 values of listed wavelengths).

## Checks
As in `PD-common-f15.md`, plus: the B-184 repro; a 4-wavelength CROW (`discover/scout/bench/crow_gen.py 3 --nwl 4`)
with a PROBE and an MLPROBE on the same nets (the PROBE trace equals the MLPROBE trace of the latched wavelength);
`circuits/add_drop.cir` unchanged; the CROW bench `C-notrace N=30` not slower (ratio vs the tip before D5,
interleaved, median of 5).
