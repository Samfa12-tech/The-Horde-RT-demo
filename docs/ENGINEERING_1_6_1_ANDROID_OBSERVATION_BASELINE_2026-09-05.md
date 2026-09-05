# Engineering 1.6.1 Android Observation Baseline - 2026-09-05

## Result and scope

The exact Engineering 1.6.1 Debug candidate passed the standard Android `Both` run on `SM-S948B`: build/install and installed-artifact parity, strict ASTC selection, honest `RayTracingPipeline` swapchain presentation, six ordered three-window measurements, the deterministic 13-waypoint replay, all 13 capture-manifest checks, and Home/resume. `summary.json` records zero warnings and zero failures, and the lead invocation returned exit code 0.

This is exact-device automated Debug observation evidence. It is not signed/public-package validation, a Release performance claim, a matched A/B comparison, full art or motion acceptance, owner-feel evidence, or evidence that itself establishes host acceptance. Candidate review was open during the phone run and subsequently closed at `4c69928` after clean rereview and CI run `33948503458` passed 35/35.

Audio/haptic manual revalidation required: **NO** - the observation/accounting work and subsequent standalone host-test linkage repair preserve feedback inputs, event routes, cues, timing, and haptic behavior.

## Exact authority and artifact identity

| Field | Recorded value |
|---|---|
| Tested source | `33832460184f342dea612fd0b1f2309e74bd6c77` |
| Source state | `sourceDirty=true`; tracked source was unchanged at build, while four pre-existing untracked tooling scratch paths were present |
| Package | `com.samfa12.hordelanternrt.debug` |
| Build identity | `1.6.1 DEBUG` (`versionName 1.6.1-debug`, `versionCode 9`) |
| Local / installed / retained APK SHA-256 | `01855453ea1804d39f75bc9d519362d8acbdc247c42897fed6b3ec200fac6130` |
| Retained APK | `reports/engineering-1.6.1-c2a-phone/run-20260905-153927/candidate-app-debug.apk` |
| Device | Sole authorised serial `R5GL219SZGK`; raw model `SM-S948B` |
| Android | Android 16 / API 36 |
| GPU / driver | Adreno 840 / `512.842.19`; Vulkan `1.4.295` |

The four pre-existing untracked tooling paths reflected by the dirty source marker were:

- `.superpowers/lead-task3e-b-shipping-mobile/`
- `.superpowers/tmp-task7-critique/`
- `.superpowers/tmp-task7-gauntlet-metadata.cjs`
- `.superpowers/tmp-task7-player-metadata.cjs`

The run targeted the Debug package only; it did not target the separate public package. The APK retained under the ignored report directory hashes byte-for-byte to the local and installed values above.

The later `4c69928df0eef46789cd5903e33312368411a913` repair changes only `CMakeLists.txt`, adding `src/telemetry/RtPerformanceEvidence.cpp` to the standalone `horde_rt_player_animation_tests` host-test target. No Android source changed between tested source `3383246` and that repair. The device evidence remains attributed to `33832460184f342dea612fd0b1f2309e74bd6c77`; it must not be relabelled with the later HEAD. Candidate review was open during the phone run, so the physical pass itself did not establish host acceptance. Host acceptance subsequently closed at `4c69928` after clean rereview and CI run `33948503458` passed 35/35.

## Invocation and run configuration

The lead ran this from the isolated worktree:

```powershell
./tools/run-android-showcase-validation.ps1 -Mode Both -Scale 75 -Capture -DeviceSerial R5GL219SZGK -ExpectedDeviceModel SM-S948B -OutputRoot ./reports/engineering-1.6.1-c2a-phone
```

The resulting run is `run-20260905-153927`. `assembleDebug` completed successfully, streamed installation succeeded, and installed `base.apk` matched the retained candidate hash. The runtime selected strict ASTC materials and `RayTracingPipeline`; the capability report records `rtScene.presented=true` after a frame reached the swapchain.

Internal rendering was `1080x2235` at 75%, the swapchain extent was `1440x2980`, and ADB physical captures were `1440x3120`. USB power was true at both recorded endpoints. Battery evidence moved from 32.3 C / 67% to 43.5 C / 64%; the final Android thermal status was 2.

## Exact RT pipeline identities

