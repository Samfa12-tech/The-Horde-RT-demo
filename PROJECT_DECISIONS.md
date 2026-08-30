# Project Decisions - The Horde RT Demo / Horde Lantern RT

This file records locked decisions for the native Vulkan hardware ray-tracing demo.

## Identity

- Public project: Samfa12 technology demo.
- Repository: `Samfa12-tech/The-Horde-RT-demo`.
- Working creative title: **Horde Lantern RT**.
- Distribution: itch hosts the canonical Android and Windows artifacts at `https://samfa12.itch.io/the-horde`; Samfa12.com links to that page instead of hosting a second copy. The live `/games/` card was rendered and link-verified on 2026-07-15.

## Core decision

> RT or nothing.

The demo must prove real native Vulkan hardware ray tracing. Browser rendering, Godot renderer work, Unreal-first work, Three.js, Babylon.js, WebGPU, raster-only rendering, SSR, baked lighting, compute-only path tracing, and fake RT are not acceptable substitutes for the core proof.

## Build / test / demo cycle decision - 2026-07-17

The supporting demo foundation is complete and device-validated: Android Debug has deterministic route checkpoints, three-window measurement, native route replay, fixed screenshots, state evidence, the live developer overlay, and a bounded one-command runner. The integrated cross-platform clean-build/package/stale-shader/licence gate and deterministic capture foundation are also complete. Fixed video/orbit presentation remains separately deferred. Detailed historical scope and guardrails live in `docs/BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md`.

The normal player-facing route remains intact; development checkpoints and overlays must stay tucked away from branded entry/pause/settings surfaces. Game-facing combat polish follows the tooling foundation. The later measured two-skeleton gate superseded this section's original one-active limit; the current ceiling is two skeletons/two pose buckets with a later singular lich, and any further increase requires a separate phone measurement.

Windows Debug uses F3 for the compact live overlay while F2 retains the full paused RT diagnostics surface. Release builds omit the F3 control, help text, menu command, and live overlay UI. Android Release keeps the native developer-overlay request empty; only debuggable builds may show the hidden view.

## Full-game foundation decision - 2026-08-01

Showcase Alpha 0.1.3 is the preserved demo baseline. Full-game work begins with a shared fixed-step simulation, input mailbox, bounded gameplay-event queue, persistent enemy IDs/state, and platform-parity tests. Do not expand the current duplicated Windows/Android render-loop orchestration directly.

The first multi-enemy slice is two skeleton instances with a dedicated `SM-S948B` performance gate. Keep the published one-active-skinned-enemy default until that gate passes; four enemies require a later measured step. Use flat fixed-capacity data and stable IDs rather than a general ECS.

Combat animations use game-owned action state and explicit animation events for damage windows. Fire uses bounded emitter data plus emissive RT geometry and raygen effects. Shallow water uses real RT-visible geometry with bounded ray-query reflection/transmission; steam uses bounded depth-aware raygen volumes. Preserve `vkCmdTraceRaysKHR`, phone-safe `rayQueryEXT`, one frame in flight, honest presentation, strict ASTC, and BGRA copy compensation throughout. The audit and acceptance plan are `docs/FULL_REPO_AUDIT_AND_GAME_PLAN_2026-08-01.md`.

## Shared simulation foundation decision - 2026-08-11

Milestone 1 centralises Windows and Android gameplay in one fixed 60 Hz `GameSimulation` on the existing owning thread. Platform input publishes `InputSnapshot`; Android uses a coherent two-slot mailbox plus monotonic edge counters. Rendering consumes immutable `SimulationSnapshot` through one `RtSceneFrameInputs` adapter, while ordered bounded `GameplayEvent` records drive platform audio and haptics.

The runner clamps render contribution to 100 ms, supports up to eight catch-up ticks, reports discarded excess, normalises two-axis movement, and clears accumulated time across pause/reset/retry/checkpoint transitions. Exact authored checkpoint imports remain a separate no-tick path because capture state must preserve the 0.1.3 floating-point and zero-delta finalization behavior.

