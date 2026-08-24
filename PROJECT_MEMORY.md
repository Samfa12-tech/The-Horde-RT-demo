# Horde Lantern RT - Project Memory

Last updated: 2026-08-25

## Identity and release state

- Working/public title: **Horde Lantern RT**.
- Purpose: native Vulkan hardware-ray-tracing game/technology demo.
- Principle: **RT or nothing**; unsupported devices receive honest diagnostics, never a fake fallback.
- Primary target: Android phone. Equal validation target: Windows RTX.
- Current release: **Showcase Alpha 1.5.2**, package version `1.5.2`, Android `versionCode 7`.
- Canonical downloads: https://samfa12.itch.io/the-horde. Samfa12.com links to itch rather than hosting a second copy; the live `/games/` card, itch link, GitHub link, thumbnail, and released status were rendered and verified on 2026-07-15.
- Source: https://github.com/Samfa12-tech/The-Horde-RT-demo.
- Windows itch channel: upload `#18339908`, build `#1913191`, `windows-x64`.
- Android itch channel: upload `#18341739`, build `#1913192`, `android`.
- Signed Android APK SHA-256: `19593f9d8902052cb54f9b989f9646ec8cad97063db5d882e1487dd56a671182`.
- Windows ZIP SHA-256: `fd929f1972c4587c6720013eb0586934ab72924c5f8f9c50ec8576a23a57690d`.
- Signing certificate SHA-256: `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`.
- The release JKS and a local-only password note live together outside Git with restricted ACLs. An independent owner backup is still required.
- Release proof: `docs/SHOWCASE_ALPHA_1_5_2_RELEASE_VALIDATION_2026-08-25.md`.

## Locked creative direction

- Historical gothic action demo: dark ruin, wet stone, fog, torch/lantern light, silhouettes, shadows, and obvious RT mood.
- Lighting and atmosphere come before broader combat.
- Showcase Alpha 1.5.2 publishes the bounded two-skeleton/parry loop, directional dodge, clear roof-water drench/catchment/drain, lich ground mist, positional waterfall ambience, cross-platform RT Lab, corrected general RT water lighting, and Windows controller path. Do not add a third enemy, held guard, broad AI, or fluid simulation without a later phone-measured plan.
- The authored coloured-light route is complete and published in Showcase Alpha 0.1.1; its design history is `docs/COLOURED_LIGHT_ROUTE_PLAN_2026-07-15.md` and its final evidence is in the Windows/Android/release validation records.

## Current renderer

- Android and Windows build BLAS/TLAS, RT pipeline/SBT, dispatch `vkCmdTraceRaysKHR`, write an RT storage image, and present it through the swapchain.
- `rtScene.presented` becomes true only after an RT-produced frame reaches successful presentation.
- The phone-safe path uses `rayQueryEXT` in raygen for primary, visibility, and bounded bounce/transmission work with pipeline recursion depth 1.
- A recursive depth-2 closest-hit experiment compiled but failed during phone pipeline creation. Do not restore it without proving capability and pipeline creation on the phone.
- The RT storage image is RGBA. Common BGRA swapchains require the presentation-format-driven `outputRedBlueSwap` path on raw copy; scaled modes use a format-aware blit.
- One frame remains in flight while held-prop TLAS transforms use host-written instance data.
- `RtGpuResources` owns checked buffer operations, acceleration-structure lifetime helpers, and updatable-BLAS state without taking ownership of the platform device; BLAS sizing/build/refit recording remains in the scene. `CharacterRenderSlot` owns two bounded skeleton pose/BLAS routes plus the singular lich route.
- Supported graphics queues expose a separately labelled Vulkan GPU RT command-buffer interval from top-of-pipe to bottom-of-pipe around acceleration-structure, trace, barrier, and copy/blit recording. It does not replace CPU frame time or include presentation/display latency; unsupported timestamp queues report `N/A` without affecting rendering.
- Android uses strict ASTC KTX2 arrays: ASTC 6x6 diffuse/ARM and ASTC 4x4 normals. Windows uses executable-relative raw RGBA8 arrays.
- `SurfaceWater = 10` is appended without renumbering the material ABI. Refracted and High reflected opaque hits share terminal ordinary material/direct/shadow/fog shading, while the water interface owns the sole bounded reflection; interface highlights share selected lights and visibility. Transparent filtering uses `gl_RayFlagsNoOpaqueEXT`, accumulated distance includes the primary segment, and the directional moon traverses physical roof/player geometry. Water-on-water secondary hits remain non-recursive. Fresh Host/deterministic Windows and exact `SM-S948B` Debug evidence pass; the owner accepted the moving Windows result. The repeated 75% Mobile medians are approximately 27.8 ms waterfall, 19.0 ms skylight, and 30.8 ms lich. The investigated real-light cost is retained rather than hidden by reducing RT quality or resolution. Lich mist is a six-step depth-clipped final-room volume.
- The unreachable stained-glass material route has been removed from the CPU material table and raygen. The retained open threshold and live clear-glass route are unchanged. Current generated-shader identity and phone/Windows measurements belong in the dated renderer validation notes rather than this rolling memory.

