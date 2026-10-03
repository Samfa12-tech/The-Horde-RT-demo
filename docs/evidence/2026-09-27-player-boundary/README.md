# Paired player boundary diagnosis

This is geometry/ownership evidence for the remaining Phase 3 seams, not an
accepted mesh repair. No asset, visibility mask, IK target, grip or shader was
changed for these captures. The ordinary phone candidate remains `b5d486b2...`.

## Exact capture extension

Debug Windows viewmodel checkpoints now retain a separate PlayerWorldBody OBJ
and hash alongside the unchanged viewmodel OBJ. Both contain their actual CPU
upload positions/normals/UVs, primitive ranges and model-to-world TLAS transform.
They are **not GPU readback** and do not perform another pose solve. The new
world capture rejects absent/stale uploads, empty or invalid primitive ranges,
and existing output paths. Its validity is cleared before each recording attempt,
including an early unsupported-scaled-presentation failure.

Fresh Debug window build and focused resource-inventory CTest pass (1/1).
Regression tests cover exact output bytes, stale/not-ready states, invalid local
indices, empty primitives, overwrite refusal and the early failure path without
Vulkan calls. Existing viewmodel bytes remain unchanged. This path is excluded
by NDEBUG; no new GPU buffer, readback, ABI field or Shipping work was added.

Native RTX captures succeeded for low/look-down, high/look-up and low/parry.
The last two used executable SHA-256
`85ca2b6f7a951efac5e3468e7fa95154e228bcda8d76e4bb3e0dd851d4ced891`.
After adding the early-record-failure guard, the low/look-down capture was
repeated with final executable
`f2b5ccac90643f30f18fa1da70ec17010631f29b7bcc2f93d162a783313006a9`.
Its PNG and both OBJs are byte-identical across the guard-only change.
The two stage receipts record these distinct executable/asset identities.

## Findings

The four-material world source still calls all connecting cloth/torso/legs
`NearFacePrimaryMasked`; simply exposing it or discarding mixed arm weights is
not a verified anatomical partition. The owner-required visible torso/legs and
nonduplicating arm ownership remain open.

The current sleeve has 278 geometric boundary edges. Of those, 198 have an
exact bind-space edge in retained NearFace cloth. The other 80 have no exact
matching edge; this **does not establish 80 true holes**, because selected-face
subdivision can leave differently segmented edges. The left upper component
also has a degree-four geometric junction, so do not assume every component is
a simple cap loop.

`compare-player-primary-boundaries.py` verifies bind/native vertex counts,
primitive order and every triangle index before comparing. World and view have
the same recorded model-to-world transform. Coincident bind-edge endpoints in
the retained cloth nevertheless separate in the posed uploads:

| Pose | Left wrist max | Left upper seam max | Right wrist max | Right upper seam max |
| --- | ---: | ---: | ---: | ---: |
| Low/look-down | 100.75 mm | 49.24 mm | 57.41 mm | 106.31 mm |
| High/look-up | 87.18 mm | 42.97 mm | 104.99 mm | 68.95 mm |
| Low/parry | 100.41 mm | 48.22 mm | 87.29 mm | 101.65 mm |

These are maximum separations at compared cloth endpoints, **not** measured
gauntlet clearance, screen-space hole dimensions, or every vertex's displacement.
Upper-seam median separation is zero; low/look-down left-wrist median is 11.10 mm.
The per-pose JSONs preserve counts and asset/OBJ hashes. The pipeline explicitly
reweights extracted viewmodel sleeves independently of retained world cloth;
this result proves a posed seam disagreement despite a shared gameplay pose.
It does not justify another generic IK or grip-roll search.

The two labelled-by-colour Blender previews are geometry inspection only:
blue = world BodyPrimaryVisible sleeves, brown = NearFace connecting cloth/body,
purple = Head, grey = gauntlets. They are not native RT acceptance screenshots.

## Next repair boundary

Repair the demonstrated seam at the offline partition/skinning layer. Preserve
accepted sleeve shape, authored UVs and gauntlet chirality/grips. Determine which
connecting faces belong to the dedicated arms versus torso, and make shared
boundary vertices deform consistently in both independently owned outputs.
Do not use an arbitrary bone-weight discard threshold or blanket caps to hide
the issue. A proposed cuff underlap was not exported: part of that area already
contains retained world cloth, and adding a strip before reconciling ownership
could create overlapping surfaces. The actual gauntlet cuff is the closed
source +X region, not the two negative-X finger/grip boundary loops.

Reproduce a report (use the paired .099 candidate recipe in the live-arm note):

```powershell
python compare-player-primary-boundaries.py WORLD_BIND.glb VIEW_BIND.glb player-viewmodel-lantern-low-look-down/145-player-viewmodel-lantern-low-look-down.player-world-body.obj player-viewmodel-lantern-low-look-down/145-player-viewmodel-lantern-low-look-down.obj
```

The adjacent `analyze_viewmodel_grip_calibration.py` supplies the reused GLB
reader. Source input identities are included in the JSONs. Phone live/owner
acceptance, wrist/shoulder repair, torso visibility and mirror penetration remain
open. Audio/haptic manual revalidation required: **NO**, diagnostic-only change.
