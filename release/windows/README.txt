HORDE LANTERN RT - SHOWCASE ALPHA 1.6.2
=======================================

This is a native Vulkan hardware-ray-tracing technology demo from Samfa12.
There is no raster, browser, or fake-RT fallback.
Package version: 1.6.2; Android companion versionCode: 10.
Android and Windows tomb update. Hardware ray tracing is required.
Canonical downloads and current availability: https://samfa12.itch.io/the-horde

WINDOWS REQUIREMENTS
- Windows 10 or 11, 64-bit
- A Vulkan driver exposing VK_KHR_acceleration_structure,
  VK_KHR_ray_tracing_pipeline, VK_KHR_ray_query,
  VK_KHR_buffer_device_address, VK_KHR_deferred_host_operations,
  and the required feature structs
- Validated target: NVIDIA GeForce RTX 5050 Laptop GPU

CONTROLS
- WASD: move and strafe
- Left mouse drag: 360 camera look
- Right mouse or Space: swing sword
- Q: parry skeleton melee attacks
- E: interact with the reward chest or lantern
- F: raise or lower the claimed reward lantern
- Backbone / compatible controller: left stick move, right stick look,
  RT attack, LT parry, B / Circle dodge, A interact, Y raise/lower,
  D-pad menus, A select,
  Menu / Start pause
- Esc: pause / resume
- R: restart route
- F1: controls
- F2: RT diagnostics
- Alt+Enter: fullscreen / windowed

SETTINGS
- Render resolution: 50-100%, with experimental 33% and 40%; desktop default 100%
- Lower percentages reduce RT ray count and upscale to the full window
- Independent Music and SFX volume, look sensitivity, display mode, and render scale persist beside the demo
- Per-Monitor V2 DPI scaling keeps menus and overlays crisp across display scales

STARTING THE DEMO
Run HordeLanternRT.exe. Keep the assets folder beside the executable.
The reports folder is created beside the executable after launch.

If the required hardware RT path is unavailable, the demo shows diagnostics
and does not silently start a fallback renderer.

SHOWCASE CONTENT
- Two distinct guards in the waterfall room and a three-turn shadow corridor
- Shared 60 Hz combat, timed parry, attacker stagger and immediate riposte; hit detection remains forgiving
- Textured PBR sword and hand torch with shared held-item sockets
- World-space volumetric torch fire with movement-reactive coloured RT light
- Modelled native-RT sleeves, hands and gauntlets with shared gameplay animation/IK/grips
- Separate world-body geometry for appropriate shadows/reflections and normal look-down presence
- Three-point vitality, encounter retry, and route restart flow
- Blue skylight chamber and four bay-selected coloured torch environments
- Open framed threshold, wet stone, and a single-bounce hero mirror
- Ray-traced transparent roof-water streams, rounded catchment and drain runnel
- Low, depth-clipped ritual ground mist in the lich room
- Positional looping waterfall ambience
- Floating staff-lit lich finale with violet charge electricity, three-hit combat,
  hit recoil/cry, death animation, and an illuminated Gothic reward chest
- Locked/open/claim interaction prompts, authored chest opening, and a reward
  lantern with acceleration-driven swing; High retains physical ray-traced glass
- Glass Off omits lantern pane geometry; saved Glass On retains real RT glass
- Two-second post-lich latch cue and chest guidance light before interaction
- Automatic GitHub Release availability checks with an optional update action
- Native Vulkan BLAS/TLAS, RT pipeline/SBT and vkCmdTraceRaysKHR presentation
- Shared RT shading on genuine Pipeline and hardware RayQueryCompute backends
- FilmCow UI, combat, movement, skeleton, and lich sound cues, plus credited
  Pixabay waterfall, torch-extinguish, chest-unlock, and chest-open effects
- Adaptive A-H What the Dark Keeps music with owner-accepted whistle-lead instrumentation
- Consent-based in-game reporting with optional bounded diagnostics and game-only screenshot
- Help > Credits & licences carries the main attribution inside the executable

KNOWN ALPHA LIMITS
- The waterfall encounter is capped at two skeletons and one attacker at a time.
- The lich is a CC0 Meshy placeholder with visible source-rig limitations.
- Larger hordes remain deferred.
- Remaining High physical-glass contact/near-edge defects are deferred future investigation.
- Android fresh/reset defaults to 50% RT resolution; sustained 30 FPS is not achieved in the measured
  current phone workloads. No sustained 30 FPS or causal performance saving is guaranteed.
- Current exact-device evidence is scoped to SM-S948B; S24 is deferred and S25 unverified.
- Graphics options provide an RT preview, resolution/effect choices, and explicit save/restore.
- Dust Low is the fresh/reset default; saved Off/custom settings are preserved.
- Sword damage uses forgiving range/cone resolution; visible contact can differ.
- Minor torch-arm motion, shafts and seamless music handover remain deferred.
- Only tested RT-capable hardware paths are supported.
- See ASSET_LICENSES.md, LICENSE, LICENSE_SCOPE.md and THIRD_PARTY_NOTICES.
- The software grant does not relicense assets, Pocket Audio Core or the mixed package.

Website: https://samfa12.com
