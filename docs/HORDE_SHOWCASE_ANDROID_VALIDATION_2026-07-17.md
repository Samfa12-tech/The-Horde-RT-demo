# Horde Showcase Route - Android Device Validation - 2026-07-17

## Verdict

**Android-device-validated on Samsung SM-S948B at the current one-active-skinned-enemy limit.**

The complete Windows-first showcase branch was built, installed, traversed, and measured on the primary Android phone. No Android renderer, route, lifecycle, or crash defect was reproduced. This is a debug/device-validation result, not a newly signed release or public upload.

## Exact artifact and device

- Commit: `323fee0d3e6da572b186bce26986bff5f1078416`.
- Debug package: `com.samfa12.hordelanternrt.debug`, version `0.1.0-alpha.1-debug`.
- APK: 58,887,407 bytes, SHA-256 `44D49F9F30BEED08A1B579546D32845B1BF4E8916816A85B58FA39A59D536273`.
- Build/install: `assembleDebug installDebug` passed for all configured ABIs and installed on `SM-S948B`.
- Device: Android 16/API 36, Adreno 840, Vulkan 1.4.295, driver 512.842.19.
- Display: physical `1440x3120`, portrait rotation 0, active density 560, font scale 1.7; swapchain `1440x2980`.

The exact record is retained in [device-build.txt](validation/horde-showcase-android-2026-07-17/device-build.txt).

## RT and asset hard gates

The device log selected:

`PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich`

It then reported:

`RT frame reached Android swapchain presentation.`

At 75%, the private report recorded `RayTracingPipeline`, internal and dispatch extent `1080x2235`, `Presented via swapchain`, and `RT scene presented: yes`. At 100%, the same report recorded internal and dispatch extent `1440x2980` and honest presentation. The app-private runtime contains only the three environment ASTC arrays, two strict ASTC lich textures, the skeleton/lich GLBs, and reports. No `.rgba` or `*-rgba8.ktx2` fallback is staged.

Evidence: [diagnostics-75.png](validation/horde-showcase-android-2026-07-17/diagnostics-75.png), [diagnostics-100.png](validation/horde-showcase-android-2026-07-17/diagnostics-100.png), [vulkan-capability-report-75.txt](validation/horde-showcase-android-2026-07-17/vulkan-capability-report-75.txt), [private-runtime-files.txt](validation/horde-showcase-android-2026-07-17/private-runtime-files.txt), and [install-launch-logcat.txt](validation/horde-showcase-android-2026-07-17/install-launch-logcat.txt).

## Route and gameplay pass

The user completed a hands-on phone run through the full route and reported no visible issues, good responsiveness, and good perceived performance. The unattended follow-up then reset and drove the shared route through the opening, all three zig-zag turns, the lantern-off skylight chamber, yellow/blue/deep-red/restrained-green bays, open threshold, and active lich finale. Collision stopped deliberately overlong gestures at the expected walls, and turning into each next leg continued without a trap or escape.

Captured device evidence confirms:

- warm opening/skeleton composition and BGRA-correct fire;
- clean zig-zag geometry and moving held-prop retraction at walls;
- lantern-off, sword-retained transition into bounded cool-blue skylight illumination;
- four distinct bay-selected colours on wet stone and the player sword;
- the rejected stained pane remains absent and the threshold open;
- the active textured lich, mirror composition, violet staff/electricity illumination, and animated finale state.

The device has no exported position, enemy-health, or encounter-state query. A retained automated finale recording exercises movement and timed swing input, but it is not misrepresented as machine-verifiable hit acceptance. Exact three-hit/two-second-lockout/death/roof logic remains covered by the host gameplay smoke test, while the user's full phone run is the hands-on device verdict. Spatial pan/distance/obstruction are likewise subjective; no crash or SoundPool/native failure appeared, but the unattended phase does not claim instrumented acoustic proof.

