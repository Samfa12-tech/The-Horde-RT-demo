# Six backend pixels: finite output-only discriminator

Base/control source03a6870f66dd9cb4bab62d2f521f4fd663368678. Investigation branch
`codex/horde-backend-six-pixel-witness`; no production shader change or promotion.
Retained normal controls are the October1 Mobile13-image RTX pipeline/compute
sets at `C:/Dev/tmp/horde-primary-opacity-20261001/control-windows-{pipeline,compute}`.
Their exact six outlier coordinates/RGB are retained in the primary-opacity record.
Current TLAS integration already preserves those control PNGs byte-for-byte.

Question: do the six pixels disagree at the actual primary hit/material, ordinary
surface shading, fire integration, or subsequent atmosphere/display processing?
Common source alone does not prove identical native hits or arithmetic.

Bounded matrix: compile/validate the real module families once, build a Mobile
Diagnostic native RTX application, capture five affected checkpoints once on
each backend. No phone operation, performance sweep, changed traversal budget,
material/geometry/light change or tolerance. Retain all ordinary ray/shading/
counter work. Five output rows encode48 floats per selected coordinate using
the existing lossless two-RGB-pixel method; the other535 rows remain normal.
This is an observer, not a corrected renderer or image-equivalence candidate.

Before interpreting the witness, require honest RT presentation, exact extent
960x540, twelve settling frames, camera/assets/CPU geometry matching the normal
control and unchanged meaningful diagnostics where checkpoint scope permits.
Compare all non-payload pixels under the unchanged maximum3/fraction0.001 gate.
Record any observer-induced differences: if the original outliers do not recur,
do not label captured field differences as the cause of the normal image failure.

## Completed output discriminator

Exact candidate source: `d64bea152e40d7083643a941be018f7f27d8aa99`.
MSVC Debug executable SHA-256:
`1b984281d5f33ba443b377e8ffe3c5c548eb24b57bfd4bd53d600d5e6e0ea730`.
All sixteen real pipeline/compute modules compiled, optimized, disassembled and
validated. All eight Shipping modules remain byte-identical to the normal
renderer, with zero diagnostic atomics and no diagnostics binding. Frozen
budget file SHA-256 remains
`f2ff4be07c08536d140ea395fb24d1e3d502882449a84fc245df121e2ad89f4d`.
Native five-checkpoint pipeline and compute captures each exit0, honestly present
RT, and preserve camera/state, CPU geometry, complete visibility/diagnostic
records and static allocations. The ten PNGs and manifests are retained here.
No performance claim follows from this Debug observer.

The comparator initially rejected the whole static-asset record because its
four BLAS build-duration observations are nondeterministic. Only those named
timing fields are excluded from immutable identity. All allocation fields and
unknown future properties remain checked; image tolerances are unchanged.
No capture rerun was required. Configuration initially lacked the WebView2 SDK;
explicitly reused the existing pinned SDK. An incorrect build-target name was
corrected to `horde_rt_diagnostic_window`. The actual successful build log is
retained; neither setup failure is reported as a renderer failure.

Decoded result: [witness-comparison.json](witness-comparison.json). Every
non-payload pixel is **byte-identical** to its matching control (maximum0,
different fraction0). All six original outlier RGB pairs recur exactly:

| Checkpoint / pixel | Pipeline RGB | Compute RGB | Same primary instance / triangle / material |
| --- | --- | --- | --- |
| worst-bend 556,378 | 29,15,9 | 33,16,9 | 3 / 6803 / 100 |
| blue 396,262 | 9,13,24 | 125,21,25 | 0 / 625 / 4 |
| red 396,262 | 20,10,12 | 54,130,58 | 0 / 673 / 4 |
| finale-roof 552,395 | 34,16,2 | 38,18,3 | 3 / 7029 / 100 |
| finale-roof 770,526 | 18,21,25 | 13,15,20 | 0 / 809 / 2 |
| two-enemy 564,393 | 118,64,18 | 26,14,8 | 3 / 6999 / 100 |

The first substantial difference is already in ordinary `shadePrimary` output
at all six points, before fire/mist/display processing. Primary identities agree;
small normal/texture or ray-position differences exist. This does **not** yet
prove whether direct shadows or reflected secondary hits amplify them. It does
exclude a different primary triangle/material and a newly introduced fire/mist
effect as the original source of these six differences.

