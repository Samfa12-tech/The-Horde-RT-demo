# Exact cloth-boundary weight reconciliation — isolated candidate

The [preceding paired capture](../2026-09-27-player-boundary/README.md) proved
that retained world cloth and viewmodel sleeves separated at shared bind edges.
This fixes that demonstrated skinning disagreement, not the entire visible
armpit/cuff problem or first-person torso visibility.

## Change

`--reconcile-sleeve-seams` is opt-in. A pure helper matches view sleeve boundary
edges to world edges incident to both BodyPrimaryVisible and NearFacePrimaryMasked.
It copies the accepted view endpoint weights to 355 coincident world vertices
at 198 matched edges. Head/gauntlet vertices, including shared-index cases, are
excluded. Conflicting view weights, nonmanifold/degenerate edges and malformed
data fail explicitly. No weight search, cap, new geometry, IK or grip change.

The processor exports the viewmodel first, applies the explicit plan only to
the retained world mesh, verifies exact canonical targets and unchanged non-target
weights/geometry, then exports a separate world candidate. Receipt source-world
identity keeps its existing meaning; paired-world identity names the derivative.
No admitted production GLB, shader, runtime visibility mask or GPU ownership changed.

## Fresh evidence

- Eleven helper tests and six existing gauntlet tests pass. CI includes the new tests.
- A no-option export reproduces the exact tracked world `e8737f10...` and
  viewmodel `1e3b041e...` hashes.
- Two independent candidate exports match: world
  `2385fbb8c0415f5c39f6b1ce7e8bdccc36aba79971492a682c6f6e6df26771c6`;
  view `6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b`.
  The latter is byte-identical to the owner's currently tested viewmodel mesh.
- Expanded world bind triangles retain exact positions, normals, tangents, UVs,
  winding and counts in all four semantic regions; skin weights are intentionally
  excluded from this comparison. World remains 27,775 triangles; view remains 14,370.
- Freshly rebuilt C++ world admission and viewmodel addressing/shared-pose/
  vertex/tangent/grip admission pass against the candidate pair.
- Khronos glTF Validator: zero errors, one `NODE_SKINNED_MESH_NON_ROOT` warning,
  identical to the control. The warning was not hidden or newly introduced.
- Three native RTX captures pass: low/look-down, high/look-up and low/parry.
  Every measured shared-edge endpoint separation is now **zero** in all three,
  versus the preceding maxima of 43–106 mm. All three viewmodel OBJ uploads are
  byte-identical to their controls. Native image differences are 134/75/251 pixels
  respectively; world secondary geometry changed, not primary view geometry.

The complete pose inputs, output hashes, masks and shader identities are in the
capture manifests and stage receipt. The retained after JSONs and preceding
before JSONs can be reproduced with the preceding evidence's comparison script. The coloured
world preview is Blender geometry inspection, not native RT acceptance evidence.

The first authoring attempt rejected a raw retained weight sum of 0.999941647.
The installed Blender 5.2 exporter (`primitive_attributes.py:151–163`) normalizes
skin weights before writing GLB. The helper therefore distinguishes old raw-world
roundoff (1e-4 sum tolerance) from copied canonical view weights (unchanged 1e-5
tolerance). It neither normalizes nor averages the copied targets. Tests reject
world sum 0.999 and view sum 0.999942; runtime/image gates are unchanged. The final
receipt records the actual authoring sum ranges.

## Reproduce and continue

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --factory-startup --threads 1 --python-exit-code 1 --python tools/process-player-viewmodel-runtime.py -- NEW_OUTPUT_DIRECTORY --gauntlet-source-hand Right --grip-roll-degrees 105 145 --blend-elbows --gauntlet-scale .099 --reconcile-sleeve-seams
```

The retained pair under `candidate/` is investigation evidence, **not production
admission**. World cloth remains secondary-only. The missing first-person connecting
surfaces/torso, actual gauntlet cuff join, subdivision T-junctions, mirror penetration
and live/phone acceptance remain open. A zero shared-edge gap must not be reported
as final visual acceptance or proof that every garment boundary is closed.
The phone remains on `b5d486b2...`; no new device install occurred here.

Audio/haptic manual revalidation required: **NO**, offline skin-weight repair only;
gameplay pose/grip authority and semantic feedback inputs are unchanged.
