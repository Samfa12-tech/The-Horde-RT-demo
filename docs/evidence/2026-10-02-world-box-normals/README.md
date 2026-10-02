# Production-only closed world-box normal correction

Base: `d2682a5` (runtime unchanged from `75d0b4c`). This integrates only the
six signed-axis metadata corrections independently demonstrated in `4e951c2`.
No isolated UV/contact/probe shader, vertex/triangle/winding/material/asset change,
ABI change, traversal budget or numerical-tolerance change is admitted.

## Cause and bounded scope

`addWorldBox` emits outward-wound closed solids but encoded all six shading normals
inward. CPU codes0..5 decode in GLSL to +Y,-Y,+X,-X,+Z,-Z; emitted faces are
-Z,+Z,-X,+X,+Y,-Y. Correct only the six labels to agree. This fixes real hemisphere
and offset orientation rather than making unrelated pixels match. Interior-facing
room quads and the separately correct `addBox` are unchanged. Player/grip authority,
geometry, skinning and primary/secondary ownership are untouched.

## Finite validation matrix

| Row | Status / result |
| --- | --- |
| Regression before runtime fix | COMPLETE: new Debug contract fails on all six faces, CTest exit8 (4.43s); CPU/GLSL signed-axis checks do not fail. |
| Affected Debug + Release host contracts | COMPLETE: character slot, initialization preflight and scene inventory each3/3 PASS, Debug10.23s / Release4.38s. Both actual Windows application builds pass. |
| Native Windows RTX images | COMPLETE, bounded: six candidate High/Diagnostic captures exit0, honest RT, RTX5050 Laptop; torch opening and held-high/low on pipeline + compute. Lead inspected the opening and both held views. No image-equivalence claim against erroneous old normals. |
| Shader/asset containment | COMPLETE: no shader, asset, shared gameplay or platform diff. Own-backend candidate/control module pairs are identical; uploaded world-body/viewmodel geometry SHA pairs match in all seven comparisons. No shader rebuild. |
| Android build preparation | COMPLETE: all four Debug native ABIs and assembleDebug PASS37s. APK retained; not installed and not a phone presentation pass. |
| Exact Android affected route | COMPLETE, bounded Debug: S26 run170028 passes13/13 replay, four authored captures and honest RT after Home/resume. Exact APK install/pullback agrees. First120s replay timeout retained. Not Shipping performance or final-candidate acceptance. |

Raw build/test/capture logs retained at `C:/Dev/tmp/horde-world-box-normals-20261002/`.
Keep exact binary/source hashes with new images; the historical isolated shader
candidate is not evidence for this production-only candidate. Corrected-normal
image changes must be identified, not forced back to wrong-normal baseline pixels.
Existing strict failures/tolerances remain intact. No performance gain claimed.

Audio/haptic manual revalidation required:NO: renderer-only surface metadata,
unchanged simulation events, listening/source inputs and playback.

## Exact candidate and bounded image results

Changed scene source Git blob `425bd8a3d22968598f43cc3328993835cf7ed2e3`;
build input file SHA256 `3a96b8ad73e474336336b09f012d84943192bb7f8d73949975b34e01f3e3d419`.
Dirty candidate was built on `d2682a5`; subsequent evidence/prose do not change
that input. Debug Windows executable
`819176a7a71b5327dc546e37f18df915322b0ce5f76d92170cf2648988e53107`;
Release `07cc42cdf18b39c63e7372a9d9e9aa1d8a868b4e75b66ea8fa2629bdfbcdfd79`;
Android Debug `e9fd31e7c9aee83a13d0a91f25e2b61497eb986940032dbb534e13d9787f4774`.
No Shipping timing or physical Android claim follows from these builds/captures.

Retained normal control executable `3526bc9d...` was previously recorded at
`d9be81e`; scene/shader/asset Git trees match the new base before this correction.
Exactly two opening control images fill a missing affected view before the old
executable is overwritten. Existing held-high/low compute controls are reused,
not rerun. [Candidate](candidate/run-receipt.json), [control](control/run-receipt.json)
and [comparison](image-comparison.json) retain provenance and unchanged criteria.

