# Phase Plan

## Phase 0 - Vulkan RT Capability Probe

Goal: prove the target devices expose the required Vulkan hardware RT support.

Deliverables:

- Clean repo scaffold.
- Android-first native Vulkan build path.
- Windows native Vulkan build path.
- Vulkan startup capability query.
- On-screen diagnostic overlay plan.
- JSON/text capability report plan.
- Unsupported screen if RT is unavailable.

Current scaffold status:

- Documentation and source layout exist.
- Skeletal C++ data structures exist.
- Real Vulkan capability probing is not implemented yet.

## Phase 1 - Minimal hardware RT scene

Goal: render a tiny scene using actual Vulkan RT.

Deliverables:

- Storage image output from ray tracing.
- Ray generation, miss, and closest-hit shaders if `RayTracingPipeline` mode is available.
- Ray query path only if needed and only if it genuinely uses Vulkan hardware ray traversal.
- Minimal torch-lit room or wet-stone test box.
- Swapchain presentation.
- Diagnostic overlay remains visible.

## Phase 2 - Torch corridor visual prototype

Goal: first real aesthetic proof.

Deliverables:

- Dark stone corridor.
- Fire torch or lantern light.
- Wet stone floor.
- Puddles.
- Fog/smoke.
- Moving shadows.
- Phone-first performance testing.

## Phase 3 - Movement and simple combat shell

Goal: make the demo playable without overbuilding.

Deliverables:

- Movement controls.
- First-person or third-person camera.
- Simple attack.
- Block.
- Dodge if feasible.
- 10 enemies only at first.

## Phase 4 - Asset upgrade

Goal: replace test geometry with high-quality commercial-safe assets.

Deliverables:

- Textured Meshy or commercial-safe main character if third-person.
- Textured Meshy or commercial-safe goblin/gremlin enemies.
- Improved corridor/ruin environment.
- Asset license manifest updated for every asset.

## Phase 5 - Playable route

Goal: a short start-to-finish level.

Deliverables:

- Tunnel start.
- Corridor traversal.
- Open ruined courtyard/colosseum transition.
- Horde encounter.
- Start/end states.

## Phase 6 - Samfa12 release package

Goal: public tech-demo release.

Deliverables:

- Windows build.
- Android install package.
- samfa12.com download page.
- Screenshots/video.
- Clear device requirements.
- SFX and Pocket Chordsmith BGM.
