# Player body RT slice - 2026-07-14

## Outcome

The first-person player now has real ray-traced torso and articulated arm geometry. It is not an overlay: the torso and four arm segments are TLAS instances, are visible to lower-view primary rays, can block direct lighting in the lower receiving region, and participate in reflective and puddle bounce queries.

The current refinement replaces the original rigid five-box blockout. A torso BLAS remains body/yaw-relative, while one reusable unit-limb BLAS supplies left/right upper- and lower-arm instances. Two-bone IK connects each shoulder to an exact prop grip target. The torch hand meets the handle, the sword hand remains on the sword grip throughout the swing, and both arms and held props follow camera pitch as a single first-person viewmodel.

This is still a low-poly presence, attachment, and lighting proof rather than final character art. Hands, legs, a skinned character asset, and authored first-person animation remain deferred.

## Renderer shape

- The TLAS contains nine instances: world, torch, enemy, sword, torso, and four arm segments.
- World and enemy geometry retain ray mask `0x01`; held torch and sword retain `0x02`; torso and arms use `0x04`.
- The torso follows camera position and yaw only, keeping the ray origin outside its faces. The limbs, torch, and sword use a camera-relative right/up/forward basis that includes clamped pitch.
- The left hand target includes the existing walk sway/bob. Torch translation is derived from the handle's local grip point, so the visible handle stays attached to that hand.
- The right hand follows a smoothed attack arc. Sword translation is solved from the sword's local grip point after rotation, so the grip cannot drift away from the hand during a swing.
- A two-bone geometric solve places each elbow; four transformed instances reuse one static limb BLAS, avoiding per-frame limb BLAS refits.
- The raygen torch-light estimate uses the same pitch-aware grip and flame placement as the torch TLAS instance.
- Primary rays include mask `0x04` only where the first-person body can be visible. Reflective/puddle rays include it, while direct-light queries include it in the lower receiving region.
- A compact dark leather/brown material branch handles `instanceCustomIndex = 4`. A more expensive per-face-normal experiment was rejected because its 38,936-byte raygen SPIR-V approached the previously observed Adreno occupancy cliff.

The implementation intentionally keeps one frame in flight because the TLAS instance buffer is host-written each frame.

## First-person presentation rationale

The refinement follows the established first-person pattern used by major engines: camera-relative arms and weapons for stable first-person composition, explicit weapon attachment, and a world-space representation that can still contribute to shadows and reflections. Relevant references:

- Epic, [First Person Rendering](https://dev.epicgames.com/documentation/en-us/unreal-engine/first-person-rendering)
- Epic, [Shooter Game](https://dev.epicgames.com/documentation/es-mx/unreal-engine/shooter-game?application_version=4.27)
- Epic, [Adding a First-Person Camera, Mesh and Animation](https://dev.epicgames.com/documentation/unreal-engine/coder-04-adding-a-firstperson-camera-mesh-and-animation?lang=en-US)
- CryEngine, [First Person Weapons](https://www.cryengine.com/docs/static/engines/cryengine-3/categories/1114113/pages/1310920)

These sources support the general camera-relative mesh/attachment approach. They do not establish the exact internal implementation of Skyrim, so this project does not depend on an unverified Skyrim-specific claim.

## Mobile performance decisions

Earlier detailed 7-12 box versions, a dedicated large material branch, and full-screen body traversal produced 34-48 FPS phone samples. The verified pre-articulation blockout kept selective mask `0x04` traversal and passed the phone gate.

Current articulated embedded raygen SPIR-V:

- Size: 37,080 bytes
- SHA-256: `8A97F4685336A6906EDE307CE069194C6FBA88CCEED13D9583F9E0D918C175BA`

This remains below the observed approximately 38.3 KiB mobile performance cliff.

## Validation

Target phone: Samsung `SM-S948B`.

### Verified pre-articulation baseline

- All four Android debug ABIs built successfully and the fresh install selected the strict ASTC material path.
- Runtime logged `RT frame reached Android swapchain presentation.`
- A clean 126-interval SurfaceFlinger sample measured 12.500 ms median, 16.667 ms p95, and approximately 80 FPS median. Thermal status moved from 0 to 1, so the 50+ FPS median gate passed.
- A later warm 126-interval sample measured 16.667 ms median and 25.000 ms p95 with thermal status 1. The user observed an approximately one-hour overall app session; because the process had been restarted during development, the independently continuous interval available to measure was approximately ten minutes.
- The retained idle, downward-look, and swing captures below are evidence for this earlier rigid blockout, not the current articulated arm placement.

Evidence:

- `docs/validation/horde_player_body_idle_phone_2026-07-14.png`
- `docs/validation/horde_player_body_lookdown_phone_2026-07-14.png`
- `docs/validation/horde_player_body_swing_phone_2026-07-14.png`

### Current articulated revision

- All four Android debug ABIs build successfully. The exact APK was installed on `SM-S948B` and selected `ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)`.
- Windows Debug builds successfully, `horde_rt_combat_smoke.exe` passes, and the RTX diagnostic remains alive through an eight-second run.
- Runtime logged `RT frame reached Android swapchain presentation.` for the articulated build.
- Live idle and peak-swing captures show the torch forearm terminating at the torch handle and the sword forearm travelling with the hilt. Both prop transforms are solved from their exact local grip coordinates, while the shared camera basis keeps arms, props, and the torch-light estimate pitch-relative.
- After sustained development use at thermal status 2 and battery temperature 44.4 C, SurfaceFlinger TimeStats covered 23,446 presented frames at 52.352 average FPS. Its present-to-present histogram has a 20 ms median bucket and 25 ms p95 bucket.
- Thirty consecutive internal 120-frame telemetry windows measured 19.718 ms median, 20.502 ms p95, and 19.715 ms mean, or approximately 50.7 FPS at the median. This passes the 50+ FPS warm median gate, but leaves little budget for broader RT effects.
- The legacy SurfaceFlinger `--latency` history returned only the refresh period after the layer/orientation recreation, so the sustained TimeStats histogram and renderer telemetry are retained instead of inventing a 126-interval result.
- Desktop capture did not reliably show the Vulkan swapchain, so it is not treated as visual evidence.

Evidence:

- `docs/validation/horde_player_articulated_idle_final_phone_2026-07-14.png`
- `docs/validation/horde_player_articulated_swing_peak_phone_2026-07-14.png`

## Deferred work

- Replace the box limbs with authored hands and a proportioned first-person rig only after the RT showcase route is stable.
- Add authored idle, walk, and attack animation without breaking exact grip constraints.
- Give body faces better normals and richer cloth/leather materials without crossing the mobile shader occupancy limit.
- Add lower body only when it materially improves downward-view and reflection readability.