## Completed surface discriminator

Exact source `ee71f98a2e00cddb5c6b88fd5ba78834e06fc32a`; Debug executable SHA-256
`743379cf9fc58a322d762d47847fa9a64dabd1a9cdd7be567e9d117c5a90e17f`.
The second observer adds direct-light, reflected-hit/radiance and fog fields
only (85 total). Changed include dependency provenance required restaging;
all16 real modules compile/validate. The [module integrity receipt](surface/module-integrity.json)
checks actual nonempty SPIR-V hashes: all8 Shipping and all4 High Diagnostic
modules remain unchanged. No production budget or transport/counter work changed.
Both five-checkpoint native runs exit0. Source pixels, camera/state, CPU geometry,
complete visibility/diagnostics and allocations recur. All six original RGB
pairs remain exact. Observer-only non-payload differences are maximum1 for
pipeline worst-bend/blue/two-enemy and0 otherwise; fraction over1 is0 everywhere.
This small quantization perturbation is recorded, not called byte identity.
The original maximum3/fraction0.001 gate passes for observer validity, **not**
for pipeline/compute parity (the original failures remain).

Decoded [surface fields](surface/witness-comparison.json) distinguish:

| Point | Demonstrated split |
| --- | --- |
| worst-bend556,378 | Nearly identical direct light; reflected ray hits the same world triangle226, but changed mapped sword normal alters direction/position and reflected texture/radiance. |
| blue396,262 | Nearly identical direct light, same bounce direction; secondary hit changes from triangle627/material4 to641/material5 at approximately0.1082997m. |
| red396,262 | Nearly identical direct light, same bounce direction; secondary hit changes from triangle675/material4 to689/material5 at approximately0.1082997m. |
| finale552,395 | Nearly identical direct light; same reflected world triangle811, changed sword normal/direction and reflected texture/radiance. |
| finale770,526 | Identical bounce hit/radiance; direct sky visibility1 versus0 and skyDiffuse0.6990025 versus0.35445005. |
| two-enemy564,393 | Nearly identical direct light; reflected skeleton hit changes from triangle7109 to2259, with strongly different reflected illumination. |

The finale receiver position is z=-12 exactly on pipeline and z=-11.999999 on
compute. Existing `activeSkyLight` closed-room comparison uses z<=-12 and therefore
selects the open-finale aperture versus ordinary moon direction for the same
authored boundary face; finale open progress is1. This is a selector-boundary
discontinuity, not proof of two inconsistent visibility queries for the same ray.
No rounding-room-boundary fix, scene/material edit or tolerance relaxation is
accepted yet. Five other points are dominated by bounce differences, so one
blanket precision or shadow change is not supported.

## Bounded outward world-box correction (isolated candidate, not promoted)

Read-only authored-geometry inventory is complete. Bracket top and flame bottom
are coplanar at y=.50 and overlap the captured blue/red bounce. More importantly,
all six `addWorldBox` normal codes oppose their actual outward triangle winding.
The affected primary -X code points inside the box at its max-X face, sending
the hemisphere bounce/spawn into the solid and onto that internal interface.
This is a geometry-normal metadata defect, not evidence to tune player assets,
remove flame/metal faces or choose an arbitrary equal-distance material winner.

Candidate `7782e8eeb54850813c74c90b188969f908f4906f` swaps only those six normal
codes to match outward winding, plus a focused actual-source face/code regression
in the existing host smoke. Vertices, triangle order/count, material codes,
textures, masks, budgets and all shader modules remain unchanged. Fresh MSVC
Debug app/test build and affected CTest1/1 PASS (4.26s). Debug executable SHA-256
`05e8247fd9b7f213c2ca328ff2673076dd424dc7807f59b821582a6c2a16b567`.
No additional shader compilation or unchanged control capture was needed.

Both five-checkpoint native captures exit0 with scene/CPU geometry/diagnostics/
allocation checks intact. Blue/red now spawn outward and hit the same ordinary
world triangles489/488 on both backends. Their originally discrepant pixels are
exactly9,13,28 and31,9,11 on both; entire non-payload blue/red images have max1,
fraction over1=0 and **pass the unchanged backend gate**. The physical normal
correction intentionally changes old buggy control colours; old-control gates
fail and are retained, not loosened or forcibly restored.