At this historical foundation milestone, the renderer, shader ABI, `vkCmdTraceRaysKHR`, recursion depth 1, `rayQueryEXT`, one frame in flight, ASTC route, Android identity, one-active-enemy limit, and public 0.1.3 artifacts were unchanged. Later bounded two-skeleton development supersedes only the development enemy capacity; it does not alter published 0.1.3 artifacts.

## Renderer resource and GPU timing decision - 2026-08-11

Milestone 2 extracts checked Vulkan buffer operations, acceleration-structure lifetime helpers, and updatable-BLAS state behind `RtGpuResources`; BLAS sizing/build/refit recording remains in the scene. At that milestone it made the skeleton-or-lich renderer path an explicit `CharacterRenderSlot`, with one selected address written to TLAS instance 2. This historical ownership/test seam was later extended by the explicitly bounded two-skeleton route; it is not permission for a third enemy or shader-ABI redesign.

GPU telemetry uses Vulkan timestamp queries only when the selected graphics queue reports non-zero timestamp bits. It resolves the prior submission after the existing frame fence, never waits inside the query read, and reports a separate **GPU RT command-buffer time**. CPU wall-clock frame timing, benchmark schema and pass/fail remain authoritative for continuity; timestamps do not include queue presentation, compositor, display, or input latency. Unsupported or failed timing remains non-fatal and visibly `N/A`.

## Measured two-skeleton combat decision - 2026-08-11

The first multi-enemy candidate is a fixed-capacity pair with stable `SkeletonA`/`SkeletonB` IDs, independent health/action/death persistence, deterministic wall collision and 0.70 m separation, nearest-only sword damage, and one attack token selected by strict distance then entity ID. Retry and route reset restore the pair; both deaths complete the opening encounter. Historical checkpoints 0-11 continue importing the original singular skeleton so the published 0.1.3 capture hashes remain exact, while checkpoint 12 is the explicit two-enemy proof.

The renderer exposes two skeleton instances and no more than two measured pose buckets. Matching actions share one pose/BLAS; divergent attack/death states may use the second. The lich remains singular. The physical totals are nine BLAS and nineteen TLAS slots; semantic custom indices 0-17 remain stable and the second pose route uses bounded custom index 18. Every masked slot retains an invertible identity transform.

This candidate does not change `vkCmdTraceRaysKHR`, `rayQueryEXT`, recursion depth one, one frame in flight, strict ASTC, swapchain presentation semantics, release identity, or published artifacts. Host validation and explicit capture review are necessary but not sufficient: promotion requires matched cooled `SM-S948B` timing-enabled/disabled evidence, the six-checkpoint 75% gate strictly below 20.000 ms, separate 100% reporting, lifecycle checks, and hands-on combat/readability/audio/haptic checks. A 75% median from 18.500 ms inclusive to below 20.000 ms is a non-failing low-headroom warning; 20.000 ms or above fails. More than two enemies remains out of scope.

The automated `SM-S948B` requirements passed on 2026-08-12 for exact clean commit `b3428a7`: matched cooled lich A/B was 19.497 ms enabled / 19.268 ms disabled, all six 75% checkpoints stayed below 20 ms, 100% opening was reported separately, and replay, 13 captures, and Home/resume passed. The owner then reported that hands-on play on the still-installed exact candidate “feels fine,” closing the broad two-enemy promotion boundary without independently certifying every positional-audio or haptic cue.

## Animation-owned combat and timed-parry decision - 2026-08-13

Player swing, parry, skeleton strike, and stagger timing belong to the shared fixed-step gameplay authority. Player swing uses wind-up/active/recovery boundaries `0-0.18 / 0.18-0.34 / 0.34-0.56 s`; skeleton melee uses `0-1.12 / 1.12-1.30 / 1.30-2.80 s`. Damage resolves once at visible active contact. The lich consumes the same player active-window range/cone contact pulse while retaining three health, its two-second accepted-hit lockout, recoil, cries, death, and finale.

