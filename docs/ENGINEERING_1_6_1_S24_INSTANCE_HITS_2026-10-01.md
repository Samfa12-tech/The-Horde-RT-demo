# S24 bounded primary-hit discriminator

Status: completed discriminators isolate TLAS UPDATE behaviour on exact S24;
bounded visibility/reference-change rebuild candidate next. Normal control source0cf4f05; exact
normal Debug8CB APK and four existing captures are retained in the
[visual follow-up](evidence/2026-10-01-s24-visual-followup/RESULTS.md).
Do not repeat the finished opacity/performance trial or tune accepted arms.

Question: do the normal primary hardware ray queries commit player TLAS hits
before hit decoding, or does metadata-based classification lose those hits?
The original primaryPlayerPixelCount is not independent of metadata.

This separate investigation branch modifies Diagnostic/Mobile/OpaqueFast compute
only, adding seven atomic instructions at the committed-hit boundary before
instance/material/vertex loads. It adds no ray query, ray path, buffer, traversal
budget, geometry/mask/material/animation or shader-quality change. High, Shipping
and the pipeline launcher must have unchanged SPIR-V. Reuse the existing fence-
owned Diagnostic SSBO and capture/native-state harness, not a new telemetry ABI.

**This APK's remapped counters are not physical glass diagnostics.** Do not run
glass gates or performance comparisons against it; never promote the mapping.

| Native state field | Investigation meaning, primary queries only |
| --- | --- |
| primaryOpenOpaqueTerminalInstanceMask | Actual committed TLAS instance-ID bits, clamped31 as overflow marker |
| primaryOpenOpaqueVolumeInstanceMask | Committed custom-index bits, clamped31 as overflow marker |
| primaryMismatchedExitCount | Count of actual ID != custom index |
| primaryOpenMissCount | Actual viewmodel instance20 committed pixels |
| primaryOpenOpaqueCount | Actual world-body instance4 committed pixels |
| primaryInterfaceBudgetCount | Actual skeleton pose0 instance2 committed pixels |
| primaryVolumeBudgetCount | Actual skeleton pose1 instance18 committed pixels |

Slots4/20/2/18 are the current named CPU registry's world-body/viewmodel/skeleton
instances, not a production raw-slot redesign. The original metadata-based
primaryPlayerPixelCount remains intact for comparison. Normal rendering and
the existing primary candidate filtering remain unchanged; zero commits would
not distinguish AS/masks from that filtering without another discriminator.

Finite run matrix: compile/validate eight compute modules once, confirm only the
one intended module's SPIR-V changes; build one Debug APK; pull/install match on
SM-S928B; capture two-enemy-combat and player-viewmodel-lantern-high at75%, same
native scene/pose/settings; compare with already retained normal images. Record
the two raw masks and four counts plus metadata-based count, RT presentation and
actual backend. Home/resume and restore normal8CB with data preserved afterward.
No S26 timing sweep, S24 performance claim or owner listening test.

Fresh eight-module compute generation/spirv-val passes. Exactly one SPIR-V
changes: Diagnostic/Mobile/OpaqueFast506304bytes (+796),12atomics (+7), still
23ray-query sites. SHA256d466e66ee4ecfb3a2f7ee2d22337eb811c8744802ac5076fe7e9176792805bac.
All seven other compute modules, including Shipping/High, have identical SPIR-V
hashes to0cf4f05. Shared source is preprocessor-inactive for the pipeline launcher;
its embedded artifacts are untouched, not claimed as freshly recompiled here.
Android Debug four-ABI build succeeds (1m16s). Frozen APK:
`C:/Dev/tmp/horde-s24-instance-hits-20261001/instance-hits-debug.apk`,
SHA256dd69eeb7947d631450780665324380766a85d3d03f42fba661ca21523ac0183c.
Logs live alongside it; no release/signing/quality override was used.

## First discriminator completed (October2)

Sourcee8b696591c8ca1fefbc094238867a845846ccd1d, tracked runtime clean. Runner
sourceDirty=true includes preserved unrelated untracked raw S24 evidence, not a
new runtime patch. Exact SM-S928B/R5CXC0G9GBW install/pull hash matches DD69 APK.
Actual ARM64 library contains the exact506304-byte probe module; all70 assets are
byte-identical to normal8CB. Run20261002-000200 passes two75% captures, strict
ASTC,12 stable RT-presented frames and Home/resume on RayQueryCompute/OpaqueFast.

