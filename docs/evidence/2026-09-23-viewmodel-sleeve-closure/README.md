# Sleeve closure checkpoint — not production admission

Phase 3 remains open. The real viewmodel still needs surface/intersection,
deformation, live-motion and exact-phone/owner acceptance. Accepted runtime GLBs,
normal block-arm presentation, gameplay/IK/grips, GPU ownership and shaders are
unchanged. No new generation, licence/distribution change or device install occurred.

## Diagnosis and rejected path

Strict 1e-6 m position-welding identifies **two** authored sleeve surfaces, not
the 230 raw UV/normal-split islands. They have 278 boundary edges in four boundary
regions; one has a degree-four pinched vertex. The broad upper boundaries span
most of an upper arm, so cutting everything back to a clean ring would remove too
much garment. In the paired world mesh, 198 of those edges have adjacent masked
cloth (`NearFacePrimaryMasked`), while 80 have no adjoining face.

A tested two-ring recovery of adjacent cloth added 284 triangles but worsened the
boundaries from four regions/one branch vertex to eleven regions/fifteen branch
vertices. Native imagery also acquired protruding cloth. That route was rejected
and removed from the active processor. Failed local source, patch, candidate and
reports remain in `reports/rejected-*`, `reports/recovered-sleeves-welded.txt`, and
`C:/Dev/tmp/horde-recovered-sleeves-20260923-a`; they are not admitted assets.

## Retained, opt-in closure mechanism

`--close-sleeves` in the existing processor implies continuous elbow weights. It
welds only cloth seams, preserves per-corner UVs, fills the five actual openings,
and uses five interpolated interior vertices to triangulate the closing panels.
Ordinary ear clipping reused three existing edges and failed the overfull-edge
guard; that variant was not retained. Both arms remain within the **unchanged**
manifest budget: 5,810 sleeve + 8,838 gauntlet = 14,648 triangles, below 15,000.
The processor now checks the manifest's triangle budget before every export.

An initial smoothing attempt introduced nine bind-pose normal misalignments.
The retained helper preserves original cloth corner normals and uses actual face
normals on the new panels; it preserves each gauntlet corner normal separately.
No screen-space geometry, shader mask or lighting workaround was added.

| Candidate | Viewmodel SHA-256 | Static normal evidence |
| --- | --- | --- |
| Closure only | `e93d3b3b3b07856cc50712e6c2c9824171dacf7157e703e3b12ac73b949280bb` | 0 negative alignments, 0 degenerate triangles; 278 new panel triangles |
| Fitted + closure | `f785d970bbc10bdef7a939d14f8bbd3bb5285a0c77631cbc090106cd24214c42` | 2 pre-existing fitted-surface negative alignments, 0 on new panels, 0 degenerate triangles |

Both use Right source, Left/Right Grip rolls 105/145 degrees, and paired world
`4050e15b5084bfa83ad9db09f8bb8ececd872be89a27d0b08447c12f9f7a501d`.
The fitted variant adds `--fit-sleeves`. Local generation roots end in
`horde-closed-sleeves-20260923-c` and `horde-closed-fitted-sleeves-20260923-c`.
A separate fitted export `...-d` has the same viewmodel hash. Sorting the weld input
is explicit; no historical parallel-export byte-order chase is involved.

## Actual checks

- Existing static/skinned admission passes for both candidates: addressing,
  shared pose/vertices/tangents/grips and invalid-pose rejection.
- Every gauntlet accessor's logical bytes match the prior corrected candidate:
  positions, normals, tangents, UVs, joints, weights and indices.
- Independent fitted-candidate position-welding confirms two physical sleeve
  components, zero boundary/overfull edges and no duplicate/degenerate faces.
- `tests/PlayerViewmodelSurfaceFixtures.py`, run in Blender 5.2, passes 3/3:
  closure plus original/new normals and untouched gauntlet corners; rejection of
  shared glove vertices; rejection of already-closed input. These are local Blender
  tests, not tests claimed to run in the portable GitHub lane.
- Six mirror/UV unit tests pass afresh. A default export still exactly reproduces
  the admitted viewmodel `1e3b041e...2538` and verifies world `e8737f10...50fd`.
- Each retained candidate completes native RTX portrait grips/upward-slice
  captures. Images, manifests, processing and stage receipts are retained here.
  The same Debug executable `23b774c9...9c312` is used as in the preceding portrait
  investigation; the renderer itself did not change during these asset trials.

## Remaining gate, without a topology shortcut

Closed edges are **not** proof of a well-shaped or self-intersection-free garment.
The new closing triangles can be skinny: maximum longest/shortest edge ratio is
26.33 for closure-only and 49.40 for fitted closure. A projection of the earlier
nonplanar fans raised concavity warnings, not a verified 3D intersection verdict.
No direct self-intersection pass is claimed. The fitted variant still has the two
original fitted-surface normal failures and is not eligible for admission.

Native review finds a narrower sleeve with fitting, but appearance is still dark
and transitions/deformation are not accepted. Finish those geometric and visual
issues before selecting a recipe for Android, then validate live motion and obtain
the owner's phone acceptance before retiring block arms. The existing isolated
Android APK still contains the earlier unclosed candidate, not these new assets.
No performance gain is claimed. Audio/haptic manual revalidation required: **NO**;
feedback, listener, playback and haptic semantics are unchanged.
