# Beyond the Tomb — 1.7.0 Scoped Update and Implementation Plan

> **For agentic workers:** Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` when available. Execute one reviewed work package at a time. This is the complete master handoff, not permission to build every subsystem concurrently.

**Owner:** Sam Small / Samfa12  
**Prepared:** 11 September 2026  
**Executor:** Codex, with Astra medium selected by the owner  
**Goal:** Turn the existing dungeon into the game's prologue, then deliver a physical rope rescue into a beautiful, voiced, moonlit woodland chapter with a cohesive gothic touch HUD and menus.  
**Architecture:** Retain native Vulkan hardware RT and the shared fixed-step simulation. Add bounded world-zone ownership, genuine vertical traversal, one reusable companion actor, data-driven dialogue, outdoor atmospheric rendering, and a shared visual specification implemented through the existing platform UI layers.  
**Tech stack:** Existing C++/Vulkan/GLSL renderer, Android Java/JNI, Windows native platform layer, existing asset import/build tools; Blender for authoring and cleanup; Meshy for selected textured character assets; image generation for reference art and selected 2D assets.  
**Spec:** Sections 1–14 of this document are the scoped design specification. Sections 15–18 are the execution, verification and handoff contract.

**Status:** Planning only. The owner approved the preceding creative direction and requested this complete scoped handoff, adding Blender, image generation and an on-theme controls/menu refresh. No 1.7.0 runtime, artwork, voice recordings or performance evidence is delivered by this document. Numerical budgets below are proposed starting budgets, not measured capabilities.

**Start condition:** Implement only after the owner has finished and accepted the 1.6.1 baseline. Do not interrupt, merge, overwrite, re-version or declare complete the unfinished 1.6.1 engineering pass. This documentation-only file may live on `main` before the implementation baseline is ready.

---

## 1. Source authority and what was actually inspected

The owner's latest instructions govern the new experience. Preserve `AGENTS.md` engineering/safety requirements and use the eventual accepted 1.6.1 source as implementation authority. Historical documents remain evidence of their own versions, not proof of current implementation or performance.

Planning inspection used `codex/horde-1.6.1-engineering-pass` at `191d799ab7fda54fab36d9792b11f908ddaaf42d`. The branch's package note calls 1.6.1 an unpublished engineering candidate. This is an inspection snapshot, **not the mandatory future starting SHA**. Re-audit the accepted baseline before changing code.

Important inspected sources, relative to repository root:

| Source | Relevant finding / responsibility |
|---|---|
| `AGENTS.md` | Native RT, shared 60 Hz gameplay authority, coherent input mailbox, ordered feedback, Android/Windows validation and honest evidence rules. |
| `PROJECT_MEMORY.md` | Existing reward lantern, static GLB/PBR importer, player/character rendering, audio, RT Lab and historical finale. The rolling summary contains older details; inspect code before relying on counts. |
| `src/gameplay/simulation/SimulationSnapshot.h` | Player location is represented by `playerX` and `playerZ`; no general player-height component is present in this inspected snapshot. |
| `src/vulkan/raytracing/SimulationFrameAdapter.cpp` | Copies horizontal player location into render inputs and resolves roof/dawn overrides. Vertical traversal is an end-to-end dependency, not just a camera effect. |
| `src/vulkan/raytracing/PresentableTinyRtScene.h` | Inspected scene declares 16 BLAS and 20 TLAS instances. These are current implementation limits, not permanent design limits or forest budgets. |
| `src/gameplay/interactions/FinaleSequence.h` | Current sequence includes `DawnRevealed` and `Complete`. The new continuation must explicitly replace the campaign ending behavior. |
| `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java` | Native Android views, buttons, panels, settings, SoundPool/MediaPlayer and menu/ending/RT Lab state. Preserve platform behavior while extracting focused helpers. |
| `src/platform/windows/DiagnosticWindow.cpp` | Existing Windows gameplay/menu/audio/controller integration entry point; inspect the final baseline before changing it. |
| `docs/RT_WATERFALL_LICH_MIST_VALIDATION_2026-08-23.md` | Existing mist is an AABB-bounded, primary-depth-clipped, six-sample transmittance volume. This does not establish a complete outdoor, shadowed volumetric lighting system. |
| `docs/ENGINEERING_1_6_1_ANDROID_OBSERVATION_BASELINE_2026-09-05.md` | Exact Debug observation at 75%, internally 1080×2235, reported six checkpoint window-average medians of roughly 41–55 ms. These are not GPU timestamps, Release performance or a matched historical comparison. |
| `CMakePresets.json` | Existing Windows Debug/Release configure, build and test presets. |

The phone evidence above is a reason to measure the combined forest/lantern/companion workload early, not a reason to declare the final 1.6.1 slow or to prescribe an arbitrary low resolution. Do not compare those instrumented Debug observations directly with old cooled or differently configured runs.

The original brainstorming described unloading the dungeon during climbing and confidently mentioned Meshy custom motion. This specification tightens both: visible/RT-relevant geometry must remain resident, and custom-motion API availability is not a dependency. Verified rigging/preset animation plus Blender cleanup is the baseline asset route.

## 2. Global constraints

- **RT or nothing:** keep actual native Vulkan hardware ray traversal and truthful `vkCmdTraceRaysKHR` swapchain presentation. No raster-only world, SSR, baked scene lighting or compute-only substitute.
- **Android first-class; Windows RTX equally validated.** Do not introduce an NVIDIA-only requirement. Reuse the final phone-safe recursion-depth-one/ray-query architecture.
- **Gameplay authority remains the shared 60 Hz simulation.** UI, rendering, audio backends and asynchronous loaders consume/publish contracts; they do not independently advance quests or traversal.
- **One frame in flight remains the default** until proper per-frame ownership is separately designed and measured. Do not break host-written TLAS instance safety.
- **Existing gameplay/feedback remains intact** outside explicitly changed prologue progression. Preserve input sequencing, parry timing, damage/death semantics, delayed impact/fall cues, retry and pause cancellation.
- **No broad engine replacement.** Build reusable components required by this chapter; do not introduce an ECS, editor, general open-world streamer or new rendering framework as a prerequisite.
- **Conventional 2D UI is allowed.** Native view drawing, text, scalable icons and compositing a menu are not a prohibited world-rendering fallback. Do not ray trace menu controls to prove compliance.
- **Blender is an offline production tool, not a runtime dependency.** Image generation provides art inputs, not proof of live engine rendering.
- **No untextured final assets or undocumented licenses.** Check source, permitted use, attribution, model/voice provenance and runtime import compatibility.
- **No new subscriptions, credit top-ups, actor hiring or unbounded paid generation without owner authorization.** Existing account access is not unlimited spending permission.
- **This task does not authorize publication.** Do not tag, sign, upload to itch/GitHub Releases, change public pages or claim release readiness without the existing release gates and explicit owner authorization.

## 3. Product scope and definition of the chapter

Working release label: **Showcase Alpha 1.7.0 — Beyond the Tomb**. This is a milestone subtitle, not a rename of Horde Lantern RT.

Deliver one complete extension: existing dungeon → lantern reward → nighttime roof opening → companion's rope rescue → climb → woodland reunion → player-controlled lantern reveal → short companion-led forest trail → an authored atmospheric stopping point with continuing-story presentation.

Required content: one starting cave-in, one coherent tomb/shaft/exterior entrance, one physical rope, one fully animated companion, one modest forest route, complete short-scene voice playback and subtitles, three-dimensional trees, mist/fog with real shadowed moon shafts, fireflies, forest ambience, and refreshed in-game controls/menus on both supported platforms.

Initial outdoor footprint: approximately **40–80 metres of authored trail**, with a clearing, two or three bends, a misty hollow and an old waymarker/gate as the final hook. Treat dimensions as blockout targets; choose final scale using player movement speed and scene composition. Do not stretch a small amount of content into a long empty walk. The player remains free to look and move within the corridor; this is not on-rails movement.

Explicitly outside 1.7.0: an open world, extra dungeons, a new forest boss, new enemy species, larger simultaneous combat groups, companion combat, escort failure, dialogue trees, inventory/economy, procedural quests, day/night simulation, advanced weather, full fluid simulation, hair/cloth simulation, multiplayer, fully simulated human climbing, or cinematic facial capture. Existing enemies and existing combat remain the tutorial.

Do not remove a required feature merely to finish quickly. When a required subsystem is blocked, record it honestly and continue independent work; final acceptance remains open. Optional density and ornamentation can scale before required features are cut.

**Story terminology:** Keep the owner's working term **the Horde**. Do not silently rename it to the Hoard, reinterpret it as an army, decide what it ultimately is, or invent a larger mythology. The pair are dungeon crawlers searching for a legendary objective; the lantern is their first tangible piece of the puzzle. Final proper names and lore are not needed for this chapter.

## 4. Authored player experience

### 4.1 Opening cave-in

At the existing spawn, turning approximately 180 degrees reveals a collapsed entrance, not a dressed-up flat wall. Use broken stone, dirt, a fractured arch and limited fallen timber consistent with the tomb. Show a blocked passage continuing beyond it where composition permits. Rubble must occupy real space, meet the floor and have a simple matching collision boundary. Keep the opening encounter and forward route clear.

Do not simulate the collapse live. It already happened. A few settling particles/stone sounds are optional; the geometry must communicate the event without them. A second exterior view of the blocked original doorway, visible from the upper clearing, can explain why the companion could not simply walk in.

### 4.2 Reward and night opening

Preserve the accepted Lich/chest sequence, including the separate two-second chest unlock if it remains the accepted 1.6.1 behavior, the guidance cue and actual claim interaction. The player must own the lantern before rescue progression can complete.

Replace the campaign's returning-dawn/ending-card transition with a moonlit opening. Use the same moon orientation, exposure policy and sky inside and outside. Opening stonework must have a believable place to move; do not lift an enormous roof through trees, terrain or the companion. Rework the lid/oculus and upper chamber geometry only as necessary for a traversable shaft.

On opening, the player sees actual sky, rim stone, roots/branches and the companion above. Preserve the ability to look around. Use a short objective and positional voice to draw attention instead of forcing a long camera turn.

### 4.3 Rescue and rope

The companion notices the player, speaks, throws the rope and steps aside. The rope pays out from a bounded authored starting arrangement, then falls and reacts dynamically to gravity, the rim and shaft. Its upper end has a visible, credible tie-off on a tree, stone ring or fixed tomb fixture; the companion does not hold the player's weight by a magically locked wrist.

The interact prompt becomes `Climb` only once the rope is deployed, reachable, anchored and the traversal destination is ready. Interaction must use the existing semantic action, not a new bespoke platform control.

### 4.4 Climb and summit

A single action begins a reliable short ascent, initially targeting about **4–7 seconds of traversal playback**, depending on the final shaft. This is an authored animation duration, not an implementation-time estimate. No repeated tapping, stamina meter, quick-time event or fall-death challenge.

Stow the sword and clip the lantern to a visible, physically plausible carrying socket before the hands grip the rope. The lantern remains owned, lit and correctly positioned; do not silently delete it or hold a sword, rope and lantern in the same hand. At the top, release the rope, mantle over solid stone and restore the appropriate held-item state.

Climbing changes the authoritative player/world position. Hands contact the simulated rope via grip targets; shoulder/elbow motion, body/shadow and held-item transforms follow the same traversal state. The camera follows a stable climbing frame, not every high-frequency rope oscillation. Provide reduced traversal motion with minimal bob/roll without disabling the rope simulation.

The summit reveals an overgrown tomb emerging from a wooded bank, not a square dungeon box on a flat plane. Roots over masonry, soil buildup, fallen stones, wet leaves and a clear trail establish place. The view back down remains coherent wherever the player can see it.

### 4.5 Reunion and lantern lesson

The companion approaches a safe conversation position, asks about the lantern and looks between player and lantern. The player raises it using the existing Raise/Lower action. Do not substitute an automatic cinematic for this interaction.

If the lantern was raised before climbing, stowing and restoring it leaves it lowered for this lesson. Consume a **new raise action during the reunion**, not a stale input edge from before the climb. The player may look away, pause or take time; the story cannot soft-lock. One delayed reminder is allowed, not constant repeated dialogue.

The warm lantern lights the companion, nearby bark and appropriate fog against cold moonlight. The player answer and companion reaction follow the raise event. This is the chapter's central lighting/character moment.

### 4.6 Woodland continuation and stopping point

After the exchange, the companion turns onto the trail and walks ahead, waiting when the player lags. Give the clearing a brief quiet interval before new instructions. Let the player admire the scene, raise/lower the lantern and inspect the tomb.

Bends reveal compositions rather than identical tree corridors. Use banks, roots, rocks, fallen trees and understory as natural boundaries. The forest continues visually beyond them; no distant-flat-image substitute for the nearby woodland.

At the final waymarker/gate, stage a restrained sound or silhouette beyond the route and let the companion wait. The chapter ends in-world with a continuing objective and an optional themed `Continue exploring / Return to menu` panel opened by the player. Do not automatically cover the forest reveal with the old completion overlay. Do not imply a further playable level already exists.

## 5. World zones, continuity and vertical movement

### 5.1 Choose residency from evidence, not from a cinematic assumption

Implement bounded zone ownership, but choose the simplest successful loading policy at an early gate:

| Candidate | Use / trade-off |
|---|---|
| Preload the modest forest and keep both zones resident | Simplest continuity. Accept only if measured peak memory, RT work and warm performance are satisfactory. No requirement to stream for its own sake. |
| Stage a shared transition region and incrementally prepare the forest | Preferred expansion path when residency needs managing. More synchronization and rollback work, but reusable for later chapters. |
| Explicit brief loading transition | Honest fallback only when seamless traversal cannot meet measured resource constraints. Requires owner review of the presentation compromise; do not silently introduce it. |

The rope climb is a useful preparation interval, **not permission to destroy a visible dungeon**. The player can look down; reflections, transmission, moon/lantern shadows and indirect paths can still depend on off-camera geometry. Frustum culling alone is not a valid RT residency policy.

Represent the world as `Dungeon`, `TombTransition` and `Forest` logical zones with stable IDs and clear ownership. Keep the relevant upper dungeon room/shaft/rim and clearing simultaneously available across the transition. Retire remote dungeon resources only after they cannot contribute to permitted view/light paths; retain the upper-room geometry while the opening can be inspected. After the first occluding trail bend, unload additional content only with hysteresis and a tested backtrack policy. No visible pop-out, disappearing shadow or fake painted view down the shaft.

A small 1.7.0 forest may reasonably remain entirely resident once entered. Do not build a general streaming open world.

### 5.2 Ownership and threading

Persistent campaign/player state belongs above zone resource lifetimes. Zone descriptors own authored placement, collision references, asset requests and scene membership; the renderer owns GPU residency/builds and exposes readiness/failure. Shared simulation consumes readiness at fixed-step boundaries.

Read/decode on worker threads only when useful and thread-safe. Submit/upload/build/publish through explicit existing ownership, with bounded upload work, transfer/RT barriers and GPU completion tracking. Never free buffers, descriptors, images, BLAS or TLAS still referenced by submitted work. Changing geometry/instance counts may require a rebuild rather than an update; follow Vulkan's exact rules [R5].

Use generation IDs to reject stale load completion after restart, save restore, retry or Android lifecycle reconstruction. Keep a last safe zone and checkpoint until destination publication succeeds. Failure leaves the player safe with a truthful retry/back-to-menu option; it must not strand them in the shaft.

### 5.3 Vertical position and ground support

Add coherent 3D transforms/height through simulation snapshots, renderer adapter, shader camera origin, collision, body/hand sockets, lantern pendulum inputs and audio listener/source positions. Do not add a height offset only inside the shader or only on one platform. Check ray distance/bounds and world-space environmental calculations outside the original corridor.

Keep the existing dungeon collision path working. For the forest use a bounded walkable surface/height representation with simple collision volumes and limited slopes/steps. Handle the climb and mantle as explicit traversal modes; a general rigid-body character controller is not a prerequisite.

Define a consistent metres/up-axis/forward-axis convention and use Blender export conversion once. Test orientation rather than assuming the tools and runtime use identical axes.

### 5.4 Finale and UI integration

Separate `prologue reward completed`, `rescue available`, `forest entered` and `chapter endpoint reached`. Do not reuse one `finaleComplete` boolean for all of them. Enumerations exposed through JNI, captured state or saved data require deliberate migration, not ordinal renumbering.

Campaign play uses the night continuation. Historical roof/dawn RT Lab controls may remain clearly labelled diagnostic/legacy controls, isolated from campaign progression. Retain RT Lab unlock access after the reward, preferably through pause; repeated finale polling must never replace the lab or a dialogue/menu surface. Update old ending/retry/checkpoint contracts explicitly while preserving historical evidence.

## 6. Physical rope and traversal contract

Use a bounded XPBD-style rope solver within the fixed-step simulation [R4]. Initial implementation settings: one rope, up to 32 nodes, fixed upper anchor, distance constraints, modest bending resistance, gravity, damping, two substeps per 60 Hz tick and a small fixed iteration budget. Treat these as starting values; measure stability and cost before locking them.

Model the deployment starting configuration and the throw impulse; do not keyframe every rope node along a prerecorded sway. Use segment/capsule-style collision against simplified shaft/rim/ground proxies, with substeps or swept contact sufficient to prevent tunnelling. Full knots, rope cutting and arbitrary rope self-collision are outside scope. Choose a loose starting fold/deployment that does not require a knot solver.

Use a single continuous deformed tube mesh with stable topology and tangent frames, driven by node state. Prefer one refittable BLAS; compare a bounded segment-instance alternative only if it proves better on the actual hardware. Keep UV density/diameter stable, and avoid geometry seams, exploding frame rotation, per-frame allocations and rebuilding the entire forest because the rope moved. Refit legality and synchronization must be tested, not assumed [R5].

During climbing, progression is controlled by the traversal mode, with bounded grip/load constraints feeding the rope. This is a **dynamic rope coupled to authored traversal**, not a full dynamic human. The visible rope must take load, change tension and respond to release. Do not drive it with a cosmetic sine wave while calling it physics.

Required checks: deterministic repeatability on the same build, finite state after repeated deployment, bounded stretch under the configured load, no rim penetration, correct anchor, stable camera, no catch-up explosion after pause, reachable interact region while swinging, and multiple interactions consumed once. Initial visual target: settled segment stretch within 3% and no visible shaft clipping; document any tuned tolerance.

Pause freezes progression and the rope. Restart cancels the sequence and pending loads. Mid-climb application termination restores a safe pre-climb checkpoint; successful summit restore does not replay the reward or duplicate ownership. Once climbing starts, ordinary locomotion/attack/parry/dodge commands are consumed rather than buffered. Pause/back remains available; no accidental jump/fall command is introduced.

## 7. Companion, animation and dialogue

### 7.1 One reusable actor, not a new enemy exception

Add one friendly actor with a stable entity identity. Reuse existing skinned-asset/animation/render-slot infrastructure, but do not overwrite a skeleton or Lich slot without explicit lifetime ownership. Avoid widening combat enemy limits as a side effect. The forest requires only the player and one companion to be animated characters.

Default visual brief: an original practical adult human dungeon crawler, travel-worn layered clothing, restrained gothic detail, belt equipment, readable hands/face and a short coat or garment that skins well. No ornate full-body armor, long simulated cloak or complex hair. Final face, presentation and colors should be selected from a small reference set; use `Companion` as the internal/speaker working name until the owner names them.

Required animation behaviors: idle/breathing, look down, throw/release rope, recover/stand, approach, concerned talk, lantern reaction, turn, walk, wait and natural look-at. Reuse compatible clips, blend layers and author missing actions in Blender. The rope release is a semantic marker in the body action, connected to the same simulation event that releases the rope.

Walking follows an authored collision-aware route with stopping points and speed matched to stride. No advanced navmesh/companion combat system is required. Wait before disappearing around a bend when the player lags. Never visibly teleport through the player, trees or tomb; prevent blocking the only route. Look-at is bounded and blended, without head spins. Idle continues during conversation unless paused.

### 7.2 Voice deliverable

Ship actual offline voice audio for the mandatory exchange, with one consistent companion voice and one consistent player voice. Owner recordings, a licensed synthetic voice or authorized actors are acceptable. Do not clone or imitate an identifiable real person without authorization. Select/verify any provider and commercial distribution terms before paid generation; no provider, subscription or specific model is assumed by this plan.

Store lossless source recordings outside runtime packaging; produce normalized, trimmed mono runtime clips through the accepted audio pipeline. Record provider/performer permission, voice identifier, generation/recording date, source hash and derivative processing. No API keys, private account data or signed temporary download URLs in Git. Gameplay must run offline, with no live TTS or network request on an interaction.

Subtitles and temporary voice are valid development scaffolding, but **subtitles-only is not completion** of the scoped voiced scene.

### 7.3 Initial script and triggers

These are the implementation script defaults. Keep line IDs stable when the owner edits wording. Direction labels are not spoken.

| Line ID | Speaker / delivery | Text | Trigger |
|---|---|---|---|
| `rescue.found` | Companion, relief calling down | There you are! I thought that cave-in had buried you. | Roof sufficiently open; actor in position. |
| `rescue.rope` | Companion, practical | Hold on. Rope coming down. | After first line; rope throw prepared. |
| `reunion.question` | Companion, eager but believable | Did you find it? Tell me you found it. | Player safely at summit and companion at reunion mark. |
| `reunion.hint` | Companion, gentle reminder | Let me see. Raise it. | Once only, after a generous idle delay during the raise lesson. |
| `reunion.answer` | Player, tired satisfaction | I found it. | Fresh valid player raise action during the lesson. |
| `reunion.proof` | Companion, wonder | Then we're not chasing a story anymore. | After answer; lantern visibly presented. |
| `reunion.first_piece` | Player, grounded | It's only the first piece. | After companion reaction. |
| `reunion.depart` | Companion, quiet purpose | Good. Let's find the rest. | Exchange complete; begin trail-leading state. |

The hint is conditional, not an extra line forced into every playthrough. Do not require camera aim at the NPC for quest progress. Avoid playing the answer before the lantern has reached its presented pose. The story context can use a short objective such as `The Horde — follow your companion`; do not add a lore monologue.

### 7.4 Dialogue infrastructure

Use a small line manifest: stable line ID, speaker/entity, subtitle, audio asset, gesture, start condition, once-per-run/checkpoint policy and skip/completion behavior. A dialogue controller sequences state; platform backends play audio and report completion with a generation/line token. Stale completion messages must be ignored after pause/reset/reload or a skip. Audio duration/metadata provides a bounded fallback when playback fails, so missing audio cannot lock the game.

Subtitles default on, with speaker label, configurable size and opaque-enough scrim. Companion speech is world-positioned with distance/pan and appropriate interior-to-exterior treatment; player speech remains centered. Retain intelligibility, and do not bake tomb reverb permanently into a clip also heard outside. Use subtle animation/head/jaw response if the rig supports it, but cinematic phoneme-perfect facial animation is not required.

Dialogue can pause/resume with gameplay. Provide an explicit skip for the current spoken line; skip is not the gameplay Interact action and must not skip the player-controlled raise lesson. Missing optional audio falls back to subtitle timing and a diagnostic. Story events are exactly-once state transitions, never dependent on the player hearing the clip.

## 8. Forest graphics, sky, mist and fireflies

### 8.1 Composition and assets

Four modestly varied tree archetypes plus a few rocks, ferns, roots and ruin modules should create the first woodland. Use real 3D trunks, branches and canopy volume with instancing/LODs. Hero trees frame the tomb and moon. Reuse materials and geometry; variation comes from approved scale/rotation, clustering and layout, not hundreds of unique downloads.

Nearby trees cannot be billboard replacements. Farther scenery may use simpler **3D** silhouettes/LODs with truthful limitations. Do not silently replace the requested forest with a skybox forest image. Benchmark actual triangle, instance, material, texture and acceleration-structure costs. Camera-visible triangle count alone does not describe RT cost.

Opaque leaf geometry versus alpha-masked leaf clusters is a measured choice, not dogma. If alpha masking is used, implement/test consistent cutout visibility for primary, shadow and secondary rays. Avoid relying on a desktop-only micromap extension. Do not substitute transparency blending and call it equivalent. Keep mobile-critical alpha-tested overlap low.

Wind is gentle deformation of selected foliage/branches. Its RT geometry/bounds and shadows must agree. Use shared pose/deformation buckets or another bounded method when beneficial; do not refit every detailed tree independently by default. No camera-space waving texture pretending to cast physically moving branches.

### 8.2 Night environment

Use one night environment definition for the dungeon opening, transition and forest: visible sky/miss environment, moon disk/direction, moon illumination and exposure. Sample the same environment for relevant reflected/transmitted paths. Separate image appearance from calibrated illumination when importing an LDR sky image; it does not become a physically calibrated HDR environment by relabelling it.

Image generation may supply a seamless sky reference or texture only after projection/seam/color checks. An analytic sky with restrained stars and moon is a valid starting implementation. Volumetric clouds, astronomy and time-of-day progression are not required. Do not paint one moon into the sky and light from a different direction.

The forest must read as night, not daylight with a blue filter. Preserve dark depth, useful silhouettes and a clearly warm lantern. Use a stable exposure policy or bounded smooth adaptation with tests for pumping while raising the lantern or looking into the sky.

### 8.3 Actual participating mist

Generalize useful existing volume integration, replacing room-specific constants with bounded medium descriptors. The required outdoor effect includes world-space density, height falloff/local pockets, extinction/transmittance and illumination, not only distance color blending. Accumulate overlapping media coherently instead of double-applying unrelated fullscreen fog layers [R6].

Moonlight and lantern illumination use scene visibility through physical occluders. Real moon shafts must break behind trunks and the tomb rim. Light cones pasted into the view, radial screen-space shafts and unshadowed blue fog do not pass. No full fluid simulation is needed: an authored animated density field is appropriate.

Start with a bounded single-scattering approximation and a small explicit sample/light budget. Initial comparison: 8 view samples for Mobile and 16 for High, then tune from image quality and GPU evidence. A lower-resolution volume buffer with reconstruction is acceptable when it is genuinely derived from the world-space medium; test disocclusion and ghosting around hands, rope and trees. Do not introduce temporal accumulation without valid reprojection/history rejection.

Support fog on sky rays and consistent treatment along the important lantern-glass/reflective paths, with explicitly documented secondary limits. Do not composite the entire outdoor fog through the player's hands or allow it to vanish whenever glass is in view. Keep the Lich's existing mist behavior intact where not deliberately changed.

### 8.4 Fireflies, surface response and sound

Use bounded world-space fireflies with deterministic seeded motion, depth/geometry occlusion and restrained emissive appearance. Starting live counts: 48 Mobile / 96 High, with no more than two additional sampled local-light contributors. Record this lighting approximation; do not claim every insect fully illuminates the scene. No screen-following sparkle overlay or hundreds of independent shadow lights.

Damp bark, leaves, stone and a few wet patches should react to the lantern/moon. Reuse current material transport rather than creating per-object shader exceptions. A new stream, large reflective lake or waterfall is outside scope.

Forest ambience includes quiet wind/foliage, distant animal/insect calls, rope movement and leaf/stone footsteps. Sound transitions follow the actual world/listener position and pause correctly. Reuse existing audio event ownership.

## 9. Blender, Meshy and image-generation production workflow

### 9.1 Who does what

| Tool | Required appropriate use | Not an acceptable substitute |
|---|---|---|
| Image generation | Establish forest/companion/UI visual direction; create selected decorative texture/background inputs; coherent reference views for modeling. | A screenshot as the forest, a generated menu image as working controls, fake text baked into UI, or a static image claimed as runtime evidence. |
| Meshy | Generate/texture the selected humanoid and selected bespoke props when it improves output. Discover supported rigging/animation options. | Untextured export, blind import of high-poly assets, assumed facial acting or mandatory custom-motion API support. |
| Blender | Author cave-in, tomb/shaft/terrain kit and placement; clean/remesh/UV/LOD assets; repair rig/weights; author missing rope-throw/climb/gesture actions; export validated runtime data. | Blender-only simulation caches presented as live rope physics, cinematic renders as engine proof, or a new dependency on opening Blender during gameplay. |
| Existing native engine | Physical rope, authoritative traversal, rendering, lighting, volumetrics, interaction, playback and UI behavior. | Hardcoded camera tricks hiding incompatible assets or missing systems. |

### 9.2 Approval and cost gates

First generate a small coherent reference set: one forest reveal composition, one companion reference sheet, and one HUD/menu direction board. Use existing game/icon assets as references only after locating and inspecting the actual files. A named or remembered image is not an available input. Do not alter the established lantern silhouette/identity accidentally.

Default exploration cap: two candidates per reference category; select/refine rather than generating endlessly. For Meshy, start with one selected companion candidate and one corrective attempt before owner review of further paid work. Check whether API credits are separate from the owner's subscription. Check actual installed tools, API schemas, balances/permissions and versions; do not invent endpoints, model IDs or access. No mass generation before scene budgets and importer checks.

The owner should review direction at this small-art gate, not every low-level implementation decision. Continue independent technical tasks when an art decision or paid access is pending. Record a specific blocker rather than pretending an asset was generated.

### 9.3 Practical reference prompts

**Forest reveal:** Original historical-gothic first-person adventure environment. Looking from the lip of an ancient stone tomb into a dense moonlit woodland clearing. Roots over worn masonry, wet stone, ferns and fallen leaves, a narrow trail bending out of sight. Tall three-dimensional trees with a readable canopy, restrained ground mist and a few fireflies. Cold moonlight filtered by branches contrasts with the warm glass lantern carried by the player. Believable traversable stone rim and rope tie-off. Compose for both a tall phone crop and a wide desktop crop. No interface, lettering, logos, existing-game characters or impossible architecture. This is concept art, not an in-engine screenshot.

**Companion reference:** Original adult human dungeon-crawling companion, grounded historical-gothic clothing, practical worn leather and cloth, short coat, belt pouches, sturdy boots, uncovered readable hands and face. Clearly separated limbs in a neutral modeling pose. Coherent front, side and rear views of the same design, neutral lighting/background, no weapons crossing the body and no long loose cloak. Match the supplied approved visual references without copying another game's character. No text. Inspect view consistency before image-to-3D use.

**UI direction:** Restrained gothic exploration interface inspired by the game's own aged brass lantern, dark stone and parchment. Thin warm-brass frames, charcoal translucent panels, simple high-contrast action silhouettes, understated corner ornament, broad uncluttered touch regions and a clear central playfield. Show consistent idle, pressed, focused and unavailable states. No fantasy calligraphy for small text, no jeweled clutter, no glowing blue science-fiction panels. No generated words; real labels will be rendered separately.

These are starting briefs. Save final prompts and selected outputs. Use image editing only against an actual supplied/local target. Generated icons are references until cleaned to consistent vector geometry or transparent production assets.

### 9.4 Production asset pipeline

1. Inspect selected references, intended size, silhouette and needed animation before generation/modeling.
2. Generate a textured Meshy humanoid where appropriate. Discover current rigging/preset actions; reject unsuitable limbs/topology. Do not depend on the earlier conversation's custom-motion claim. The official API supports rigging and animation, but availability and formats must be rechecked [R1, R2].
3. Import into Blender. Normalize scale/orientation; remove hidden/internal garbage; fix normals, tangents, UVs, materials, weights and clipping. Author missing actions and hand sockets. Keep the physical rope separate from any decorative coil prop.
4. Author a modest reusable forest/tomb kit and placement scene. Prefer procedural Blender assistance for repeatable layout/variation where it produces good assets, not for an unbounded procedural world.
5. Create measured LODs and simple collision proxies. Bake high-to-low normals/roughness/material detail where useful; **do not bake scene illumination, moving-light shadows or reflections** into runtime appearance.
6. Export GLB plus placement/collision metadata through supported importer contracts. Constraints/modifiers/Geometry Nodes do not automatically become runtime systems; realize/bake the required mesh or animation data and test the export. Blender export support does not guarantee the engine consumes every glTF extension [R3].
7. Validate required clips, skeleton, loop boundaries, event markers, root motion policy, texture references, alpha modes, finite bounds, material count and coordinate conversion. Round-trip into the actual game.
8. Generate production ASTC/KTX2 or Windows assets through the existing packaging route. Record runtime decoded bytes, vertex/triangle counts, instance count and hashes, not only GLB file sizes.
9. Capture turntables/action tests in Blender for authoring review, then separate live RT captures/motion evidence on both platforms. Only the latter proves the runtime result.

Suggested new authoring locations: `assets/source/beyond_the_tomb/` for retained source art/Blender/voice provenance, `assets/models/environment/forest/`, `assets/models/npcs/companion/`, `assets/ui/gothic/`, `assets/audio/dialogue/`, and `assets/scenes/beyond_the_tomb/`. Adapt to the final baseline's accepted manifest conventions rather than inventing a competing importer. Keep large source files in configured Git LFS or the approved external source store; verify retrieval and exclude sources from APK/ZIP runtime packaging. Never commit provider credentials.

## 10. On-theme touch controls, HUD and menus

This is a mandatory workstream, not optional polish after the forest. The UI should belong to the same world as the lantern without compromising input, readability or accessibility.

### 10.1 Visual system

Use **warm aged brass + dark slate/stone + pale parchment**, with restrained cool accents reflecting moonlight. Starting design tokens: panel `#14191F`, elevated panel `#222930`, primary text `#F2E9D8`, secondary text `#C9C4B8`, accent brass `#D4B16A`, danger `#D96F65`. These are art starting points; verify contrast against the actual composited background and adjust.