The capture manifest identifies the selected pair as Diagnostic/Mobile, not Shipping:

| Role | Key | SHA-256 |
|---|---|---|
| Opaque fast | `diagnostic_mobile_opaque_fast` | `d7b0722e572e63bd96e62bae85cc6a5b968ae71bf157526b513b50b56e515c3b` |
| Generic dielectric | `diagnostic_mobile_generic_dielectric` | `d8f4c915955836f6693c5c5a073c23155944a8a3048dbfdcee6c3a0c4d8f57f7` |

Every capture records this exact pair, `presented=true`, `sceneOnly=true`, its expected zone matching the actual zone, and at least 12 stable presented frames.

## Timing observations

Each timing row is the median of three 120-frame window averages after warmup. The timer is CPU wall-clock from frame start through `vkQueuePresentKHR`; it is not a Vulkan GPU timestamp and it is not a per-frame median.

| Order | Checkpoint | Median (ms) | Derived FPS | Thermal status | Battery (C) | Samsung GPU thermal power |
|---:|---|---:|---:|---:|---:|---:|
| 1 | opening | 53.467 | 18.703 | 0 | 36.1 | 0 |
| 2 | two-enemy-combat | 49.271 | 20.296 | 1 | 38.0 | 0 |
| 3 | worst-bend | 47.447 | 21.076 | 2 | 38.9 | 0 |
| 4 | skylight | 41.301 | 24.212 | 2 | 39.4 | 0 |
| 5 | green | 45.237 | 22.106 | 2 | 39.8 | 0 |
| 6 | lich | 55.140 | 18.136 | 2 | 40.2 | 0 |

All six rows are below the descriptive 30 FPS reference and therefore also below the 50 and 60 FPS references. Crossing a reference is descriptive, not an automatic product failure. This run is not a thermally and operationally matched A/B against a historical build, so it supports no speedup or regression conclusion.

GPU instrumentation was enabled, but the final capability report was recreated after the run and records `valid=false`, zero samples, and “awaiting the first RT submission.” No GPU aggregate is inferred from that final report.

## Replay, captures, and lifecycle

- Replay reached 13 of 13 waypoints, finished with `replayComplete=true`, and did not report failure.
- The manifest contains 13 of 13 captures. Independent hash checks matched all retained PNGs; every expected zone matched, every entry was scene-only and honestly presented, and the minimum stable-presented-frame count was 12.
- Home/resume passed, including honest presentation after resume.
- The run summary contains zero warnings and zero failures.

The lead visually inspected the original opening, skylight, mirror, and two-enemy-combat captures. This bounded sample shows a rendered scene and held sword; authored warm torch lighting where expected; a dark mirror/body silhouette; and the still-procedural arms at the lower corners. This is inspection of four originals, not visual approval of all 13 images, full art/motion acceptance, or closure of a dedicated viewmodel or reward-glass gate. The remaining nine captures are covered by automated manifest/hash/zone/presentation checks only.

## Resource endpoints

Both resource snapshots refer to PID 18126 and include capture work plus lifecycle recreation:

| Measure | Before (KB) | After (KB) |
|---|---:|---:|
| Native heap private | 502804 | 236616 |
| Graphics | 279160 | 260668 |
| Total PSS | 826179 | 550269 |
| Total RSS | 944520 | 664280 |

The thread-list files contain 35 lines before and 33 lines after, including their header. These are endpoints, not exact thread-count claims. The decreases do not prove leak freedom, allocator steady state, or broad resource stability.

## Evidence boundary

The ignored local evidence directory is:

`reports/engineering-1.6.1-c2a-phone/run-20260905-153927/`

The primary files are `summary.json`, `timing.csv`, `capture-manifest.json`, `route-replay-state.json`, `vulkan_capability_report.json`, `logcat.txt`, `lifecycle-home-resume-logcat.txt`, package/install records, battery/thermal/resource endpoints, all 13 state JSON files and PNGs, and the retained candidate APK. These report artifacts remain intentionally untracked.

This baseline establishes exact Debug-package behavior on this `SM-S948B` / driver configuration only. It does not certify other devices, the public package, sustained Release performance, a matched historical comparison, hands-on control comfort, perceived audio/haptics, owner feel, full visual approval, or host acceptance.
