# Showcase Route Blockout Validation - 2026-07-16

> Historical Slice A record. The coloured-light route, lantern failure, mirror, complete low-poly player, lich finale, and moving roof were subsequently completed, Android-validated, and published in Showcase Alpha 0.1.1. See `DOCUMENTATION_CHECKPOINT_2026-07-17.md`.

## Result

Slice A is implemented as a geometry-first extension of the existing native Vulkan RT scene. It preserves the opening room, material gallery, animated skeleton encounter, controls, deterministic reset, one-frame-in-flight ownership, ASTC routing, BGRA correction, `vkCmdTraceRaysKHR`, and honest `rtScene.presented` reporting.

No coloured illumination, lantern failure, lower body, active stained glass, mirror reflection, water, lich, or new shader behavior was included in this geometry-only milestone.

## Implemented route

- Replaced the sealed wall at `z=-6.4` with a framed 1.8 m doorway and retained solid wall returns outside it.
- Added the specified four overlapping walkable legs and three turns, with coarse masonry, arches, returns, and bars for future moving-shadow compositions.
- Added the skylight chamber with a physical roof aperture at `x=-6.7..-4.3`, `z=-16.6..-13.8` and a 1.1 m raised masonry shaft for readable roof depth.
- Added four five-metre torch bays with unlit aged-metal brackets only.
- Added an empty framed transmission opening and a dry, empty finale with an architectural hero-mirror surround.
- Kept the existing skeleton as the only active enemy; the finale contains no encounter.
- Leashed the skeleton to the original room and applied shared-obstacle collision to its swept movement. It idles when the player enters the extension and cannot continue walking through the gallery, arch posts, or old far wall.

`src/gameplay/ShowcaseRoute.h` is the shared Android/Windows contract for walkable rectangles, solid obstacles, spawn/room coordinates, `ShowcaseZone`, and the pure position-to-zone query. Collision now receives previous and proposed positions, performs swept validation against the radius-inset union, attempts X-only and Z-only wall sliding, and otherwise retains the previous position.

## Automated validation

| Check | Result |
| --- | --- |
| Windows Debug configure/build | Passed |
| Windows Debug CTest | 2/2 passed (`HordeLanternCombatSmoke`, `ShowcaseRouteSmoke`), including enemy collision/leash coverage |
| Windows Release configure/build | Passed |
| Windows Release CTest | 2/2 passed |
| Android `assembleDebug installDebug` | Passed for all configured ABIs and installed on `SM-S948B` |
| Route traversal | Passed through opening, skeleton room, all corridor legs, skylight chamber, four torch bays, threshold, and finale |
| Collision cases | Passed corners, wall sliding, diagonal-shortcut rejection, old-wall rejection, gallery table, and arch posts |

The route smoke moves in 0.08 m increments through every authored waypoint and uses the same header-only collision code compiled into both platform paths.

## Device evidence

### Android - `SM-S948B`

- Strict texture route reported: `ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)`.
- Logcat reported `RT frame reached Android swapchain presentation.`
- The opening, active skeleton, framed route entrance, first bend, and long unlit-bracket passage were visually sampled with warm fire presentation and no red/blue inversion.
- The patched skeleton leash was confirmed by leaving it visible inside the original-room doorway after the player entered the extension. The raised skylight shaft depth and empty finale mirror surround were also confirmed in the live phone pass; a coplanar lip overlap found during that pass was removed before the final rebuild.
- Mixed 100% interactive 120-frame averages observed during the pass ranged from roughly 12.1 ms to 19.1 ms. This is a presentation and extent check, not a controlled per-zone benchmark.

Evidence images:

- `validation/showcase-route-2026-07-16/android-start.png`
- `validation/showcase-route-2026-07-16/android-leg1.png`
- `validation/showcase-route-2026-07-16/android-torch-passage.png`
- `validation/showcase-route-2026-07-16/android-threshold.png`

### Windows - RTX 5050 Laptop GPU

- The Debug executable reached native ray-tracing-pipeline swapchain presentation and rendered the warm route correctly.
- The laptop is internally limited to 165 FPS. Desktop FPS is therefore treated as cap-bound presentation/stability evidence and not as a renderer performance ceiling.

## Shader integrity

`minimal.rgen` was not edited and SPIR-V was not regenerated.

- `shaders/raytracing/minimal.rgen` SHA-256: `7BB578C4BC7898B7421E224EA3387EC1923FD6442E851C6FD58E26BD7D9E911A`
- `src/vulkan/raytracing/MinimalRayGenShader.inc` SHA-256: `244DD48D4984AE5DBEFD653841B332128821F7A30E2228985575E3F97C48A3E5`

## Remaining hands-on gate

The code/build/device-presentation gate is complete. Before treating Slice A as performance-certified, perform the planned uninterrupted manual phone pass at 75% render scale: wall-hammer every segment, exercise reset and pause/resume, and record three consecutive 120-frame windows in the existing chamber, worst bend, skylight chamber, far torch passage, and finale. Each zone still needs a controlled median at or below 20 ms. Report 100% separately without a 50 FPS requirement.

No signed release was created and this greybox was not uploaded.

## Recorded follow-up

Enemy and player sound currently needs a dedicated spatial-audio pass: directional positioning, distance rolloff, and sensible attenuation/occlusion through walls. That work is intentionally not being disguised as part of this geometry-only slice.
