# Six retained Mobile RTX backend pixels

Finite investigation, not a renderer correction or performance experiment.
Normal source/control03a6870f66dd9cb4bab62d2f521f4fd663368678; isolated source
d64bea152e40d7083643a941be018f7f27d8aa99 on
`codex/horde-backend-six-pixel-witness`. Its source and generated observer
modules remain isolated; only evidence enters engineering. Debug RTX executable
SHA-2561b984281d5f33ba443b377e8ffe3c5c548eb24b57bfd4bd53d600d5e6e0ea730.

Five checkpoints captured once per backend,960x540,12 settling frames,
Diagnostic/Mobile on NVIDIA GeForce RTX5050 Laptop. Both native runs exit0,
honestly present RT and match control camera/state, CPU geometry, allocations
and complete visibility/diagnostic records. Top5 rows encode48 lossless float
fields per selected coordinate; all ordinary ray/shading/counter work remains.
The other535 rows are byte-identical to their matching original controls:
maximum channel difference0, different fraction0. Original controls were not
rerun. All six original outlier RGB pairs recur exactly.

| Checkpoint / pixel | Pipeline RGB | Compute RGB | Shared primary instance / triangle / material |
| --- | --- | --- | --- |
| worst-bend556,378 | 29,15,9 | 33,16,9 | 3 / 6803 / 100 |
| blue396,262 | 9,13,24 | 125,21,25 | 0 / 625 / 4 |
| red396,262 | 20,10,12 | 54,130,58 | 0 / 673 / 4 |
| finale-roof552,395 | 34,16,2 | 38,18,3 | 3 / 7029 / 100 |
| finale-roof770,526 | 18,21,25 | 13,15,20 | 0 / 809 / 2 |
| two-enemy564,393 | 118,64,18 | 26,14,8 | 3 / 6999 / 100 |

The substantial colour difference is already in ordinary surface shading before
fire/mist/display processing. Primary identities agree; small texture/normal or
ray-position differences exist. Direct shadows versus reflected secondary hits
are not yet distinguished by this first observer. This does not certify backend
parity or identify the correct baseline pixels. Existing maximum3/fraction0.001
image tolerances remain unchanged.

All16 pipeline/compute modules compiled, optimized, disassembled and validated.
All8 Shipping SPIR-V modules remain byte-identical to the normal renderer, each
with0 atomics/no diagnostics binding. Frozen budgets are unchanged. Observer
module size/timing is not Shipping/phone performance or register-pressure proof.
The comparator excludes only four named nondeterministic BLAS build-duration
observations from asset identity. Allocation/schema mutation negatives pass;
all other scene identity and image checks remain strict. No invalid-capture rerun
was needed for that comparator correction.

Receipts: [decoded fields](witness-comparison.json),
[pipeline manifest](pipeline/capture-manifest.json),
[compute manifest](compute/capture-manifest.json),
[module compilation](shader-stage.log), [actual build](build-app.log).
PNG hashes in the manifests bind all ten retained lossless images.

## Completed direct-versus-bounce observer

Exact sourceee71f98a2e00cddb5c6b88fd5ba78834e06fc32a; Debug executable SHA-256
743379cf9fc58a322d762d47847fa9a64dabd1a9cdd7be567e9d117c5a90e17f.
The85-field observer captures the same five checkpoints once per backend.
Both runs exit0; original six pixels, complete diagnostics and scene/geometry
identities recur. Non-payload observer changes are at most1 channel value,
fraction over1 is0; this is not byte identity. The unchanged observer validity
gate passes, while original backend parity failures remain open.

The [surface receipt](surface/witness-comparison.json) proves that five points
are dominated by reflected-bounce differences, not primary direct shadows:
blue/red hit different world triangle/material pairs (627/4 versus641/5 and
675/4 versus689/5), the worst-bend/finale sword points reflect different
texture/radiance from the same world triangles, and the two-enemy point reflects
different skeleton triangles7109/2259. Slight primary mapped-normal changes
alter those sword reflection directions. This is not a reason to retune accepted
player geometry or restore buggy baseline pixels.

The remaining finale770,526 point has identical bounce hit/radiance. Its receiver
z=-12 versus-11.999999 crosses the existing `activeSkyLight` closed-room boundary,
selecting the open-finale aperture versus ordinary moon direction. SkyDiffuse
0.6990025/0.35445005 and visibility1/0 corroborate that selector discontinuity;
finale open progress is1. These are different selected rays, not proven unequal
visibility for the same ray. No boundary/geometry/tolerance fix is accepted yet.

After changed include provenance was restaged, all16 actual modules compile/
validate again. [Actual hash checks](surface/module-integrity.json) preserve all8
Shipping and all4 High Diagnostic modules byte-for-byte; Shipping stays0 atomics/
no diagnostic binding. No physical transport, budgets or normal settings change.

## Outward-box candidate: real metadata defect, acceptance still open

