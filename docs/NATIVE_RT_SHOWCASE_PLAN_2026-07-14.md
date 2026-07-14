# Native RT showcase plan - 2026-07-14

## North star

Turn Horde Lantern RT into a downloadable 60-90 second guided showcase whose memorable fact is that the same native Vulkan hardware-ray-traced scene runs on a phone and Windows RTX.

This should be a short authored route, not a broader game. Each room proves one RT idea, composes it with everything already learned, and remains runnable on the target phone after every slice.

## Visitor route

1. **Torch corridor** - the existing opening: moving emissive torch, genuine shadow visibility, wet stone, puddle reflection, fog, and first bounce.
2. **Materials gallery** - a stone demonstration table with angled slabs for dry stone, wet cobble, mossy masonry, damp ground/puddle, aged metal, mirror, clear glass, and stained glass.
3. **Mirror threshold** - one large framed wall mirror reveals off-screen architecture and proves player-arm, sword, torch, and skeleton reflection.
4. **Glass exhibit** - bounded thin clear and coloured panes prove genuine transmission/refraction without pretending to solve thick nested dielectrics.
5. **Final crypt** - the existing animated skeleton is seen first in the mirror or through stained glass, wakes, and enters the existing one-enemy attack/death loop.

The strongest final composition is player arms and torch reflected in the mirror, the skeleton visible through a coloured pane, and a real torch shadow crossing the room.

## Production order

### Slice 0 - preserve the articulated baseline: complete

- Grip-locked, pitch-relative arms/torch/sword are live as real RT geometry.
- Android all-ABI and Windows builds pass; the exact Android build selected ASTC and reached honest RT swapchain presentation.
- Sustained warm phone evidence is approximately 50-52 FPS at thermal status 2. This is the minimum baseline and provides little spare query/shader budget.

### Slice 1 - explicit material plumbing and gallery table

- Replace fragile world-material selection by primitive-number ranges with a compact per-triangle material ID path.
- Reuse the existing five ASTC texture layers; do not import another large batch yet.
- Build one bounded table/plinth set with consistent lighting angles so roughness, metallic, normal strength, wetness, and albedo differences are obvious at a glance.
- Add mirror and glass identifiers as routing types, even before their expensive behavior is enabled.
- Gate: Windows composition, Android ASTC selection/RT presentation, then warm 50+ FPS median.

### Slice 2 - framed mirror wall

- Add one large, deliberately framed mirror that reveals content outside the direct view.
- Keep exactly one reflected ray segment: no mirror-in-mirror and no recursive pipeline depth increase.
- Directly light the reflected hit so the mirror shows a readable scene rather than only a dim base/emissive approximation.
- Pay the extra torch-visibility query only on mirror pixels. Reclaim budget first by skipping the current bounce query on low-reflectivity diffuse pixels.
- Include held props and player body in the reflection mask; retain geometric-normal origin bias.
- Gate on phone before adding glass.

### Slice 3 - honest thin-pane transmission

- Add a separate static glass BLAS/TLAS instance and ray mask (planned `0x08`) so ordinary opaque traversal stays simple and fast.
- On a primary glass hit, use bounded Schlick Fresnel plus one transmitted/refracted query and a constant thin-pane absorption tint.
- Start with one clear pane and one stained pane. Market this honestly as thin dielectric transmission.
- Defer thick crystals, multiple entry/exit surfaces, nested dielectrics, dispersion, caustics, and repeated internal bounces.
- Add coloured transmission to shadow queries only if the phone budget survives the primary proof.

### Slice 4 - final crypt reveal

- Keep exactly one 9,402-triangle skeleton and its current 30 Hz CPU skin/BLAS refit cadence.
- Trigger it in the final room rather than expanding enemy count or AI.
- Stage the reveal so the mirror or glass shows the skeleton before direct line of sight, then reuse the existing approach/attack/death/respawn loop.
- Final phone gate must include mirror/glass visibility while the skeleton animates and the sword swings.

### Slice 5 - downloadable demonstration release

- Replace the oversized instructional HUD with compact effect labels and a collapsible panel.
- Show `Native Vulkan hardware RT active` only when the real capability mode and honest RT presentation state support that claim.
- Add guided reset and benchmark modes so reviewers can reproduce the route and performance sample.
- Produce a signed Android release APK and a self-contained Windows zip.
- Publish device compatibility, controls, known unsupported diagnostics, build notes, APK/zip SHA-256 hashes, screenshots/video, and the asset-license manifest.

## Hard technical boundaries

- Preserve `vkCmdTraceRaysKHR` presentation and the phone-safe `rayQueryEXT` work inside raygen.
- Do not retry pipeline recursion depth 2 as part of the showcase route; it already failed phone pipeline creation.
- Existing geometry and rays are opaque (`VK_GEOMETRY_OPAQUE_BIT_KHR` / `gl_RayFlagsOpaqueEXT`). Route thin glass explicitly instead of making every traversal non-opaque.
- Mirror and glass logic must execute only for pixels that hit those materials. The current shader already pays for primary, torch visibility, moon visibility, and a bounce.
- Keep the embedded raygen below the observed mobile shader-size/occupancy cliff and verify phone pipeline creation after every shader change.
- Preserve one frame in flight while TLAS instance data remains host-written.
- Preserve the warm 50+ FPS median gate after every slice. If headroom is needed, reduce bounded effect area or offer genuine-RT quality tiers; do not substitute fake reflections or transparency.

## Immediate next implementation

Build Slice 1: compact explicit material IDs plus the materials gallery table. It gives the demo a clear exhibit immediately and creates the clean routing foundation required by both mirror and glass.
