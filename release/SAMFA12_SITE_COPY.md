# Horde Lantern RT - Showcase Alpha 1.6.0

Public package version: `1.6.0`
Canonical downloads: https://samfa12.itch.io/the-horde

Horde Lantern RT is a short native Vulkan hardware-ray-tracing action showcase for compatible Android phones and Windows RT GPUs. Carry a true world-space fire through a wet gothic ruin, fight two skeletons with timed sword contact and parries, then cross a clear roof-water curtain that extinguishes the flame. Defeat a staff-lit lich, follow the audible chest unlock to claim a physical Gothic reward lantern, and carry its refractive amber light into the returning dawn.

## Showcase Alpha 1.6.0

- Textured PBR sword, opening torch, reward chest, Gothic lantern, and player-character foundation through a reusable runtime GLB/material pipeline.
- Real deterministic volumetric torch fire whose visible flame, coloured direct light, shadows, reflections, movement response, and flicker share one emitter state.
- A post-lich reward sequence with locked/unlocked prompts, audible latch and opening cues, warm chest guidance light, interaction, lantern claim, and high/low carry poses.
- Closed lantern glass with bounded ray-traced Fresnel reflection, transmission, refraction, thickness, attenuation, and transparent shadows instead of alpha-only panes.
- Fixed-step physical lantern swing driven by movement, stopping, strafing, turning, dodge, and raise/lower acceleration.
- Edge-forward PBR sword presentation and a downward cut that chains into a smooth upward slice on a second press.
- Optional background update checks against public GitHub Releases; updates are never installed silently.

- Unlock the cross-platform RT Lab after defeating the lich, then tune the live waterfall span, finale roof and dawn, ritual fog, authored light groups, and bounded RT workload while the native ray-traced scene keeps rendering.
- Water now uses the same active lights, material response, atmosphere, and real ray-traced world/player visibility as ordinary opaque surfaces through both refraction and High-quality reflection.
- Improved RT Lab scrolling and controller/keyboard navigation on Windows, with reliable slider repaint after scrolling and reopening.

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
- Switchable Android RT water quality: Mobile by default, High, or Off. This changes bounded water-ray work; it does not replace the scene with raster or lower the selected RT render resolution. Lantern glass retains the same physical model with finite Mobile/High budgets.
- Moving lantern shadows, coloured lighting, wet stone, a hero mirror, emissive enemies, water optics, and bounded volumetric mist.

This project is RT or nothing. Unsupported devices receive a clear diagnostic report instead of a raster, browser, baked, screen-space, or fake-ray-tracing fallback.

## Downloads and compatibility

- Windows: portable x64 zip. Requires 64-bit Windows 10/11 and a Vulkan driver exposing acceleration structures, RT pipeline, ray query, buffer device address, and deferred host operations. Validated on an NVIDIA GeForce RTX 5050 Laptop GPU.
- Android: stable-key-signed APK. Packaging supports Android 7/API 24 and later, but hardware compatibility is deliberately much narrower. Samsung raw model code `SM-S948B` / Adreno 840 on Android 16 is the current validated target; 75% RT resolution with Mobile water/glass is recommended. The exact 1.6.0 signed package passed static guards but still needs an exact-device release smoke.
- Source and issue tracking: https://github.com/Samfa12-tech/The-Horde-RT-demo

On the exact accepted `SM-S948B` Debug runtime at the recommended 75% RT scale, instrumented feature medians ranged from 53.481 ms for the chest unlock view to 91.176 ms for extreme lantern motion; the held-lantern glass views were the heaviest. These are correctness/image observations under instrumentation, not Release frame-rate promises. RT quality and resolution were not reduced to conceal the cost.

## Controls

### Windows

- `WASD`: move and strafe
- Left mouse drag: camera look
- Right mouse or `Space`: attack
- `Q`: timed parry
- `E`: interact with the reward chest/lantern
- `F`: raise or lower the reward lantern
- `Esc`: pause/resume
- Controller: left stick move, right stick look, RT attack, LT parry, B/Circle directional dodge, gameplay A interact, gameplay Y raise/lower, D-pad menu navigation, A menu select, Menu/Start pause
- `R`: restart route; `F1`: controls; `F2`: diagnostics; `Alt+Enter`: fullscreen

### Android

- Left-side drag: move and strafe
- Right-side drag: camera look
- `SWING`: attack
- `PARRY`: timed skeleton-melee parry
- `INTERACT`: open the unlocked chest and claim its lantern
- `RAISE` / `LOWER`: change the carried lantern pose
- Android Back: pause/resume

## Credits

- Environment materials: Poly Haven, CC0.
- Sound effects: FilmCow Royalty Free Sound Effects Library.
- “Water Dripping” by DRAGON-STUDIO via Pixabay, Pixabay Content License.
- “Wooden Trunk Latch 1” by floraphonic via Pixabay, Pixabay Content License.
- “Chest Opening” by spookymodem (Freesound), via Pixabay.
- “Fire extinguishing” by MUSICHOLDER via Pixabay, Pixabay Content License.
- Production sword, torch, reward chest, and reward lantern created with Meshy; runtime processing by Samfa12/Codex, conservative CC BY 4.0 attribution route.
- Original Free Stylized Dark Fantasy Skeleton: Hotstrike Studio; derivative texture, rig, and animation processing created with Meshy, CC BY 4.0.
- Placeholder lich: created and animated with Meshy, CC0.
- Application icon: created for this project with OpenAI image generation.
- Full provenance and licence details: https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/main/ASSET_LICENSES.md

## AI assistance disclosure

AI tools assisted code development. Meshy was used for the credited production props, character-processing work, and CC0 placeholder lich. OpenAI image generation created the application icon. On itch this project is classified as AI Assisted with Code and Graphics selected.
