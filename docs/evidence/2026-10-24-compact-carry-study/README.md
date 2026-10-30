# Compact torch carry: one authored endpoint rejected

The cross-chest r1 pose is **rejected**. Actual RT close views replace the dark
wall with a large, obstructive wrapped torch head. The wide view regains some
arm pixels, but complete flame containment and square hand framing still fail.
The accepted runtime and original test/capture sources are restored byte for
byte, and the native app/test targets rebuilt. No transition or normal binding
was admitted.

[Current control](control-diag13.png) and [authored endpoint](candidate-diag13.png)
are original Windows hardware RT pixels at the same witness. The separate
[editable source](../../../assets/models/player/source/compact-torch-carry-study-r1/README.md)
retains the pose intent and a 2.4 MB Blender reference. It is not an exported
skeletal clip or a finished compact-carry asset.

## Bounded proof and actual failure

Source basis `699340bd`. One manually authored Grip frame draws the unscaled Rag
torch across the lower chest, with the head toward the view centre. Mirrored
endpoint intent is diagnostic staging only; no per-frame side-selection policy,
new solver, camera, collision inset, fire size, light power or production animation
change is proposed. Original shoulders, roots, right arm and all mesh/bind data
remain. The [pose record](../../../assets/models/player/source/compact-torch-carry-study-r1/compact-carry.pose.json)
and [temporary patch](temporary-authoring-proof.patch.gz) preserve the exact study.

The existing archived actual-rig oracle is reused with all original 60 poses,
10 framing witnesses, square/wide aspects, pitches, phases, socket guards and
30 mm roof requirement unchanged. Both control and candidate report 52 failing
cases; equal totals do not mean identical defects. See the retained
[control](control-actual-rig.txt) and [candidate](candidate-actual-rig.txt) logs.

At CPU witness diag13/wide:

| Quantity | Control | Authored r1 |
| --- | ---: | ---: |
| Projected left-gauntlet area sum | 0 | 0.103212 |
| Grip / Light depth | 0.248953 / 0.225799 m | 0.400000 / 0.106490 m |
| Minimum local roof headroom | 30.506 mm | 213.744 mm |
| Actual mesh maximum world X | 2.003642 m | 1.823666 m |
| Full flame-box maximum world X | 2.214501 m | 1.922571 m |

The physical right wall is X=1.85 m. The candidate's mesh, Light and four shadow
endpoints fit this plane, but the unchanged complete flame box still extends
**72.571 mm through it**. Square diag13/21 still have zero clipped left-gauntlet
area. The independent component check reconstructs the rigid transform from the
unaltered GLB/native OBJ correspondence and verifies all 4,825 mesh vertices to
less than one micrometre; it is the CPU witness, not the nearby native capture's
identical numerical pose. Whole imported-structure roof and room-union guards
remain authoritative. The earlier 29.4214 mm global roof shortfall is unchanged.

Ten current hidden native RT frames retain five control/candidate witnesses
11/13/21/40/48. They match camera, actual 960x540 internal/output, DRS off,
Diagnostic High Pipeline shader identities and flame strength. All have real RT
dispatch/copy and owning completion/presentation evidence in the [receipt](receipt.json).
Right diag13 arm pixels rise 0 to 4,111, but the directly inspected torch head
dominates the image. A nonzero arm counter does not establish usable framing.
Bend40 and centre48 images remain byte-identical. These endpoints fail before a
transition trial is justified; there is no claim of smooth turns, physical input,
phone quality or performance. The native bend yaw0 and CPU bend yaw-pi remain
separate witnesses, as in the original retained study.

## Preservation, checks and next boundary

The source bodies and original bind/geometry hashes remain exact. Three focused
accepted transition/clearance/socket CTests pass after restoration (25.75 s);
[results](restored-focused-tests.txt). No assertion or deadline changed. The app
target and final-rig target rebuild successfully. The initial wrong app target
and a left/right-handed offline reconstruction mistake are retained in raw
reports; the corrected analysis keeps the same all-vertex error guard. This is
not a new performance measurement or a closure of the original torch defect.

All 151 parked owner files, campaign/settings and foreground custody remain
unchanged. No phone or activation attempt occurred. The raw meshes, ten original
images, exact study binaries, build logs and Blender construction receipts remain
under ignored `reports/compact-torch-carry-20261024/`. Source/preview/runtime
boundaries are explicit; the study is excluded from package inputs.

Do not repeat this mirrored cross-chest family or add smoothing to hide it. The
next torch step requires a genuinely different authored geometry/pose feasibility
choice that keeps the hand and a readable head visible with the full unchanged
flame envelope. This result does not prove all possible poses impossible, nor
authorize shrinking the prop/fire, moving the camera or changing player stand-off.
Those would be separate product/geometry decisions. No broad solver or runtime
candidate is justified by this failed endpoint.

Audio/haptic manual revalidation required: **NO** for the restored source-only
study. Existing listening, phone and natural-play acceptance remains pending.
