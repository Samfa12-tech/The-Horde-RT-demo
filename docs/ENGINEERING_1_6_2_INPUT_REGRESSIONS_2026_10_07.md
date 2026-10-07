# 1.6.2 input/settings regression slice — 7 October 2026

Base: PR18 branch `codex/horde-1.6.2-engine-readiness`, documentation checkpoint `5fe070e42541fe8a4b4bc856360b73e1d94c7489`, prior validated runtime `1334cc9c58ec97940ac10d861f0397143ca7a9f4`. This slice does not replace the sealed phone evidence in [the pause checkpoint](ENGINEERING_1_6_2_PAUSE_2026_10_05.md).

## Changes and reproductions

- Fresh Graphics Back/Restore without `markPending` now records the existing unstored-mobile-default marker before writing `graphics_pending=false`. Fresh 50%/Glass Off stays authoritative on subsequent preference reads; genuine pending-only legacy records still migrate to the historical 75%/Glass On baseline. The new no-preview preference regression failed on the old implementation before the fix.
- Whole Android `ACTION_CANCEL` clears both movement/look pointer assignments and movement axes. Pause and menu cleanup also cancel parry touch ownership, preventing a later release from becoming a new click. Real production `MotionEvent` listener tests exercise both pointer orders, menu cleanup, pause and subsequent gestures. A temporary old action-index-only production implementation failed this regression; the patched file was restored byte-for-byte afterward.
- Windows controller polling suppresses actions and axes while the app is unfocused. The first focused sample reseeds XInput/WinMM gameplay/menu/button/trigger latches; held buttons do not become new presses. Deactivation invalidates the baseline even when polling is suspended. Host contract tests cover focus return and release/repress. Physical non-minimized Alt-Tab/controller reproduction remains required.
- Graphics rebuilds retain independent vertical-page and horizontal-preview-row positions, stable control focus and accessibility focus. Focus on disabled preview choices waits for the matching native acknowledgement. Image-only view and stale layout callbacks preserve the prior control-strip state. Transaction acknowledgement and the normal 15-second confirmation policy are unchanged.

## Current-run checks

Host-only, unsigned validation:

- Full Android unit suite: **163 tests, 27 classes, zero failures/errors/skips**. Lint: **zero errors, 50 warnings**. The added accessibility-focus warning is for restoring the previously focused control after rebuilding its native view, not choosing an unrelated control. Existing checkpoint lint had 49 warnings.
- The final viewport-sentinel guards were added after the full suite; the affected two Graphics viewport tests were rerun and passed.
- Windows Debug application compiled; `DesktopControllerInputTests` passed (1/1 CTest).
- `git diff --check` passed.

Evidence logs remain outside the public source checkout in the existing `task-4` evidence directory: `post-reset-input-java-full-lint-01.log`, `post-reset-graphics-viewport-final-01.log`, `post-reset-touch-production-negative-01.log`, `post-reset-windows-input-build-02.log`, and `post-reset-windows-input-ctest-01.log`. Earlier failed fixture runs remain there too; the preview fixture required native text measurement and an explicit layout event before its scroll precondition was valid.

These checks are neither physical Android lifecycle/touch evidence nor controller feel acceptance. No new APK/package, sustained-performance result or owner visual/audio/haptic acceptance is claimed. The prior exact-build phone results remain valid for their own runtime and do not cover these changes.
