# B-001: FD write merges only with the queue top: stale per-λ value wins in multi-λ OP/DC
severity: high | category: bug | effort: XS | fix-class: needs-human | verdict: CONFIRMED
location: src/optical_output_port.cpp:371-405, src/utils/pqueue.h:12, src/optical_signal.h:120-123
## Problem
In NO_DELAY mode (every `.op`/`.dc`, and the OP phase of `.tran`), `immediateWriteFrequencyDomain` checks only
the queue top (`m_queue.cbegin()`, line 380/388) for an entry with the same (now, λ). When another wavelength is
on top, a second entry for the same λ is pushed. The OOP pops one entry per delta, and equal-time entries are
ordered by the inverted `OpticalSignal::operator<` (larger |E| first) through `pair<>` + `greater<>`. As a
result, an older and smaller partial value of a λ can pop last. It is then emitted as the converged value and
propagated downstream. No error is reported. Single-λ circuits are not affected.
## Evidence
`backlog/repro/a1-fd-multilambda-stale.cir` (two λ, three mergers, hand-computable):
```
expected  m(1550 nm) = (0.353553 + 0.707107)/sqrt2 = 0.750000 (0.5625 W)
observed  ROOT/m (OPTICAL) = [0269] @ 1550 nm (0.500000 V.m⁻¹, 0.250000 W)   <- v/sqrt2, stale partial sum
          ROOT/o (OPTICAL) = ...    @ 1550 nm (0.500000 V.m⁻¹, 0.250000 W)   <- propagated
control (λ2 sources replaced by 1550 nm / 0 W): ROOT/m = 0.750000 (0.5625 W)
```
The result is identical over 5 runs and under several glibc malloc tunables.
## Expected
`m` = 0.75 √W at 1550 nm (0.5625 W), whatever other wavelengths share the port.
## Suggested fix
Search the whole queue for an entry with the same timestamp and λ, as `delayedWriteEventDriven` already does
(lines 341-343). Replace lines 380-401 with:
```cpp
    auto it = std::find_if(m_queue.begin(), m_queue.end(), [&value,&now](const auto &x) {
            return now == x.first && value.m_wavelength_id == x.second.m_wavelength_id; });
    if (it == m_queue.end()) {
        m_queue.push(make_pair(now, value));
        m_event_queue.notify(SC_ZERO_TIME);
    } else if (m_use_deltas)
        it->second += value;
    else
        it->second = value;
```
I validated this patch: the repro gives 0.750000 and all 44 golden cases pass unchanged. Optional follow-up
(design): order equal-time entries FIFO (sequence number) instead of by |E|, and drop the inverted
`operator<`.
## Golden impact
None (validated: `run_golden.py check` passes 44/44 with the patched binary). Add the repro as a new golden case.
## Related
B-011 (OP printout shows one λ per net), B-015 (perf of `find_if`), B-022 (`operator<`). A3 S1.10 (probes on WDM nets).

- Discovery seeds: S1.05, S5.18 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-01.
## Verification
Ran the repro with `/tmp/ws-review-build/specs`: wrong value. Ran the control: right value. Relinked with the patched
`optical_output_port.cpp` only: right value, golden 44/44 PASS. This is tagged needs-human only because it
changes results of valid simulations; the fix itself is mechanical.

**Independent second check (V3, of F-A1-01): CONFIRMED.** Method: reproduced with the reviewer's repro and with 2 new netlists of my own (2 lambda and 3 lambda, .op and .dc); dumped per-lambda m_emitted_fields with an instrumented build; applied the fix, rebuilt, golden check --atol 0

Own netlist 1 (fields a1=0.3@1300, a2=1@1310, b1=0.8@1300, b2=0.2@1310; u=M(a1,a2), v=M(b1,b2), m=M(u,v)). Hand values: m = 0.55@1300 and 0.60@1310. Base gives 0.40 and 0.50 (both lambdas stale: v/sqrt2 and u/sqrt2). Own netlist 2 (3 lambda, 5 mergers) gives out@1310 = 0.1768 instead of 0.3536; the other lambdas are correct. In the reviewer's repro, m@1550 = 0.5 instead of 0.75 and m@1560 = 1.05 is correct. The same topology as a .dc power sweep is wrong at the first sweep point (0.40 vs 0.55). Later points are right because merger memories persist. Root cause confirmed by a delta-by-delta trace. Two inputs pop the lambdas in opposite order (the order is by |E|). While another lambda is on top, the merger writes lambda1 in the same delta before the OOP pops, so a second (t, lambda1) entry is pushed. The equal-time entries then pop larger-|E|-first, so the stale smaller partial pops last. The suggested patch (find_if over the whole queue) gives correct values for every lambda in all 3 netlists. Golden check is 44/44 PASS, 0 notes, at atol 0. Severity high is justified: silent wrong values for valid multi-lambda CW circuits (OP, the first DC point, and the OP phase of .tran). needs-human is right. Note for the design follow-up: the fix (like delayedWriteEventDriven) mutates a non-top heap element whose payload takes part in the comparator. With unique (t, lambda) the final values do not depend on pop order, but a FIFO sequence key would also remove the reliance on a technically invalid heap.