## Current alpha scene and controls

- The complete route is two-skeleton encounter -> three-turn moving-shadow corridor -> roof-water drench/lantern drop -> rounded catchment and drain runoff -> blue skylight -> yellow/blue/red/green bays -> open framed threshold -> light-aware hero mirror -> misted staff-lit lich -> opening roof -> returning dawn -> epilogue.
- The rejected stained pane is not present. The water slice is a narrow architectural feature with no fluid simulation, collision, or movement slowdown.
- The held and dropped lantern are RT geometry. Visible flame and direct-light contribution both reach zero after the authored fall; the old fullscreen overlay must not return.
- The rebuilt low-poly player uses a layered travelling coat, shaped shoulders/belt/collar/tails, articulated capsule arms and legs, pelvis/boots, foot lift and toe roll, torso counter-rotation, a head shadow/reflection instance, and wall-aware held-prop retraction.
- Player vitality is three points with a one-second damage lockout, short fatal hold, encounter retry, route restart, and platform-native death overlays.
- The procedural sword swings independently of the torch and the lich requires three accepted hits with a two-second lockout. Its death now opens the roof, reveals warm dawn, and ends in a contextual Continue/Begin Again/Quit epilogue.
- The skeleton uses Hotstrike Studio's base asset processed with Meshy. The CC0 Meshy lich uses restrained `Idle_02`/`Dead` skinning plus whole-instance hover/orbit; its visibly distorted walking clip is deliberately not presented.
- Showcase Alpha 0.1.4 animates/refits/renders up to two skeletons in the opening encounter, with at most two pose buckets, nine BLAS, and nineteen physical TLAS slots; the later lich route remains singular.
- Android: left drag movement/strafe, right drag 360 look, Swing and press-down Parry buttons, Android Back pause/resume.
- Windows: WASD/mouse plus Backbone/XInput/WinMM controller support. The verified Backbone path uses left stick move, right stick look, RT attack, LT parry, B/Circle directional dodge, D-pad menus, A select, Menu/Start pause, and D-pad render-scale adjustment.

## UI, settings, diagnostics, and audio

- Both platforms have branded entry, pause, controls, settings, RT diagnostics, restart, and quit flows.
- Post-lich RT Lab owns the overlay while it is open. Android `showEndingOverlay()` and Windows `ShowEndingMenu()` both guard `rtLabVisible` so recurring completed-finale polling cannot replace or mutate the lab. The owner exposed the Android issue on exact Debug APK `326e024adcb8d5bfb9c5a66fecde9fbb137f19af44cda5c203468fb21dde76d7`; the later process exit was Codex cleanup, not crash evidence. Windows trackbars now use opaque native painting, explicit redraw, routed wheel/page scrolling, and a real vertical-scroll style. Waterfall width scales world-Z cross-lane geometry and matching shader profiles instead of imperceptible world-X depth. Owner Windows acceptance and fresh Host validation pass; exact fixed-APK phone confirmation remains pending because no device was connected for the signed 1.5.2 release smoke.
- Both platform menus expose a release-safe two-pass benchmark: pass 1 warms the deterministic 13-waypoint route, pass 2 measures it, and completion produces a selectable/copyable/exportable text report plus automatically archived JSON evidence. Windows and `SM-S948B` Android device validation pass.
- Both platform menus include `More by Samfa12`, which opens https://samfa12.com/ in the system browser.
- Technical output stays tucked away unless requested or startup fails.
- Both platforms persist a 50-100% RT render-resolution slider with 100% default.
- Android also persists SFX enable/volume, look sensitivity, and compact HUD.
- Windows persists SFX, sensitivity, display mode, and render scale beside the executable.
- Android diagnostics report internal resolution, FPS, frame time, dispatch resolution, and honest RT presentation.
- The shared Debug developer overlay reports build/shader identity, GPU/API, RT mode/presentation, render scale/extents/timing, route/lantern/enemy state, BLAS/TLAS/instance counts, active skinned count, and material route. Windows F3 and Android long-press are live-validated; Android passed 1.7-font layout, Home/resume state, and a 75% overlay-active opening measurement.
- Seventeen FilmCow WAVs cover UI, sword, normalized alternating player/skeleton footsteps, skeleton attack, and lich charge/impact/fall/hurt reactions. A mono DRAGON-STUDIO/Pixabay waterfall loop is the eighteenth shipped audio asset.
- Android uses SoundPool per-cue gain plus lifecycle-paused MediaPlayer for the waterfall. Windows uses XAudio2 per-voice matrices with WinMM fallback. Both consume native world-space waterfall gain/pan.

## Validated Android release state

