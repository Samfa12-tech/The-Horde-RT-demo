# Developer Overlay - Windows Validation - 2026-07-17

## Verdict

**PASS on Windows Debug and the connected Android Debug device.**

The compact developer overlay is live, non-pausing, Debug-only, and hidden from branded entry, pause, settings, and full-diagnostics surfaces. Windows F3 toggles it independently while F2 retains the existing paused RT diagnostics view.

## Live Windows evidence

- GPU: NVIDIA GeForce RTX 5050 Laptop GPU.
- Vulkan API reported live: 1.4.341.
- RT mode: `RayTracingPipeline`.
- Honest presentation: `presented YES` only after an RT-produced frame reached the swapchain.
- Persisted test scale: 80%, internal/dispatch extent `986x642`.
- Live renderer sample during the opening: approximately 6.1 ms / 165 FPS. This measurement ends with `RenderFrame`; the throttled Win32 text refresh occurs afterward and is not included in that renderer timing.
- Renderer counts: 8 BLAS, 1 TLAS, 18 TLAS instances, one active skinned enemy.
- Build/shader identity, route zone, lantern phase, selected enemy/phase/health, and material encoding updated in the visible panel.
- Local reviewed captures: `reports/windows-developer-overlay-debug.png` and `reports/windows-developer-overlay-lich-debug.png`.

## Interaction checks

- F3 showed the live overlay without changing pause state.
- F2 hid the compact overlay; closing diagnostics restored it.
- Esc pause hid the compact overlay; resume restored it.
- Five Debug F8 validation-point steps updated the overlay from the opening to `SCENE finale` and selected the active lich.
- The Win32 overlay control is disabled as an input target, does not take focus, and cannot intercept mouse-look initiation.
- The compact DPI-scaled font kept all seven telemetry lines visible at the active Windows scale.

## Build and test gates

- Windows Debug: build passed; all six CTests passed.
- Windows Release: build passed; all six CTests passed.
- Release executable inspection found no F3 live-overlay help/menu marker.
- Android: `assembleDebug assembleRelease lintRelease` passed for all configured ABIs.
- The shared formatter smoke test covers presented yes/no, explicit BLAS/TLAS/instance counts, active and absent enemy health, timing/extents, state, material, build, and shader identity.

## Android device addendum - 2026-07-18

Android Debug contains the native 4 Hz snapshot publisher, hidden non-interactive overlay view, `horde.debug.overlay` intent, and debuggable long-press toggle. Android Release returns an empty developer-overlay request and never shows the view.

On `SM-S948B`, the overlay displayed all seven telemetry lines at the existing 1.7 font scale, reported correct Adreno/Vulkan/RT/extents/state/material values, toggled through the RT-status long press, and survived Home/resume. The standard five-checkpoint 75% gate and route replay passed at thermal status 0. A separate overlay-active opening sample retained honest presentation and a 17.473 ms median of three 120-frame averages, below the existing 20 ms validation budget.

The view is non-clickable. The owner subsequently passed the requested movement/look/Swing, perceived-audio, and visual-legibility check without reporting an issue. Full details are in `IN_APP_BENCHMARK_ANDROID_VALIDATION_2026-07-18.md`.
