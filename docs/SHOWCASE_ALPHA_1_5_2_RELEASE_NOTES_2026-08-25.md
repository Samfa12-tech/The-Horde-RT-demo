# Horde Lantern RT - Showcase Alpha 1.5.2

Package version: `1.5.2`
Android version code: `7`

## What changed

- Added the unlockable RT Lab on Android and Windows, with live controls for waterfall width, finale roof and dawn state, lich fog density, authored light groups, and bounded RT workload presets.
- Made the Windows RT Lab reliably scrollable by mouse wheel, keyboard, D-pad, Page Up/Down, Home, and End; sliders now repaint correctly after scrolling and reopening.
- Corrected the waterfall-width control so it changes the visible cross-lane span of the falling curtain while leaving its catchment, drain runnel, collision, and route trigger fixed.
- Replaced water-only lighting approximations with shared active-light, material, atmosphere, and real ray-traced visibility logic for refracted and reflected opaque surfaces.
- Preserved real player, roof, world, torch, moon, passage, and staff-light shadows through and across the water without screen-position masks or baked transport percentages.
- Kept reflected and refracted water shading finite: secondary opaque hits receive terminal direct lighting and visibility, while water-on-water paths do not recurse or spawn another glossy bounce.
- Corrected the Android post-finale overlay ownership so an open RT Lab remains open until the player explicitly leaves it.

## Rendering and compatibility

- Native `vkCmdTraceRaysKHR` swapchain presentation, phone-safe `rayQueryEXT` shading, pipeline recursion depth one, strict Android ASTC, one frame in flight, and the existing gameplay authority remain unchanged.
- Water remains real world geometry with bounded real-time RT optics and lighting. There is no raster fallback, SSR, baked lighting, or screen overlay.
- The accepted waterfall, rounded catchment, drain-connected runoff, lich mist, controller bindings, positional water loop, combat, and finale remain intact.
- Android remains intended for explicitly validated Vulkan hardware-RT phones; unsupported devices receive diagnostics instead of a fallback renderer. Windows requires a Vulkan hardware-ray-tracing GPU and current driver.

## Evidence boundary

- The owner accepted the corrected Windows waterfall lighting, puddle/runoff presentation, waterfall-width control, and RT Lab scrolling/repaint behavior in hands-on play.
- Fresh source, exact package, Windows RT, Android build/package, and connected-device evidence is recorded in `SHOWCASE_ALPHA_1_5_2_RELEASE_VALIDATION_2026-08-25.md`.
- `Audio/haptic manual revalidation required: NO` - this release changes RT lighting, bounded renderer tuning, UI ownership, and release identity; listener/source event data, playback, cues, feedback timing, and haptic routing are unchanged.

See `WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md`, `RT_LAB_VALIDATION_2026-08-24.md`, and `ASSET_LICENSES.md` for detailed implementation, validation, and licence records.
