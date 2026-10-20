# Zero-arm capture diagnosis - 20 October 2026

Both retained arm meshes are **outside the image framing**, below the bottom edge.
The strict gate correctly rejects these frozen shots. No missing geometry,
unexplained GPU-hit failure or corrective renderer change is demonstrated.
Runtime, camera, crop policy and acceptance gates remain unchanged.

Work continues in
`C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.7-wp2-vertical-proof`,
branch `codex/horde-1.7-wp2-vertical-proof`, draft PR 27. Source base is `180ba111`.
The native executable remains the same 14,035,968-byte Debug binary, SHA-256
`223b1f02398f852f05bbf6d26239a748b996b2d4e0d448d16cad899827c85579`.

## Projection and discriminating evidence

The offline recipe reads each retained OBJ's exact uploaded vertices, triangles
and `model_to_world_row_major_3x4`. It uses the actual production equations in
`rt_frame.glsl`: support-to-eye conversion, pitch clamp/-0.05 bias, normalized
camera basis, 1.22 forward scale, 0.74 vertical scale and 960/540 aspect. The pose
file contains support Y, not eye Y. Authored float32 camera values agree with its
rounded text within 1e-5; shader equations are evaluated on CPU in double precision.
This is not GPU floating-point emulation or an occlusion simulation.

Every triangle is clipped against positive depth and all four homogeneous frustum
planes; this is not a vertex-only test that could miss a crossing triangle.
After horizontal clipping, the nearest possible arm point is already below the
540-pixel image:

| Frozen view | Viewmodel triangles | Triangles intersecting frustum | Closest projected Y | Below bottom |
| --- | ---: | ---: | ---: | ---: |
| `rescue-journey-start` | 14,370 | 0 | 557.40 px | 17.40 px |
| `chapter-niche-rect` | 14,370 | 0 | 640.15 px | 100.15 px |
| Fresh first-bend positive control | 14,370 | 7,489 | 331.71 px | In frame |

Most failed-view triangles have positive forward depth; this is lower framing,
not the whole mesh being behind the camera. Full world-body meshes are retained
as context, but their complete triangles do not represent the selective primary
body-remainder policy and are not used to predict arm hit counts.

Fresh hidden native captures of the same two failed scenarios reproduce **the
strict rejection**, native exit 1 / `armPixels=0`. Their PNGs, uploaded arm/world
meshes and pose files are byte-identical to the originals. They retain
`complete:false` and empty admitted capture rows: **these are not passing full
captures**. The original [opening inspection failures](../2026-10-20-opening-tomb-inspection/README.md)
remain unchanged; no crop flag, camera shift or gate exception is added.

The fresh current first-bend control passes full capture admission with **22,391
primary player pixels** and visible modelled forearm/hand in the original PNG.
Its completed dispatch/copy/presentation and owning serial 12 match after twelve
settling frames. The same native executable therefore records actual player hits
in an in-frame pose. This isolates the failed frozen views without expanding into
a broad renderer investigation. It does not certify every production pose.

[projection.json](projection.json), [original-vs-fresh.json](original-vs-fresh.json)
and [receipt.json](receipt.json) retain the numerical results and identities.
Original new PNGs/OBJs remain only in ignored `reports/arm-admission-20261020-r1/`;
no large image, mesh, EXE or APK is duplicated in this evidence directory.

## Staging findings and focused regression checks

The Windows generic ID192 import and frozen scene wrapper use historical combat
policy and pitch -0.05. An explicitly configured live New Game uses current rules
and configured pitch 0; its hand target also differs. Those are distinct scenario
identities. Switching paths blindly would change the experiment, and no evidence
shows that this distinction causes both framing exclusions. Both paths stay as
they are; the frozen shot is not labelled naturally played or live-start proof.

ID218's two authored reward imports retain the camera/support before resolving
the hand hinge and pendulum. No stale landing position or hinge relocation is
found. The new production-config/16:9 fixture confirms zero walk amount/time,
crouch and rope eye offsets, HeldHigh and coherent hinge/body. Restaging retains
those exact transforms and adds one zero-delta publication tick, not elapsed
locomotion. The 37.9771 m authoritative relocation from base11 is recorded
separately from rendered walking; it does not cause shader bob.

**20 added host checks** cover the ID218 production pose and ID192 generic/frozen/
configured-live distinction. Numeric diagnostics explicitly say
`visibility=NOT_EVALUATED`. Final Release builds of both affected targets and
**3/3 selected CTests pass in 18.68 s**: development static assets, rescue journey
and authored-terrain journey. The latter two registrations share one executable.
No game/application or Android rebuild is needed for these test/evidence changes.

Initial failures remain in the logs. The first added fixture attempted a private
reward-import API; it now uses the existing public stager. Two new expectations
were then shown wrong by component diagnostics: authoritative relocation is not
zero rendered movement, and zero elapsed time is not an unchanged publication
tick. The corrected tests assert the measured relocation, one new tick, exact
camera/hinge/pendulum equality and zero elapsed locomotion. No runtime behavior or
existing visibility assertion is altered. An existing authored-roster assertion
also expected 30 although HEAD already has 33 entries, including the three pine
views. The independent HEAD-source audit retains that mismatch; the fixed closed
roster now includes those existing views. No checkpoint is removed or skipped.

All three native processes exit at their expected codes; none is killed or left
pending. Settings/owner/source/asset/campaign identities match the prior custody
record; the campaign directory remains absent. Foreground APIs remain null and
unchanged, which cannot certify a particular external HWND. No phone, public push,
release, paid work, new worktree or Reef operation occurs. Mobile Mist Off remains.
Raw evidence bytes and final test/source identities are pinned in
[source-freeze.json](source-freeze.json) and `evidence-sha256.json`.

## Remaining 1.7 acceptance

These two strict full-capture gates remain unadmitted because their framing does
not show arms. Projection explains the rejection; it does not grant equipment or
whole-opening acceptance. No production fix is selected merely to obtain a pass.
The useful next milestone is exact-candidate acceptance/disposition of existing
content rather than another renderer or polish loop:

- Natural tomb/Keeper-to-lookout/return play, physically earned saves/death Load,
  contact teaching and touch/controller comfort remain open.
- Actual audio character/mix, spatial balance, ducking, mute and output lifecycle
  listening remain pending. **Audio/haptic manual revalidation required: NO for
  this test/diagnosis change**; earlier audible changes still require their own
  pending listening acceptance. No output was auditioned here.
- Warm Keeper/two-light and exterior combined-workload acceptance remains open at
  mobile Mist Off; this diagnostic makes no new performance claim.
- Whole-opening/live-route art and unfinished planned mist/shafts, fireflies and
  renderer wind retain their existing acceptance/disposition requirements and
  present heavy-effects exclusions.
- The six morning recovery policy choices remain unchanged. No recovery method,
  grant, capacity, refill, interruption, death or migration rule is invented.
