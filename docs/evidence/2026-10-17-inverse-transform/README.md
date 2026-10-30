# Inverse-transform experiment — 17 October 2026

**A promising Opaque-only Pipeline result, not a production adoption or a Keeper-fight fix.** One inverse-transform treatment was tested. Warm Pipeline dispatch medians fell about 6–8% in the two ordinary interior views and 3% outside. The existing frozen Keeper still view changed by less than1%. QueryCompute’s later control is thermally incomparable and is excluded from speedup claims. The generalised source candidate fails unchanged Generic byte/word ceilings. Production shaders, quality, assets and loading rules remain unchanged.

## Treatment, source and limits

The private PBR hit decoder replaces `transpose(inverse(objectToWorld))` with `transpose(mat3(rayQueryGetIntersectionWorldToObjectEXT(query, true)))`. The [Khronos ray-query definition](https://raw.githubusercontent.com/KhronosGroup/GLSL/main/extensions/ext/GLSL_EXT_ray_query.txt) supplies the committed intersection’s world-to-object matrix. For an invertible affine transform its 3×3 part is the inverse required by the normal matrix. Forward transforms for positions, geometric normals and tangents, winding/hemisphere handling, normal-map strength, materials and all ray decisions remain unchanged.

Native inputs are current `db4c00a05211738d5d3b046c6e5a2dddd1ea59f2`; clock observation is compiled out. Shader inputs deliberately use the already-working **private r10 region-page profile**, whose own historical source reference is `2147e26b0af6327073b9a31003897563de26d22b`. This is not a reset, the current production shader tree or a pristine later-commit APK. All four baseline Shipping/Mobile module hashes independently match the byte-verified installed control. Only one line of shader source differs; exact original/private source and the patch are archived. The native override contains only variant headers/modules, not old gameplay headers. [Profile and hash custody](source-receipt.json), [installed-control module matches](control-module-parity.json).

CPU tests independently compare cofactor inversion and elimination-derived inverse matrices for identity, rotated nonuniform scale, shear, mirrored shear, wide scale and both tangent handedness signs. They verify normal/tangent/bitangent and mapped-normal agreement, plus a wrong-transpose counterexample. This is mathematical coverage, not proof of Vulkan driver results or admission of mirrored assets; existing import restrictions remain intact.

## Frozen static limits

All eight Pipeline variants compile/validate and retain their existing structural strategy. All four Opaque variants fit every frozen ceiling. All four Generic variants have28 fewer instructions but exceed bytes by64 and words by16; the normal frozen publisher correctly rejects the bundle. No ceiling/check was changed or waived. Shipping/Mobile Opaque falls from588,184 to579,920B and32,695 to31,962 instructions (−8,264B/−733instructions), retaining one function/no calls/27query sites. Its six MatrixInverse static sites become six world-to-object query sites. Static counts are not dynamic GPU cost or driver ISA.

For the physical experiment, **only the budget-passing Shipping/Mobile Opaque pair is substituted**. Generic and all other runtime policies retain exact baseline artifacts. Both selected modules validate, contain no clock operation/diagnostic atomics/binding22, and are bound to the actual completed frames. Compute Opaque is581,052B. Generic runtime performance/transport and High runtime performance are not claimed. This curated private pair is not a published fresh all-policy catalog. [All eight budget results](pipeline-budgets.json), [exact selected modules](private-modules.json), [live frame identities](live-module-identities.json).

## Artifacts and matched images

Experimental APK SHA256 `84e878a3dd8bbc0b56f5ead59fb9304e46c07a83f0bc4c8de25e63c760c7ce2d`,183,655,002B. Original/control `f13dda30ad56536a774004ee963c621aec2396f0a25179c490f32e72294fa527`. Both use the same development signer/versionCode10. Candidate install/pullback and restored-control bytes match. All169 asset payloads and original preference bytes are identical; the release app is untouched. The original control is restored and Debug stopped after the tests. No uninstall, app-data clearing, signing, merge or release.

SM-S948B/API37/Android17, Adreno840/Vulkan1.4.295, driver2150932499; native Debug/O2 and actual Shipping/Mobile shaders. Saved40%/Current,576×1188 internal,1440×2971 swapchain, cap30, water1/fire0, glassOff, mistOn and dust1 remain unchanged. No sparse shader clock is active; existing timestamp/stage/route evidence is common to both builds. Optimized Debug phone observations are not a signed Release candidate or owner smooth-play acceptance.

Four frozen checkpoints on each backend have identical camera/player state, animation time, extent/exposure, water/RT Lab settings and ordinary geometry. Landing, chamber-after-death and the legacy active Keeper view are pixel-identical on both backends. Each forest-bend image differs at50 of4,492,800 pixels, at most one channel code out of255. Six exact and two near-identical comparisons cover35,942,400 pixels;100 differing pixels total. No new tolerance was used to force a production gate. Actual Keeper Pipeline and bend Query images were opened and inspected without editing; no visible change was found. [Full numeric parity](parity.json). [Keeper /Pipeline](keeper-pipeline.png), [bend /QueryCompute](bend-query.png). All original image pairs are archived.

## Ordinary route measurements

Order: Query candidate, Pipeline candidate, restore control, Pipeline control, Query control. Each build/backend gets one unchanged input-driven route. Original120s route/30s arm/2s drain and adverse guards remain; no fixed-delta override, forced pose, readiness bypass or mid-timing image readback. Source, compiler output and failed preflights are retained. Strict analyzers join all2,708 completed frames to their exact owned stage and actual display records. Holds retain60–300simulation ticks; each view is stationary, with control/candidate arrival differences below5mm and identical angles.

| Pipeline held view | Control dispatch median | Candidate dispatch median | Observed change |
| --- | ---: | ---: | ---: |
| landing-sweep | 142.224ms | 138.185ms | -2.84% |
| lower-sweep | 68.020ms | 62.523ms | -8.08% |
| passage-sweep | 66.085ms | 61.817ms | -6.46% |
| repeat-ascent | 142.143ms | 137.668ms | -3.15% |

Pipeline starts match AP/BAT/SKIN within0.4/0.3/0.1°C at framework status1. Ends are status1 but control is warmer by1.3/0.6/0.3°C. This is a **promising descriptive single-pair observation**, not repeated sustained proof or a causal gain isolated from clock/power/driver-cache effects. Query starts differ by4.0/4.0/2.9°C and status0 versus2; its later hot control is retained for functional evidence only and excluded from speedup claims. No thermal retry or additional inverse variant was started. Hardware counter, clock, power, cache/occupancy and driver ISA remain unavailable/unmeasured.

| Run | Exact stage/display joins | Actual display median /p95 /maximum | Full → retired → restored /observed peak B |
| --- | ---: | ---: | --- |
| query-candidate | 681 | 83.33 /154.17 /158.33ms | 160830720 → 146581056 → 160830720 /174306880 |
| pipeline-candidate | 690 | 83.33 /158.33 /170.83ms | 160831232 → 146581568 → 160831232 /174307392 |
| pipeline-control | 664 | 83.33 /162.50 /175.00ms | 160831232 → 146581568 → 160831232 /174307392 |
| query-control | 673 | 87.50 /162.50 /175.00ms | 160830720 → 146581056 → 160830720 /174306880 |

All four routes unload and restore once, save14,249,664B (13.59MiB), restore exactly and have zero readiness waits. Peak overlap above full remains13,476,160B (12.85MiB); no resource/allocation count or owner changed. The certified84,219-triangle V/I/private-primitive/BLAS detail owner retires; coarse world/terrain/landmarks, shared texture/materials, collision, actors/equipment and uncertain secondary-ray contributors remain. Cached CPU sources/package I/O/default-production residency admission retain their prior limits. Actual display medians83–88ms and maxima158–175ms remain poor; GPU medians are not playable FPS, scanout or latency. This is not a completed chapter or smooth streaming acceptance.

## Keeper-room observation

The added existing `lich` checkpoint shows a live Keeper in **maintaining-range**, health3, animation time0, with the same two fire emitters and exact still image. It is legacy benchmark framing, not an authored full-chapter camera or a live attack. Each of four runs completes the original120-frame warmup plus three120-frame host-loop windows. Periodic GPU aggregate snapshots contain390 samples, including warmup; they are not final/phase-isolated benchmark medians or actual display pacing. These supplemental snapshots show Pipeline42.613ms control versus42.247ms candidate (−0.86%). Host window means remain about44ms. Query control becomes hot/status1 versus candidate0 and is excluded. [Exact Keeper states/windows/thermal records](keeper-summary.json).

**The owner's lightning/charge performance complaint has not been reproduced by this still view.** Neither these data nor the post-death route hold explains the actual worst combat frame. Do not interpret the inverse change as fixing that complaint or infer the cost of a particular light. The next useful performance investigation is an exact authored Keeper wind-up/discharge frame or short phase-labelled fight capture, preserving both lights, shadows, mist and quality.

## Checks and disposition

New normal/tangent tests3/3 pass. Private Android four-ABI assemble/lint passes. An isolated optimized Windows Shipping/Mobile Release build passes; no Windows RT image/performance claim is made. Production shader source, embedded modules and frozen budget hash `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf` remain unchanged. The workflow now executes the three CPU experiment tests. Audio/haptic manual revalidation required: **NO**, because feedback, simulation timing and playback are unchanged. [Validation receipt](validation.json).

The first freeze command lacked its required explicit absolute artifact/catalog/budget paths and stopped before compilation. Correcting only those invocation arguments then produced the real Generic budget rejection. A retained matrix compilation gathers all eight exact statistics; it is not another shader candidate or a weakened publisher. No production files were published by either attempt.

**Disposition: retain one promising experimental Opaque optimization; do not adopt the generalised shader change or raise Generic ceilings.** A later production-bundle trial could retain the existing Generic calculation and restrict this treatment to Opaque, then prove full source/catalog freshness and both backend/image/performance admission. Do not spend more inverse iterations claiming this fixes the active Keeper. Current/Higher and saved Lower→Current physical acceptance, active combat worst-frame attribution and wider chapter/streaming acceptance remain open. Mist28, canopy/Easy Tree and later features stay deferred.

[Complete route summary](summary.json), [exact APK identity](candidate.json), [source/run packet](source-and-runs.zip), [packet receipt](receipt.json).
