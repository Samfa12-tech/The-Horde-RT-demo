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

Next unfinished step: inspect the blue/red authored secondary faces for real
coplanarity/overlap and retain exact geometry/ray evidence; separately design
a bounded numerical correction for the demonstrated closed-room light selector.
Do not repeat completed observers/controls, rejected precision/phone performance
experiments or owner checks. No probe enters normal shaders or runtime settings.
Audio/haptic manual revalidation required:NO; semantic playback unchanged.
