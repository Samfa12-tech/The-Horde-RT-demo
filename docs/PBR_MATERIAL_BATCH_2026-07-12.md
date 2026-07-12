# First Imported PBR Material Batch - 2026-07-12

## Implemented

- Three Vulkan 2D-array images with five layers each: sRGB diffuse, OpenGL normal, and packed AO/roughness/metal.
- Repeat sampler and raygen descriptors at bindings 3-5.
- World-space planar mapping for the existing corridor geometry, avoiding a broad UV/mesh rewrite.
- Dry medieval wall, wet cobblestone, mossy wall, damp puddle-edge ground, and rusty metal materials.
- Puddle areas blend into the damp-ground layer and reduce roughness.
- Android stages the derived runtime arrays from packaged assets into app-private storage before native RT initialization.

## Mobile budget

- Original retained maps: 1K JPG sources.
- Runtime proof: 512 x 512 x 5 layers x 3 RGBA arrays, approximately 15 MiB of uncompressed image data.
- Initial unfiltered debug APK artifact: 93,855,324 bytes.
- A Gradle-generated runtime-only asset set now packages only the live skeleton GLB and three derived arrays. The rebuilt debug APK is 46,793,811 bytes, a reduction of 47,061,513 bytes (about 50.1%).
- App-private staging verified all three runtime arrays at 5,242,880 bytes each.
- Follow-up: use mobile GPU compression after visual mapping is settled.

## Phone validation

- Fresh install and cold launch succeeded.
- Log confirmed `RT frame reached Android swapchain presentation.`
- 126 SurfaceFlinger intervals: 16.667 ms median, 20.833 ms p95, approximately 60 FPS median.
- Warm internal CPU record time remained approximately 0.5-0.7 ms.
- Evidence: `docs/validation/pbr_batch_phone_2026-07-12.png`.

The 50+ FPS regression gate passed. The full textured sword LOD remains out of scope until static GLB material upload is measured separately.