- Device: Samsung `SM-S948B`, Adreno 840, Vulkan 1.4.295, Android 16.
- Showcase Alpha `1.5.2` / `versionCode 7` is published as Android itch build `#1913192`, SHA-256 `19593f9d8902052cb54f9b989f9646ec8cad97063db5d882e1487dd56a671182`. The exact APK passed the established certificate, version, layout, static-runtime, 16 KiB APK/ELF, credit, waterfall-asset, and native-library guards. No ADB device was connected at publication, so it has no exact signed-device, pullback, lifecycle, performance, or owner-feel claim.
- Showcase Alpha `0.1.5-alpha.1` / `versionCode 6` is published as Android itch build `#1908331`, SHA-256 `1e81238a6e1b0e934c50eb15e80fc8efd39c06f16ca8960c428b22e5f5d5a7f2`. The exact public APK passed established-certificate, version, strict layout, static-runtime, 16 KiB APK/ELF, credit, waterfall-asset, and native-library checks. It was installed on `SM-S948B`, pulled back byte-for-byte, and passed strict ASTC plus honest presentation before and after Home/resume. With that exact release still installed, the owner approved waterfall audio and confirmed good haptics and working pause/resume, closing the change-triggered feedback gate.
- The exact stable-key-signed 0.1.3 APK is published as build `#1845896`, `versionCode 4` / `versionName 0.1.3-alpha.1`. On 2026-08-01 the old stable 0.1.2 and Debug packages were removed, and the exact published APK was clean-installed on `SM-S948B`; the installed `base.apk` SHA-256 matched `a4eb996104c03734a7fa8a16be1f8f701d5b19c861c066af002f96f9a199eee9` byte-for-byte.
- The signed Release selected strict ASTC, loaded all 17 SoundPool cues, visibly identified Alpha 0.1.3, explicitly rejected Debug capture automation, and honestly presented before and after Home/resume with no fatal marker. Evidence: `reports/release-smoke/android-signed-20260801-165934/` and `docs/SHOWCASE_ALPHA_0_1_3_RELEASE_VALIDATION_2026-07-31.md`.
- The same feature source before the release-identity bump passed the full connected `SM-S948B` Debug gate, body/finale screenshots, real Continue and Begin Again actions, 13-waypoint replay, 12 captures, and Home/resume; see the 0.1.3 release proof for exact hashes and boundaries.
- Strict environment plus lich ASTC and `RayTracingPipeline` selected; honest RT swapchain presentation reconfirmed.
- All seventeen SoundPool clips loaded; no native renderer or Android runtime failure occurred.
- The full route, touch controls, combat, reset, pause/Home lifecycle, and accessibility-scale UI passed hands-on validation.
- At 75%, every required warm route zone remained below 13.7 ms median-of-three-window averages at thermal status 3. The 0.1.2 publication gate measured 10.288-13.929 ms across the cool/status-0 default set and completed all 13 replay waypoints plus 12 captures.
- At 100%, full `1440x2980` image/extent presentation passed without a 50 FPS requirement. The latest automated opening was 23.353 ms.
- Evidence: `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`, `docs/ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md`, and `docs/SHOWCASE_ALPHA_0_1_2_RELEASE_VALIDATION_2026-07-22.md`.
- The player benchmark and Debug overlay device gate passed on 2026-07-18, including the owner's follow-up movement/look/Swing, audio-cue, and visual-legibility check. At 75%, the two-pass benchmark measured 12.330 ms median / 19.844 ms P95 across 1,838 frames with honest presentation throughout; 100% completed separately at 17.725 ms median. See `docs/IN_APP_BENCHMARK_ANDROID_VALIDATION_2026-07-18.md`.

## Validated Windows release state

- GPU: NVIDIA GeForce RTX 5050 Laptop GPU.
- Release builds as `HordeLanternRT.exe` with GUI subsystem, icon/version resource, static MSVC runtime, and executable-relative assets.
- The exact final 0.1.3 candidate extraction launched without the source tree, selected `RayTracingPipeline`, dispatched `1232x803`, and honestly presented the RT scene.
- The historical published-route validation used seven CTests. The current Vulkan-enabled Debug/Release host configurations contain thirteen CTests; hands-on route, collision, mirror, combat, lighting, reset, and spatial-audio evidence remains qualified to the corresponding exact candidates.
- The Windows Release in-app benchmark is live-validated at 100%: 2/2 frame-symmetric laps, 26/26 waypoints, 1,838 measured frames, honest RT presentation throughout, selectable/copyable UI, and parseable timestamped text/JSON. See `docs/IN_APP_BENCHMARK_WINDOWS_VALIDATION_2026-07-17.md`.
- The package includes both enemy GLBs, raw environment and lich textures, seventeen FilmCow WAVs, the Pixabay waterfall loop, release notes, controls, and `ASSET_LICENSES.md`.
- The 2026-07-22 foundation gate produced all 12 deterministic 960x540 scene-only RT captures on the RTX 5050, with fixed animation time, honest presentation, complete capture identity/hashes, and no pixel changes across the raygen A/B. Fresh Debug and Release builds and all seven CTests passed in both configurations.