[Full candidate backend scan](outward-boxes/backend-parity.json) retains four
original >3 outliers plus one newly exposed two-enemy point542,311 (max7,
183,134,52 versus190,140,54). Worst-bend max4, finale max6 and two-enemy max92
still fail. This is not full parity or production acceptance. Correct normal
orientation is demonstrated; promotion still needs investigation of that extra
point, normal uninstrumented/full affected RTX images, High physical fixtures
and affected Android-device evidence when available. No phone build/install or
performance claim belongs to this candidate.

[Selected-sky source reconstruction](surface/sky-selector-reference.json)
corroborates the two different selected directions against captured skyDiffuse
within2.48e-7/2.39e-8. This is source-derived double reference evidence, not a new
GPU direction/blocker payload or proof of the underlying shadow blocker.

Next unfinished step: implement/test the smallest justified closed-room selector
numerical correction on an isolated candidate; keep the outward-normal candidate
and newly exposed point separate. Do not repeat completed geometry inventory,
observers, controls or rejected precision/phone measurements. Later promotion
must earn real uninstrumented/cross-platform image gates. S26 is disconnected
by the owner until reconnection; do not poll/install or re-request owner checks.
No main merge, release or publication. Normal worktree/configuration unchanged.

Audio/haptic manual revalidation:NO: output-only diagnostics, semantic playback
inputs unchanged. Existing owner music-focus/S24 checks remain untouched.

## Closed light-region arithmetic candidate (latest completed stage)

Candidate `cc0491e601bc6665ae57a99b8920dcbc38efd719` is isolated on the same
witness branch, after outward-box control `94f0c4e`. This stage changes the
closed receiver-region predicate, not geometry, materials, ray masks, traversal
budgets, receiver positions, shadow-origin offsets or shadow transport. Corrected
region selection necessarily changes the selected light ray at the affected
boundary; identical buggy pixels are not the acceptance target.

The captured rays/t are **not identical**. At770,526, pipeline t=5.2704535 and
directionZ=.60715836 reconstruct z=-12; compute t=5.270455 and
directionZ=.6071583 reconstruct z=-11.999999. The authored triangle809 lies on
z=-12, which the existing closed finale region deliberately includes. A
conservative reconstruction-arithmetic neighbourhood preserves this inclusive
boundary policy instead of selecting the ordinary moon for one backend.

