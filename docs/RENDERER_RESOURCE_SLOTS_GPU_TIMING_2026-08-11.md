# Renderer resource slots and GPU timing

Date: 2026-08-11
Stacked base: `codex/shared-simulation-foundation` at `b0a7b76ad2ca8128a4537a6488aed9d70c724f42`

## Purpose

This post-foundation slice extracts narrow Vulkan resource and character-rendering seams from `PresentableTinyRtScene` and adds honest GPU command-buffer timing. It does not add a second enemy, a second active character slot, a renderer rewrite, an ECS, a shader change, a descriptor change, or new content.

The preserved baseline remains native `vkCmdTraceRaysKHR` presentation, phone-safe `rayQueryEXT`, recursion depth one, strict Android ASTC, one frame in flight, eight resident BLAS, eighteen TLAS instances, and one active skinned enemy at TLAS instance index 2.

## Pre-edit baseline

The clean stacked-branch Host gate passed in `reports/foundation-runs/run-20260811-182230` before renderer edits. Windows Debug and Release, all ten CTests, twelve deterministic Windows RT captures, Android clean Debug/unsigned Release/lint, shader staleness, negative safety, package/licence, and evidence hashing all passed. ADB reported zero connected devices.

## Resource boundary

Low-level buffer allocation/upload/destruction, acceleration-structure lifetime helpers, and updatable triangle-BLAS state move behind a checked renderer resource context. BLAS sizing/build/refit recording, texture images, descriptors, pipeline, shader binding table, output image, capture readback, and platform device ownership remain in `PresentableTinyRtScene`.

The helper borrows Vulkan device/dispatch handles. The platform still owns the device and must reach device-idle before destroying or moving a live scene. Destroy clears borrowed handles and function pointers; the helper never destroys the platform device.

## Character slot boundary

One explicit character render slot owns the existing skeleton and lich CPU models, skinned vertices, vertex buffers, resident BLAS/scratch resources, animation caches, and audited lich staff sample. Both archetype resources remain resident, but only the selected archetype is uploaded/refit and only one acceleration-structure address is placed in TLAS instance 2.

The extraction preserves the historical animation cadence, clip selection, transforms, custom index, mask, staff sample accumulation order, floating-point expressions, eight-BLAS report, eighteen-instance TLAS, and one-active-character ceiling. The slot permits spare roster capacity for future architecture while rejecting an active count above one or a selected/active archetype mismatch before touching GPU resources.

## GPU timing contract

GPU time is measured with Vulkan timestamp queries around the RT command buffer after the existing frame-slot fence completes. Query support requires a non-zero graphics-queue `timestampValidBits` and a finite positive physical-device `timestampPeriod`. Unsupported or failed timing remains non-fatal and reports `N/A`; it never disables RT rendering.

Timestamp results are read without `VK_QUERY_RESULT_WAIT_BIT`, use availability values, and account for reduced-bit counter wrap. The reported value is labelled **GPU RT command-buffer time**. It includes the recorded acceleration-structure, trace, barrier, and copy/blit work between the top- and bottom-of-pipe markers. It excludes CPU skinning, command recording, acquire/fence waits, queueing delay outside the markers, `vkQueuePresentKHR`, compositor work, display latency, and input latency.

Existing CPU frame time, FPS, benchmark schema, benchmark pass/fail, Android checkpoint parsing, and presentation honesty retain their prior meaning.

## Focused tests

- `horde_rt_gpu_timestamp_math_tests` covers ordinary conversion, 16/32/36/48/64-bit masking and wrap, zero duration, invalid bit widths/periods, and non-finite results.
- `horde_rt_character_render_slot_smoke` covers historical skeleton/lich clip and transform mapping, 30 Hz refresh cadence, exact asset vertex counts, the audited staff sample, rejection of a two-character roster, and rejection of selected/active roster mismatch.
- `horde_rt_developer_overlay_smoke` covers available and unavailable GPU text plus the compile-time `Available -> ResultUnavailable/QueryError` current-sample contract.
- Fresh Windows Debug and Release each passed 12/12 CTests. The separate Vulkan-off Release lane passed 8/8 host tests, including timestamp math.

## Final validation