## Foundation validation and capture

- Daily host gate: `tools/run-foundation-validation.ps1` (equivalent to `-Mode Host`). It performs fresh Windows Debug/Release builds, both current thirteen-test Vulkan-enabled CTest passes, thirteen Windows captures, Android clean Debug/Release builds, Release lint, shader-staleness, package/layout, asset/licence, release-identity and evidence-hash gates. The separate portable GitHub Actions lane runs nine Vulkan-disabled tests and is not a hardware-RT claim.
- Milestone/device evidence: `tools/run-foundation-validation.ps1 -Mode Full`. It adds the `SM-S948B` six-checkpoint sustained 75% report, separately labelled 100% opening, Home/resume evidence, and all thirteen Android scene-only captures. Frame times are reported against descriptive 60/50/30 FPS reference lines; crossing 20.000 ms is not an automatic failure. Honest RT presentation/state and lifecycle remain hard requirements, while matched regressions above 15% require investigation.
- Reports are timestamped beneath ignored `reports/foundation-runs/`. Validation ZIP/APK artifacts stay under `reports/`, are explicitly unpublishable/unsigned, and never read or require release-key secrets.
- `tools/compile-raygen.ps1 -Check` compiles into an evidence directory and compares SPIR-V words with the embedded include without mutating the checkout or depending on text line endings.
- Windows Debug supports `--capture-showcase <directory>`; Release rejects it. Android capture/checkpoint intent handling is Debug-only and Release rejects the automation path. Video and orbit-camera work remain deferred.
- Exact 2026-07-22 Full-gate and shader A/B evidence: `docs/FOUNDATION_VALIDATION_2026-07-22.md`.

## Shared deterministic simulation foundation

- Windows and Android now consume one fixed 60 Hz `GameSimulation` for player pose/movement/collision, walk state, lantern, encounter selection, skeleton and lich actions, vitality/death/retry, finale progression, and semantic events.
- Android JNI publishes complete `InputSnapshot` values through a reader-pinned two-slot mailbox; swing, parry, dodge, route reset, and retry use independent monotonic counters so repeated requests cannot collapse into one boolean.
- A fixed-capacity 64-event queue uses stable player/skeleton/lich IDs and unique event sequences. Windows maps drained events to XAudio; Android forwards ordered event records with per-event spatial gains to SoundPool and haptics. Nonfatal accepted hits emit `PlayerDamaged`; the lethal hit emits only `PlayerKilled`, preventing duplicate fatal feedback.
- The shared fixed-step runner clamps a render contribution at 100 ms, permits up to eight catch-up ticks, reports overruns, normalises diagonal movement at 1.9 m/s, and resets its accumulator across pause/lifecycle/reset/retry/checkpoint transitions.
- `SimulationFrameAdapter.cpp` is the single `SimulationSnapshot` to `RtSceneFrameInputs` conversion. `PresentableTinyRtScene`, shader ABI, raygen, RT dispatch/presentation, and public release identity remain unchanged.
- Exact captures import existing `BuildShowcaseCheckpointState` results, perform the historical zero-delta skeleton/lich finalization, and freeze. The post-migration Windows set is 12/12 byte-exact against the 2026-08-11 pre-edit run.
- Focused host tests cover 30/60/120 Hz parity, a 100 ms hitch, diagonal normalization, pause/resume, input edge retention, bounded event identity/overflow, encounter seam persistence, retry/reset, direct-vs-mailbox platform parity, and concurrent mailbox coherence.
- Additive `CMakePresets.json` presets and a Vulkan-off host CI lane improve reproducibility. CI explicitly does not prove hardware RT presentation or phone behavior.
- Detailed implementation and validation evidence: `docs/SHARED_SIMULATION_FOUNDATION_2026-08-10.md`.

## Renderer resource and GPU timing foundation

- Low-level buffer allocation/upload/destruction, acceleration-structure lifetime helpers, and updatable triangle-BLAS state are extracted from `PresentableTinyRtScene` behind `RtGpuResources`; BLAS sizing/build/refit recording, platform device ownership, descriptors, textures, pipeline, SBT, and capture remain unchanged.
- Historical renderer-foundation evidence used one selected skeleton-or-lich address at TLAS instance 2, eight BLAS, and eighteen instances. Current development source supersedes that capacity with two bounded skeleton routes, nine BLAS, nineteen physical TLAS slots, custom index 18 for the second pose route, and a singular lich route.
- Windows and Android use a non-blocking two-query Vulkan timestamp lifecycle tied to the existing one-frame fence. Reduced-width timestamp wrap and availability are handled; unsupported/query-failure states remain diagnostic only.
- CPU frame timing method, shader ABI, raygen, and presentation honesty remain unchanged. Benchmark schema 7 supersedes the former hard 20 ms criterion with descriptive 60/50/30 FPS bands and governor context. The 23.604 ms lich result is historical unmatched/hot evidence: exact `b3428a7` matched cooled A/B later measured 19.497 ms timing-enabled and 19.268 ms timing-disabled, while the separate warm `daa5892` parry candidate measured 20.246 ms at lich.