Use a clear readable body/button font already licensed for distribution, with an optional restrained serif for large headings. No blackletter for settings, subtitles or small actions. Preserve the real game logo. Do not generate text into textures. Keep ornate detail to edges; a border is not the tap target.

Define tokens for typography, spacing, corner/frame thickness, safe-area padding, focus/pressed/disabled/selected states, touch hit sizes and panel opacity. Share the specification/data between platforms, with platform-native implementation. Avoid adopting an entire new UI framework just for this update.

Image generation can help with a subtle menu background, decorative stone/brass texture or motif. Convert final control symbols into clean scalable assets with consistent stroke, optical size and padding. Keep low-frequency ornament away from labels. No expensive live background blur by default; a scrim over the correctly paused world is sufficient.

### 10.2 Touch layout and semantics

Preserve left-side movement/strafe and right-side 360-degree look. Do not switch the control scheme to tap-to-move or a visible fixed joystick without owner approval. A subtle touch-origin ring is optional. Preserve all existing swing, press-down parry and directional-dodge semantics and whichever touch mapping the accepted baseline uses.

Use an intentional lower-right action cluster, a compact upper menu control and a quiet vitality display matching the existing three-point health system. Do not add decorative mana/stamina bars unsupported by gameplay. Give interaction and lantern Raise/Lower stable contextual positions; labels and states may change, but controls must not jump under a held finger.

