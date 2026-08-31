# Horde Lantern RT - Showcase Alpha 1.6.0

Package version: `1.6.0`
Android version code: `8`

## What changed

- Replaced the placeholder sword and opening torch with measured, textured PBR assets loaded through a reusable static GLB/material/instance path.
- Added deterministic world-space volumetric torch fire whose emissive geometry, visible flame, coloured direct light, reflections, shadowing, flicker, and movement share one emitter state.
- Preserved the roof-water torch failure and added the credited extinguish cue when the flame is put out.
- Added a Gothic reward chest in the lich room. It reports its locked state, unlocks two seconds after the lich dies, plays latch and opening cues, and is highlighted by a warm ray-traced guidance light.
- Added shared interaction and held-light commands on Windows and Android. The chest opens through its authored hinge and exposes a claimable reward lantern.
- Added a bounded reusable dielectric route for the lantern's closed glass, including Fresnel reflection, transmission, refraction, IOR, attenuation, thickness, and transparent shadows through finite ray-query budgets.
- Added fixed-step physical lantern swing driven by hand/pivot acceleration, movement, stopping, strafing, turning, dodge, and raise/lower transitions.
- Added a reusable skinned-character, animation-layer, IK, socket, and CPU skin/refit foundation. The accepted player-facing view remains the stable block-arm presentation while skinned gauntlet art is refined for a future update.
- Added asynchronous GitHub Release update checks with an optional update action; gameplay authority remains in the shared native simulation.
- Reworked sword presentation and attack flow so the blade edge faces forward and a second press during the downward cut chains into an upward slice.

## Rendering and compatibility

- Native `vkCmdTraceRaysKHR` swapchain presentation, phone-safe `rayQueryEXT` shading, strict Android ASTC, deterministic 60 Hz simulation, one frame in flight, and shared cross-platform gameplay authority are preserved.
- Fire, glass, props, chest interaction, lantern carry, and physical swing are reusable engine paths rather than raster overlays, screen-space effects, alpha-only glass, or platform-specific gameplay.
- Android remains intended for explicitly validated Vulkan hardware-RT phones. Windows requires a Vulkan hardware-ray-tracing GPU and current driver. Unsupported devices receive diagnostics instead of a fallback renderer.
- The Mobile lantern-glass path is physically bounded but expensive. The recommended Android RT resolution remains 75%; 100% is reported separately and is not the recommended play tier.

## Accepted alpha boundaries

- Normal gameplay uses the accepted block-arm viewmodel. Skinned first-person gauntlets and arm/body shadow/reflection polish are deferred until they are ready in every held-item scenario.
- The opening encounter remains capped at two skeletons, and the lich remains singular.
- Finite Mobile glass budgets can terminate difficult pane stacks instead of recursing or risking instability; this remains a measured optimisation/correctness target.

## Evidence boundary

- The owner accepted the final phone composition, downward/upward sword combo, fire, chest guidance, natural lich-to-chest progression, reward-lantern carry, audio, and haptics on the exact final Debug runtime.
- Fresh source, signed package, Windows RT, Android package, prior exact-Debug device, and publication evidence is recorded with their separate boundaries in `SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md`.
- `Audio/haptic manual revalidation required: NO` - the owner already accepted the final runtime and cues; the remaining release delta is version identity, signing, packaging, and publication only.

See `FIRE_PBR_REWARD_LANTERN_PLAYER_UPGRADE_VALIDATION_2026-08-30.md`, `GITHUB_RELEASE_UPDATE_FOUNDATION_2026-08-30.md`, and `ASSET_LICENSES.md` for detailed implementation, validation, and licence records.