## Measured two-skeleton combat candidate - 2026-08-11

- The opening encounter is now a fixed pair at `(-0.75, -4.65)` / `(0.75, -4.65)` with stable `SkeletonA`/`SkeletonB` IDs, strict nearest-then-ID attack ownership, nearest-only sword hits, independent persistent death, world-valid 0.70 m separation including around fixed corpses, and completion only after both die. Retry/reset restores both; no third enemy or general ECS was added.
- `SimulationSnapshot` publishes a bounded two-entity array, alive count, attacker ID, and encounter completion. Ordered events retain the bounded queue contract and identify A/B; a missed swing targets `Invalid` rather than inventing an entity.
- `CharacterRenderSlot` emits up to two skeleton instances. Matching actions share pose bucket 0; divergent action/death poses use at most bucket 1. Final dead poses clamp to their clip duration so corpses converge to a shared bucket and stop redundant refits. The lich stays singular.
- The scene now reports nine BLAS and nineteen physical TLAS slots. Semantic custom indices 0-17 remain stable; the second pose route uses custom index 18. Unused slots are masked and keep invertible identity transforms.
- Historical checkpoint imports 0-11 remain singular and all twelve published 0.1.3 PNG hashes are byte-exact. Checkpoint 12, `two-enemy-combat`, is the explicitly reviewed new capture and the default phone gate now measures six checkpoints.
- The Debug runner records enabled/disabled GPU-timing mode, verifies local and installed APK hashes match, and records commit/dirty/shader identity. `tools/compare-android-gpu-timing-ab.ps1` enforces same-artifact/device/run shape, comparable AP/SKIN/BAT starting temperatures, and the 15% matched-regression investigation threshold; it no longer turns the descriptive 20 ms/50 FPS reference into a product gate.
- Host validation passed. Exact clean commit `b3428a7` `SM-S948B` evidence then passed matched cooled lich A/B (19.497 ms timing enabled / 19.268 ms disabled, 1.188% difference), all six 75% checkpoints below 20 ms, 13/13 replay/captures, 18.674 ms report-only 100% opening, and Home/resume. The owner subsequently reported that hands-on play on that still-installed exact candidate "feels fine," closing the broad subjective promotion gate without creating a detailed cue-by-cue audio/haptic certification. This is historical exact-candidate evidence, not proof of later parry/current source. See `docs/TWO_SKELETON_COMBAT_ANDROID_VALIDATION_2026-08-12.md`.
- PRs #10 and #11 merged to `main` at `6ec3119`. Final review preserved the historical 140 ms separation between Android sword impact and enemy-fall audio, exposed platform-event overflow in diagnostics, and removed duplicate lethal-hit haptics. The owner subsequently reported that controls, audio, and haptics all worked correctly hands-on on `SM-S948B`. This is owner-reported development-build evidence without a new exact-artifact check; the later exact automated two-skeleton gate separately clears performance/presentation, but neither result certifies comfort, spatial-audio quality, cue tuning, or exact hands-on artifact provenance.
- Detailed implementation and validation evidence: `docs/RENDERER_RESOURCE_SLOTS_GPU_TIMING_2026-08-11.md`.

## Animation-owned combat and timed parry - 2026-08-13