Initial primary action hit target: around 64–72 dp, never below 48×48 dp for interactive controls. Visual ornaments may be smaller than their hit region. Keep meaningful separation and safe-area/cutout/system-gesture clearance. Android's 48 dp recommendation and text-contrast guidance are useful floors, not a reason to make action controls cramped [R7].

Required states: idle, press-down, unavailable/cooldown, keyboard/controller focus, contextual action and explanatory label. Use shape/label plus color, not color alone. Parry must activate on the intended down edge exactly once; do not also trigger on the later click callback. Keep attack timing unchanged by decorative animation.

Input tests must cover simultaneous movement + camera look + attack/parry, pointer reassignment, sliding off a button, `ACTION_CANCEL`, app focus loss, menu opening and resume. Consumed UI touches never leak into look/attack. Transparent visual areas may pass gestures only where intended. Maintain coherent mailbox publication and monotonic counters.

During climbing, show only useful traversal/pause information; suppress unavailable combat affordances without buffering their inputs. During the reunion show an explicit `Raise lantern` cue near its real control. Subtitles must avoid the action cluster, vitality, objective and lantern where possible. Hide touch-only chrome appropriately for mouse/controller use without hiding available contextual actions.

Required adjustable presentation: UI scale or compact/comfortable layout, HUD opacity within readable limits, subtitle size and reduced traversal/camera motion. A full drag-to-reposition HUD editor is deferred. Preserve saved settings and offer reset-to-defaults only as an explicit user action.