Geometry inventory is complete: the metal bracket top and flame bottom overlap
at y=.50. All six closed `addWorldBox` normal codes oppose their actual outward
winding. The affected max-X face consequently spawns its ordinary bounce inside
the bracket and onto that internal interface. This is not a player/IK issue or
a reason to hide geometry, remove faces or invent an equal-distance tie winner.

Isolated candidate7782e8eeb54850813c74c90b188969f908f4906f corrects only the six
normal codes and adds an actual emitted-face/code source contract to the existing
smoke. Vertices, triangles/materials, shaders, masks and budgets stay unchanged.
Fresh Debug app/smoke build, CTest1/1 PASS4.26s; [log](outward-boxes/host-smoke.log).
Exact observer executable05e8247fd9b7f213c2ca328ff2673076dd424dc7807f59b821582a6c2a16b567.
Both five-image native runs exit0, preserving complete diagnostics/CPU geometry/
allocations. Blue/red bounce outward to identical world triangles489/488 on both
backends, with exact final pixels9,13,28 and31,9,11. Entire non-payload blue/red
images pass the unchanged backend gate (max1/fraction over1=0).

[Complete paired scan](outward-boxes/backend-parity.json) still fails worst-bend,
finale and combat. Four earlier outliers persist; newly exposed combat542,311
adds max7 (183,134,52 versus190,140,54). Original-buggy-control comparisons also
fail after this physically motivated normal correction; those differences are
retained, not forced away or admitted by looser tolerances. The source fix remains
**isolated, not promoted**. No phone evidence/performance claim or accepted
uninstrumented/full High/image gate belongs to it. Exact source, ten lossless
images, manifests, field receipts and build log are preserved at
[investigation94f0c4e](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/94f0c4ea0121a4704347bfc49d4e4b8d1211c218/docs/evidence/2026-10-02-backend-pixel-witness).

[Sky direction source reference](surface/sky-selector-reference.json) matches
captured skyDiffuse within2.48e-7/2.39e-8, corroborating different selected rays;
it is not a new GPU direction/blocker payload.

Next unfinished step: the smallest justified numerical correction for the
closed-room light selector, on an isolated candidate. Keep outward-normal
acceptance and new542,311 point separate. No repeat geometry inventory,
completed observers/controls, rejected precision/phone performance experiments
or owner checks. S26 is disconnected until owner reconnection. Normal renderer
and settings remain unchanged; only receipts enter engineering.
Audio/haptic manual revalidation required:NO; semantic playback unchanged.

## Engineering checkpoint CI (not isolated-candidate acceptance)

Exact engineering source970b520a05f6b9dca68219402b43c70a937bf15f passes fresh
[push36935141405](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36935141405)
and [PR36935147063](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36935147063).
All ten actual job logs inspected: each GCC56/Clang56/MSVC62/Vulkan CPU-host15/
Android76 passes, with0 failures/errors/skips. Android four-ABI Debug build,
required suite admission, lint/package gates pass. These jobs do not certify the
isolated7782e8e normal candidate, physical phones or full RT/backend acceptance.
PR15 draft/open/CLEAN/MERGEABLE; no merge. Synthetic merge
1da7cd772857c923a5fbf52542c92ed56dfac5f3 has exact mainbda1b99/source970b520
parents and matching tree d71dda2d1ba26e4dc46cee390ec5c6fddcbc64d2.
Raw logs remain at C:/Dev/tmp/horde-backend-pixel-witness-20261002:
push SHA-2561a5c05caf482390b48824413fad1318bf1dae4275aa70d9d9c77c4a85df7604e;
PR SHA-25659131bcb31bca141c8ec3a4ce0d65d70a7c55fd7f8d5ad96f6762372765bcade.
Later prose/receipt updates are not silently relabelled as this exact-source CI.

## Closed light-region candidate: completed finite matrix, not promoted

Isolated source `cc0491e601bc6665ae57a99b8920dcbc38efd719`, after outward-box
control94f0c4e, replaces exact coordinate membership with the conservative
reconstruction-arithmetic neighbourhood of the **same closed light regions**.
The captured rays/t differ, not just their rounded positions. This policy
preserves inclusion of the authored z=-12 wall; it does not certify undocumented
hardware intersection-t error. Actual ray components avoid a normalization
assumption. Geometry/materials/masks/traversal budgets, receiver positions,
shadow-origin tolerances and physical visibility transport remain unchanged;
the corrected selected light ray necessarily differs at the affected boundary.

Fresh MSVC Debug/Release affected smoke each1/1 PASS4.40/1.96s exercises the
actual scalar GLSL, captured pair, eight region endpoints/one-step neighbours/
excluded millimetre offsets and12 double-reference separate/FMA fixtures.
Native Debug app build passes, executable SHA-256
`e5e68dd444dd8077f7c5d4b072caabb8c4c9134c20ee6a7eb0c6b501eecb58df`.
All16 real SPIR-V modules compile/optimise/disassemble/validate, and all actual
embedded byte hashes match their nonempty catalog fields. All8 Shipping modules
remain zero-atomic/no binding22. This is a shader change: unlike preceding
output-only stages, Shipping and High modules are not byte-identical controls.