- `SwordCombat` now publishes explicit player swing/parry phases and skeleton locomotion/wind-up/active/recovery/stagger/dead actions. Sword and skeleton damage each resolve once on entry to their visible active window; off-angle, out-of-range, and recovery contacts miss.
- Lich sword damage is no longer accepted on the command edge. The shared player active-contact pulse must pass the same range/cone test before the existing lich two-second lockout accepts one of its three hits.
- Android and Windows publish an independent monotonic parry sequence. A 40 ms startup plus 220 ms active window can cancel a real frontal skeleton strike, emit `PlayerParrySucceeded`, and hold that attacker/token in an 800 ms stationary stagger. Failed parry completes 240 ms recovery; success permits a normal next-tick riposte. Unavailable and death-state inputs are consumed without buffering.
- Android adds a 104 x 72 dp `PARRY` button beside `SWING`, publishes its monotonic command on touch-down while retaining accessibility click activation without release double-fire, reuses `sword_hit_2` for the clang, and routes success to a distinct short strong vibration. Windows binds `Q`. Diagnostics expose the consumed parry sequence and readable player/attacker action phases.
- Renderer composition reuses the existing sword/right arm and Attack clip. Stagger remains stationary in gameplay but procedurally moves the existing Attack clip from contact toward recovery with bounded whole-instance recoil/lean; it is not a frozen contact pose. `CharacterRenderSlot` now caches one frame plan for skin/refit and TLAS; nine BLAS, nineteen physical TLAS slots, and the two-pose-bucket ceiling remain unchanged.
- Windows Debug/Release each pass all 12 CTests. The fresh Host foundation run `reports/foundation-runs/run-20260813-204944` passed shader staleness/negative gates, fresh builds/tests, 13 captures, Android Debug/Release/lint across four ABIs, packaging/licence checks, and evidence hashes. All 13 Windows captures are pixel-identical to the exact two-skeleton baseline; matched capture timing changed by 0.178%.
- Exact clean `daa5892` Debug APK `a3eca0ed1ae49800541f1de95d329af8cb3bef50d53dac302a20238ade419302` installed byte-for-byte on `SM-S948B`. Strict ASTC, honest RT presentation, six checkpoint states, replay, 13 captures, and Home/resume passed; the warm 75% run was not a complete performance pass because lich measured 20.246 ms. The owner reported that parry timing felt good and requested press rather than release dispatch. The subsequent `ACTION_DOWN` source change was intentionally merged without rebuild, reinstall, or retest, so the installed exact-APK and hands-on evidence do not cover that follow-up revision.
- Current reconciliation source includes Android press-down parry, animated skeleton stagger, listener-at-event-time feedback, bounded Android platform transport, and matched positional skeleton hit/fall routing on Windows. Final exact clean runtime commit `547d89d` / APK `f68fe4cccf2755ef579826080bf76364fe2a48744b23aba765df02a17e2d1dfa` passed the complete Full gate: matching installed APK, strict ASTC, honest RT presentation, 12/12 fresh Debug and Release CTests, Android build/lint, replay, 13 pixel-stable captures, and Home/resume. Sustained 75% medians were 11.340 / 12.145 / 9.267 / 10.375 / 13.035 / 23.069 ms; the last-in-order lich is retained as 30-50 FPS-band evidence at GPU thermal power level 2. Earlier exact `88868f4` diagnostics found flat graphics/native/PSS/RSS/thread state while Samsung reduced the GPU from 1300 MHz to 578-646 MHz at power level 7, supporting descriptive 60/50/30 FPS bands and matched-regression investigation instead of a hard 20 ms rule. The owner then reported “all good” on the exact final candidate and specifically observed the stagger-back/death sequence, closing the broad audio/haptic/stagger baseline without claiming calibrated positional accuracy. This runtime is now published in Showcase Alpha 0.1.4; see `docs/CURRENT_DEVELOPMENT_BASELINE_VALIDATION_2026-08-21.md` and `docs/SHOWCASE_ALPHA_0_1_4_RELEASE_VALIDATION_2026-08-22.md`.

## Audio/haptic manual-validation policy - locked 2026-08-21

- Manual owner audio/haptic validation is change-triggered, not required after every milestone. Every milestone must state `Audio/haptic manual revalidation required: YES/NO` and give a one-sentence reason. The default is **NO**.
- Require **YES** when a change can materially affect spatial-audio mathematics, listener pose/yaw or event-time listener data, source coordinates/identity, attenuation/obstruction/pan, playback backend or gain/cue/assets, gameplay-event delivery/timing, Android/Windows platform transport, haptic routing/cue/pattern/intensity, or damage/death feedback semantics.
- Unrelated RT/shader optimisation, environment/lighting/texture/UI/build/documentation/telemetry work, unrelated AI or animation work, and packaging do not automatically invalidate the accepted baseline when automated contracts pass and semantic audio/haptic inputs are unaffected.
- Current reconciliation classification: **YES** — listener-at-event-time routing plus platform feedback transport/timing changed, so the final exact Android candidate required and received an owner check. That baseline is now accepted; future checks return to the change-triggered rule. See `docs/OWNER_RELEASE_SAFETY_CHECKLIST.md` for the separate owner-only signing-safety checklist.
- Detailed implementation evidence: `docs/ANIMATION_COMBAT_PARRY_SLICE_2026-08-13.md`.

## Asset and licence state

- Five Poly Haven environment sets are retained under CC0 and packed into current platform formats.
- The skeleton derivative ships under Hotstrike's finished-project/modification permission plus the conservative Meshy Free-plan CC BY 4.0 attribution route.
- FilmCow SFX use FilmCow's custom royalty-free project-use terms; the complete source archive is not redistributed.
- Meshy sword source/LOD and torch study are staged only. Neither is loaded or distributed until a measured static GLB/PBR path and appropriate attribution route exist.
- Preserve public Hotstrike/Meshy credit and keep `ASSET_LICENSES.md` with Windows packages and linked from the download page.

## Important files