### 10.3 Menu coverage and accessibility

Refresh entry/main, pause, settings, controls/help, death/retry, optional chapter-end, credits and RT Lab framing. Keep diagnostics/benchmark/share/update functionality available and truthful, even if their dense technical text remains minimally decorated. Preserve `More by Samfa12`, explicit update behavior, quit/back and error/unsupported screens. OS file pickers and other system-owned dialogs remain native.

Settings must expose separate **Music**, **SFX/Ambience** and **Dialogue** volume controls, plus existing graphics/render-scale and control settings. Reuse any final 1.6.1 music implementation instead of installing a second audio engine. Retain Android Back and desktop Escape/controller navigation, visible focus, slider semantics, scrolling and persistence. Confirm sliders change actual runtime gain/value rather than just the drawn thumb.

Menu pause owns a single mutually exclusive surface/state; no recurring completed-finale poll may steal focus or replace settings, dialogue, the RT Lab or benchmark results. Closing a menu must not execute a touch that began while it was open.

Support system font scaling without changing the user's device settings. Test Android font scales 1.0, 1.3, 1.7 and 2.0, supported orientations/aspect ratios and safe insets. Prefer scroll/reflow over clipping. Test Windows 100%, 150% and 200% DPI, resizing, mouse, keyboard and controller. Give controls accessibility names/roles and expose real text to assistive technology. Target at least 4.5:1 for ordinary text and 3:1 for large text/relevant control boundaries [R7]. Do not assert accessibility from a single screenshot.

