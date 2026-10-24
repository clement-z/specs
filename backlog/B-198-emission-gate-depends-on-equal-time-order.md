# B-198: Results depend on the order of equal-time emissions (the gate sees half-updated values)
severity: low | category: physics | effort: L | fix-class: needs-human | verdict: CONFIRMED | status: later (maintainer 2026-10-24: good on paper, worried about complexity and performance)
location: src/optical_output_port.{h,cpp} (emission gate, `emit_due_entry()`), the `.op`/`.dc` fixed point in src/specs.cpp

## Problem
The emission gate of an output port compares each new value with the last value emitted at its wavelength,
including values emitted earlier in the same time step. A device with two inputs that receive one wavelength in
different delta cycles emits a half-updated value in between; it can pass the gate and then suppress the final
value (within tolerance). Which half-updated value appears depends on the order in which the upstream ports emit
their equal-time entries (|E| order before phase N, wavelength order after). PD2 measured it on a 4-channel CROW at
default tolerances: per-wavelength values differ by up to 8.4e-4 relative (about 8x reltol) between two orders;
identical at abstol = reltol = 0 (`backlog/sprint-2026-10c/PD2-REPORT.md`, item 3).
Phase N (2674103) switched from the |E| order to the wavelength order: on the 4-channel CROW of the pinning case
`regress/pn-wdm-crow-equal-time-order` (MLPROBEs on through, inside ring 2 and at the drop), one channel
(1551.2 nm) moved by up to 9.0e-5 of its peak, the other three were identical; `regress/f15-wdm-crow-mlprobe`
4.1e-5 at the drop. The order is now fixed by the keys (time, wavelength), so the results no longer depend on
the heap layout or the standard library, but they still depend on the order itself.

## Expected
Per-wavelength results independent of the equal-time order (and of the standard library's heap algorithm).

## Suggested fix (option 3 of 2026-10-24, needs design)
Per port and wavelength keep R, the value settled at the end of the previous time step; within a step values
propagate as needed; the settled value of the step is F (the final value) if |F - R| > max(abstol, reltol |R|),
else R (written back if a half-updated value went out). The settled values then depend only on settled inputs.
Open points: how a port knows the step has settled (e.g. `sc_pending_activity_at_current_time()` or a settle
process per step), the extra events and the per-event cost (the maintainer's main concern), the `.op`/`.dc`
fixed-point convergence (which relies on the gate). A design discussion first, then a phase with a re-record and a
pinning case.

## Golden impact
Values at tolerance level in multi-wavelength circuits with zero-delay paths; to be predicted.

## Related
F-15 Q8 (equal-time order), phase N (wavelength order, bucket queue), B-188.