Parry is a separate monotonic edge command and a skeleton-only timed action: 40 ms startup, 220 ms active, and 240 ms failed recovery. Android publishes that edge on `PARRY` touch-down, while retaining click activation for accessibility without republishing on release; Windows continues to use non-repeating `Q` key-down. A real in-range frontal skeleton contact during the active phase emits one entity-aware success event, suppresses damage, and holds that token holder stationary for 800 ms. Successful recovery ends on the next fixed tick so a normal unassisted riposte may begin immediately. Unavailable inputs are consumed without buffering; parry never affects the lich.

Rendering maps these states onto existing sword/arm transforms and the existing skeleton Attack clip. A staggered skeleton is stationary for gameplay but visually advances procedurally from Attack contact toward recovery with bounded lean/recoil; it is not a frozen pose. `CharacterRenderSlot` has one cached frame plan shared by skin/refit and TLAS routing. The fixed nine-BLAS, nineteen-slot TLAS, two-pose-bucket, recursion-one, strict-ASTC, one-frame-in-flight, and honest `vkCmdTraceRaysKHR` presentation contracts remain unchanged. Alpha 0.1.3 artifacts remain immutable and this milestone is not published automatically.

## Audio/haptic manual validation decision - 2026-08-21

Manual owner audio/haptic validation is **change-triggered, not milestone-triggered**. Every milestone must state `Audio/haptic manual revalidation required: YES/NO` and a reason; the default is **NO**. A check is required only when a change can materially alter spatial-audio mathematics, listener state at event time, source identity/coordinates, attenuation/obstruction/pan, playback backend/gain/cues/assets, event transport/timing, haptic routing/cues/patterns/intensity, or player damage/death feedback semantics. Unrelated RT, visuals, UI, build, packaging, documentation, telemetry, unrelated AI, and unrelated animation changes do not trigger one when automated contracts pass and the semantic inputs are unchanged.

Current reconciliation classification: **YES** — listener-at-event-time routing and platform feedback timing changed, so the final exact Android candidate required an owner check. The owner subsequently gave the exact candidate a broad “all good” audio/haptic verdict and explicitly observed its stagger-back/death sequence. The accepted baseline therefore returns to change-triggered testing. This decision is separate from owner-only signing recovery work in `docs/OWNER_RELEASE_SAFETY_CHECKLIST.md`.

## Sustained phone-performance evidence decision - 2026-08-21

The project owner confirmed that the former strict-below-20.000 ms rule was not a product requirement. It was an engineering guardrail selected during development. The promotion wording in the dated 2026-08-11 two-skeleton decision above remains an accurate record of the policy used for that candidate, but it is superseded for current and future development.

Current phone reporting uses 16.667, 20.000, and 33.333 ms as descriptive approximately 60/50/30 FPS reference lines. Crossing 20.000 ms does not by itself fail a candidate. Standard evidence should preserve one-process sustained route order, temperatures, Android thermal status, GPU thermal power level when available, frame-time windows, honest RT presentation, and workload identity. Cooled or fresh-process samples are diagnostic controls and must not replace ordinary sustained behavior.

Investigate exact matched regressions above 15%, unexplained frame-time spikes, growing heap/PSS/graphics allocation/thread/resource counts, changed workloads, crashes, invalid renderer state, or dishonest presentation. Do not spend repeated runs cooling the phone to manufacture a sub-20 result, and do not weaken RT, silently lower resolution, or conceal thermal/governor behavior. The current two-skeleton limit remains the validated development boundary rather than a permanent game-design ceiling; larger encounters, fire, water, improved lanterns, and a proper player model require separately measured scaling work.

## Showcase Alpha 0.1.4 publication decision - 2026-08-22

The validated two-skeleton, animation-owned combat, timed-parry, stagger, and event-time feedback runtime is published as `0.1.4-alpha.1` on the existing itch Windows and Android channels. Android retains the established application ID, release certificate, and update identity at `versionCode 5`; Windows and Android build IDs are `#1903586` and `#1903587`. Release identity/packaging changes do not invalidate the exact `547d89d` runtime/device evidence, but the public signed APK itself remains a static/package-verified artifact until it is installed on the phone. The 0.1.1 through 0.1.4 lines are immutable and future Android updates require a version code greater than 5.