### 10.4 Implementation route

On Android, extract theme/control/panel helpers from `MainActivity.java` into focused classes/resources while preserving native view accessibility and established event routing. On Windows, use focused native menu/theme helpers and measured owner drawing only where necessary; preserve native slider, focus, scroll and controller behavior. The same design language does not require byte-identical rendering or a new cross-platform UI engine.

Use design reviews on captured **real phone/Windows screens**, including bright moon sky, dark tomb, lantern close-up and misty forest. An image-generated direction board is not UI implementation evidence.

## 11. Music, ambience and audio integration

The owner has separately planned Pocket Chordsmith adaptive music for exploration, torch loss, combat, Lich and roof opening. Treat this as owner context, not proof that a particular music pack or engine is already in the final code. Inspect final 1.6.1 and available assets first.

Extend the existing accepted music route with a short rescue/reveal transition and a restrained forest-exploration state. Preserve compatible musical motifs and avoid playing a second independent music engine. If the pack is absent, use the existing audio abstraction and record the missing integration input; do not invent a nonexistent asset or claim Chordsmith is integrated.

Dialogue should gently duck music/ambience and recover smoothly; volume zero, muted SFX and pause/resume must remain correct. Route foreground dialogue separately from ambient loops and existing combat feedback. Preload the small mandatory voice set to avoid line-start latency, within measured memory limits.

Do not delay the physical rope release indefinitely waiting for a missing voice callback. Conversely, avoid talking over the lantern reveal with a looping tutorial instruction. Voice timing, animation markers and simulation transitions must cooperate through line/event IDs, not fragile frame-number polling.

Every work package records `Audio/haptic manual revalidation required: YES/NO` with the change-triggered reason. The chapter's new voice, spatial ambience, changed listener height and mixing require YES for affected candidates. Pure UI decoration or documentation alone does not automatically require an audio/haptic check. Preserve existing haptics unless a deliberate reviewed change is made.

## 12. Save, replay and recovery

Add only the persistence required by this chapter. Use versioned, validated campaign state and atomic replacement, or extend the accepted save system if 1.6.1 has one. Preserve existing settings and do not confuse the persistent RT Lab-unlocked preference with complete campaign progress.

Required durable checkpoints: safely in the reward room with the lantern claimed and rescue available; safely at the summit; and the forest endpoint. Save logical state/IDs, not raw Vulkan resources or OS audio handles. Retain health, lantern ownership and relevant dialogue flags. Do not serialize a fragile instantaneous rope configuration as the only recovery path.

At a mid-climb termination, restart at the lower safe checkpoint with the lantern retained and a redeployed usable rope. At summit/endpoint restore, do not replay completed lines or re-award the lantern. Android Home/resume should normally preserve the current live state and pause correctly; process death uses durable checkpoints. Corrupt/newer saves produce a clear safe recovery choice without overwriting the only recoverable copy blindly.

Support repeatable Debug checkpoints for the new sequence, plus versioned deterministic replay. Existing scenario names/data remain historical evidence; append new fixtures and explicitly document intentional roof/dawn changes rather than rewriting old screenshots or silently accepting all differences.

## 13. Initial budgets and performance evidence

All values in this section are **proposed starting constraints**. Confirm or revise them with measured, recorded reasons at the early combined-scene gate. They are not guarantees, universal hardware limits or automatic pass/fail definitions.

| Resource | Initial bounded direction |
|---|---|
| Outdoor route | 40–80 m authored trail; clearing + 2–3 bends; no added combat population. |
| Companion | One actor; start near 10–20k rendered triangles at close LOD and a measured bone/material budget. |
| Trees | About four reusable archetypes; initial 60–100 placements across the authored footprint, with distance-appropriate real 3D LODs. Benchmark a dense representative view before scaling. |
| Rope | One rope, at most 32 simulation nodes, stable mesh topology, one preferred dynamic BLAS. |
| Volumes | A small bounded set of authored fog regions; initially compare 8/16 Mobile/High samples and the real cost of visibility queries. |
| Fireflies | Initial 48/96 Mobile/High live emitters, at most two additional sampled light contributors. |
| Textures | Reuse/atlas where appropriate; initially 1K shared environment maps and up to 2K hero maps, adjusted from actual texture quality/residency evidence. |
| UI | Small reusable texture/icon set; no continuous scene readback or expensive full-screen blur; source art excluded from runtime packages. |

