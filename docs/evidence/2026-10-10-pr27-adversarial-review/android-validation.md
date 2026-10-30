# Android teaching findings 4 and 5: reproduction and validation

Input checkout HEAD: `36ce124c819b144cc4c9326cdfa31c7d1033fa7e` (working tree includes this scoped Android Java change and concurrent parent native changes). No commit was created. Reproduction logs are retained in this directory.

## Findings reproduced before edits

- Finding 4: with the unchanged 180 ms `runtimePoll`, a fake native 1.2 second Keeper charge and its 8 tick (133 ms) DodgeNow interval were tested across 12 phase offsets (0 through 165 ms in 15 ms increments). The original source missed the cue at phases 90, 105, and 120 ms. The original prompt formatter is retained in `review-android-teaching-baseline-reproduced-20261010.log`.
- Finding 5: packed native progress bytes 0, 64, 128, 191, 255 were compared with displayed marks; unchanged Java produced 0, 4, 6, 6, 6 instead of expected 0, 2, 3, 4, 6. The original result is recorded in the same reproduction log.

## Change and current checks

- `CombatTeachingPreferences` now rounds `6 * progressByte / 255`; cues 4–6 request timely UI refresh only when teaching is enabled and visible.
- `MainActivity` keeps telemetry on its existing 180 ms schedule and independently refreshes only active charge/dodge/recovery prompts every 32 ms. It observes native teaching snapshots without changing simulation or `surfaceStarted` authority. Pause and surface teardown cancel the timer and clear prompt content immediately. This is a UI scheduling bound under a running main looper, not a guarantee during a stalled main thread.
- `CombatTeachingUiSchedulingTest` drives the production `MainActivity.runtimePoll` and actual recurring prompt runnable against a fake-clock JNI shadow. It checks all 12 phase offsets, cue appearance within 32 ms, DodgeNow removal within 32 ms of the valid interval end, recovery fade clearing, and prompt-poll cancellation/content clear on pause and detach.
- Before the fix, the new regression tests failed against unchanged production. After the fix, 4 selected classes passed: `CombatTeachingPreferencesTest` (8), `CombatTeachingUiSchedulingTest` (2), `AndroidControllerInputTest` (11), and `SurfaceSuspensionLifecycleTest` (7); 28 tests total, 0 failures/errors/skips.
- Offline Gradle validation passed: `:app:assembleDebug`, `:app:assembleRelease`, `:app:lintDebug`, `:app:lintRelease`, and the four selected Java test classes, with `HORDE_VALIDATION_UNSIGNED=1`. Full log: `review-android-validation-20261010.log`; exit 0, 186.9 s, Gradle reported BUILD SUCCESSFUL (3m 6s). All four native ABI configure/build tasks appear for both Debug and RelWithDebInfo Release: `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`. No packages were installed and no physical device was used.

APK outputs (root owns later package inspection):

- `app/build/outputs/apk/debug/app-debug.apk` — 133,693,579 bytes; SHA-256 `AAD258E955DC26163991F501352D5CAA4E9BCCA1DF3735AD2CC22953933F031B`.
- `app/build/outputs/apk/release/app-release-unsigned.apk` — 121,491,698 bytes; SHA-256 `101DA7323F47892BD6137879A5A77A92D7332D01B3B01D4C6354B2A68B42EDAF`.

The targeted Gradle build emitted the existing Java deprecation note for `MainActivity.java`; it did not prevent compilation or lint completion. No other lint failure was reported.

Validation command: $gradle --offline :app:assembleDebug :app:assembleRelease :app:lintDebug :app:lintRelease :app:testDebugUnitTest --tests com.samfa12.hordelanternrt.CombatTeachingPreferencesTest --tests com.samfa12.hordelanternrt.CombatTeachingUiSchedulingTest --tests com.samfa12.hordelanternrt.AndroidControllerInputTest --tests com.samfa12.hordelanternrt.SurfaceSuspensionLifecycleTest --console plain