For non-overflowing finite binary32 arithmetic, the ordinary two-operation
reconstruction bound is gamma(2)(|origin|+|direction*t|). Eliminating the unavailable
origin gives gamma(2)/(1-gamma(2))(|computedPosition|+2|direction*t|). The scalar
GLSL helper rounds its nonnegative bound operations outward. Using the actual
ray component avoids a normalization assumption; accumulated nonnegative hit t
bounds the local parameter. This follows the standard
[floating-point error-bound method](https://www.pbr-book.org/4ed/Shapes/Managing_Rounding_Error),
but **does not certify hardware intersection-t error or the exact geometric
side of an arbitrary hit**. No universal backend-parity claim follows.

The existing host smoke includes the actual scalar GLSL body, the captured
boundary pair, all eight region endpoints, one-step neighbours and excluded
millimetre offsets, plus12 reconstruction fixtures evaluated with separate and
fused arithmetic against double references. Fresh MSVC Debug1/1 PASS4.40s and
Release1/1 PASS1.96s; app build PASS. Debug executable SHA-256
`e5e68dd444dd8077f7c5d4b072caabb8c4c9134c20ee6a7eb0c6b501eecb58df`.
Read-only independent review found no concrete derivation/call-site flaw within
that finite, non-overflowing arithmetic contract.

All16 real module families compile, optimise, disassemble and validate. Decoding
each actual embedded artifact reproduces its nonempty top-level catalog SPIR-V
hash and byte count. All8 Shipping modules still have zero diagnostic atomics
and no binding22. Unlike earlier output-only stages, this shader candidate
intentionally changes Shipping/High modules too. Relative to94f0c4e it adds
3060bytes/180 instructions to Generic and13064bytes/791 instructions to Opaque
in both backends. It exceeds existing frozen cost limits; their exact file hash
remains `f2ff4be07c08536d140ea395fb24d1e3d502882449a84fc245df121e2ad89f4d`.
**Cost admission FAIL; not promoted.** No budgets were raised. Shader size is
not a register/occupancy/performance measurement.

The finite matrix is complete: one new five-checkpoint capture per backend,
each exit0 with honest RTX presentation at960x540,12 settling frames and time0.
The retained control comparison preserves scene/CPU geometry/allocations/full
diagnostics; its old-colour image failures are explicit expected differences,
not passing regression gates. No old control was rebuilt or recaptured.
[Full paired scan](closed-light-region/backend-parity.json),
[actual fields](closed-light-region/witness-comparison.json), all ten PNGs,
manifests/OBJs and shader/build/run logs are retained in `closed-light-region/`.
Native finale image inspected; payload rows remain diagnostic-only.

770,526 now yields exactly18,21,25 on both backends; both sky visibilities=1,
skyDiffuse=.6990025/.69900256, direct colours agree within1e-9. Blue/red still
pass max1/fraction over1=0. Four remaining >3 outliers are unchanged:
worst556,378 max4; finale552,395 max4; combat542,311 max7 and564,393 max92.
Thus three of the six original discrepant pixels have demonstrated isolated
corrections, with one extra combat point exposed by the outward-normal stage;
whole backend parity still fails. No tolerance relaxation or phone/performance
acceptance is claimed.

Next unfinished step: inspect the unobserved combat542,311 reflection layer;
reuse the85-field discriminator if new GPU evidence is necessary, with only a
bounded two-enemy capture on each backend, not another five-checkpoint/control
rerun. Keep the three original mapped-normal/reflection outliers and High
row43 separate. The boundary candidate is parked pending cost/integration
admission and uninstrumented images; do not repeat its completed finite matrix
or start a precision/epsilon sweep. Normal worktree/configuration remains
untouched; no phone action, main merge, release or publication.

Audio/haptic manual revalidation required:NO: receiver lighting only, unchanged
gameplay/event-time/listener/PCM/haptic semantics. Existing owner gates unchanged.

## Combat reflection localization (completed; next sampling discriminator)

Source `1975e0317cc538e29a44486076d85768666021fd` retargets only the existing
Mobile/Diagnostic witness and limits its native capture to checkpoint12. Exact
Debug executable SHA-256
`2d9ebdb347c0c6dca8912b3113f8a03553588dc079bbaa53af672f054d889896`.
Fresh app/affected-host build PASS; affected MSVC Debug CTest1/1 PASS4.82s.
All16 real shaders compile/optimise/disassemble/validate. The8 Shipping and4
High/Diagnostic SPIR-V hashes remain byte-identical to parked `ac15d64`;
only4 Mobile/Diagnostic observer modules change. Cost admission remains the
already-recorded FAIL; no budgets or normal Shipping configuration changed.

The finite matrix is complete: one checkpoint12 native RTX capture on each
backend, both exit0. [Decoded fields](combat-point/witness-comparison.json)
preserve all non-payload pixels exactly against the retained closed-light-region
control, plus camera/state, complete visibility diagnostics, CPU geometry and
allocations. [Full paired scan](combat-point/backend-parity.json) still fails the
unchanged max3/.001 gate: two outliers, maximum92. Do not rerun these captures.
Manifests and PNGs are retained; identical CPU-geometry OBJ identities are checked
against the preceding control instead of duplicating its large files.

At542,311 both primary rays/t/positions and instance3/triangle5523/material100
are identical. Primary mapped normals differ by up to8.2606e-5; direct RGB differs
by at most.00050387. Reflected directions differ by up to.0001612584, selecting
world triangle5 at t2.709058 versus triangle23 at t1.2331331. Reflected red
radiance is.00027323925 versus.19996978. Display RGB remains183,134,52 versus
190,140,54 (max7). The earlier564,393 outlier still selects skeleton triangles
7109/2259 (max92). Fire/mist do not cause either divergence. This localizes the
new point to mapped-normal/reflected-hit sensitivity, not a new glass-transport
or selected-light-region failure; it does not yet establish which arithmetic
or sampling operation produces the primary-normal difference.

Source inspection and actual pipeline disassembly show PBR texture operations
already use explicit LOD0; changing GLSL `texture()` to `textureLod(...,0)` is not
an evidence-backed fix. No geometry, material, texture, normal smoothing,
reflection direction, tolerance or quality change is admitted from this result.

Next finite discriminator: capture the actual primary barycentrics, interpolated
UV/tangent frame and normal/base/ORM samples at these same two points. Reuse the
existing reserved output rows and primary query, with no extra rays, GPU-buffer
ABI or framework. One new checkpoint12 capture per backend; require exact
non-payload recurrence before interpreting its fields. Compare Shipping/High
bytecode to this control. Do not repeat the completed five-scene matrix or begin
a precision/epsilon sweep. A demonstrated differing layer, not confidence,
must justify any subsequent candidate. S26 remains owner-disconnected; no phone
poll/install, main merge, release or publication.

Audio/haptic manual revalidation required:NO: investigation output only;
gameplay/event-time/listener/PCM/haptic inputs unchanged.

## Primary PBR sampling discriminator (completed; one targeted candidate next)

Source `a41afdb41f21e632419b061682669abde1b50ba5` appends30 fields from the
existing primary PBR decode to the85-field observer. No additional query or
texture sample is performed; five output rows remain reserved, no GPU-buffer
or HitInfo ABI changes. Exact Debug executable SHA-256
`f4f13c0ab13a40cdc34ef0450f1b3d3e86918c89b6db55dbbed9021eea9afe10`.
Fresh app/host build PASS; affected Debug CTest1/1 PASS4.25s. All16 shader
families compile/optimise/disassemble/validate;8 Shipping and4 High/Diagnostic
SPIR-V hashes remain byte-identical to `c007066`, with all8 Shipping modules
still zero atomics/no binding22. Only4 Mobile/Diagnostic observer modules change.

Finite matrix DONE: one checkpoint12 capture per backend, both exit0. The
[115-field comparison](primary-sampling/witness-comparison.json) reproduces
every non-payload pixel exactly against `1975e03`, with complete scene/CPU
geometry/visibility/allocation identity. [Backend parity](primary-sampling/backend-parity.json)
still fails: the same two outliers, max92. These fields are interpretable
because the original rendering recurs; they are not a corrected-image pass.
Do not repeat this completed pair or its unchanged control build.

At both542,311 and564,393, the captured hardware barycentric components are
bit-identical across backends. UV X differs exactly one binary32 ULP (pipeline
larger), UV Y is identical. Tangent-frame components differ only about1e-7,
whereas sampled normal X differs9.1554e-5 and.0007019043 respectively. Base and
ORM samples also differ. This demonstrates that interpolation has already
diverged before the texture operations; it does not claim no other differing
operation exists or guarantee that matching UVs alone will close parity.

Next bounded candidate: constrain contraction/reassociation only at the existing
three-vertex PBR UV interpolation with GLSL `precise`. No UV quantisation,
texture/LOD/material changes, smoothing, reflected-ray adjustment or tolerance
relaxation. Inspect actual SPIR-V and frozen-cost delta, then one checkpoint12
capture pair against this retained control. Preserve expected corrected-image
differences and evaluate the unchanged backend gate separately. This is not
permission for a wider precision sweep. Normal Shipping remains unchanged;
the earlier light-region candidate still fails cost admission. No phone action,
main merge, release or publication. Owner-disconnected S26 gates remain open.

Audio/haptic manual revalidation required:NO: output-only investigation;
gameplay/event-time/listener/PCM/haptic inputs unchanged.

## Ordered PBR UV candidate (combat gate PASS; remaining scenes next)

Source `877f5670f2093c72730e315491b61139fc320667` changes one shader expression
to `precise vec2 uv`, preserving all three authored vertex weights. The existing
host smoke now guards that source contract. No UV snapping, texture/LOD/material
change, normal smoothing, ray offset/direction policy, geometry or tolerance
change. The115-field observer is unchanged. Independent read-only review
supports this bounded experiment, not general cross-device equality.

Exact Debug executable SHA-256
`4a9f27250cc6742b5fe5b00f88556e85237e7ec1e9a0d1ab63103dd6e345d430`.
Fresh Debug app/test build and Release test build PASS; affected CTest each1/1
PASS (Debug4.21s, Release1.97s). All16 real shaders compile/optimise/disassemble/
validate. Each Generic grows84bytes/7 counted instructions and each Opaque
504bytes/42 counted instructions in both backends. Actual pipeline Mobile
Shipping **and Diagnostic** disassembly is otherwise identical to the preceding
control after removing `NoContraction` decorations. Those constraints cover the
two weight subtractions, three vector/scalar products and two additions at each
interpolation site. This does not identify native FMA/register/occupancy costs.
All8 Shipping modules remain zero diagnostic atomics/no binding22.

The finite combat pair is DONE, both native captures exit0 and preserve scene,
CPU geometry, complete visibility diagnostics and allocations. Expected image
changes versus the old unconstrained-UV control are retained, not labelled a
regression pass. At542,311 the pipeline image changes to the compute result;
at564,393 compute changes to the pipeline result. There is no forced restoration
of one backend's entire old image. [New fields](precise-pbr-uv/witness-comparison.json)
show bit-identical UVs and sampled normals at both points; reflected triangles
now agree (world23 and skeleton7109). Small later floating-point differences
remain. [Whole two-enemy paired scan](precise-pbr-uv/backend-parity.json) passes
the **unchanged** max3/.001 gate at max1, fraction over1=0, no outliers. Native
candidate image inspected; diagnostic payload remains confined to five rows.
This demonstrates the ordered interpolation removes the observed sampling fork
on these exact RTX artifacts, not universal parity or phone acceptance.

No normal renderer change or cost admission is claimed. Even without the parked
light-region helper, the normal frozen Generic ceiling equals its243852-byte
baseline; this candidate adds84bytes. Opaque's normal bytes/instruction headroom
is412/18 versus this candidate's504/42 decoration growth. Cost gates remain open;
no budgets were raised. A shader size increase is not a measured frame-time loss.

Next finite step: check the remaining original reflection checkpoints2 and11
with this exact shader set, one two-checkpoint native run per backend. Do not
recapture completed checkpoint12 or rebuild unchanged shader modules. Then
prepare a clean uninstrumented candidate with only demonstrated, separable
corrections and earn actual cost/full affected RTX/High/device gates before
promotion. No precision sweep, phone action, main merge, release or publication.

Audio/haptic manual revalidation required:NO: UV arithmetic only; gameplay,
event-time/listener/PCM/haptic inputs unchanged.

## Remaining ordered-UV reflection scenes (completed; clean admission next)

Source `36c9ec5ad1945a0c2df9b84a50fed58bb9c67ad0` changes only the isolated
capture roster/manifest and comparison scripts to checkpoints2,11. Shader source,
actual embedded modules and generated catalogs remain unchanged from877f567;
no shader recompilation or completed checkpoint12/control rerun was performed.
Fresh native app build PASS; exact executable SHA-256
`ee024233a2e27021246b1403814e582e91b2277060a14fa911d70a3ec798b60b`.
The unchanged shader/test candidate retains fresh Debug/Release1/1 evidence
above; this harness-only step does not claim another host-test run.

Finite matrix DONE: one two-checkpoint capture per backend, both exit0, with
honest RT presentation. [Control comparison](ordered-uv-remaining/witness-comparison.json)
preserves scene/CPU geometry/complete visibility diagnostics/allocations against
the existing closed-light-region control; expected UV-corrected pixels differ
and old-image failures are retained. [Paired scan](ordered-uv-remaining/backend-parity.json)
passes unchanged max3/.001: worst-bend max1/fraction over1=0; finale max3/fraction
over1=3.894080996884735e-6, no >3 outliers.556,378 now exactly29,15,9 on both;
552,395 exactly38,18,3. Reflected triangles226/811 and radiances now agree up to
small floating-point differences. The corrected boundary770,526 remains exactly
18,21,25. A negative comparison using the wrong one-checkpoint roster is rejected
before result creation; strict finite coverage was not weakened.

Together with the completed combat pair, all four remaining >3 reflection
outliers in this isolated Mobile RTX investigation are closed. This is not a
single full13-image/uninstrumented production gate. Earlier blue/red passes
belong to their exact retained source, not a newly rerun whole matrix. High row43
is still a separate demonstrated false-candidate problem. Cost, full physical
High fixtures, normal Shipping/Diagnostic and exact-phone gates remain open.

Next unfinished step: prepare a clean **uninstrumented** candidate containing
only separable outward-world-box normals and ordered PBR UV interpolation on
the normal engineering source; do not merge the witness branch or automatically
include the cost-failing light-region helper. Record exact cost admission and
normal full affected RTX evidence, with the known boundary/High negatives honest.
Do not repeat these completed finite observers or start a precision sweep.
S26 is owner-disconnected; no polling/install or new phone claim. Normal renderer
unchanged; no main merge, release or publication.

Audio/haptic manual revalidation required:NO: capture/UV arithmetic only,
unchanged feedback semantics.