Representative evidence: [opening-75.png](validation/horde-showcase-android-2026-07-17/opening-75.png), [worst-bend-75.png](validation/horde-showcase-android-2026-07-17/worst-bend-75.png), [skylight-lantern-off-75.png](validation/horde-showcase-android-2026-07-17/skylight-lantern-off-75.png), [yellow-bay-75.png](validation/horde-showcase-android-2026-07-17/yellow-bay-75.png), [blue-bay-75.png](validation/horde-showcase-android-2026-07-17/blue-bay-75.png), [red-bay-75.png](validation/horde-showcase-android-2026-07-17/red-bay-75.png), [green-bay-75.png](validation/horde-showcase-android-2026-07-17/green-bay-75.png), [finale-active-75.png](validation/horde-showcase-android-2026-07-17/finale-active-75.png), [lich-charge-electricity-75.png](validation/horde-showcase-android-2026-07-17/lich-charge-electricity-75.png), and [lich-finale-automation-75.mp4](validation/horde-showcase-android-2026-07-17/lich-finale-automation-75.mp4).

## Lifecycle and stability

- Pause/menu/resume and deterministic restart were exercised during the controlled traversal.
- Home/resume destroyed and recreated the native surface as designed, restaged strict ASTC assets, restored the persisted 75% scale, and reached honest swapchain presentation again.
- The long live process survived the hands-on thermal session and subsequent automation. Logcat contained no current `FATAL EXCEPTION`, native signal, renderer initialisation failure, failed scale application, or unexpected render-loop termination.
- Historical package exit information contains only prior package-update stops and an older background low-memory reclaim; there is no exit entry from this validation session.

## 75% sustained measurement

Android publishes one short 30-frame sample after renderer creation/scale changes and then 120-frame average windows. It does not publish a per-window median or p95. For each controlled zone, the first post-clear line was discarded and the next three consecutive 120-frame average totals were retained. The table reports the median of those three window averages; it does not rename that value as a median individual-frame time.

| Zone | Three 120-frame average windows (ms) | Median of window averages | Derived FPS | Thermal |
|---|---:|---:|---:|---:|
| Opening | 12.313 / 15.587 / 13.613 | 13.613 ms | 73.5 | 3 |
| Worst zig-zag bend | 10.143 / 10.274 / 10.233 | 10.233 ms | 97.7 | 3 |
| Lantern-off skylight | 10.097 / 10.976 / 10.819 | 10.819 ms | 92.4 | 3 |
| Far green passage | 12.002 / 11.122 / 11.989 | 11.989 ms | 83.4 | 3 |
| Active lich finale | 10.406 / 10.458 / 10.425 | 10.425 ms | 95.9 | 3 |

Every required 75% zone remains below the 20 ms gate at thermal status 3. Raw values are retained in [android_showcase_timing_75.csv](validation/horde-showcase-android-2026-07-17/android_showcase_timing_75.csv).

## 100% report

At the full `1440x2980` internal/dispatch extent, the opening produced three consecutive 120-frame averages of 19.767, 21.700, and 23.991 ms: a 21.700 ms median of window averages, approximately 46.1 FPS derived. Image extent, strict ASTC selection, and honest presentation passed. As planned, no 50 FPS requirement is imposed at 100%. Raw values are retained in [android_showcase_timing_100.csv](validation/horde-showcase-android-2026-07-17/android_showcase_timing_100.csv).

## Thermal context and completion boundary

The phone started at thermal status 0 and reached status 3 during the extended hands-on session. The controlled 75% and 100% sweeps were therefore warm sustained measurements, with battery readings around 43.7-44.1 C, not cold-start claims. See [thermal-before-after.txt](validation/horde-showcase-android-2026-07-17/thermal-before-after.txt).

The recommended persisted tier was restored to 75% after testing. The current route is now **Windows-validated / Android-device-validated** at one active skinned enemy. Raising that count remains a separate Horde scalability slice requiring its own phone measurement. This validation run did not create or upload a signed release; the subsequently rebuilt and rechecked 0.1.1 release is recorded separately in `SHOWCASE_ALPHA_RELEASE_VALIDATION_2026-07-17.md`.