- Android UI: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- Android native bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`
- Windows presentation/UI: `src/platform/windows/DiagnosticWindow.cpp`
- Shared RT scene: `src/vulkan/raytracing/PresentableTinyRtScene.cpp`
- Renderer resource seam: `src/vulkan/raytracing/RtGpuResources.cpp`
- Bounded two-skeleton/singular-lich slot: `src/vulkan/raytracing/CharacterRenderSlot.cpp`
- Vulkan GPU timer: `src/vulkan/GpuFrameTimer.cpp`
- Raygen source: `shaders/raytracing/minimal.rgen`
- Embedded raygen: `src/vulkan/raytracing/MinimalRayGenShader.inc`
- Shared showcase route: `src/gameplay/ShowcaseRoute.h`
- Collision/combat: `src/gameplay/CorridorCollision.h`, `src/gameplay/SwordCombat.h`
- Licences: `ASSET_LICENSES.md`
- Release readiness: `docs/ALPHA_RELEASE_READINESS_2026-07-15.md`
- Packaging: `tools/package-alpha.ps1`, `tools/package-signed-alpha.ps1`, `tools/push-alpha-to-itch.ps1`
- Build/test/demo cycle plan: `docs/BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md`
- RTXPT-derived performance/quality/workflow reference and no-regression gates: `docs/RTXPT_1_8_1_REFERENCE_AND_REGRESSION_GATES_2026-07-17.md`
- Developer overlay Windows validation and Android build boundary: `docs/DEVELOPER_OVERLAY_WINDOWS_VALIDATION_2026-07-17.md`

## Next-step sequence

1. Preserve the published alpha and its stable signing identity.
2. Back up the JKS and both passwords independently.
3. Treat the complete 0.1.3 body/vitality/dawn route as the preserved playable baseline. The shared fixed-step simulation/input/event foundation and the bounded renderer resource/character-slot plus GPU-timing foundation are complete; see `docs/FULL_REPO_AUDIT_AND_GAME_PLAN_2026-08-01.md`.
4. The deterministic checkpoint, three-window benchmark, native route replay, and bounded Android evidence runner foundation is complete and live-validated.
5. The integrated cross-platform clean-build/package/stale-shader/licence gate and deterministic 13-checkpoint Windows/Android PNG capture foundation are complete. The original 12 captures remain preserved and the appended two-enemy capture passed the exact current device gate. Keep video/orbit-camera presentation work deferred until it has a separately bounded need.
6. Measure each meaningful renderer/gameplay-route change on the phone at 75%; report 100% separately, compare sustained matched runs, and retain the short hands-on touch/audio/lifecycle pass.
7. Keep real RT and honest diagnostics. Reduce bounded effect area/ray cost before expanding gameplay or substituting fake effects.
8. Treat `docs/BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md` as the detailed backlog and `docs/DOCUMENTATION_CHECKPOINT_2026-07-17.md` as the documentation authority map.

## Full-game pause-point audit - 2026-08-01

- The next work is a short pre-production foundation, not direct horde expansion: one shared `GameSimulation` tick, an Android-safe input mailbox, normalized fixed-step movement, a bounded gameplay-event queue, persistent encounter state, and deterministic platform-parity tests.
- The audit found a real Android input data race and frame-dependent movement. Fix both before adding enemies or richer combat.
- Current validated multi-enemy capacity is two skeletons with two physical TLAS routes and at most two CPU-skinned/refit pose buckets. This is a milestone ceiling rather than a permanent design limit; the lich remains singular and four/five enemies require a later measured design.
- First expansion gate is two skeleton instances sharing a model/pose and using an attacker token. Four enemies require a later, separate phone pass.
- Combat animation follows the shared simulation and uses action states plus explicit animation hit events. Rendering consumes snapshots and never decides damage.
- Fire uses bounded emitter records, emissive RT geometry, and phone-budgeted raygen effects. Shallow water uses real geometry plus bounded reflection/transmission queries; steam uses bounded raygen density volumes. No particle BLAS swarm, SSR, raster fallback, or fluid simulation.
- The audit introduced a named `RtSceneFrameInputs` renderer snapshot and one checked buffer-write helper without changing shader output or renderer policy.
- Before wider source collaboration, resolve the pending raw-skeleton public redistribution permission or replace/remediate the asset. This pause-point commit normalized the six current Poly Haven texture arrays that conflicted with the repo's Git LFS attributes; any older-history rewrite remains a separate decision.
- Audit validation passed Windows Debug/Release builds and 7/7 CTests in each configuration, a 12/12 deterministic Windows RT capture set bit-exact against the 2026-07-31 baseline, Android Debug/unsigned Release across all configured ABIs, and `lintRelease`. No new renderer-visual or performance phone claim was made; exact-device haptics evidence was added separately.
- The owner corrected the earlier haptics report: haptics had not previously been checked or perceived on `SM-S948B`. The audit revision adds direct Android vibration for swing/damage/fatal cues plus an enabled-by-default setting/preview and view fallback. Exact Debug APK `14e63fcfa4d3b2ed5fc6982ea4f0e35d5ea4b1a0bb78ef7cf7ba15e4d4cb3380` installed byte-for-byte with permission granted; after unlock, Android recorded completed preview, Swing, damage, and fatal effects through the real encounter, and the owner confirmed the haptic was physically felt. Basic operation passes; intensity/comfort/cue distinction remain later tuning. Full-game haptics should consume the shared gameplay-event queue beside audio.

## Showcase route blockout - 2026-07-16 historical milestone

- The original room, material gallery, skeleton encounter, controls, reset, and shader path remain intact.
- A shared route definition now owns walkable rectangles, preserved obstacles, spawn/room coordinates, showcase zones, and the pure position-to-zone query for both platforms.
- The route extends through a three-turn shadow corridor, physical-aperture skylight chamber, four unlit torch bays, empty transmission frame, and dry empty finale with a mirror surround.
- Collision resolves from previous to proposed position with swept union checks and X/Z wall sliding; `ShowcaseRouteSmoke` covers every zone, corners, shortcuts, the old far wall, and preserved obstacles.
- Windows Debug/Release tests and the Android debug build/install passed. Strict ASTC and honest RT swapchain presentation were observed on `SM-S948B`; see `docs/SHOWCASE_ROUTE_BLOCKOUT_VALIDATION_2026-07-16.md` for evidence and the remaining manual performance sweep.
- Hands-on feedback added a raised masonry skylight shaft plus a skeleton arena leash and collision-aware movement. Directional attenuation, distance rolloff, and wall-aware audio remain an explicit follow-up rather than part of the geometry slice.

## Alpha refresh validation - 2026-07-16

- Android debug uses `com.samfa12.hordelanternrt.debug` beside the stable public package; release keeps `com.samfa12.hordelanternrt`.
- Android native packaging is 16 KiB-page compatible through a static C++ runtime, `0x4000` ELF `LOAD` alignment, and verified 16 KiB APK alignment.
- Android publishes a first 30-frame timing sample so resolution, FPS, and frame time appear promptly; later updates retain the 120-frame window.
- Windows carries a Per-Monitor V2 manifest, DPI-scaled layout/fonts, minimum sizing, and in-app credits. The freshly extracted package passed the full live sweep at the machine's active 125% scale.
- The live itch page now directly credits FilmCow, Poly Haven, Hotstrike Studio, Meshy, CC BY 4.0, and the full licence manifest; Code and Graphics are selected in the AI disclosure.
- Hotstrike public-source permission was requested at `https://itch.io/post/16578566`. Finished-game packaging remains permitted, but the public raw-GLB/history question is still pending their reply.

