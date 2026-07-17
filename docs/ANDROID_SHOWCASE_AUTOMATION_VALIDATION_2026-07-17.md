# Android Showcase Automation Validation - 2026-07-17

## Verdict

**PASS — live debug harness validated on `SM-S948B`.**

The first full default-checkpoint, timing, screenshot, strict-ASTC, honest-presentation, and deterministic route-replay pass completed on the Android 16 / API 36 phone. A second fast pass, skipping build and install, also completed after adding the Samsung-compatible thermal-status fallback.

## Build under test

- Package: `com.samfa12.hordelanternrt.debug`
- Version: `0.1.1-alpha.1-debug`, `versionCode 2`
- APK SHA-256: `1db493ac0362ef4a49dcc2286d31d7388e98751b12607f833a36f5abdfebe962`
- APK size: `57,093,003` bytes
- Display: physical `1440x3120`, density override `560`
- 75% internal RT extent: `1080x2235`
- 100% internal RT extent: `1440x2980`
- Timing method: CPU wall-clock from frame start through `vkQueuePresentKHR`, not a Vulkan GPU timestamp

## Canonical cool automation baseline at 75%

Each value is the median of three consecutive 120-frame window averages after a separate 120-frame warm-up.

| Checkpoint | Zone | Median ms | Derived FPS | Thermal status | Result |
|---|---|---:|---:|---:|---|
| Opening | opening | 11.931 | 83.815 | 0 | Pass |
| Worst bend | shadow corridor | 7.830 | 127.714 | 0 | Pass |
| Skylight | skylight chamber | 7.667 | 130.429 | 0 | Pass |
| Green bay | green torch bay | 11.366 | 87.982 | 0 | Pass |
| Lich | finale | 16.123 | 62.023 | 0 | Pass |

Every required 75% checkpoint remained below the 20 ms gate. Battery temperature reported by Android rose from 30.4 C at the opening sample to 33.7 C at the lich sample. The thermal service remained at status 0 throughout the corrected run.

These cool deterministic figures are regression evidence. They do not replace the warm thermal-status-3 hands-on route certification in `HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`; compare future timing runs within similar thermal bands.

## 100% report-only result

The full run measured the opening at 100% as `25.191 ms` median-of-window-averages, approximately `39.697 FPS`, with honest RT presentation retained. As specified, no 50 FPS acceptance gate was imposed at 100%.

## Renderer and route evidence

- Strict environment ASTC 6x6 diffuse/ARM plus ASTC 4x4 normals selected.
- Strict ASTC 6x6 lich textures selected.
- `RT frame reached Android swapchain presentation` recorded for every restarted checkpoint session.
- Native state JSON agreed with each requested checkpoint, expected zone, render scale, three completed windows, and `presented: true`.
- Deterministic replay reached all 13 waypoints through the collision resolver, ended at `x=-33.7, z=-15.2` in the finale, retained one active lich, settled the lantern, and reported no stall or failure. Adversarial wall slide, shortcut, post, and escape cases remain host-smoke evidence.
- No fatal Android/runtime/renderer marker was present in the scoped log.
- Six post-timing benchmark screenshots plus one replay-finale screenshot were captured and inspected for gross presentation failure. They are supporting evidence, not a substitute for hands-on artistic approval.

## Evidence locations

- Durable reviewed digest: `docs/validation/android-showcase-automation-2026-07-17/`
- Local corrected 75% run: `reports/android-showcase-runs/run-20260717-101642/`
- Local full 75% + 100% + screenshot run: `reports/android-showcase-runs/run-20260717-101403/`

The first run exposed that `cmd thermalservice get-current-thermal-status` is unsupported on this Samsung Android 16 build. Its raw thermal dump still recorded status 0. The runner now falls back to `dumpsys thermalservice`; the corrected second run recorded status 0 directly in every CSV row.

## Remaining hands-on boundary

This automation validates deterministic renderer state, performance windows, collision-integrated route traversal, zone transitions, ASTC routing, and honest RT presentation. It does not replace subjective touch feel, perceived spatial audio, visual art approval, accepted sword-hit feel, or pause/Home lifecycle checks. Those remain the short manual pass described in `ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md`.
