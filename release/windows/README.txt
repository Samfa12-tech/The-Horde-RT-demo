HORDE LANTERN RT - SHOWCASE ALPHA 0.1.1
=======================================

This is a native Vulkan hardware-ray-tracing technology demo from Samfa12.
There is no raster, browser, or fake-RT fallback.

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
- Esc: pause / resume
- R: restart route
- F1: controls
- F2: RT diagnostics
- Alt+Enter: fullscreen / windowed

SETTINGS
- Render resolution: 50-100% of the window, 100% by default
- Lower percentages reduce RT ray count and upscale to the full window
- Sound, look sensitivity, display mode, and render scale persist beside the demo
- Per-Monitor V2 DPI scaling keeps menus and overlays crisp across display scales

STARTING THE DEMO
Run HordeLanternRT.exe. Keep the assets folder beside the executable.
The reports folder is created beside the executable after launch.

If the required hardware RT path is unavailable, the demo shows diagnostics
and does not silently start a fallback renderer.

SHOWCASE CONTENT
- Skeleton encounter followed by a three-turn shadow corridor
- Authored lantern gutter, drop, and first-person body transition
- Blue skylight chamber and four bay-selected coloured torch environments
- Open framed threshold, wet stone, and a single-bounce hero mirror
- Floating staff-lit lich finale with violet charge electricity, three-hit combat,
  hit recoil/cry, death animation, and a post-defeat sliding roof aperture
- Native Vulkan BLAS/TLAS, RT pipeline/SBT and vkCmdTraceRaysKHR presentation
- Phone-safe ray-query shading work inside raygen
- Seventeen FilmCow UI, combat, movement, skeleton, and lich sound cues
- Help > Credits & licences carries the main attribution inside the executable

KNOWN ALPHA LIMITS
- One skinned enemy is rendered and animated at a time.
- The lich is a CC0 Meshy placeholder with visible source-rig limitations.
- Water, simultaneous hordes, and the staged textured sword remain deferred.
- Only tested RT-capable hardware paths are supported.
- See ASSET_LICENSES.md and ALPHA_RELEASE_NOTES.md.

Website: https://samfa12.com
