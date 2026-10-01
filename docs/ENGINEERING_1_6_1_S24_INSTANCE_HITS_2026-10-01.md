# S24 bounded primary-hit discriminator

Status: first discriminator complete; one follow-up query pending. Normal control source0cf4f05; exact
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

Next unfinished step: two second-probe captures and comparison; then restore
normal8CB. Do not rerun the completed first-probe matrix.
Audio/haptic manual revalidation required: **NO** (unchanged semantic playback).
No main merge, release or publication authorised.
