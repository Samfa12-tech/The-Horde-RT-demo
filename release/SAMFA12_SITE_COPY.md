# Horde Lantern RT - showcase alpha 0.1.3

Public package version: `0.1.3-alpha.1`
Canonical downloads: https://samfa12.itch.io/the-horde

Horde Lantern RT is a short native Vulkan hardware-ray-tracing technology demo for Android phone hardware and Windows RTX. Carry a lantern through a wet gothic ruin, fight a skeleton, watch the flame fail, cross a blue skylight chamber and four coloured-light bays, then defeat a floating, staff-lit lich and return dawn to the ruin.

## This alpha proves

- Native Vulkan BLAS/TLAS acceleration structures.
- A ray-tracing pipeline, shader binding table, and `vkCmdTraceRaysKHR` frame dispatch.
- Honest swapchain presentation of the RT-produced image.
- Phone-safe `rayQueryEXT` shading inside raygen rather than a raster fallback.
- Moving lantern shadows, bounded coloured lighting, a hero mirror, wet stone, and emissive enemy effects.
- One shared scene and gameplay route across Android and Windows.
- A rebuilt low-poly player body with articulated walking, vitality, encounter retry, and a complete post-lich epilogue.

## Downloads

- Android: stable-key-signed APK for compatible Vulkan-RT phones. Validated on Samsung `SM-S948B`; 75% RT resolution is the sustained recommendation.
- Windows: portable x64 zip for compatible Vulkan hardware-RT GPUs. Validated on an NVIDIA GeForce RTX 5050 Laptop GPU.
- Source and issue tracking: https://github.com/Samfa12-tech/The-Horde-RT-demo

## Important compatibility note

This demo is RT or nothing. Unsupported devices receive a clear diagnostic report; they do not receive a raster, browser, or fake-ray-tracing fallback.

- Android packaging minimum is Android 7 / API 24, but only Samsung `SM-S948B` on Android 16 is currently device-certified. The driver must expose Vulkan acceleration structures, RT pipeline, ray query, buffer device address, deferred host operations, and required ASTC formats.
- Windows requires 64-bit Windows 10/11 and a Vulkan driver exposing the same RT extension set. The validated GPU is an NVIDIA GeForce RTX 5050 Laptop GPU.

At the recommended Android 75% RT scale, the 0.1.3 connected Debug gate completed all five measured checkpoints, all 13 replay waypoints, all 12 scene captures, and Home/resume with honest RT presentation. The measured checkpoints ranged from 7.096 to 15.727 ms; two later cool/status-0 lich samples measured 13.723 and 13.600 ms. The full 100% extent remains report-only and can fall below 50 FPS.

## Controls

- Android: left-side drag moves/strafe, right-side drag looks, `SWING` attacks, Android Back pauses/resumes.
- Windows: `WASD` moves, left mouse drag looks, right mouse or Space attacks, Esc pauses, R restarts, F1/F2 show controls/diagnostics, and Alt+Enter toggles fullscreen.

## Alpha scope

This is a deliberately bounded technology showcase. It renders and animates one skinned enemy at a time: the skeleton owns the opening encounter and the CC0 Meshy lich owns the finale. Simultaneous hordes, water, and broader combat systems remain later measured slices.

## Credits

- Environment materials: Poly Haven, CC0.
- Sound effects: FilmCow Royalty Free Sound Effects Library.
- Original stylized skeleton: Hotstrike Studio; texture, rig, and animation processing created with Meshy.
- Placeholder lich: created and animated with Meshy, CC0.
- Full provenance and licence details are included in the Windows zip and linked from the download page.

## AI assistance disclosure

AI tools assisted code development. Meshy was used for the credited character-processing work and CC0 placeholder lich. OpenAI image generation created the application icon. On itch, classify the project as AI Assisted and select both Code and Graphics.