The final canonical Host gate passed all seven stages at `reports/foundation-runs/run-20260811-185715`:

- shader staleness and negative-safety gates passed; compiled raygen remained 75,708 bytes / 18,927 words / 4,266 instructions / 620 branches / 6 loops / 235 selection merges, with SPIR-V SHA-256 `440ae38774b95e33b078bdb1acdd475033403f3bbdb13a8c031e1332b660a0bc`;
- fresh Windows Debug and Release builds passed 12/12 CTests in 24.56 s and 14.30 s;
- all twelve 960x540 scene-only RTX captures were honestly presented and byte-identical to the clean pre-edit run `run-20260811-182230` (zero PNG hash mismatches);
- the final RTX 5050 capture manifest reported CPU wall-clock median 6.052750 ms / mean 6.817525 ms across 144 samples, and a separately labelled GPU RT command-buffer latest 0.484544 ms / average 0.591915 ms across 143 samples, with 64 valid timestamp bits and 1.0 ns period;
- Android clean Debug and validation-unsigned Release/lint passed for arm64-v8a, armeabi-v7a, x86, and x86_64; the Debug APK SHA-256 was `bca4c05b7a3b3285a17f294e84a83d6d7bccd2e422a9ab6ef6a37f51c4ca2e5e` and the unsigned Release APK SHA-256 was `6f884a1b8691fc5425b0b8381c878ba299b3314decb6e34c6b63b0c5c992881d`;
- validation packaging/licensing and evidence hashing passed; `git lfs fsck` and `git diff --check` passed.

ADB reported zero connected devices. No APK was installed, signed, or published, and no Android GPU-timestamp, presentation, frame-time, thermal, touch, audio, haptic, or lifecycle result is claimed from this milestone.

## Android device follow-up

The authorised `SM-S948B` was subsequently reserved for this project and tested from the exact clean source commit with:

`.\tools\run-android-showcase-validation.ps1 -Mode Both -Scale 75 -Capture -TimeoutSeconds 300`

The retained bundle is `reports/android-showcase-runs/run-20260811-201708`; the durable record is [`RENDERER_RESOURCE_SLOTS_ANDROID_VALIDATION_2026-08-11.md`](RENDERER_RESOURCE_SLOTS_ANDROID_VALIDATION_2026-08-11.md). The freshly built Debug APK SHA-256 was `4e782c89a323a4b00db2eb23a5de3c5ff44862880014e7c9394bcb982f8001af`.

On Android 16/API 36, the Adreno 840 (driver 512.842.19, Vulkan 1.4.295) selected strict ASTC and `RayTracingPipeline`, exposed every required extension/feature, and honestly presented RT-produced swapchain frames. GPU RT command-buffer timing was supported and valid: the final capability snapshot recorded 5.486302 ms latest / 5.953842 ms average across 29 samples, 48 valid timestamp bits, 52.0833 ns period, zero unavailable results, and zero errors. The deterministic replay reached 13/13 waypoints, all 12 scene-only captures completed, and Home/resume retained honest presentation.

This was not a full performance pass. Ordered 75% CPU medians were 10.327 / 7.109 / 8.353 / 11.220 / 23.604 ms for opening / worst bend / skylight / green / lich. Lich exceeded the 20 ms gate, so the runner exited non-zero. Recorded HAL temperatures rose from AP 31.9 C / battery 28.9 C / skin 31.1 C / status 0 before the run to AP 51.1 C / battery 35.4 C / skin 40.3 C / status 1 afterward. The historical zero-device Host-gate statements above remain intact because they describe the earlier run conditions.

## Known limits

- One active character render slot and one enemy TLAS entry remain enforced.
- Both skeleton and lich resources remain resident; this slice does not reduce the current memory footprint.
- Timestamp queries add small profiling overhead and provide approximate GPU execution intervals, not presentation latency.
- Phone GPU timing, functional presentation, replay, capture, and lifecycle behavior are now exact-device verified on `SM-S948B`; sustained 75% performance remains unresolved because the lich checkpoint failed the 20 ms gate.
- Automated device testing does not establish human touch feel, camera comfort, perceived audio/haptics, artistic correctness, or behavior on another model/driver.
