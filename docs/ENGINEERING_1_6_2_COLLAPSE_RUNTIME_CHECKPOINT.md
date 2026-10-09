# Approved collapse: static runtime admission checkpoint

3 October 2026. Owner approved revision2 Form for runtime export. Art owner owns
the editable Blender source, export, map receipts and provenance. Renderer source
implements only ordinary static-PBR admission and the physical native roof/floor
handoff. In-game Form, performance and device acceptance remain separate gates.

## Current source

- Required Showcase asset: `assets/models/world/runtime/collapsed-entry/`
  `asset.manifest.json` and `collapsed-entry-lod0.runtime.glb`. GraphicsPreview
  returns through its existing compact loader before this admission.
- One unique Static geometry asset, two primitives/material families ordered
  `Boulder01Rock`, `MedievalWall02`. Ordinary static-PBR instance21, stable `COLL`,
  emitter0, identity world transform and normal physical RT visibility/shadows.
  One extra immutable BLAS uses the existing registered-static build path.
  No per-rock instance, role, shader branch or dynamic BLAS update was added.
- ABI instance metadata21→22 and unique static assets9→10. Existing indices,
  bindings, 64B vertices, 128B material records and 32 primitive/32 material/16
  texture-layer bounds remain. The declared BLAS maximum is18 including the
  optional viewmodel; reports enumerate actual owners. Move, destruction,
  partial admission/readiness, live inventory and read-only handle snapshots
  include the new owner.
- Atlas append10→12 BC/normal/ORM layers preserves prior ordinals and formats;
  emissive remains the existing fallback. Atlas owner supplies exact bytes and
  per-level preservation evidence. Compact preview subset1/8/9 remains unchanged.
- Approved core excludes four context meshes and totals8422 triangles with
  12467 vertices/25266 indices. Final runtime GLB is652980B, SHA256
  `c67471b522c92f354d9a5880dea6e48b4f22b5095207841c35c95b5cfc75c4d4`;
  manifest SHA256
  `952e1920fbc60646154d0424556c2935a18e7cf882866a9fe9f4f45ef38be623`.
  Art owner reports zero-error/zero-warning Khronos source/runtime validation
  and actual native-loader frame/material/bounds checks. One zero Mikk tangent
  was corrected during export using a generic perpendicular basis; positions,
  UVs and approved editable source were preserved. Source budget remains at
  most25266 vertices/indices and exactly2 primitives/materials. Imported vertex
  payload797888B and index payload101064B exclude AS, materials and textures.
  These are admission measurements, not frame-performance evidence.
- Both former rear flat caps at3.4/3.47m are removed. The accepted imported
  enclosure must load successfully before Showcase readiness. Collision remains
  at3.4m; the stairwell is a sealed visual remnant, not a traversable route.
- Existing flat roof ends2.94m. The imported roof begins2.92m and rises from
  underside1.60→1.80m over2.92..4.78m, then1.80→12.04m over4.78..17.48m.
  Neutral plane gradients feed shared held-item clearance without drawing
  duplicate procedural cards. Actual post-CSG left quad/right triangular shoulder
  undersides are also shared; their minimum heights are1.242511/1.233418m.
  Their footprints come from the accepted source's actual cut faces, not the
  wider pre-CSG recipe. A closed ordinary masonry seam X±2m,
  Z2.92..2.94m,Y1.35..1.80m closes the confirmed central step gap.
  Ordinary unchanged cobble floor extends X±1.92m,Z3.4..17.48m because the
  exported inspection floor is deliberately excluded.

## Checks and next action

ABI generator produced matching CPU/GLSL text with definition hash
`943cb6bd395b70fbfb42f681647dc2e917d0566ec243741d562d17ab1115501e`.
Scoped diff checks passed. This source reviewer performed no shader compile,
GPU or phone operation. The lead subsequently regenerated and verified the
finite Pipeline8/Compute8 and compatibility source-bound matrix successfully.

Independent read-only CPU triangle audit of the exact runtime GLB confirmed one
identity node and104/104 sampled rearward opening rays stopped by the imported
geometry alone (origin0,.70,1.85; directionX-.6..+.6,Y0..+.35,Z1). This does not
certify every seam/angle, combined native floor/roof or in-game appearance. Atlas
owner reports prior10 layers byte-identical at every mip; exact appended payload
delta is Android5301408B and Windows33554424B, unchanged formats and emissive.

New `horde_rt_static_collapse_asset_tests` loads the actual core and full production
registry: exact topology/material count, normal/tangent frames, opaque PBR,
metre-space bounds, sampled physical rearward occlusion, appended10/11 atlas routes,
prior-route preservation and unchanged material/texture bounds. Shared clearance
tests add the near-roof walking/look sweep to the existing portal cases. Inventory
fixtures explicitly seed the additional owner and verify move, resize retention
and exact synthetic allocation accounting rather than changing only expectations.

Packaging owner reports55 policy cases and two existing player inventory checks
passed against the pinned runtime bytes. Expanded real imported-rig clearance
found61 failures: side-envelope undersides were initially absent and the retained
rear masonry arch has an underside nearY.917m, below the smooth rising roof.
Two side-envelope volumes now coverY1.35 atX[-2,-1.85]/[1.85,2],Z2.92..3.68.
Exact approved-source arch extraction added conservative left/right underside
minimaY.87432492/1.14999998 at their actual bounded footprints,Z3.4..3.66,
excluding four floor-base faces. Eight imported underside volumes now feed the
existing shared smooth world-down/retraction solver. No source GLB, reserve,
grip tolerance, gameplay/combat timer or renderer-only socket offset changed.

The independent corrected real-rig sweep passes:3,107 poses,1,632 additional
actual15,855-vertex viewmodel skins,3,178 actual GLB plane samples and57,970 disk
samples; zero failures. New collapse worst headroom32.2373mm; overall legacy
minimum32.2371mm unchanged against the strict30mm gap. Maximum final grip error
12.1731micrometres. Archived-.335003m arch collision now clears with the same
coherent hand/torch/flame/light pose. CPU clearance is source-ready; owner in-game
appearance, perceived lowering and lead's real RT motion views remain pending.

Next: coordinated affected native checks, Windows/Android builds and lead-only
real RT captures/lifecycle/performance checks. Real imported-rig near-roof clearance,
physical seam/escape views and in-game visual acceptance remain required.
Keep the independent [A1 compatibility deferral](ENGINEERING_1_6_2_A1_COMPATIBILITY_DECISION.md)
open; this asset work does not change presentation completion requirements.
