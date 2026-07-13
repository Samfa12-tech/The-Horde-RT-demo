# RT Lighting Seam and Moon Repair - 2026-07-13

## Outcome

- Bright exterior seams at the floor, wall, ceiling, and end-wall joins are removed in the Windows RT presentation.
- Room-two moonlight now comes through one irregular physical roof breach and reads as one composed surface-light patch instead of three ruler-straight stripes.
- The visible moon, direct moonlight, and first-bounce moon direction are aligned.
- The phone-safe path still dispatches with `vkCmdTraceRaysKHR` and uses the existing single ray-query moon visibility test per shaded pixel. No fake shaft or extra moon shadow ray was added.

## Seam repair

- Static material shading retains its texture normal, but secondary-ray origins now use a separate geometric normal.
- Secondary query bias was reduced from centimetres to millimetres: general trace `tMin` is `0.002`, visibility `tMin` is `0.0015`, and the geometric-normal origin offset is `0.004` scene units.
- Eight hidden outer-shell triangles sit 7 cm behind the side walls, floor, and end wall. They catch numerical edge misses without covering the entrance or roof breach.
- Existing visible primitive/material indices remain unchanged because the blockers are appended after the authored world geometry. Primitives 50-57 have a dedicated subdued stone material and face-correct geometric normals in case a grazing primary ray reaches them.

## Moon and output repair

- One near-unit `kMoonDirection` is used by the sky disc, direct light, and bounce shading.
- Four physical roof quads frame an irregular opening over room two. Ray-query visibility remains the authority for whether moonlight reaches a surface.
- The previous untextured blue overlay and reversed-edge room mask were removed. Moon diffuse is multiplied by material albedo, with one cheap reflectivity-shaped specular term for wet stone and metal.
- All reversed-edge `smoothstep` uses in the active raygen shader were rewritten with defined GLSL ordering.
- Distance fog was reduced and can pick up a restrained moon tint where direct visibility exists.
- The final pass now uses an ACES-style filmic curve and explicit linear-to-sRGB encoding before the existing RGBA-to-BGRA presentation swap.

## Validation

- `glslangValidator` compiled `shaders/raytracing/minimal.rgen` for Vulkan 1.2 successfully.
- The generated `MinimalRayGenShader.inc` byte-for-byte matches the 34,828-byte compiled SPIR-V (`SHA-256 97774EAEEAEA4850D10980F2FDA9BB8776757D941E9C6A24C5DF3864A574D520`).
- A fresh Windows Visual Studio build produced and ran `horde_rt_diagnostic_window`; the RT scene reached visible presentation.
- Desktop evidence: `docs/validation/moonlight_seam_fix_windows_2026-07-13.png`.
- Android `assembleDebug` completed successfully for `arm64-v8a`, `armeabi-v7a`, `x86`, and `x86_64`.
- APK: `android/app/build/outputs/apk/debug/app-debug.apk`, 46,793,811 bytes, SHA-256 `B976A7B2564A786EA6AF93BBEC5BFF21D6E33B898D902ED9273F9BBCDF688117`.
- No Android device was connected on 2026-07-13, so install, cold-launch, RT-present log, screenshot, and performance regression checks remain pending.