**Frozen cost gate FAIL:** relative to94f0c4e, Generic adds3060bytes/180
instructions and Opaque adds13064bytes/791 instructions in both backends.
The existing budget file/hash is unchanged; no limits are raised and no
performance/register/occupancy gain is claimed. Keep this candidate isolated.

One new five-checkpoint native capture on each RTX backend exits0, honestly
presents RT and preserves scene/CPU geometry/allocations/full diagnostics.
[Paired scan](closed-light-region/backend-parity.json) retains unchanged
max3/fraction.001 gates.770,526 now exactly18,21,25 on both, with skyVisibility1
and skyDiffuse.6990025/.69900256. Blue/red remain max1/fraction over1=0. Four
remaining outliers are unchanged: worst556,378 max4; finale552,395 max4;
combat542,311 max7 and564,393 max92. Whole parity still fails. Old buggy-control
colour changes are retained as expected differences, never relabelled passes.

[Exact pushed source/full native evidence](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/ac15d641e6497bbd3fbbe3fff204e0d6c96f4863/docs/evidence/2026-10-02-backend-pixel-witness)
contains all ten lossless PNGs, manifests/geometry, actual fields and build/run
logs. This engineering tree contains only five curated receipts; no experimental
source/modules/configuration are integrated. No phone operation or main merge,
release or publication. No unchanged control/artifact was rebuilt or recaptured.

Next unfinished step supersedes the earlier selector task: localize the new
combat542,311 reflection layer using existing85 fields, with only bounded
two-enemy captures per backend if needed. Do not repeat the completed candidate
matrix or conduct another precision/epsilon sweep. Keep remaining original
reflection outliers, High row43 and shader-cost/promotion gates distinct.
Audio/haptic manual revalidation required:NO; lighting only, no semantic
listener/event/playback/haptic changes. Previously requested owner gates unchanged.

## Latest: ordered PBR UV arithmetic (isolated image gates pass)

The former next combat-localization task is completed; do not restart it.
Exact isolated source `877f5670f2093c72730e315491b61139fc320667`, complete pushed
[evidence `dc11e8985f3f74774b0648b9514a5941867f955d`](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/dc11e8985f3f74774b0648b9514a5941867f955d/docs/evidence/2026-10-02-backend-pixel-witness).
This engineering checkpoint contains only three additional curated paired-image
receipts, not the observer code, candidate modules or a renderer/config change.

The85-field combat observer localizes the new point to a reflected-hit fork;
the subsequent115-field probe preserves every non-payload control pixel exactly.
At both combat pixels primary hardware barycentrics match, but interpolated UV X
differs one binary32 ULP before texture normal/base/ORM samples diverge. One
bounded `precise vec2 uv` candidate preserves the authored three-vertex expression,
without snapping UVs, changing assets/LOD/materials or smoothing normals. UVs and
normal samples then match; both reflected hits agree. Paired combat image max1
passes unchanged max3/.001. Remaining checkpoints2/11 also pass (max1/max3).
Expected changed old-control pixels are retained as differences, not fake passes.

Curated [combat](ordered-uv-combat-backend-parity.json),
[remaining scenes](ordered-uv-remaining-backend-parity.json) and
[prior failing combat control](combat-point-backend-parity.json) preserve the
negative-to-candidate comparison. Each finite native run exits0, honestly presents
RT and preserves camera/state/CPU geometry/complete diagnostics/allocations.
Actual manifests, lossless images/fields, exact executable hashes and full logs
are in the linked immutable investigation snapshot. No completed control,
checkpoint12 or shader set was rebuilt for the two-scene follow-up.

Fresh Debug/Release affected CTest each1/1 passes4.21/1.97s; all16 shaders compile/
optimise/disassemble/validate. Pipeline Mobile Shipping/Diagnostic disassembly
is otherwise identical after removing only new NoContraction decorations.
Generic adds84bytes/7 counted instructions, Opaque504bytes/42 in both backends;
all8 Shipping modules remain zero atomics/no binding22. These are constraints,
not measured native instruction/register/performance costs. Normal frozen
ceilings do not admit the growth; no budget raised or Shipping promotion.

All four remaining reflection outliers are closed **in this isolated matrix**.
It is not full13-image/uninstrumented/cross-device acceptance. Prior blue/red
evidence remains tied to its own source. Closed-region candidate cost/integration
and separate High row43 physical false-candidate failure remain open. Normal
production code is unchanged. No phone action, main merge, release or publication.

Next unfinished step: clean uninstrumented outward-normal/PBR-UV candidate from
normal engineering source, without automatically including the cost-failing
light-region helper; exact cost/full affected RTX/High/device admission remains
required. Reuse completed finite evidence; no new precision sweep or audit restart.
Owner-disconnected S26 checks stay parked until reconnection. Audio/haptic manual
revalidation required:NO: arithmetic/diagnostics only, feedback inputs unchanged.