## Showcase Alpha 0.1.5 publication decision - 2026-08-23

Publish the accepted bounded water/mist/controller slice as `0.1.5-alpha.1` on the existing itch channels: Windows build `#1908330`, Android build `#1908331`, Android `versionCode 6`. Water remains real RT world geometry and bounded ray-query optics rather than a fluid simulation, raster fallback, SSR, or overlay. The lich volume remains ground-clipped and sightline-preserving. Directional dodge stays shared-gameplay state, while the exact owner-tested Backbone identity uses the measured WinMM controls and retains mouse/keyboard support.

The exact Windows ZIP and established-certificate Android APK are the published artifacts. The signed APK is installed and byte-matched on raw model code `SM-S948B`; strict ASTC and honest RT presentation pass before and after Home/resume. Because the feature slice added a positional loop and changed listener movement, manual audio validation was required. The owner accepted the exact Windows candidate, then approved waterfall audio and confirmed good haptics plus working pause/resume on the exact installed Android release. The change-triggered feedback gate is therefore closed.

The 0.1.1 through 0.1.5 lines are immutable and future Android updates require `versionCode > 6`. A shared `tools/release-version-policy.ps1` now owns this rule for unsigned packaging, signed packaging, and Butler upload so the three entrypoints cannot drift independently.

## General RT water-lighting decision - 2026-08-24

Water must consume the same active lights, material response, world/player visibility, and atmosphere as ordinary opaque geometry. Refracted and High-quality reflected opaque hits use the terminal direct-light path with no further bounce: the water interface already owns the bounded reflection, so evaluating the submerged receiver as another glossy primary double-counts reflection and fills real shadows. Interface highlights sample the same active local and sky lights with real visibility. Water-on-water secondary hits terminate without recursion.

Do not restore fixed water-only torch/moon transport percentages, waterfall-coordinate light targets, or screen-position-dependent shadow-caster masks. Transparent filtering must force opaque BLAS triangles through candidate handling, secondary path distance must include the primary segment, and the directional moon must be occluded by real roof/player geometry. Those rules make the result portable across camera, light, water, and material placement. The bounded ray budget is an explicit engine-quality choice and must be measured on Android before release; never hide it by lowering RT quality or resolution. Preserve `vkCmdTraceRaysKHR`, `rayQueryEXT`, pipeline recursion depth one, High/Mobile/Off identities, strict ASTC, TLAS count, and gameplay authority. See `docs/WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md`.

The exact `SM-S948B` measurement completed on 2026-08-25. Two same-APK runs repeated approximately 27.8 ms at `lantern-drop` and 19.0 ms at `skylight`; the former is about 51% slower than the rejected fixed-transport candidate but remains in the descriptive 30-50 FPS band. The stable regression is accepted as the bounded cost of real transmitted-receiver and water-interface light visibility. Do not replace it with fixed transport, remove the required visibility, or lower RT quality/render scale merely to recover the older number.

## Post-finale RT Lab overlay ownership decision - 2026-08-25

An open RT Lab owns the menu scrim until the player explicitly selects Back. Android's recurring completed-finale poll must not call `showEndingOverlay()` while `rtLabVisible` is true; otherwise it removes the newly created lab views and immediately rebuilds the ending card. Windows applies the same ownership rule in `ShowEndingMenu()` so its per-frame finale poll cannot mutate overlay state behind an open lab. Closing the lab clears `rtLabVisible` first and may then deliberately restore the ending overlay. Keep this as an explicit host contract because Debug checkpoint automation does not exercise a genuine persisted finale unlock. The source fixes are Android-built and Windows-lab checked, but Android still requires an exact fixed-APK phone retest before device acceptance.

## RT Lab waterfall-width and Windows control ownership decision - 2026-08-25

