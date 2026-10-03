HORDE LANTERN RT - SHOWCASE ALPHA 1.6.1 DEVELOPMENT CANDIDATE
=======================================

This is a native Vulkan hardware-ray-tracing technology demo from Samfa12.
There is no raster, browser, or fake-RT fallback.
This `1.6.1` package/version-code-9 candidate is not published. The latest
published itch release remains the exact `1.6.0` / Android versionCode 8 line.

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
- Render resolution: 50-100% of the window, 100% by default
- Lower percentages reduce RT ray count and upscale to the full window
- Independent Music and SFX volume, look sensitivity, display mode, and render scale persist beside the demo
- Per-Monitor V2 DPI scaling keeps menus and overlays crisp across display scales

STARTING THE DEMO
Run HordeLanternRT.exe. Keep the assets folder beside the executable.
The reports folder is created beside the executable after launch.

If the required hardware RT path is unavailable, the demo shows diagnostics
and does not silently start a fallback renderer.

SHOWCASE CONTENT
- Two-skeleton opening encounter followed by a three-turn shadow corridor
- Animation-owned sword contact, timed parry, attacker stagger, and riposte window
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
- Mobile deliberately omits lantern pane geometry rather than faking transparency
- Two-second post-lich latch cue and chest guidance light before interaction
- Automatic GitHub Release availability checks with an optional update action
- Native Vulkan BLAS/TLAS, RT pipeline/SBT and vkCmdTraceRaysKHR presentation
- Phone-safe ray-query shading work inside raygen
- FilmCow UI, combat, movement, skeleton, and lich sound cues, plus credited
  Pixabay waterfall, torch-extinguish, chest-unlock, and chest-open effects
- Adaptive A-H What the Dark Keeps music with owner-accepted whistle-lead instrumentation
- Consent-based in-game reporting with optional bounded diagnostics and game-only screenshot
- Help > Credits & licences carries the main attribution inside the executable

KNOWN ALPHA LIMITS
- The opening encounter is capped at two skeletons and one attacker at a time.
- The lich is a CC0 Meshy placeholder with visible source-rig limitations.
- Larger hordes remain deferred.
- Remaining High physical-glass contact/near-edge defects are deferred future investigation.
- Android defaults to 75% RT resolution; sustained 30 FPS is not achieved in the measured
  current phone workloads. Performance is accepted as-is for 1.6.1, not guaranteed.
- S24 is working but not fully tested; exact S25 remains unverified.
- Full graphics-options menu is planned for 1.6.2.
- Only tested RT-capable hardware paths are supported.
- See ASSET_LICENSES.md and ALPHA_RELEASE_NOTES.md.

Website: https://samfa12.com
