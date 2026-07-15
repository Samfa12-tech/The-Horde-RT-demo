# Coloured-light dungeon route plan - 2026-07-15

## Outcome

Extend Horde Lantern RT into a short authored dungeon route that demonstrates moving hard shadows, loss of light, soft cool skylight, coloured direct lighting, coloured transmission, reflection, and an animated emissive threat. This is still a native RT showcase rather than a larger combat game.

The emotional sequence is the organising idea: **warm safety -> blackout -> cold isolation -> controlled colour study -> luminous threat**.

This plan extends `docs/NATIVE_RT_SHOWCASE_PLAN_2026-07-14.md`. It retains the bounded mirror and thin-transmission work, but replaces the abstract gallery/final-crypt route with a more dramatic walk through the dungeon.

## Visitor route

1. **Zig-zag shadow corridor**
   - Extend the existing dungeon through several offset turns, arches, bars, chains, or broken masonry.
   - Let the held lantern remain the dominant local light so each turn throws clearly changing silhouettes across the next wall.
   - Compose occluders for large readable shadows rather than adding geometric clutter.

2. **Lantern failure**
   - At the final bend, the flame gutters out, the player releases the lantern, and the left arm lowers.
   - The lantern becomes real dropped RT geometry on the floor. Its emissive and direct-light contribution both reach zero; do not leave an invisible light behind.
   - Use a short authored state transition: grip-held, extinguishing, released/falling, settled. Exact hand IK remains locked until the release frame, after which the left arm blends to a lowered pose.

3. **Blue skylight chamber**
   - The blackout opens into a room lit only through one physical roof opening by soft desaturated blue light.
   - Reuse the existing directional roof-gap visibility route. Make softness bounded with a very small area/direction sample set or other phone-measured approach; do not introduce full participating media or a fake screen-space shaft.
   - Use the dropped lantern and completed lower body as silhouettes/reflection subjects.

4. **Chromatic torch passage**
   - Continue through a sequence of torches spaced approximately five metres apart: yellow, blue, then a small curated progression such as red and green.
   - Each bay should reuse comparable stone, metal, wet, and cloth/leather surfaces so the colour change is unmistakable.
   - Geometry may show every torch, but lighting must not scale with the total torch count. Select the nearest or current bay light, with at most a tightly measured two-light crossfade at a boundary.
   - Keep colours plausible and readable: warm yellow, saturated cool blue, deep red, and restrained green. Avoid a rainbow arcade look.

5. **Stained-light threshold**
   - Put one coloured transparent pane or hanging glass screen between a torch and a receiving stone surface.
   - The primary view keeps the existing honest thin-pane transmission model. Add coloured transmission to a bounded shadow query only if the phone budget survives; label it as thin coloured transmission, not volumetric caustics.

6. **Reflection/water proof**
   - Use one framed, one-segment hero mirror to reveal off-screen architecture, the player's now-complete body, or the final threat.
   - Add a small shallow flooded strip or pool only after the mirror and transmission gates pass. It may use a bounded reflective/transmissive surface, but not waves, repeated refraction, caustics, or a full water simulation in this phase.
   - Do not build a room of mutually facing mirrors. Mirror-in-mirror would either exceed the one-segment proof or misrepresent what the phone path computes.

7. **Emissive enemy finale**
   - The proposed placeholder is the staged Meshy lich at `assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb`; do not run it concurrently with the skeleton by default.
   - The audited asset has one 9,188-triangle skinned primitive, embedded 2K base-colour and emissive textures, a 24-joint biped rig, and nine clips. See its adjacent metadata file.
   - Its robe, body, and staff are fused, with no staff or cloth bones. Staff bending and robe deformation are accepted placeholder faults, not renderer tasks; replace or re-rig it if they remain distracting.
   - Import the model only after its licence, triangle count, texture sizes/formats, animation clips, and mobile BLAS-refit cost are audited.
   - Split the effect into two honest parts: an emissive material makes selected model surfaces appear self-lit, while one or a few compact light samples attached to named bones or authored local points make it illuminate the room and cast RT shadows.
   - Light sampling follows the animated pose without turning every emissive triangle into a light. Reuse the existing one-enemy approach/attack/death loop unless the asset forces a narrowly scoped adaptation.

## Player completion

Legs are promoted from deferred polish to a prerequisite for the new route because the dropped-lantern beat, lower camera views, wet floors, and mirror reveal will expose the missing lower body.

- Add a low-cost pelvis plus upper/lower legs and simple feet using reusable low-poly BLAS geometry and selective body mask `0x04` traversal.
- Start with authored camera/yaw-relative walk poses or a compact procedural gait. Do not begin with a fully skinned first-person character asset.
- Keep the camera origin outside body geometry and suppress self-intersections in primary and reflected rays.
- Preserve the current grip/pitch system for arms. The left-arm lowered pose is a state layered onto that system, not a rewrite of it.
- Judge the slice from downward view, moving shadow, puddle/mirror reflection, lantern release, and phone cost. Hands and richer clothing remain separate art polish.