| Checkpoint | ID / custom masks | Viewmodel20 | World4 | Skeleton2 /18 | ID/custom mismatches |
| --- | --- | --- | --- | --- | --- |
| two-enemy-combat |15 /15 (0,1,2,3) |0 |0 |15496 /0 |0 |
| player-viewmodel-lantern-high |131469 /131469 (0,2,3,7,8,17) |0 |0 |12 /0 |0 |

Both images are byte-identical to the previously retained normal8CB images.
Direct image inspection still finds no hands. These are per-pixel committed
primary hits, not candidate/triangle/entity counts. This demonstrates absent
player commits before decoded shading, rather than merely a metadata-based
counter losing them. Skeleton2 does have hits; skeleton18 zero does not prove
why the second enemy is absent from the view. No performance or glass pass.

Next bounded discriminator: in the same primary invocation count raw viewmodel
candidates and run one independent viewmodel-mask-only Opaque hardware query,
same camera/direction/min/max. If it hits while normal primary misses, distinguish
candidate filtering/traversal from absent AS geometry. If both miss, investigate
AS upload/build/instance transform/mask; do not tune IK or change asset geometry.
This extra query is diagnosis only, never Shipping or a traversal-budget change
to physical transport. Repeat only the two affected captures for the changed
module; keep first receipts, no performance rerun.