Do not hardcode arbitrary new scene capacities throughout C++/GLSL. Extend checked generated metadata and resource inventory contracts with explicit overflow/failure behavior. Distinguish shared BLAS geometry bytes, instanced geometry, skinned refit buffers, TLAS/scratch, texture residency and staging peaks. Disabling a render instance does not prove its resources were freed.

Aim for a playable sustained phone profile and the established desktop quality goal, but preserve repository policy: report the 60/50/30 FPS reference bands rather than fabricating a pass. Investigate a matched regression above 15% in unchanged dungeon checkpoints. Changing preset/resolution, using fresh cooled runs or turning off required effects is not an optimization result.

Required measurements: final accepted baseline versus candidate at identical internal dimensions and preset/build type; warm behavior plus cold-start context; CPU frame timing, valid GPU intervals, rope simulation/refit, actor skinning, TLAS work, volume work where measurable, transition latency and resource peaks. Include full pixel dimensions, shader identities, device/driver, build/hash, checkpoint order, duration, charging state, temperature and thermal/power data when available. Do not call a median of window averages a per-frame median or substitute CPU-present time for GPU time.

Measure the **combined worst case** early: lantern glass raised near companion + dense tree view + fog + fireflies + wind. A fast empty forest and a fast isolated rope do not prove the integrated scene. Profile in native Release as well as diagnostic configurations before making player-facing performance claims. Persistent warm memory growth must be investigated across repeated entries/resets; two endpoint measurements alone do not prove leak freedom.

## 14. File and interface responsibility map

Existing file paths here were inspected or are established repository integration points. Proposed new paths are design suggestions, not claims that files already exist. Reconcile with the final baseline before creating duplicates; record justified adjustments in the execution log.

| Area | Existing integration | Proposed focused ownership |
|---|---|---|
| Campaign/vertical traversal | `src/gameplay/simulation/GameSimulation.cpp`, `SimulationSnapshot.h`, input/event contracts | `src/gameplay/world/ChapterProgress.*`, `src/gameplay/traversal/RopeClimb.*`; logical progress and 3D movement snapshots. |
| Zone lifecycle | `PresentableTinyRtScene.*`, `RtGpuResources.*`, `SimulationFrameAdapter.*` | `src/scene/world/WorldZone.*` and `src/vulkan/raytracing/WorldZoneResources.*`; CPU zone description and GPU lifetime kept distinct. |
| Rope | Shared fixed-step simulation and GPU resource helpers | `src/gameplay/physics/RopeSimulation.*`, `src/vulkan/raytracing/RopeRenderSlot.*`; state and continuous geometry/refit. |
| Companion | Existing character/player asset and render slots | `src/gameplay/actors/CompanionController.*`; extend reusable actor rendering rather than a hardcoded enemy impersonation. |
| Dialogue | Ordered `GameplayEvent` and existing platform audio | `src/gameplay/dialogue/DialogueSequence.*`, line manifest and platform playback adapters. |
| Atmosphere | `shaders/raytracing/minimal.rgen` and current includes/material ABI | Focused reusable environment/medium definitions and shader includes, compiled through current variant tooling. |
| UI | Android `MainActivity.java`; Windows `DiagnosticWindow.cpp` | Android theme/HUD/menu helper classes/resources; Windows native UI helper files; shared `assets/ui/gothic/` theme/asset specification. |
| Authoring | Existing asset importer/manifests and `ASSET_LICENSES.md` | `tools/blender/` repeatable author/export validation scripts plus retained source files. |
| Persistence | Existing preferences/checkpoint handling | Small versioned campaign save adapter, or extension of a verified existing one. |
| Evidence | `tests/`, existing Host/Android runners and compatibility records | Focused world/rope/dialogue/UI/save tests and new checkpoint fixtures; dated 1.7.0 evidence. |

Interface contracts to settle before parallel code work:

- **World readiness:** request generation + zone ID → preparing/ready/failed result. No successful publication until required collision and render resources are both ready.
- **Traversal snapshot:** mode, authoritative 3D pose, normalized progress, active rope/anchor ID, hand targets and carried-item socket. Renderer/UI never independently compute progress.
- **Actor snapshot:** entity ID, 3D transform, animation state/time, look target and gesture marker state. Stable identity across the rescue and woodland.
- **Dialogue command/result:** generation + line ID + speaker + asset → completion/skipped/failed. Exactly one accepted completion; safe subtitle fallback.
- **Theme/input:** style tokens are presentation; actions remain semantic commands and monotonic input edges. Platform focus/touch ownership is explicit.
- **Save checkpoint:** schema version, stable checkpoint ID, logical progression, player/lantern and dialogue state. No renderer pointers.

Do not freeze speculative C++ method signatures against an unfinished baseline. At the start of each work package, write its small concrete API/test contract in the execution log, verify neighboring consumers, then implement it. This is refinement of the scoped design, not permission to reopen every creative decision.

## 15. Ordered implementation work packages

Use a test-first loop for behavioral changes: write a focused failing test reproducing the required contract; run and inspect the expected failure; implement the smallest complete change; rerun focused and affected regression tests; inspect runtime output; review; commit only that work package. For visual work, combine asset/layout checks with genuine runtime captures and motion review—screenshots alone do not prove input or rope behavior.

Create `docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0-execution-log.md` when execution starts. Record baseline SHA, decisions, exact commands/results, current package, blockers and next unblocked package. Do not check boxes below from static inspection alone.

### WP0 — Accepted baseline, tool discovery and scope lock

**Reads:** This plan, `AGENTS.md`, final 1.6.1 decisions/memory, open work/validation, asset contracts, scene/input/audio/UI sources.

- [ ] Verify owner acceptance of 1.6.1; record its exact source and package identity. If still unfinished, limit work to safe planning/tool discovery; do not implement onto its active branch.
- [ ] Inspect Git status and existing worktrees. Start an isolated feature worktree/branch from the accepted baseline without discarding local changes. Import this documentation-only file if it is not yet present there.
- [ ] Establish fresh baseline tests/captures and inventory existing scene, vertical position, save, music and UI behavior. Do not hardcode historical CTest counts as current requirements.
- [ ] Discover Blender executable/version, image generation capability, Meshy API/credits, and voice-production options. Log unavailable/cost-gated capabilities precisely.
- [ ] Create the execution log, reconcile proposed file ownership and define initial combined-scene budgets. No release bump/publication.

**Gate:** A known-good source is protected; mandatory dependencies and baseline evidence are explicit. **Audio/haptic check:** NO for documentation/discovery alone.

### WP1 — Reference direction and reusable asset contracts

**Produces:** Small forest/companion/UI reference set; selected visual direction; source/runtime paths and validated import/export recipe.

- [ ] Use image generation for the three reference categories, within the cost cap. Locate actual game/icon references first. Mark all outputs as concepts.
- [ ] Create a small Blender tomb/shaft/terrain blockout and placeholder actor with real scale/clearance. Exercise export/import before commissioning detailed art.
- [ ] Write asset tests rejecting missing textures, unsupported animation/material data, invalid bounds and runtime inclusion of large source files. Run negative fixtures before fixing them.
- [ ] Obtain owner direction review of the small set. Record selected IDs/prompts; then start the chosen Meshy/Blender production route, not dozens of parallel candidates.

**Gate:** A coherent direction and working asset round trip, not merely attractive images. Technical WP2 work may proceed while a nontechnical art decision is pending.

### WP2 — World-zone and genuine vertical-transform foundation

**Touches:** Shared simulation/snapshots, frame adapter, scene/GPU resource ownership, collision and listener transforms.

- [ ] Add failing tests for a nonzero player height reaching camera, hands, lantern and listener consistently, with unchanged dungeon X/Z behavior.
- [ ] Add tests for stale destination readiness after reset, failed-load rollback and attempted destruction of resources still referenced by submitted work.
- [ ] Implement bounded zone descriptors/ownership and the 3D traversal/ground-support path. Keep the old dungeon rendered through its existing implementation or a thin adapter.
- [ ] Compare preloaded residency versus staged transition using blockout geometry. Test views up/down, reflection/shadow dependencies and peak residency before choosing a policy.
- [ ] Run existing dungeon regressions and exact-device presentation/lifecycle checks after meaningful renderer changes.

**Gate:** Moving between meaningful heights and zone states works on both platforms without faking camera movement or exposing invalid GPU lifetime. **Audio/haptic check:** YES when listener height/spatial inputs change.

### WP3 — Combined outdoor rendering feasibility

**Consumes:** WP1 blockout/import route and WP2 ownership. **Produces:** A small representative forest test scene and an evidence-backed resource decision.