## Recommended production order

### Slice A - route blockout and light choreography

- Greybox the zig-zag, skylight room, torch bays, transmission threshold, and final room.
- Define trigger volumes, reset behaviour, collision, approximate five-metre spacing, and which light is active in each bay.
- Validate that the full walk reads at the current lighting cost before adding new renderer behavior.

### Slice B - lower body and lantern state transition

- Add phone-bounded legs/feet and selective ray visibility.
- Implement extinguish, release, settled lantern, and left-arm lowering as one deterministic sequence.
- Gate Android/Windows build, honest RT presentation, visual continuity, collision/reset, and warm phone performance.

### Slice C - zig-zag shadows and blue skylight

- Author large moving shadow compositions with existing direct-light visibility.
- Tune one bounded soft blue skylight through physical geometry.
- Avoid new broad per-pixel work until this composition is accepted.

### Slice D - coloured torch selector

- Add the curated torch colours and bay-based nearest-light selection.
- Prove correct light colour on diffuse, wet, metal, body, and enemy materials.
- Measure the two-light boundary crossfade separately; fall back to one selected light if it breaks the phone gate.

### Slice E - thin coloured shadow transmission

- Retain the existing thin-pane primary transmission.
- Add one bounded coloured-shadow proof only in the exhibit region and only if shader size and frame time remain safe.

### Slice F - hero mirror, then optional shallow water

- Build exactly one framed mirror with one reflected segment and deliberately useful off-screen composition.
- Add the shallow pool only if it contributes a visibly different proof from the existing wet floor and survives the phone gate.

### Slice G - emissive enemy replacement

- Audit and stage the supplied asset, then integrate it through the existing one-enemy slot.
- Add emissive material IDs first; add bone/local-point direct-light sampling second.
- Finish with shadow readability, attack/death-state lighting, and a warm phone run containing animation, mirror/transmission where visible, and combat.

### Slice H - guided showcase release

- Add compact effect labels, reliable route reset, benchmark mode, signed Android package, Windows zip, device notes, hashes, captures, and asset attribution.

## Priority of optional RT proofs

1. **Coloured transparent shadow** - highest value because it combines the new colour route with genuine visibility/transmission and already has a bounded thin-glass foundation.
2. **One hero mirror** - high value and honest within the existing one-reflected-segment design; it also exposes the completed player body and final enemy before direct sight.
3. **Small shallow water area** - worthwhile only if it adds transmission/depth or a strong animated reflection beyond the existing puddles.
4. **Room of mirrors** - reject for this phase. Multiple facing mirrors imply repeated reflection that the phone-safe route deliberately does not compute.

Other low-cost showcase ideas worth considering after the core route works:

- a rotating barred gate or slowly swinging chain casting moving coloured shadows;
- a narrow stained pane whose projected colour crosses wet stone and the player's legs;
- emissive enemy light changing or dying with its animation state;
- an off-screen enemy silhouette visible first in the hero mirror;
- a small metal object moving from yellow to blue bays to make roughness and colour response easy to compare.

## Technical and scope boundaries

- Preserve `vkCmdTraceRaysKHR` presentation and phone-safe `rayQueryEXT` work in raygen; do not retry recursive pipeline depth 2 for this route.
- Current warm articulated evidence is only about 50-52 FPS and the raygen is close to the observed mobile shader-size cliff. Every effect must be spatially/material bounded and phone-gated.
- Do not loop over all coloured torches per pixel. Torch count in the level is not light count in the shader.
- Emissive appearance alone does not prove emitted direct light. Keep the visible emissive material and sampled room-light contribution consistent in position, colour, intensity, and animation state.
- Preserve the presentation-format-driven red/blue output swap so warm/cool showcase colours remain truthful on BGRA swapchains.
- Preserve one frame in flight while host-written TLAS instance data is shared without per-frame ownership.
- Keep glass separately routed (planned mask `0x08`) and keep ordinary traversal opaque.
- Record all supplied/imported asset licences in `ASSET_LICENSES.md` before release.
- Do not expand enemy count, AI breadth, block/dodge, or unrelated gameplay during this route. The new enemy supersedes the skeleton finale unless a later measured plan explicitly budgets simultaneous enemies.

## Remaining enemy decisions

- Inspect the embedded emissive texture and decide which surfaces should glow and whether their colour/intensity changes by animation state.
- Choose visually acceptable clips. The asset has idle, walk, running, turning, pain/fall, and death animation, but no dedicated attack, cast, or staff clip.
- Is the intended finale a replacement for the skeleton, or is the skeleton seen earlier and despawned before the emissive enemy becomes active?
- What is the mobile triangle/texture budget after measuring the completed route?
