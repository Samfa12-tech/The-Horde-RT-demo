# Production-shader performance loop

This checkpoint enables measurements of the normal production shader bank. It does not adopt a rendering optimization or establish 30 FPS headroom. Source starts at `28cf707f1558deffcf7a6a1247f5f3d21c11e56a`; earlier inverse-transform and torch-transition measurements used private shader artifacts and remain historical evidence.

## Repeatable control

Build Android Debug with `:app:assembleDebug :app:lintDebug -PhordeRtInstrumentationOverride=Shipping -PhordeRtDielectricQualityOverride=Mobile -PhordeRtRouteTiming=true -PhordeFrameStageProbe=false -PhordeDispatchClockProbe=false`. Do not supply a private region-page root. Native Debug retains its existing `-O2`. This is development instrumentation around production shaders, not a signed Release performance claim.

`chapter-performance-route` reuses the existing collision-resolved out-and-back input schedule: exterior landing hold, descent, lower hold, passage hold, return and second ascent hold. It starts only from the existing admitted reunion checkpoint, waits for current RT presentation, uses ordinary input and shared simulation time, and retains the original 30-second arming, 120-second run and two-second presentation drain limits. No image readbacks occur during timing. Ordinary residency policy remains active. Zero residency saving is an eligible observation; unfinished restoration is not. The distinct streamed-comparison scenario still requires a positive unload/restore pair.

`keeper-torch-transition` remains available for the live room reveal. Neither route is a complete combat, sustained-session or touch-response acceptance test. The strict analyzer joins every completed submission to its actual Android image-presentation timestamp and rejects unresolved, missing, duplicate or discontinuous ownership.

Current phone settings are preserved: 40% dimensions (576×1188 internal, 1440×2971 swapchain), CURRENT shadows, water 1, fire 0, glass off, mist on and dust 1. The saved cap 30 applies to preview policy and does not establish normal-play 30 FPS pacing. SM-S948B/Adreno 840; record each backend separately. Temperature, charging and firmware/driver identities accompany each accepted comparison. GPU duration is distinct from actual display intervals.

## Recorder validation

- Four affected CTests passed in both native Debug and Release, including authored-terrain out-and-back at the original deadlines.
- All 15 strict display-analyzer tests passed, including the new scenario identity and incomplete-tail rejection.
- Android Debug assembly and lint passed.
- Four actual Gradle negative configurations rejected missing Shipping/Mobile policy, malformed booleans, conflicting old/new properties and stage profiling without a private bank.
- Both fresh eight-module shader-bank checks passed with unchanged budgets. A sandbox temporary-output compiler failure was retained; the identical QueryCompute check passed in the normal compiler environment.

Raw APKs, ledgers, logs, temperature samples and full-size images remain in the ignored local `reports/production-performance-2026-10-17-r01/` directory. The pre-existing Debug APK (`f13dda30…4fa527`) and byte-identical saved preferences are preserved there by reference. No release application, app data, rendering quality or production residency defaults were changed.

Measurements and optimization disposition will be appended only after matched runs and correctness inspection. One targeted shader change is tested at a time; unsuccessful changes are reverted.
