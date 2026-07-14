# Material gallery slice - 2026-07-14

## What changed

- Added compact per-world-triangle material IDs and normal codes, uploaded through a storage buffer at raygen binding 6.
- Replaced primitive-range material routing in `minimal.rgen` with explicit material IDs for stone, wet cobble, mossy masonry, damp ground, aged metal, flame, dark figure, hidden shell, mirror, clear glass, and stained glass.
- Added a left-wall gallery table with five angled ASTC material swatches using the existing dry stone, wet cobble, mossy stone, damp ground, and aged metal texture layers.
- Added wall-connected collision for the gallery table so the player is pushed back into the central lane.
- Tightened the Android HUD to stay compact under large accessibility font scaling.

## Validation

- Regenerated `src/vulkan/raytracing/MinimalRayGenShader.inc` from `shaders/raytracing/minimal.rgen`.
  - SPIR-V size: 32,808 bytes / 8,202 words.
  - SHA-256: `13DBF79A02F9CDE24A25DC2136B591FA9BC21EC5B9AB793E6765C92E5D90AB87`.
- Windows Debug build passed with Visual Studio bundled CMake.
- `build/Debug/horde_rt_combat_smoke.exe` passed.
- Android Debug `assembleDebug` passed for all configured ABIs.
- Installed and launched `android/app/build/outputs/apk/debug/app-debug.apk` on Samsung `SM-S948B`.
- Phone logs confirmed strict ASTC material selection:
  - `PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)`
- Phone logs confirmed honest RT presentation:
  - `RT frame reached Android swapchain presentation.`

## Phone thermal sample

The app had already been running for a long thermal session before this validation. After reinstalling the final build and sampling for 45 seconds:

- Last 24 engine timing averages were mostly 13-15 ms.
- Median of the sampled `total` averages: about 14.4 ms.
- Worst sampled `total` average in the 45 second window: 16.271 ms.
- Thermal status: 2.
- HAL thermal readings included AP 46.8 C, skin 41.4 C, battery 40.2 C.
- SurfaceFlinger listed the game `SurfaceView` at `averageFPS = 72.551` for the sampled layer output.

## Notes for Slice 2

- The gallery proves the material-ID route and gives the demo a first exhibit, but the table is still camera-angle dependent. Before widening the route, turn the exhibit into a more deliberately framed wall/table composition that reads from the normal walking path.
- Mirror and glass IDs are now present as clean shader routes; enable the expensive behavior only in bounded screen regions in the next slices.