`WATERFALL WIDTH` means the visible span across the terminal lane. The player approaches along world X, so the control scales world Z on dedicated waterfall TLAS instance 19; it must not scale the millimetre-thin world-X transmission depth. Raygen stream centres and Z radii use the same clamped scale so intersection geometry, normals, reflection, and refraction remain physically aligned. The catchment, runnel, drain, collision, and roof slot stay fixed.

Windows owns vertical RT Lab scrolling at the top-level window. Focused child controls forward wheel input there, Page Up/Down and Home/End remain available, and the window carries a real vertical-scroll style. Trackbars are opaque native controls and are redrawn when returning from a hidden scrolled state; transparent sibling painting over the layered panel is prohibited because it copies stale Vulkan/label pixels and loses the track/thumb.

## Showcase Alpha 1.5.2 publication decision - 2026-08-25

Publish the reviewed RT Lab and general water-light correction as literal package version `1.5.2`, Android `versionCode 7`, on the existing itch channels. Source `f4891c4` is the release provenance. Windows build `#1913191` and Android build `#1913192` are active at user version `1.5.2`; their exact SHA-256 values are recorded in the release validation.

The Windows package passed a fresh isolated launch with `RayTracingPipeline`, honest swapchain presentation, and exit code 0. The Android package passed the established certificate and static/package compatibility guards, but ADB exposed no device at publication; do not infer an exact signed-device, pullback, lifecycle, performance, or owner-feel pass. The 0.1.1 through 0.1.5 and 1.5.2 lines are immutable, and the next Android update requires `versionCode > 7`.

## Fire/PBR/reward-lantern development decision - 2026-08-30

New production props use the fixed-capacity static GLB/PBR asset, instance-metadata, material-metadata, socket, and immutable-BLAS routes. Fire is bounded emitter data plus RT-visible emissive geometry and a deterministic world-space raygen volume; its visible flame, coloured direct light, reflection energy, and flicker share one emitter transform and state. Glass uses the generic bounded dielectric path with exact entry/exit interfaces, Fresnel, IOR, roughness, Beer-Lambert attenuation, transparent shadow transmittance, and finite Mobile/High `rayQueryEXT` budgets. Do not reintroduce primitive-range object branches, alpha-only lantern glass, a screen-space flame, or independent light motion.

Chest, reward, interaction, held-light pose, sword combo, and lantern secondary motion remain authoritative in the shared fixed-step simulation. Platform layers publish monotonic commands and consume ordered events. The physical lantern solver responds to actual pivot acceleration, gravity, damping, torsion, and bounded stops; renderer interpolation must not invent authority. Windows/Android updater UI may query public GitHub Releases and open a release page, but may not mutate gameplay state or install a package silently.

Normal gameplay uses the stable block-arm presentation for sword, torch, and reward lantern until a later skinned-hand/gauntlet asset and animation pass is accepted in every scenario. `player-body-*` checkpoints retain the reusable skinned player/IK/socket foundation for development only. Arm/body appearance in shadows and reflections is owner-deferred to the next update. This is an explicit quality decision, not permission to remove the reusable character foundation.

The retained generic raygen strategy uses functions and three source ray-query sites; the legacy comparison remains fully inlined. The physical world-bounds transparent-shadow fast path is retained because it improves the measured glass route without changing transport. Nonphysical scalar shadowing and unsuccessful compact-ABI/sphere experiments were reverted. The exact `SM-S948B` sustained warm `lantern-held-high` Debug sample remains 99.145 ms at 75%; do not hide this by lowering scale or disabling glass/fire. The owner reported that phone play feels good and accepted the preceding candidate's sound/haptics. The final candidate deliberately delays the unlock event by two seconds and therefore requires one new owner-listening check. See `docs/FIRE_PBR_REWARD_LANTERN_PLAYER_UPGRADE_VALIDATION_2026-08-30.md`.

Lich death now begins an exact two-second shared-simulation seal-breaking phase. At its boundary, one ordered `ChestUnlocked` event plays the latch cue and a real warm world-space RT guidance light above the collision-bearing chest turns on. The later skylight/dawn reveal remains pickup-gated. Held-prop clearance excludes the low chest footprint while player collision retains it; real masonry continues to use the previously validated wall-retraction pose.