Second-probe additional mapping: primaryOpenOpaqueTerminalMaterialMask is the
raw normal-primary candidate ID mask; shadowImplicitOriginExitCount counts
normal-query instance20 candidates (not unique pixels). shadowMismatchEmptyCount
counts pixels with an independent Opaque/viewmodel-only committed hit;
shadowOpenMissCount counts those whose ID is unexpectedly not20;
certifiedClosedVolumeRecoveryReasonMask is the independent committed ID mask.
These fields are also **not** physical-transport diagnostics. [Vulkan opacity
precedence](https://docs.vulkan.org/spec/latest/chapters/raytraversal.html#ray-opacity-culling)
defines Opaque as overriding build/instance opacity; masks and ray extents stay
identical to the intended viewmodel part of the normal primary query.

Second module compiles/validates at507096bytes,17atomics,24ray-query sites
(one additional primary-only diagnostic query). SHA256
8f3bd5d4b69c1ee82c6e8be308fbe017e4fe7df70a5f3751afdd24cac34657c9.
Other seven compute SPIR-V identities remain normal. Fresh four-ABI Debug build
passes (7s incremental), actual ARM64 module bytes verified, all70 assets equal
normal8CB. Frozen `C:/Dev/tmp/horde-s24-instance-hits-20261001/instance-hits-second-debug.apk`,
SHA2563c11c209e9fa0b7cf37c27dea86be17ec0102579392afcc911c28b01254c1e92.

## Second discriminator completed

Source3e6c020dbfde479895fb9b96fce2206a2f5320c5, exact3C11 APK install/pull
matches on SM-S928B. Run20261002-001043 passes the two75% captures and Home/resume.
Both PNGs again byte-identical to normal8CB. Primary normal candidate masks equal
committed masks15 combat/131469 high; viewmodel20 candidate counts0. Independent
Opaque/viewmodel-only query counts0 hits in both; unexpected-ID count0 and direct
ID mask0. Original metadata-based player count remains0. The first committed
counts are unchanged. Thus changing primary candidate handling or material shading
alone cannot supply these absent intersections in this probe. It does not yet
prove a driver defect, bad AS update or CPU pose/instance error.

Next discriminator is one high-lantern capture with Android-Debug-only CPU facts:
actual masks, finite uploaded-vertex/world bounds, buffer/BLAS/TLAS addresses,
scratch size and the actual reported scratch alignment/remainder. No shader
recompilation, extra query or animation change. Source review notes a missing
explicit scratch-alignment guarantee, not demonstrated S24 misalignment; initial
vs update geometry opacity is also a latent material risk (current player parts
are opaque). Do not fix a speculative cause or repeat completed shader captures.

CPU-fact candidate is built and frozen, without shader regeneration. Initial
Debug build failed because directly linking vkGetPhysicalDeviceProperties2 is
not supported by the existing Android API24 link target. Resolving the core/KHR
entry point through the existing instance mechanism fixes that diagnostic-only
link failure; minimum API and production Vulkan loading remain unchanged.
Corrected four-ABI Debug build passes (21s); logs retain both attempts. Frozen
`C:/Dev/tmp/horde-s24-instance-hits-20261001/instance-cpu-facts-debug.apk`, SHA256
2904c7e2bf82c3690cd443d2ce58343cd3b236f8da5b8958609ba0c4d9481e55.
All70 APK assets are byte-identical to the second-probe artifact. This candidate
retains the second-probe shader and its investigation-only counter meanings.

## CPU facts completed

Sourceeef3c79d8e80bbdebccb76eebd58b9ad3945b10d, exact2904 APK install/pull matches;
run20261002-002823 passes one75% high-lantern capture and Home/resume. PNG SHA256
d6fe26c2557867bba69a1e8fb0ecd1737c0f3f37d3a6a36328237f48c88d673a remains byte-identical
to normal8CB. Player committed/candidate/direct-query counts remain0.
The runner filters logcat to bridge/audio/runtime tags, excluding the diagnostic
HordeLanternRT tag. Reading that still-retained tag separately supplies the CPU
receipt; no recapture/rebuild was necessary.

World-body: mask144,5 geometries,34304/34304 finite vertices,2195456-byte vertex
allocation. Viewmodel: mask64,2 geometries,15855/15855 finite vertices,1014720
bytes. Both BLAS addresses equal their actual CPU TLAS references. Reported
scratch alignment64; both scratch-address remainders0 (sizes4451424/2300224).
High-lantern viewmodel world bounds approximately[-11.3603,0.131978,-15.4931] to
[-10.5961,0.587804,-14.9264], camera[-10.65,0.7,-15.2]. This does not demonstrate
a GPU upload/AS build pass, but excludes nonfinite/empty CPU vertices, zero/wrong
CPU masks/references and misaligned player scratch addresses in this run. The
source's missing explicit alignment guarantee remains a separate portability
risk, not the demonstrated cause of these missing hands.

Next bounded discriminator: one same high-lantern capture with only the player
BLAS update mode changed to BUILD and source handle null on this Android Debug
investigation branch. Existing allocation covers max(buildScratch,updateScratch);
geometry, pose, masks, shaders, TLAS UPDATE and presentation stay unchanged.
This distinguishes the player BLAS update path; it is not a proposed production
rebuild/performance policy. Preserve negative results, then restore normal8CB.
No repeated performance matrix or shader recompilation. Normal Shipping stays
unchanged. Next unfinished step: build/freeze that new candidate once, capture.
Audio/haptic manual revalidation required: **NO** (unchanged semantic playback).
No main merge, release or publication authorised.

## Player BLAS BUILD discriminator completed

Source9eddddb, frozen `C:/Dev/tmp/horde-s24-instance-hits-20261001/player-build-debug.apk`,
SHA2563b5a241f9765aa0674ca4c7e29bd34d8c9fc55814fca21aa5ee71cdcf1e0db63.
Fresh four-ABI Debug build passes (21s); all70 assets and the actual embedded
ARM64 probe shader are identical to the CPU-fact control. Exact SM-S928B
run20261002-003413 passes high-lantern75% capture and Home/resume. PNG is again
byte-identical to normal8CB, player committed/candidate/direct-query counts0,
ID mask131469 unchanged. Finite CPU uploads and aligned scratch remain recorded.
This rejects a player-BLAS-UPDATE-only explanation; no rebuild policy is promoted.

Next bounded discriminator restores normal player BLAS UPDATE and changes only
TLAS UPDATE to BUILD (null source), using the same instances, masks, shaders,
poses and already max(build,update)-sized scratch. Capture high-lantern and the
two-enemy checkpoint, because both viewmodel and second-skeleton visibility are
open. No performance rerun or asset/IK changes. Build/freeze once, capture, retain
the result, then restore normal8CB. This is diagnosis, not a production policy.

## TLAS BUILD discriminator completed

Sourcefaef9de, fresh four-ABI Debug build passes (21s). Frozen
`C:/Dev/tmp/horde-s24-instance-hits-20261001/tlas-build-debug.apk`, SHA256
fdbb32cd050c4110210e5b0155fde462535f28942dd8ece60b17a03ca2c49d1c.
All70 assets are byte-identical to the CPU-fact control; shaders unchanged.
Exact SM-S928B run20261002-003825 passes two75% captures and Home/resume.

| Checkpoint | Viewmodel20 / World4 committed pixels | Independent viewmodel-only hit pixels | Skeleton2 /18 committed pixels | Metadata-based player pixels |
| --- | --- | --- | --- | --- |
| two-enemy-combat |44480 /22104 |67986 |5717 /7282 |66584 |
| player-viewmodel-lantern-high |133366 /34169 |168745 |12 /0 |167535 |

Lead image inspection confirms modelled hands now render in both captures. These
are expected corrected visibility changes, not a reason to restore buggy control
pixels or weaken image tolerances. Second skeleton now receives primary hits;
this is not yet live two-enemy/owner acceptance. Geometry, poses, materials, ray
masks, player BLAS UPDATE and shaders did not change. TLAS BUILD instead of UPDATE
is sufficient in these views; neither player geometry nor IK is implicated.

Independent source/spec review finds no illegal player activation transition:
an inactive Vulkan instance has a zero AS reference, not merely mask0. Slots4,
20 and18 retain valid nonzero references; changing masks/references is permitted
by TLAS UPDATE. Thus this result isolates the TLAS-update behaviour but does not
prove a particular driver defect or universal workaround.

Next finite candidate starts from normal0cf4f05 (no diagnostic counter remapping,
extra query or CPU logging). Rebuild only for discrete instance-definition changes
(visibility mask, BLAS reference and other non-transform fields), retaining UPDATE
for ordinary transforms and BLAS refits. Test the two affected S24 captures, live
route/lifecycle and relevant host contract before adoption. If this does not
retain visibility, do not claim the cheaper policy works. Preserve all negatives.
Normal8CB restoration is next before integration; no new performance claim.

Normal8CB restoration completed in run20261002-004452: exact install/pull SHA256
8cb976891e5719eceb7fd809ec91aac939ff3ec58eb2d6df44f15b5526f4ff87, honest RT
Home/resume, no data clearing or performance/capture repetition. Probe code is
not integrated into normal; durable receipts and this journal are retained.

## Bounded normal-shader candidate (device result pending)

Separate branch `codex/horde-tlas-instance-refresh` starts from normal6ec30f0,
whose runtime is0cf4f05. A small backend-neutral predicate selects BUILD for
changed masks, BLAS references, custom indices, SBT offsets or flags. Transforms
and BLAS-content motion retain UPDATE. Existing scratch/AS allocations and all
barriers remain unchanged. Two fixed21-instance CPU caches add2688 bytes plus
flags, no GPU buffers/allocations/traffic or shader changes. Cache advancement
occurs only after successful RT submission, independent of optional telemetry;
a new recording discards prior unsubmitted pending state. Move/destroy own both
caches. Submission is not falsely described as completed GPU execution.

An initial303DA6FB candidate built/tested but was **not installed**: review found
its cache advanced during recording before submit success. That is a concrete
validity problem requiring the corrected build, not an unchanged-artifact rerun.
Corrected four-ABI Debug build passes (24s); Windows full application Debug and
Release build, affected resource/preflight tests2/2 each PASS. Added regression
cases cover all discrete fields, normal transforms, count changes, move/destroy,
unsubmitted/rejected recording and successful-submit/idempotence behaviour.
Frozen `C:/Dev/tmp/horde-s24-instance-hits-20261001/bounded-submission-refresh-debug.apk`,
SHA256a627c4a6431f40708e14327f9585bd0e6c0e7ca3133a9ded97242dfd7763d761.
All70 assets equal normal8CB; CPU diagnostic probe string absent from ARM64.
Normal shaders/counters apply (no remapping or added query). Audio/haptic manual:NO.

Next finite matrix: two affected S24 captures first, then existing live deterministic
13-waypoint route/Home-resume if restored. Relevant S26/Windows images and current
source CI remain required before normal integration; no S25 or sustained-performance
acceptance is implied. Keep the exact artifact, completed tests and next step here.
