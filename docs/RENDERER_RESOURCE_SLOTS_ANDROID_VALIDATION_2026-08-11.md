# Renderer resource slots Android validation

Date: 2026-08-11
Device: Samsung Galaxy S26 Ultra (`SM-S948B`)
Evidence type: direct local automated exact-device testing

## Scope

This follow-up validates the stacked renderer resource-slot and Vulkan GPU-timestamp milestone on the authorised primary Android device. It uses the same one-active-character renderer boundary and does not add a second enemy, change the shader, alter assets, sign a release, or publish a build.

The retained device bundle is `reports/android-showcase-runs/run-20260811-201708`. The report directory is ignored working evidence; this document preserves the durable result.

## Exact candidate and command

- Source commit: `14db30a99e974fe3200d24d957c76e945521e9a1`
- Branch: `codex/renderer-resource-slots-gpu-timing`
- Debug package: `com.samfa12.hordelanternrt.debug`
- Debug APK SHA-256: `4e782c89a323a4b00db2eb23a5de3c5ff44862880014e7c9394bcb982f8001af`
- Command: `.\tools\run-android-showcase-validation.ps1 -Mode Both -Scale 75 -Capture -TimeoutSeconds 300`

The phone was reserved exclusively for this project, the project Debug app was stopped, the display was put to sleep, and the run began after cooling. The recorded pre-run HAL temperatures were AP 31.9 C, battery 28.9 C, and skin 31.1 C at Android thermal status 0.

## Device and RT capability

- Android 16 / API 36
- GPU: Adreno 840
- Vulkan API: 1.4.295
- Driver: 512.842.19
- Selected mode: `RayTracingPipeline`
- Internal 75% extent: 1080 x 2235
- Required extensions present: acceleration structure, ray-tracing pipeline, ray query, buffer device address, and deferred host operations
- Required features present: acceleration structure, ray-tracing pipeline, ray query, and buffer device address
- Strict ASTC route: ASTC 6x6 diffuse/ARM, ASTC 4x4 normal, and strict ASTC 6x6 lich
- Honest RT presentation: yes; an RT-produced frame reached successful swapchain presentation

The final capability report recorded GPU RT command-buffer timing as supported and valid: latest 5.486302 ms, average 5.953842 ms, 29 samples, 48 valid timestamp bits, 52.0833 ns timestamp period, zero unavailable results, and zero query errors. These Vulkan timestamps cover the recorded RT command-buffer interval; they are not presentation or input latency. During the sustained lich benchmark, the rolling GPU average reached 21.569 ms after 409 samples, consistent with the CPU-side fence wait dominating that checkpoint.

## Automated functional results

- Deterministic route replay: 13/13 waypoints, completed in the finale, one active skinned enemy, honest presentation retained.
- Deterministic captures: 12/12 named scene-only checkpoints, animation time 0, at least 12 stable presented frames each, valid 1440 x 3120 PNGs.
- Lifecycle: Home/resume passed and a new honest RT presentation marker was observed after surface recreation.
- Fatal Java, native, or renderer markers: none reported.

## Performance result

CPU wall-clock timing runs from frame start through `vkQueuePresentKHR`; it remains separate from GPU timestamp telemetry.

| 75% checkpoint | Median of three 120-frame windows | Derived FPS | Battery C | Result against 20 ms gate |
| --- | ---: | ---: | ---: | --- |
| opening | 10.327 ms | 96.834 | 29.3 | Pass |
| worst bend | 7.109 ms | 140.667 | 29.9 | Pass |
| skylight | 8.353 ms | 119.717 | 29.9 | Pass |
| green | 11.220 ms | 89.127 | 31.8 | Pass |
| lich | 23.604 ms | 42.366 | 33.0 | **Fail** |

The automated gate therefore exited non-zero with one failure: `lich median 23.604 ms exceeded the 20 ms 75% gate.` The three lich windows were 24.153, 23.552, and 23.604 ms. By the end of the complete benchmark, replay, capture, and lifecycle sequence, HAL temperatures had risen to AP 51.1 C, battery 35.4 C, and skin 40.3 C at thermal status 1.

Earlier measurements made before the phone was reserved exclusively were affected by another concurrent device-testing task and are not acceptance evidence. A temporary timestamp-cadence experiment was reverted and never committed. The branch remained clean at the exact source commit above for this retained run.

## Conclusion and boundaries

This exact device/driver validates the resource-slot path, strict ASTC selection, real RT presentation, Vulkan timestamp support, deterministic route/capture behavior, and Home/resume recovery. It does **not** pass the current sustained 75% performance gate because the lich checkpoint exceeded 20 ms. The result must not be presented as a full Android performance pass or used to rebaseline the guardrail.

Automated testing does not prove human touch feel, camera comfort, perceived audio or haptics, artistic correctness, or behavior on another model or driver. No signed release was installed and nothing was published.

## Owner hands-on follow-up

The project owner subsequently reported that controls, audio, and haptics all worked correctly during a hands-on test on `SM-S948B`. This is owner-reported local device validation, separate from the automated exact-APK evidence above; this task did not capture a new APK hash, installed-package check, log, or screenshot for the hands-on session.

The report closes basic control operation, audible feedback, and perceived haptic operation for the owner-tested installed development build. It does not change the 23.604 ms lich performance failure or establish touch/camera comfort, spatial direction or distance accuracy, audio-mix quality, haptic intensity or cue distinction, artistic approval, signed-release behavior, or another device/driver.