- [ ] Assemble representative trees, the actual accepted lantern glass, one representative skinned actor and preliminary bounded fog/fireflies in the same camera view.
- [ ] Add rendering checks for matching sky/moon direction, 3D tree silhouettes, nonzero-height lighting and cutout visibility if used.
- [ ] Profile combined resource/timing peaks and repeated zone resets on the phone and Windows. Record actual internals/preset/build/thermal context.
- [ ] Settle residency, foliage representation and bounded atmosphere approach. Record sample/capacity changes with evidence rather than silently reducing the user's resolution.

**Gate:** The expensive combination is understood before scaling art. A failed gate triggers focused optimization, not a bigger forest or fake RT fallback.

### WP4 — Narrative geometry and continuation state

**Touches:** Spawn cave-in assets/collision, upper tomb geometry, `FinaleSequence`, campaign progress and platform ending/RT Lab routing.

- [ ] Add failing progression tests: no rescue before lantern ownership; roof opens once; campaign night does not trigger the old ending overlay; RT Lab cannot steal or lose menu ownership.
- [ ] Build the real cave-in, traversable shaft/rim, credible lid motion and exterior tomb dressing in Blender, then import and validate.
- [ ] Implement campaign continuation and shared night environment. Preserve historical diagnostic behavior behind explicit mode boundaries.
- [ ] Capture spawn turned around, roof opening, view through the shaft and upper-rim clearance. Confirm original encounters/chest flow still work.

**Gate:** The story geometry and night progression are coherent on the actual route. No floating dungeon/roof, empty void or reward bypass.

### WP5 — Rope simulation, grip and climb

**Consumes:** WP2 transforms/readiness and WP4 shaft geometry. **Produces:** Reliable action-driven ascent with dynamic rope and proper carried equipment.

- [ ] Write failing tests for anchored deployment, deterministic load response, bounded stretch, rim/ground contact, duplicate interact edges and invalid destination readiness.
- [ ] Implement the solver and continuous rope mesh/refit. Add runtime throw/contact/sway tests from multiple views.
- [ ] Add failing tests for command suppression during climb, safe mantle destination, stow/restore ownership, zero duplicate lantern awards and pause/restart behavior.
- [ ] Implement grip/mantle animation and stable/reduced-motion camera through the authoritative traversal snapshot. Attach lantern to the actual carry socket.
- [ ] Repeatedly climb, look down, pause, background/resume and restart on-device. Inspect hands, rope, shadows, glass and equipment in motion.

**Gate:** No physics substitute, floating/equipment conflict, camera-only climb, unreachable rope or transition soft-lock.

### WP6 — Companion production, voiced reunion and trail behavior

**Touches:** Selected Meshy/Blender actor, reusable actor controller/rendering, line manifest, dialogue/audio adapters.

- [ ] Produce and clean the textured companion, author/repair required clips and event markers in Blender, and verify them in the runtime, not just the asset viewer.
- [ ] Add failing tests for rope-release marker exactly once, NPC identity continuity, wait-for-player behavior and path nonblocking.
- [ ] Record/generate authorized voices, validate/normalize clips and license metadata, and implement generation-safe playback/subtitle sequencing.
- [ ] Add failing dialogue tests for missing audio, stale completion, pause/resume, line skip, repeated hints, and requiring a fresh reunion raise action.
- [ ] Complete the actual lower-call → throw → reunion → manual raise → depart scene, with world-space light/audio and bounded look-at.

**Gate:** All mandatory spoken lines play offline, the actor moves convincingly, and no audio/animation failure blocks progression. **Audio/haptic check:** YES for voice/spatial/mix changes.

### WP7 — Themed HUD and complete menus

**Consumes:** WP1 approved theme plus existing input semantics; includes new dialogue/traversal presentation from WP5–6.

- [ ] Capture the baseline controls/menus, then build shared theme tokens and cleaned scalable artwork. Implement Android/Windows focused helpers without changing gameplay timing.
- [ ] Write failing interaction tests for multi-touch move/look/action, parry down-edge duplication, pointer cancellation, controls moved under held fingers, UI-to-game event leakage and paused-input buffering.
- [ ] Apply the theme across all listed player-facing surfaces and preserve diagnostics/RT Lab/benchmark/update/credits access.
- [ ] Add actual Music/SFX-Ambience/Dialogue gain controls, subtitle size and required HUD/reduced-motion settings, preserving stored preferences.
- [ ] Validate supported phone layouts and font scales, Windows DPI/controller focus and real slider effects. Capture bright/dark backgrounds and verify readability.

**Gate:** The controls look intentional and are at least as usable as before. Accessibility and input/mixing checks are not waived because the theme looks good. **Audio/haptic check:** YES for mixing changes; NO for purely decorative changes with unchanged semantics.

### WP8 — Forest art, atmosphere, music and final trail hook

**Consumes:** Measured WP3 budget and playable WP4–7 systems.

- [ ] Complete the bounded trail, tree/ruin kit, 3D LODs, natural boundaries and hero compositions using the Blender production route.
- [ ] Finish world-space shadowed moon/lantern volumetrics, bounded wind/fireflies and consistent important glass/sky paths. Add occluder-on/off comparison captures.
- [ ] Add forest/rope/footstep ambience and integrate the accepted adaptive music route, with tested ducking and volume-zero/pause behavior.
- [ ] Add the final marker/gate and quiet story hook. Companion waits; control remains available; no automatic fullscreen completion takeover.
- [ ] Run the combined worst-case and traversal/regression route again after final art—not only with placeholders.

**Gate:** The finished scene delivers the promised woodland, not just a technical blockout. More density is earned by performance evidence, not assumed.

### WP9 — Durable checkpoints, interruption and upgrade safety

**Touches:** Existing/new campaign save adapter, traversal/dialogue generation handling, platform lifecycle and settings migration.

- [ ] Write failing tests for corrupt save, version mismatch, atomic replacement interruption, duplicate reward, mid-climb termination and restored summit dialogue flags.
- [ ] Implement/migrate the minimal checkpoint schema and checkpoint restore contract; preserve settings and RT Lab unlock independently.
- [ ] Verify Android background/resume and process-death recovery during voice, rope deployment, climb, reunion and forest walking. Verify desktop exit/relaunch and focus changes.
- [ ] Repeat transitions/resets while observing resource high-water marks and delayed events. Reject stale loads/voice callbacks from previous generations.

**Gate:** A interrupted short phone session cannot erase the reward, replay all progress or strand the player. No raw renderer handles in saves.

### WP10 — Integration, acceptance and handoff

- [ ] Run clean Debug/Release tests, current shader freshness/variant gates, Android package/lint/asset checks and deterministic route validation.
- [ ] Capture and review the new evidence scenes and actual motion listed below on both platforms, with exact artifact hashes and honest runtime presentation.
- [ ] Compare unchanged dungeon checkpoints against the accepted baseline under matched conditions; investigate significant regressions and warm resource growth.
- [ ] Complete required owner art/touch/audio checks on the exact candidate. Document remaining limitations without upgrading automated checks to owner acceptance.
- [ ] Update decisions/memory, licenses, device compatibility evidence and release notes to reflect what is actually implemented. Use the final version contract; do not rewrite historical version evidence.
- [ ] Produce the reviewed source and local test packages with a clear status. Stop before signing, tagging or public upload unless explicitly authorized under the existing release process.

**Gate:** All mandatory acceptance items pass or are explicitly listed as unresolved; no silent scope reduction and no unsupported completion claims.

## 16. Verification matrix and commands

### 16.1 New scenarios

Add stable, versioned fixtures/checkpoints along these lines, adapting names to the final capture schema:

`opening-cavein`, `rescue-roof-night`, `rescue-rope-deploy`, `rescue-rope-ready`, `rescue-climb-mid`, `rescue-summit`, `reunion-lantern-low`, `reunion-lantern-raised`, `forest-dense-mist`, `forest-trail-bend`, `forest-endpoint`.

Cover the legacy dungeon route as well. Verify the exact visible source/zone, stable presented frames, shader identity and artifact hashes. Scene-only captures and UI-on captures are separate. New deterministic particles/wind/rope use controlled seeds and imported snapshot state; do not require byte-identical images across different GPUs without justification.