This programme is an unsigned development candidate. It does not alter Showcase Alpha 1.5.2 identity, sign a package, publish a GitHub/itch release, or authorize Butler upload.

## Target devices

- Primary target: Samsung Galaxy S26 Ultra.
- Secondary / equal target: Windows laptop with RTX 5050.

Phone is a first-class target, not a late port.

## Rendering path

- Native Vulkan.
- `VK_KHR_ray_tracing_pipeline` is the preferred path.
- `VK_KHR_ray_query` is acceptable only when it genuinely uses Vulkan hardware ray traversal and is clearly labelled as RayQuery mode.
- `VK_KHR_acceleration_structure` is essential for Vulkan RT.
- Unsupported devices must show diagnostics instead of silently falling back.

## Phase 0 decision

The first real project step is **Phase 0 - Vulkan RT Capability Probe**.

Phase 0 does not build gameplay. It creates the repo foundation, build-path documentation, source layout, and then the real capability probe.

Phase 0 must eventually query/report:

```text
Backend: Vulkan
RT mode: RayTracingPipeline / RayQuery / Unsupported
GPU name
Vendor ID
Device ID
Driver version
Vulkan API version
VK_KHR_acceleration_structure: yes/no
VK_KHR_ray_tracing_pipeline: yes/no
VK_KHR_ray_query: yes/no
VK_KHR_buffer_device_address: yes/no
VK_KHR_deferred_host_operations: yes/no
Internal render resolution
FPS / frame time
```

Feature structs checked and enabled by the current capability path:

```text
VkPhysicalDeviceAccelerationStructureFeaturesKHR
VkPhysicalDeviceRayTracingPipelineFeaturesKHR
VkPhysicalDeviceRayQueryFeaturesKHR
VkPhysicalDeviceBufferDeviceAddressFeatures
```

## Creative direction

- Historical gothic.
- Dark torch-lit tunnels and corridors.
- Wet stone, puddles, fog, fire torches, lanterns, shadows, and global illumination where feasible.
- Later transition into an open ruined courtyard or colosseum.
- Later simple combat against goblins/gremlins.
- Environment, lighting, textures, and RT proof matter more than complex combat.

## Source/reference policy

Use this hierarchy:

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_raytracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

Do not dump a giant third-party engine into this repo. If code is later adapted from a permissive source, preserve license notices and document the source.

## Asset rules

- All assets must be commercial-safe.
- Asset source and license must be recorded in `ASSET_LICENSES.md`.
- Meshy-assisted assets are allowed when the underlying source permits use and the applicable Meshy attribution route is recorded.
- Meshy models must be textured before export.
- Do not import untextured Meshy models and call them complete.
- Prefer glTF/GLB where practical.
- Use high-quality PBR textures from the start when actual visual work begins.
- Phase 0 should not import big assets.

## Deferred from the original scaffold

The following were intentionally outside the original scaffold. Gameplay, Meshy integration, the torch room, BLAS/TLAS, SBT, audio, and packaging have since been implemented in bounded alpha form; broader versions remain deferred.

- Gameplay systems.
- Horde/enemy behaviour.
- Meshy asset import.
- Pocket Chordsmith audio.
- Torch-lit RT room implementation.
- Shader binding table implementation.
- BLAS/TLAS implementation.
- Platform packaging.

## Tested Phase 1 status - 2026-07-10

The project has moved beyond the original Phase 0 scaffold/probe.

Current proven path:

- Android app builds, installs, and launches on Samsung `SM-S948B`.
- Native Vulkan swapchain path presents an RT-produced storage image to screen.
- The scene uses BLAS/TLAS acceleration structures, an RT pipeline/SBT, `vkCmdTraceRaysKHR`, and a storage-image-to-swapchain copy.
- Reports only set `rtScene.presented = true` after successful swapchain presentation.
- The visible scene is now a first-person gothic corridor/ruin prototype with a handheld medieval torch, reflective objects, puddle/wet-stone response, horde silhouettes, fog, and moonlight through a physical second-room roof breach.
- The current source represents the held torch as a real emissive BLAS/TLAS instance rather than a screen overlay. Its laptop and target-phone visual proofs are complete.
- The live laptop and phone proofs show the real orange flame and a deterministic ray-query reflection in a high-reflectivity wall insert. The raw RGBA-to-BGRA copy is explicitly compensated so the flame cannot turn cyan at presentation.
- Controls are now left-drag movement/strafe and right-drag 360 look.
- Windows now runs the same RT corridor as an interactive desktop scene: `WASD` moves, left mouse/trackpad click-drag looks, and `Esc` pauses/resumes.
- The RTX 5050 laptop reported `RayTracingPipeline` and successful RT swapchain presentation at 984 x 661 in the interactive Windows build.
- A Hotstrike Studio stylized skeleton, subsequently textured, rigged, and animated with Meshy, has 11 correctly named animations and is live through a narrow CPU-skinned RT path. The sword remains separate; the 12,358-triangle sword LOD stays staged until a measured static GLB/PBR upload path exists.

Important technical finding:

- A recursive path-tracing attempt using closest-hit secondary `traceRayEXT` calls and pipeline recursion depth 2 failed on the phone at RT pipeline creation.
- The stable phone path uses `rayQueryEXT` from raygen to query the same TLAS for primary hits, shadow rays, and a first bounce sample while keeping pipeline recursion depth 1.
- This keeps the project aligned with RT-or-nothing because ray queries use Vulkan hardware acceleration-structure traversal.

## Initial alpha 0.1.0 route decision - 2026-07-15 historical

- Initial Showing Alpha `0.1.0-alpha.1` is published on separate Windows and Android itch channels.
- The 2026-07-16 hardened refresh was published as Windows build `#1798649` and Android build `#1798652`; its exact hashes remain in the dated 0.1.0 validation records.
- Android uses strict ASTC KTX2 arrays and a stable Samfa12 signing identity; Windows uses a portable executable-relative asset tree.
- Final `SM-S948B` validation passed at 100%, 75%, and 50%. The 75% tier is the sustained phone recommendation; report 100% separately rather than treating it as an unconditional 50+ FPS promise.
- The coloured route was subsequently completed for 0.1.1. The stained transmission pane was rejected in hands-on review and removed; the threshold remains open.
- Keep the textured sword LOD out of runtime until static GLB/PBR support is measured on phone.
- Preserve phone-safe ray-query shading and real `vkCmdTraceRaysKHR` presentation unless a stronger RT route is proven on-device.

## Showcase alpha 0.1.1 publication - 2026-07-17

- Published the completed route as **Showcase Alpha 0.1.1**, package version `0.1.1-alpha.1`, with Android `versionCode 2`.
- Keep the existing itch channels and stable Android update identity. Public build IDs are Windows `#1801016` and Android `#1801017`; exact hashes and validation evidence are recorded in `docs/SHOWCASE_ALPHA_RELEASE_VALIDATION_2026-07-17.md`.
- Treat 75% RT resolution as the sustained Android recommendation. The 100% pass proves full extent and image correctness but carries no 50 FPS promise.
- Keep one rendered/animated skinned enemy at a time. A simultaneous horde requires its own measured phone slice.

## Initial alpha refresh hardening - 2026-07-16

- Keep the local r26 NDK, link the C++ runtime statically, and require 16 KiB ELF/APK alignment in the packaging gate. This removes the unaligned r26 `libc++_shared.so` without adding a second toolchain dependency.
- Publish Android's first performance sample after 30 frames so diagnostics do not appear broken; retain 120-frame steady-state updates afterward.
- Treat 125% as the completed live Windows DPI validation for this refresh. Explicit 100%/150% repeats remain a non-blocking later compatibility check.
- Do not rewrite public Git history until Hotstrike answers the explicit permission request or the owner chooses history remediation.
