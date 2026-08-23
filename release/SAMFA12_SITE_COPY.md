# Horde Lantern RT - Showcase Alpha 0.1.5

Public package version: `0.1.5-alpha.1`
Canonical downloads: https://samfa12.itch.io/the-horde

Horde Lantern RT is a short native Vulkan hardware-ray-tracing action showcase for compatible Android phones and Windows RT GPUs. Carry a lantern through a wet gothic ruin, fight two skeletons with timed sword contact and parries, then cross a clear roof-water curtain that extinguishes the flame. Follow the reflected runoff to its drain, move through four coloured-light studies, and defeat a staff-lit lich surrounded by cold ritual ground mist before dawn returns to the ruin.

## Showcase Alpha 0.1.5

- Real world-BLAS waterfall geometry: three broken roof streams, a rounded catchment, connected floor runoff, and a recessed barred drain.
- Dynamic clear-water reflection and refraction using water IOR, Schlick Fresnel, Beer-Lambert absorption, animated flow normals, and bounded ray queries.
- Low, depth-clipped lich-room mist that preserves the enemy silhouette, sword, and roof finale.
- Positional looping waterfall ambience with world-space attenuation and stereo pan.
- Two independently killable animated skeletons, one active attacker, animation-owned sword contact, timed parry, visible stagger, and three-point player vitality.
- Collision-safe directional dodge.
- Full tested Backbone One support on Windows: left stick move, right stick aim, RT attack, LT parry, B/Circle dodge, D-pad menus, A select, Menu/Start pause, and controller render-scale adjustment.
- Mouse/keyboard remains fully supported.

## Native RT, without a fallback

- Vulkan BLAS/TLAS acceleration structures.
- Ray-tracing pipeline, shader binding table, and `vkCmdTraceRaysKHR` frame dispatch.
- Honest swapchain presentation of the RT-produced image.
- Phone-safe `rayQueryEXT` shading inside raygen at recursion depth one.
- Switchable Android RT water quality: Mobile by default, High, or Off. This changes bounded water-ray work; it does not replace the scene with raster or lower the selected RT render resolution.
- Moving lantern shadows, coloured lighting, wet stone, a hero mirror, emissive enemies, water optics, and bounded volumetric mist.

This project is RT or nothing. Unsupported devices receive a clear diagnostic report instead of a raster, browser, baked, screen-space, or fake-ray-tracing fallback.

## Downloads and compatibility

- Windows: portable x64 zip. Requires 64-bit Windows 10/11 and a Vulkan driver exposing acceleration structures, RT pipeline, ray query, buffer device address, and deferred host operations. Validated on an NVIDIA GeForce RTX 5050 Laptop GPU.
- Android: stable-key-signed APK. Packaging supports Android 7/API 24 and later, but hardware compatibility is deliberately much narrower. Samsung raw model code `SM-S948B` / Adreno 840 on Android 16 is the current certified target; 75% RT resolution with Mobile water is recommended.
- Source and issue tracking: https://github.com/Samfa12-tech/The-Horde-RT-demo

On the exact `SM-S948B` Debug candidate at the recommended 75% RT scale, median CPU-present-loop measurements were 12.541 ms at the waterfall drench, 11.363 ms in the no-torch skylight view, and 20.457 ms in the heaviest misted lich view. The same candidate completed the 13-waypoint replay, all 13 captures, and Home/resume with honest RT presentation. These are exact-device observations, not frame-rate promises for other phones.

## Controls

### Windows

- `WASD`: move and strafe
- Left mouse drag: camera look
- Right mouse or `Space`: attack
- `Q`: timed parry
- `Esc`: pause/resume
- Controller: left stick move, right stick look, RT attack, LT parry, B/Circle directional dodge, D-pad menu navigation, A select, Menu/Start pause
- `R`: restart route; `F1`: controls; `F2`: diagnostics; `Alt+Enter`: fullscreen

### Android

- Left-side drag: move and strafe
- Right-side drag: camera look
- `SWING`: attack
- `PARRY`: timed skeleton-melee parry
- Android Back: pause/resume

## Credits

- Environment materials: Poly Haven, CC0.
- Sound effects: FilmCow Royalty Free Sound Effects Library.
- “Water Dripping” by DRAGON-STUDIO via Pixabay, Pixabay Content License.
- Original Free Stylized Dark Fantasy Skeleton: Hotstrike Studio; derivative texture, rig, and animation processing created with Meshy, CC BY 4.0.
- Placeholder lich: created and animated with Meshy, CC0.
- Application icon: created for this project with OpenAI image generation.
- Full provenance and licence details: https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/main/ASSET_LICENSES.md

## AI assistance disclosure

AI tools assisted code development. Meshy was used for the credited character-processing work and CC0 placeholder lich. OpenAI image generation created the application icon. On itch this project is classified as AI Assisted with Code and Graphics selected.
