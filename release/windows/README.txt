HORDE LANTERN RT - INITIAL SHOWING ALPHA 0.1.0
================================================

This is a native Vulkan hardware ray-tracing technology demo from Samfa12.
There is no raster, browser, or fake-RT fallback.

WINDOWS REQUIREMENTS
- Windows 10 or 11, 64-bit
- A Vulkan driver exposing VK_KHR_ray_tracing_pipeline and the required
  acceleration-structure / buffer-device-address features
- Tested target: NVIDIA GeForce RTX 5050 Laptop GPU

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

ALPHA CONTENT
- Torch-lit historical-gothic corridor and materials gallery
- Native Vulkan BLAS/TLAS, RT pipeline/SBT and vkCmdTraceRaysKHR presentation
- Phone-safe ray-query shading work inside raygen
- Wet stone, PBR material arrays, mirror-routing groundwork, player body,
  held torch/sword, one animated skeleton and a small combat loop
- FilmCow UI, combat, footstep and skeleton-attack sound effects
- Help > Credits & licences carries the main attribution inside the executable

KNOWN ALPHA LIMITS
- This is the corridor/material-gallery initial showing, not the completed
  60-90 second mirror/glass/final-crypt showcase.
- Only the tested RT-capable hardware paths are supported.
- See ASSET_LICENSES.md and ALPHA_RELEASE_NOTES.md.

Website: https://samfa12.com
