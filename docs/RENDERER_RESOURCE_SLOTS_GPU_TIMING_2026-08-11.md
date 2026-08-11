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

The extraction preserves the historical animation cadence, clip selection, transforms, custom index, mask, staff sample accumulation order, floating-point expressions, eight-BLAS report, eighteen-instance TLAS, and one-active-character ceiling. The slot rejects roster capacity/count above one and rejects a selected/active archetype mismatch before touching GPU resources.

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

## Known limits

- One active character render slot and one enemy TLAS entry remain enforced.
- Both skeleton and lich resources remain resident; this slice does not reduce the current memory footprint.
- Timestamp queries add small profiling overhead and provide approximate GPU execution intervals, not presentation latency.
- Current phone timing and thermal behavior remain unverified until the authorised `SM-S948B` is connected.
