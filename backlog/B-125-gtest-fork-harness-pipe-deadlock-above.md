# B-125: gtest fork harness: pipe deadlock above 64 KiB, ASSERT_* / non-std exceptions escape the footer, dead stderr pipe, ignored return values
severity: med | category: test-gap | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 538d8f8
location: src/test/utils.h:28-76 (lines 30-32, 40-41, 44-49, 56-60, 63, 68-76)
## Problem
1. The parent calls `waitpid` (line 40) before draining the pipe and before closing its own write end (line 41). A child that writes more than the pipe capacity (64 KiB) blocks forever in `write`: the test hangs.
2. `ASSERT_*`/`FAIL()` expand to `return;`, which leaves `TestBody` in the child without reaching the `exit()` of `SPECS_TEST_FOOTER`. The child then continues inside gtest's runner, runs the rest of the suite (each remaining test forks again, with a grandchild), and prints a second gtest summary into the pipe. The parent only reports "Test case exited abnormally". A `throw 42` (anything not derived from `std::exception`) behaves the same because the footer only catches `std::exception`.
3. `pipe_stderr` is created, closed and never used; `pipe()` and `write()` results are ignored (`EXPECT_NE(dup2(...), -1)` in the child only records a failure and continues); the "non-zero status" message (lines 59-60) is unreachable because line 57 already fails for any non-zero status with the misleading text "exited abnormally"; the child reports nothing about the signal that killed it.
4. Output is copied one byte at a time with one flush per byte.
## Evidence
`backlog/repro/a5-test-harness-probe.cpp` (4 probe tests) run against the original harness:
```
Probe.BigOutput  (4000 lines x 71 chars)      -> hangs; killed by `timeout 20`, no output
Probe.AssertEscapes: ASSERT_TRUE(false) in child 532562 -> output shows "> > child 532563 parent 532562 runs ZAfter"
                     i.e. the next test ran again inside the first child, nested in the pipe prefix, and a full gtest summary was printed twice
Probe.ThrowInt: "Unknown C++ exception thrown in the test body" printed by the child's gtest, same escape as above
```
With `backlog/repro/a5-test-utils-harness.patch` applied: BigOutput completes, AssertEscapes and ThrowInt fail cleanly once each ("Test case exited with non-zero status: 1"), ZAfter runs exactly once, the real tests keep passing.
## Expected
Each test body runs exactly once in exactly one child; any assertion/exception makes the child exit non-zero; arbitrary output volume is relayed; the failure message states the cause.
## Suggested fix
Apply `backlog/repro/a5-test-utils-harness.patch` (applies after the angle patch of B-123):
- parent: `close(pipe_stdout[1])` first, drain until EOF, then `waitpid`; report `WIFSIGNALED`/`WTERMSIG` and the exit status separately;
- child: wrap the body in a lambda (`auto specs_test_body_ = [&]() -> void` in the header macro, `;` + `try { specs_test_body_(); } catch (std::exception&) {...} catch (...) {...}` in the footer) so `return` from `ASSERT_*` cannot skip `exit(HasFailure())`;
- check `pipe()`/`dup2()` results, drop `pipe_stderr`.
Optional: relay with a 4 KiB `read` buffer instead of a byte loop (BigOutput takes 1.2-3.9 s byte-wise).
## Golden impact
none (gtest PASS/FAIL set unchanged; the probe tests are not part of the patch).
## Related
B-123, B-142.

- Discovery seeds: S4.09 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-05.
## Verification
Built in the scratch copy; ran the 4 probes before/after (above) and the full suite after: A.B, Waveguide.* PASS. The patch is test-infrastructure only. `git apply --check` is clean on the repo after the B-123 patch.

**Independent second check (V6, of F-A5-05): CONFIRMED.** Method: built a5-test-harness-probe.cpp (4 probe tests) into the scratch unit_tests against the angle-only header (original harness) and against angle+harness patch; ran each probe with timeout, then the real tests

Original harness: Probe.BigOutput (4000 x 71 chars) hangs (killed by timeout 20, exit 124). Probe.AssertEscapes: the child falls out of TestBody, runs the rest of the suite and prints a full gtest summary into the pipe; with AssertEscapes+ZAfter the log shows `> > child ... runs ZAfter` (nested run) and the parent only says "Test case exited abnormally". Probe.ThrowInt: child gtest prints "Unknown C++ exception thrown in the test body." and the parent says "exited abnormally". With the harness patch: BigOutput passes (1.3-2.1 s, byte-wise copy), AssertEscapes and ThrowInt each fail exactly once with "Test case exited with non-zero status: 1", ZAfter runs once, the real tests (A.B, Waveguide.*, and the 6 new model tests) keep PASSing, so the PASS/FAIL set of real tests is unchanged. Patch applies cleanly after the angle patch (git apply --check), no new warnings with -Wall -Wextra -pedantic. Lambda wrapper has no side effect on existing bodies (they are plain `{ ... }` blocks, no `return`). Golden impact none confirmed (the gtest check stays at PASS for all pre-existing tests).