| Test domain | Required acceptance evidence |
|---|---|
| Progression | Complete natural route, lantern required, rope available once, reunion requires fresh raise, endpoint reachable without an old ending overlay. |
| Physics/movement | Real rope impulse/load/contact/release, authoritative height, safe mantle, valid carried-item sockets and usable reduced-motion climb. |
| RT continuity | Same sky/moon through opening and outside; look-down continuity; correct glass/shadows at transition; no frustum-only culling artifacts. |
| Atmosphere | Shafts blocked by real geometry, depth-clipped fog, credible sky/hand/glass interaction, no screen-following mist/fireflies. |
| Companion/voice | Runtime clip blending/foot placement, stable identity, no path blocking/visible teleport, offline speech, subtitle/skip/missing-audio safety. |
| UI | Multi-touch action while moving/looking; font/DPI/inset coverage; readable themed states; focus/Back/Escape/controller operation; no input leakage. |
| Recovery | Pause/resume, process death, save corruption/version handling, repeated restart/transition and stale-generation rejection. |
| Performance | Matched baseline/candidate, combined worst case, warm behavior, explicit pixel/preset/build identity and resource peaks. |
| Packaging | Correct version source, licenses/provenance, no secrets/source-art bloat, shader freshness and actual packaged runtime assets. |

Automated motion tests and screenshots do not replace a human checking climbing comfort, readable thumb placement, companion acting and voice balance. An unconnected phone is a named verification blocker, not a fabricated pass.

### 16.2 Existing commands to confirm against the accepted baseline

These entry points existed during inspection. Read their current parameters before use; do not copy a stale fixed test count or assume all require identical local tooling.

```powershell
# Repository root: discover and run established desktop tests.
cmake --list-presets
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release

# After changing shaders, use the accepted variant-generation workflow.
.\tools\compile-raygen.ps1
.\tools\compile-raygen.ps1 -Check

# Inspect these runners' current parameter blocks, then invoke their supported gate.
Get-Help .\tools\run-foundation-validation.ps1 -Detailed
Get-Help .\tools\run-android-showcase-validation.ps1 -Detailed

# Android build/lint from the android directory.
Push-Location android
.\gradlew.bat assembleDebug lintDebug --console=plain
Pop-Location
```

Blender discovery can use `blender --version` where configured, followed by a repository-owned headless export/validation script using the installed executable. Do not invent a working Blender path or distribute absolute owner-machine paths as universal instructions. Register new focused C++/asset/UI tests with the appropriate existing build/test harness; selecting a test name that does not exist is not a successful validation.

## 17. Astra medium execution rules

Treat the owner's model choice as the operating context, not a product benchmark. No specific subagent model, API access or reasoning-setting mechanism is assumed.

1. **Read the whole master handoff once, then work from the current package and log.** Avoid repeated full-repository audits. Inspect the smallest relevant sources and neighboring consumers.
2. **Protect the baseline.** Start runtime work from the completed accepted 1.6.1 state, not the older public main solely because this plan lives there. Never merge the unfinished engineering branch automatically.
3. **Use concrete package contracts.** Before each package, settle its public data/API and failing acceptance tests against the actual baseline. Keep files focused and do not turn `PresentableTinyRtScene`, `GameSimulation` or `MainActivity` into larger feature dumps.
4. **Delegate sparingly.** At most two non-overlapping implementation workers by default, after interfaces are frozen. Good independent work: UI artwork cleanup, asset validation, bounded host tests. The lead owns world-state/render/input integration. Do not have two workers edit shared shader ABI, simulation snapshots or native bridge contracts concurrently.
5. **Review before integration.** Use a fresh focused review for resource lifetime, input races, progression and changed shader paths. Available subagents are optional; perform an explicit local review when absent. Do not claim unseen reviewer/test results.
6. **Prefer a measured simple design.** Do not implement both an elaborate streamer and a complete alternative UI framework to avoid one focused experiment. Do not code speculative future chapters.
7. **Use tools actively but honestly.** Blender must produce/repair real source assets; image generation must be called for actual selected art work when available; Meshy outputs must be textured and validated. Record real job IDs/results. Missing tools are blockers or reasons to use a genuinely available approved path, not permission to invent completion.
8. **Keep cost bounded.** Reuse accepted assets, record credits/generation attempts and avoid duplicate paid jobs on uncertain timeouts. Do not assume named services are free or installed. Ask only for non-resolvable permissions, paid-budget decisions or art acceptance; continue unrelated work meanwhile.
9. **Do not confuse placeholders with deliverables.** Temporary actors, silent dialogue and blockout trees keep development moving, but cannot pass the final art/voice requirements.
10. **Leave durable progress at each boundary.** Record files, source SHA, tests actually run, screenshots/motion inspected, measurements, limitations, audio/haptic revalidation status and the next unblocked package. When a session ends, leave a coherent commit and log, not a claim that remaining work is running in the background.

Do not guess completion time or promise a single session will finish this programme. The task is one scoped update made of separately reviewable work packages.

## 18. Ready-to-use Codex instruction

```text
Read docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md in full.
It is the approved scoped direction and master handoff for the next update,
including Blender/image-generation/Meshy production, physical rope traversal,
the moonlit forest, voiced companion and the themed touch HUD/menus.

First verify that 1.6.1 is completed and owner-accepted. Protect existing local
work and use an isolated feature branch/worktree from that accepted baseline.
The plan was saved separately on main; obtain the documentation without
merging or replacing unfinished runtime work. If 1.6.1 is still unfinished,
report the start-condition blocker and restrict yourself to safe planning
and tool discovery rather than changing its implementation.

When the start condition is satisfied, execute WP0 onward in gated, tested
vertical slices. Use Astra medium as selected. Keep a durable execution log,
use bounded delegation only where available and useful, and make focused
commits. Derive exact interfaces/tests from the final source rather than
copying historical counts or guessed APIs. Reuse the existing engine and
accepted Chordsmith/audio work where present; do not create parallel systems.

Use Blender and image generation for the concrete production tasks in the
plan, not only as recommendations. Verify Meshy/voice capabilities and credit
permissions before paid work. Respect the art-review and device-validation
gates. Maintain real hardware RT on Android and Windows. Do not silently
replace physical/world-space systems with visual tricks or declare temporary
assets complete. Preserve existing input, feedback, diagnostics and RT Lab.

Continue through unblocked approved packages, recording what was actually
verified, what remains blocked and the next action. Do not tag, sign or
publish a release without separate explicit authorization.
```

## 19. Technical references and evidence boundaries

External references were checked for this planning document on 11 September 2026. They support the relevant tool/engineering principles; **they do not prove this game's implementation, device performance or account access**. Verify current schemas and installed versions again at execution. No external game's art, characters or interface should be copied; the owner's Blades comparison concerns bounded mobile-friendly exploration, not asset reuse.

- **R1 — Meshy Rigging API:** <https://docs.meshy.ai/en/api/rigging>. Standard textured humanoid rigging is the dependable starting assumption; inspect current input constraints.
- **R2 — Meshy Animation API:** <https://docs.meshy.ai/en/api/animation>. Discover supported actions and returned formats. Search/open snapshots differed on custom-motion support during planning; this plan intentionally does not depend on it or on a claimed launch date.
- **R3 — Blender glTF export documentation:** <https://docs.blender.org/manual/en/4.0/addons/import_export/scene_gltf2.html>. Used for the stable mesh/material/skinning/export principles; the executor must use documentation matching the actually installed Blender version, not install 4.0 because of this reference.
- **R4 — XPBD, original author overview:** <https://blog.mmacklin.com/2016/09/15/xpbd/>; Macklin, Müller and Chentanez, DOI <https://doi.org/10.1145/2994258.2994272>. A suitable compliant constraint-solver basis, not a requirement to import a large physics engine.
- **R5 — Vulkan acceleration structures:** <https://docs.vulkan.org/spec/latest/chapters/accelstructures.html>. Respect build/update compatibility and resource lifetime; do not assume topology/count changes are legal refits.
- **R6 — PBRT, Transmittance:** <https://pbr-book.org/4ed/Volume_Scattering/Transmittance>. Physical basis for extinction and accumulated transmission; the scoped renderer remains a bounded real-time approximation, not a claim of unbiased full volumetric path tracing.
- **R7 — Android accessibility guidance:** <https://developer.android.com/guide/topics/ui/accessibility/apps>. Touch target and text visibility guidance; preserve the project's existing Views implementation unless a separately justified migration is approved.

Repository evidence for the planning snapshot is available at commit `191d799ab7fda54fab36d9792b11f908ddaaf42d`, especially the files in Section 1. This document introduces no new runtime evidence and changes no existing release, benchmark result or owner acceptance record.

---

**Final acceptance statement:** The dungeon now feels like a prologue; the rescue physically carries the player into a convincing moonlit woodland; the animated, voiced companion and player-controlled lantern reveal establish the larger adventure; the touch controls and menus belong visually to that world; and the engine gains reusable, tested capability without sacrificing honest hardware RT or phone usability.
