# Two-skeleton combat Android validation

**Date:** 2026-08-12

**Device:** Samsung `SM-S948B`, Android 16 / API 36, Adreno 840, driver 512.842.19, Vulkan 1.4.295

**Result:** Automated exact-device performance, presentation, replay, capture, and lifecycle gate passed. Owner hands-on combat/readability/audio/haptic judgment remains separate.

## Exact candidate

- Source commit: `b3428a7cb9b4f4d1ed3d77940bb7d4b177e4b5d6`, clean worktree.
- Debug APK SHA-256: `245c7526051b5323f9521ea3407b3c412908af427078148c9123a28296f7cafa`.
- Installed `base.apk` SHA-256: exact match.
- Embedded raygen SHA-256: `2e889222178fba1d973d1a9b9795c42d725f75972b29b712ac2001702499f7e4`.
- Runtime selected strict ASTC 6x6 diffuse/ARM, ASTC 4x4 normals, strict ASTC 6x6 lich, `RayTracingPipeline`, and honest RT-produced Android swapchain presentation.

No resolution, RT, recursion, ASTC, presentation, or 20 ms gate concession was made. No APK was signed or published.

## Matched GPU-timing A/B

The same installed APK ran isolated 75% lich sessions after cooling. The comparator required matching device, Android version, scale, checkpoint order, source, raygen, local/installed APK hashes, thermal status, and AP/SKIN/BAT starting temperatures within 2 C.

| GPU timestamps | Starting AP / skin / battery | Thermal status | Three 120-frame averages | Median |
|---|---:|---:|---:|---:|
| Enabled | 26.0 / 27.3 / 25.1 C | 0 | 19.497 / 19.472 / 19.527 ms | **19.497 ms** |
| Disabled | 25.7 / 27.4 / 25.2 C | 0 | 19.620 / 19.268 / 18.994 ms | **19.268 ms** |

Enabled was 1.188% slower than disabled, below the 15% investigation threshold; both remained below the strict 20 ms gate. The exact A/B comparator passed at `reports/android-showcase-runs/gpu-timing-ab-20260812-lich.json`.

This did not identify timestamp instrumentation as the material cause of the earlier 23.604 ms lich failure under the tested cooled conditions. That older run rose from AP 31.9 C to 51.1 C and was not thermally matched. The recovered cooled results support thermal/run-order variance rather than a persistent renderer regression; this is an inference from one matched pair, not a claim that timestamps have zero cost.

## Six-checkpoint 75% gate

The clean candidate then ran the default six-checkpoint gate with GPU timing enabled, three 120-frame windows per checkpoint, strict ASTC, and honest RT presentation throughout.

| Checkpoint | Window averages | Median | Derived FPS | Thermal status |
|---|---:|---:|---:|---:|
| opening | 10.639 / 10.589 / 10.566 ms | **10.589 ms** | 94.438 | 0 |
| two-enemy-combat | 12.139 / 12.189 / 12.139 ms | **12.139 ms** | 82.379 | 0 |
| worst-bend | 9.246 / 9.270 / 9.212 ms | **9.246 ms** | 108.155 | 0 |
| skylight | 8.780 / 8.888 / 8.924 ms | **8.888 ms** | 112.511 | 0 |
| green | 11.024 / 11.104 / 11.060 ms | **11.060 ms** | 90.416 | 0 |
| lich | 19.691 / 19.809 / 19.735 ms | **19.735 ms** | 50.671 | 0 |

Every checkpoint passed below 20 ms. The run began at AP 27.9 C / skin 28.6 C / battery 26.4 C and ended at AP 44.6 C / skin 36.4 C / battery 31.6 C, while Android thermal status remained 0. The final GPU RT command-buffer snapshot reported 12.037 ms latest / 9.760 ms average across 1,840 samples.

The `two-enemy-combat` native state reported two active/rendered skeleton entities, attacker entity ID 2 (`SkeletonA`), one shared pose bucket, fixed animation time for the authored checkpoint, and honest presentation. The deterministic route replay reached 13/13 waypoints, finished in the finale, and retained honest presentation.

## 100%, captures, and lifecycle

After cooling, a separate report-only opening run measured:

- 75% opening: 11.321 ms median;
- 100% opening: **18.674 ms median** / 53.550 derived FPS;
- thermal status: 0.

The same exact APK produced all 13 Android scene-only checkpoint captures at 75%, each after 12 stable presented frames with overlays hidden. `two-enemy-combat` capture SHA-256 is `404d1bcf04380c3565449b88d204c4b2da9cd320bd470163cfd7c3e9cddfada0`. Home/resume recreated the surface and produced a new honest RT presentation marker.

Evidence bundles:

- timing enabled: `reports/android-showcase-runs/run-20260812-025659`;
- timing disabled: `reports/android-showcase-runs/run-20260812-025928`;
- complete six-checkpoint/replay gate: `reports/android-showcase-runs/run-20260812-030159`;
- 100%/captures/lifecycle: `reports/android-showcase-runs/run-20260812-030520`.

## Evidence boundary

This closes automated exact-device performance, honest RT presentation, strict ASTC, two-entity/pose-bucket telemetry, deterministic replay, 13 captures, and Home/resume for this exact `SM-S948B` candidate.

Automation and screenshot review do not prove touch/camera comfort, two-enemy readability during live movement, perceived positional audio, haptic cue distinction, corpse/survivor readability, or combat feel. Those remain owner hands-on checks before calling this the fully playtested phone baseline.
