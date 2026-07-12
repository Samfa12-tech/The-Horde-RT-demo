# Animated Skeleton Performance Recovery - 2026-07-12

## Result

The one-enemy Android RT path recovered its phone performance budget without removing hardware ray tracing, lowering the RT dispatch resolution, removing the skeleton, or weakening swapchain-presentation honesty.

- Target: Samsung Galaxy S26 Ultra (`SM-S948B`).
- SurfaceFlinger sample: 126 intervals.
- Median frame interval: 16.667 ms (approximately 60 FPS).
- P95 frame interval: 20.833 ms.
- Warm internal frame averages: approximately 12.9-16.7 ms total, 11.8-15.3 ms waiting for the previous GPU frame, 0.4-0.6 ms CPU recording, and 0.4-0.7 ms submit/present.
- Fresh launch logged `RT frame reached Android swapchain presentation.`
- Visual evidence: `docs/validation/skeleton_perf_phone_2026-07-12.png`.

## Changes that mattered

1. Removed the unconditional 16 ms sleep that was being added after every already-synchronized presented frame.
2. Replaced frame-count-based animation advancement with measured elapsed time.
3. Capped skeleton CPU skinning and dynamic BLAS refit at 30 Hz while keeping camera-held props, ray dispatch, and presentation responsive every frame.
4. Skin the 9,854 unique source vertices once, then expand them into the 28,206 non-indexed RT vertices. The previous path repeated four-weight skinning for every expanded triangle vertex.
5. Compile the locally installable Android debug native library with `-O2` while retaining debug symbols and the debug APK workflow.
6. Added rolling 120-frame Android log telemetry for fence wait, CPU recording, submit/present, and total frame time.

## Remaining constraint

CPU skeleton preparation is no longer the dominant cost. Warm frames are primarily bounded by the previous GPU frame/fence wait. Preserve the current one-frame-in-flight ownership model until TLAS and host-written buffers gain explicit per-frame ownership.

The next visual slice may begin, but every texture and full sword-LOD step must retain this benchmark as a regression gate.
