# Player Vitality and Encounter Retry Slice

Date: 2026-07-31

Status: host validation and automated `SM-S948B` device validation passed. The project owner subsequently corrected the earlier hands-on report on 2026-08-01 because haptics had not actually been checked. A revised direct-vibration Debug build was then installed byte-for-byte; Android recorded its preview, Swing, damage, and fatal effects through the live encounter, and the owner confirmed the revised haptic was physically felt. Basic haptic operation is now device-validated, while intensity/comfort/cue distinction remain tuning questions.

## Delivered behavior

- The player starts each selected encounter with three vitality.
- A connected skeleton attack or charged lich hit removes one vitality.
- Accepted hits start one second of invulnerability and reuse the existing RT damage vignette.
- The third accepted hit enters `Dying` for 0.65 seconds. Player movement, camera input, attacks, lantern sequence, and enemy simulation freeze while the RT frame continues through the normal swapchain presentation loop.
- `Dead` opens `YOU FELL` with `RETRY ENCOUNTER`, `RESTART ROUTE`, and `QUIT DEMO`.
- Retry is release-safe and deliberately restricted to checkpoint `0` (`opening`) for the skeleton and checkpoint `9` (`mirror`) for the lich.
- Selecting a new active encounter refills vitality. Benchmark, deterministic replay, and capture measurement paths do not apply player damage.

## Platform integration

Android publishes vitality and life phase through atomics to Java, displays a compact accessibility-labelled vitality HUD, uses normal `View` haptics for accepted/fatal hits, blocks Back and touch actions while dead, and restores the deterministic retry camera pose through a dedicated release JNI request. Retry remains paused until native code acknowledges the restored `Alive` state; the UI no longer guesses that an asynchronous request has completed. Native stop/reset paths clear queued retries and stale life-state atomics, and surface recreation returns to the established fresh-route main-menu policy. Hiding the HUD now also hides the attack button. No new Android permission was added.

Debug builds register a runtime-only retry receiver after the Activity UI exists. It invokes the same Java retry handler used by `RETRY ENCOUNTER` without forcing an Activity lifecycle transition; non-debuggable builds never register it. This closes the device-automation gap without adding a release retry shortcut or changing the visible button.

Windows displays the same vitality state with amber/orange/red severity, reuses the existing pause controls as the death menu, blocks `Esc` and gameplay input while dying/dead, and restores the same deterministic checkpoints. Debug enemy selection is frozen with the rest of gameplay while paused, dying, or dead. The developer overlay on both targets reports player life phase, vitality, and the actual damage-eligibility state, including pause and automation exclusions.

The RT renderer, raygen shader, acceleration-structure path, one-active-skinned-enemy limit, held-prop TLAS ownership, and BGRA red/blue swap route were not changed.

## Automated evidence

The final clean Host validation was:

```powershell
.\tools\run-foundation-validation.ps1 -Mode Host -TimeoutSeconds 300
```

Result: passed all seven stages. The evidence is in `reports/foundation-runs/run-20260731-160235` and records:

- raygen staleness/integrity and deliberate negative safety gates passed;
- clean Windows Debug and Release builds passed;
- all seven CTests passed in both Debug and Release;
- all twelve fixed Windows RT captures completed;
- clean Android Debug and unsigned Release builds passed across `arm64-v8a`, `armeabi-v7a`, `x86`, and `x86_64`;
- Android `lintRelease`, validation packaging/licence checks, and evidence hashing passed.

Test coverage includes non-repeating skeleton hit pulses routed into player vitality, exact invulnerability timing, a clamped oversized fatal-frame update, the 0.65-second dying hold, immutable dead state, reset behavior, safe retry checkpoint state, and the developer-overlay player line.

The final generated Debug APK is 56,353,171 bytes with SHA-256 `4ff3f8d2aeff2edaf1b41ce554f8df36b6d994163c7d1b29931f3791db14ce00`. The final unsigned Release APK is 56,084,270 bytes with SHA-256 `7d90eee8acd4378cd938ad508b089fe074bc76707714aae799bdaaa7c12ea249`; the Host run's staged Windows executable is 845,824 bytes with SHA-256 `2253dc2df1b30408a72f073c80477595e3058220637e936e4755c5b15384abe3`. Validation packages remain explicitly unpublishable.

Android string resources also passed XML parsing, and `git diff --check` reported no whitespace errors.

## `SM-S948B` device evidence

The exact clean Debug APK passed `tools/run-android-showcase-validation.ps1 -Mode Both -Scale 75 -Capture -SkipBuild -SkipInstall` in `reports/android-showcase-runs/run-20260731-160650`: five checkpoints, all 13 replay waypoints, all 12 scene captures, and Home/resume with honest RT presentation completed with no recorded failures. In that uninterrupted run the opening / worst bend / skylight / green / lich medians were 9.942 / 7.993 / 7.167 / 9.801 / 12.634 ms, all below 13.7 ms at thermal status 0 and 28.3-30.6 C battery temperature.

The focused `tools/run-android-vitality-validation.ps1 -SkipInstall` trial in `reports/android-vitality-runs/run-20260731-162245` used that same exact APK and let the real skeleton and lich AI resume after deterministic benchmark sampling. Both encounters reached the Android `YOU FELL` UI, exposed `RETRY ENCOUNTER`, `RESTART ROUTE`, and `QUIT DEMO`, accepted the Debug-only retry broadcast through the production Java handler, logged native retry to `opening` or `mirror`, cleared death, and restored the accessibility-labelled 3/3 vitality HUD. Four valid PNGs, UI XML, scoped logcat, exact screenshot hashes, and a failure-free structured summary preserve that evidence.

An earlier extended run at 35.7-36.0 C exceeded 13.7 ms in opening, green, and lich, so temperature and run-order sensitivity remain relevant stress evidence. The later exact-clean-APK run above reconfirmed every required zone below target in one uninterrupted pass.

The project owner later reported the hands-on pass complete on the same `SM-S948B`, covering human-finger activation of the death actions, perceived accepted/fatal-hit haptics, perceived spatial audio, and the 0.65-second fatal hold. On 2026-08-01 the owner corrected that report: haptics had not actually been checked and were not perceived in the current demo. Preserve the original history, but withdraw only its haptics qualification. The preserved automation evidence above still uses the documented Debug-only receiver because ADB shell touch did not dispatch the visible retry button on this Samsung build.