| Comparison | Max RGB difference | Pixels >1 / >3 | Strict old tolerance |
| --- | --- | --- | --- |
| Old/new torch opening, each backend |60|13306 /10642|FAIL: expected corrected-normal lighting, not an equivalence pass |
| Old/new held-high, compute |94|332 /210|FAIL: retain physical shading changes |
| Old/new held-low, compute |94|270 /129|FAIL: retain physical shading changes |
| Candidate pipeline/compute torch opening |2|2 /0|PASS |
| Candidate pipeline/compute held-high |53|9 /6|FAIL: known backend outliers remain parked |
| Candidate pipeline/compute held-low |14|7 /5|FAIL: known backend outliers remain parked |

Corrected lintel/metalwork shading is a consequence of outward face orientation,
not a reason to restore buggy old pixels. Cameras/extents and player geometry
match. All six candidate captures retain zero overflow/unclosed/stack failures;
interface/volume/mismatched-exit counters are0, real TIR counts1096 high/659 low
on each backend. This bounded result does NOT close the separate edge/contact/
row43 physical cases or prove all live High glass paths. No diagnostic suppression,
counter reset, tolerance change or performance inference.

Source-text regression pins actual authored faces and CPU/GLSL signed-axis ABI;
independent review checked signed-axis cross products and
found no room-card/player coupling. It is formatting-sensitive, not a substitute
for full runtime mesh validation. PNGs/manifests/logs are retained here; unchanged
large OBJ exports remain at the raw root and their SHA agreement is in the JSON.
Initial wrong application build target (`HordeLanternRT` rather than
`horde_rt_diagnostic_window`) failed MSBuild before compilation; corrected build
passes. Image-analysis stdout contained a Pillow deprecation warning; the raw
output is retained outside Git, the unchanged JSON body was recovered without
re-running captures. No failing test or image metric was removed.

## Exact-phone continuation (owner reconnected)

Retained APK `e9fd31e7...` was rehashed and installed only to the S26 Debug package;
no rebuild or app-data clear. First finite Replay attempt `run-20261002-165649`
exceeded the default120s deadline after10/13 waypoints in red-torch-bay, with
honest RT and no scoped crash marker. No capture/lifecycle row had yet run.
`timeout-state.json` and `timeout-logcat.txt` preserve that incomplete attempt.
This is not a route pass or a Shipping performance trial.

The one retry `run-20261002-170028` uses the existing supported300s deadline,
SkipBuild/SkipInstall, the same exact APK and75% scale. It exits0:13/13 waypoints
reach finale, four captures reach their12-frame/honest-presentation markers,
Home/resume recreates the surface and honestly presents RT. Strict ASTC,
RayTracingPipeline, Diagnostic/Mobile/OpaqueFast,1080x2235, modelled-viewmodel,
60Hz skinning, dedicated nonduplicating primary ownership and exact grip checks
pass. Source receipt records00af842/dirty (unrelated raw S24 evidence and later
documentation); runtime input blob and retained APK above are unchanged.

Lead inspected all four images: modelled hands/props persist, high/low open-aperture
lantern lighting/shadows remain, and the combat fixture's two enemy bodies are
partly visible at the lower edges of its close authored camera. Native state
confirms two active enemies; this is not a new claim that both full silhouettes
are in that camera. No frozen-image or pixel-equivalence/performance gate is
manufactured. Timing CSV is empty because Mode Replay deliberately runs no
benchmark. [Retained finite phone receipts](phone/run-20261002-170028/summary.json)
and [initial timeout](phone/run-20261002-165649/timeout-state.json).

Do not repeat completed host/RTX/phone rows or resume UV/sub-pixel sweeps without
a specific validity problem. A later rapid Home/resume from the real report form
hit an input-dispatch ANR during UI-thread Vulkan pipeline creation; that separate
lifecycle failure is OPEN in the report continuation record, not concealed by
this runner's successful settled Home/resume row.
