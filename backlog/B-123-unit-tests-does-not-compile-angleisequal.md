# B-123: unit_tests does not compile; angleIsEqual is wrong even once it compiles
severity: high | category: build | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/test/utils.h:21-25, src/test/utils.h:79-107, src/test/devices/waveguide.cpp:166, src/test/CMakeLists.txt:7-11, install.sh:28-29
## Problem
`unit_tests` cannot be built. `utils.h:81` has an unconditional `static_assert(false, ...)`. Behind it, `EXPECT_ANGLE_EQ` (utils.h:21-22) passes a 2-argument, bool-returning `angleIsEqual` to `EXPECT_PRED_FORMAT2`, which needs `(const char*, const char*, T1, T2) -> AssertionResult`. The target is in `all` when the CMake option `BUILD_TB` is ON (the default), so `ninja`/`make` of everything fails (CLAUDE.md "known"), and `install.sh:28-29` (`cmake .. -DBUILD_TB=1; make -j`) fails too.
The predicates themselves are also wrong, so deleting the `static_assert` is not enough: `fmod(arg(v) - angle, 2*pi) < epsilon` (utils.h:100) is one-sided (every negative difference passes, any positive difference above 2.2e-16 fails) and is not a modulo-2pi comparison; the generic overload (utils.h:86) takes `arg()` of a real number. Result in the out-of-tree patched build recorded in the golden baseline: `Waveguide.OperatingPoint` FAILS at 1551 nm although actual and expected phase are both 2.3941731247791935 up to rounding, while a phase that is completely wrong but smaller than expected would PASS.
## Evidence
```
$ c++ ... -fsyntax-only src/test/main.cpp          (original utils.h)
test/utils.h:81:15: error: static assertion failed: Work in progress: implement angleIsEqual (it should validate angle modulo 2pi
$ (static_assert removed) c++ ... -fsyntax-only src/test/devices/waveguide.cpp
test/utils.h:22:1: error: no matching function for call to 'angleIsEqual(const char [16], const char [28], const OpticalSignal&, double)'
```
Patched copy (discover/meson/unit-tests-minimal.patch) run:
```
[ RUN ] Waveguide.OperatingPoint
> waveguide.cpp:166: Failure
> ...evaluates to false, where tb.b_out.read() evaluates to [0086] @ 1551 nm (0.707107 V.m^-1, 0.500000 W, 2.3942 rad)
> tb.expected_phase_delay(wl) evaluates to 2.3941731247791935
```
backlog/repro/a5-angle-predicate.cpp (standalone compile): `angleIsEqual(polar(1,0.1), 2.39)` prints 1 (should be 0); `angleIsEqual(polar(1,2.0000000000001), 2.0)` prints 0 (should be 1).
With a correct predicate (`abs(remainder(a-b, 2pi)) < 1e-9`) all six wavelengths of `Waveguide.OperatingPoint` pass, i.e. the device model is fine and only the test helper is broken.
## Expected
`unit_tests` compiles; `ninja`/`make` of the default configuration succeeds; phase comparisons are symmetric and modulo 2pi.
## Suggested fix
Apply `backlog/repro/a5-test-utils-angle.patch` (src/test/utils.h only):
- delete the `static_assert` and the four `angleIsEqual` templates (utils.h:81-107), add
  `inline bool angleIsEqual(double a, double b, double tol = 1e-9) { return std::abs(std::remainder(a - b, 2 * pi)) < tol; }`
  (add `#include <numbers>` and `<cmath>`);
- `#define EXPECT_ANGLE_EQ(sig, phi) EXPECT_TRUE(angleIsEqual((sig).phase(), (phi))) << "phase " << (sig).phase() << " rad != " << (phi) << " rad (mod 2pi)"` and the same for `ASSERT_ANGLE_EQ` (OpticalSignal has `phase()`, optical_signal.h:92). No change to waveguide.cpp is needed.
## Golden impact
Only the gtest part: `Waveguide.OperatingPoint` FAIL -> PASS. `compare_gtest` reports it as a non-fatal "improvement; re-record" note; re-record with `--unit-tests <real in-tree unit_tests>` in the same commit (the current gtest baseline comes from a patched out-of-tree copy). All 44 `specs` cases are unchanged.
## Related
B-125 (fork harness, same header), B-142 (header hygiene), B-121 (CMake: unit_tests in `all`), discover/04-build-test-ci.md section 5.

- Discovery seeds: S1.04, S4.01 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-01.
## Verification
Built a copy of the repo in /tmp/ws-review-A5/repo with the patch (ninja -j2, googletest from a local clone via FETCHCONTENT_SOURCE_DIR_GOOGLETEST). Ran `unit_tests`: `A.B`, `Waveguide.Instantiation`, `Waveguide.OperatingPoint` all PASS. Applied the patch to the repo's utils.h with `git apply --check`: clean. Safe because only test-helper code changes and nothing in `specs` or `libspx` includes this header.