## Windows-first complete showcase route - 2026-07-16

- The playable route now sequences skeleton -> deterministic lantern gutter/drop -> bounded blue skylight -> yellow/blue/deep-red/restrained-green torch bays -> open framed threshold -> light-aware one-bounce hero mirror -> floating staff-lit lich -> post-death sliding skylight.
- Player presence is complete with a reusable-BLAS leather pelvis, articulated thighs/shins/boots, procedural gait, exact retained hand grips, and reset-only lantern failure/lowered left arm.
- `ShowcaseGameplay.h` owns deterministic lantern, lower-body, lighting, plural roster/director, and lich charge/recovery state. Only one skinned enemy is selected/rendered/refit at once; the capacity remains configurable for later Horde measurements.
- The lich uses continuous living `Idle_02` and non-looping `Dead` clips; whole-instance hover/orbit replaces the visibly distorted walking clip. Its separate 48-byte UV stream, raw Windows KTX2, strict Android ASTC 6x6, derived violet emissive map, and forty skin-weighted staff vertices drive the visible staff light/electricity. It takes three hits with a two-second lockout; each accepted hit produces recoil plus a positional cry, and death opens the finale roof over 4.5 seconds.
- Player travel and skeleton cadence now produce accepted audible footsteps. Skeleton and lich spatial cues share equal-power pan, distance rolloff, and route-obstruction attenuation through XAudio2 on Windows and published left/right SoundPool gains on Android. Android playback passed hands-on testing, while perceived stereo directionality and distance remain explicitly uncertified.
- Windows Debug/Release and all five CTests pass, and the final hands-on Windows route/audio/combat verdict passed. The complete route is also device-validated on `SM-S948B`: strict environment/lich ASTC, honest RT presentation, full hands-on traversal, lifecycle recovery, and controlled warm 75% measurements all pass. Every required zone's median of three 120-frame average windows was below 13.7 ms at thermal status 3; see `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`. Label this state **Windows-validated / Android-device-validated**.
- Preserve the latest raygen artifact recorded by the current foundation evidence run. The lich's Meshy CC0 evidence is retained at `assets/models/enemies/meshy/lich_placeholder_source_licence.png`. Two skeletons remain the current milestone's validated capacity; four/five are a future measured design rather than prohibited product scope. The two-skeleton automated phone measurement and broad owner hands-on promotion judgment are passed.
