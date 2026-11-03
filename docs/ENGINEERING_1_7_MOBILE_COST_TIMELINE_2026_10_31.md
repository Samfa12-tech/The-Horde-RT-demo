# Phone performance history and selectable-cost investigation

31 October 2026. Owner direction: recover the history of the earlier fast phone
builds, identify added costs, and provide measured lower options while preserving
the features and full-quality ceiling. Fire is a hypothesis, not an established
single cause. This extends the [current baseline checkpoint](evidence/2026-10-31-mobile-efficiency/README.md);
it does not authorize feature removal or a broad historical rebuild/bisect.

## What can be traced

| Stage | Evidence and feature change | Attribution limit |
| --- | --- | --- |
| 12 July, `3408f3d4` | One animated enemy, textured/PBR RT scene. [Retained recovery report](SKELETON_PERFORMANCE_2026-07-12.md): 126 SurfaceFlinger intervals, median16.667 ms / p9520.833 ms. Removed an extra16 ms sleep, indexed skinning before RT expansion,30 Hz enemy skin/refit, native Debug `-O2`. | Small earlier scene; not today's chapter. This is the earliest directly traceable fast presentation sample found in this bounded review, not a rebuilt/current-driver reproduction. |
| 31 July, body/vitality/dawn feature build | Locally retained raw report and timing CSV were reread. Exact APK `a9fe35932548f38e067c39ef604739aaa7eacd12005d119a88c0b77e2d7fa81e`; SM-S948B/Android16,75%,1080x2235 ->1440x2980. Opening/bend/skylight/green/lich medians of three120-frame window averages: **11.402 /7.767 /7.096 /11.209 /15.727 ms**. | CPU frame start through present return, not GPU timestamps or achieved display FPS. Temperature/run order changed. APK identifies `0.1.2-alpha.1 DEBUG` because it preceded the0.1.3 release identity. The subsequent feature commit `5bc64ef9` is an ancestry reference, not asserted exact APK provenance. |
| July baseline content boundary | [0.1.3 release validation](SHOWCASE_ALPHA_0_1_3_RELEASE_VALIDATION_2026-07-31.md): one active skinned enemy,18 TLAS instances/eight BLAS; no water or simultaneous-enemy expansion. Earlier shader already had real torch/sky visibility and a deterministic RT bounce. | It was not an unlit raster baseline. Do not infer that all later shadow or bounce work is new. Exact early APK rebuildability with today's cached tools/assets is not yet established. No old package was installed. |
| 23-26 August | `2bda83aa` RT waterfall mist; `f4891c48` real-water/RT Lab release; `bde918c3` coherent world-space fire volumes; `acdd136a` coherent torch light; `2670d610` imported dielectric properties; `e361a5c0` transparent shadow transmittance. Skinned player and physical reward-lantern integration also expanded the workload. | Git establishes additions and ordering, not their individual cost. Later hot Diagnostic samples must not be divided by July CPU averages to invent a regression multiplier. |
| 30 September, targeted fire isolation | [Retained experiment](evidence/2026-09-30-opening-lighting-isolation/README.md),75% Shipping/Mobile: opening control GPU66.670/66.423 ms, fire-shadow removal isolate52.867 ms, volume removal isolates63.364/63.484 ms. | Nonphysical/removal isolates were diagnostic only. Cooling changed and no clock trace existed. They point toward fire **lighting/visibility**, but do not establish a deployable saving or say that flame marching explains everything. |
| 5 October, two Keeper flank torches | Exact ABBA [comparison](ENGINEERING_1_6_2_KEEPER_TORCH_COST.md): pooled route GPU62.401 ->72.086 ms; finale61.923 ->129.912 ms. Geometry, direct fire lighting and source-occluded mist were added together. | Hot context, incomplete closing-run observer endpoint and several simultaneous costs; not an isolated volume cost or portable FPS delta. Requested lights remained. |
| 9 October, lower-resolution motion | [Exact motion cohort](ENGINEERING_1_6_2_MOTION_PRESENTATION_2026_10_09.md):33% actual presentation rate46.171/s over a short approximately6.5 s span, with milestone readback overhead. | Earlier source/scene and short workload. This cannot certify today's outdoor33% result or sustained performance. |
| Current `a89362dc` | Android17,33%, Shipping/Mobile: opening-to-Keeper17.916 actual images/s; ordinary chapter control14.864/s. Exterior held shader dispatch approximately100 ms versus AS approximately0.55 ms; skin CPU approximately6-10 ms.31 TLAS/22 BLAS, briefly23 at transition. [Exact receipt](evidence/2026-10-31-mobile-efficiency/receipt.json). | Current chapter includes the outdoor world and actors absent from the early scene. Failed full480 forest and actor worker were OFF. Closing-control drift prevents causal speedup claims. This does not measure future dense-forest cost. |

The same model/GPU and raw driver number appear in the July and current retained
reports, but OS/API, output extent, assets, scene, instrumentation and timing
definitions differ. An unchanged raw driver number does not prove identical driver
binaries. The historical CSV's `median_derived_fps` column is deliberately not used.

Private raw July files are still available at their original retained report
location; this pass checked summary, timing, opening/lich state and capability
bytes read-only. Full private paths, device serials and raw logs are not published.
Their hashes are retained in the current private experiment directory. This is
stronger than relying solely on recollection, but is not a reconstructed binary
comparison or a reason to roll back the game to July.

## Current options and the next measurement

`FireEmitterBuffer::ResolveFireEmitterQualityBudget` currently selects:

| Fire detail | Volume steps | Reflection samples | Fire-light visibility work |
| --- | ---: | ---: | --- |
| Low | 2 | 1 | Same per-emitter RT visibility path |
| Mobile | 4 | 1 | Same per-emitter RT visibility path |
| High | 10 | 2 | Full-quality path retained |

The shared shader already avoids a shadow query when both lighting terms are
mathematically zero. It otherwise evaluates admitted fire emitters at receivers;
the normal finite-distance shadow ray remains authoritative. Low flame sampling
does not independently reduce the illumination range or this query work. The
ordinary exterior receipt contains one lantern emitter, so merely lowering a
four-emitter capacity would not address that sampled view.

The existing Low versus Mobile option was tested on the same current APK at
33%, with native quality-budget witnesses and temporary fixed display brightness.
Actual presented-image rates were **19.319 / 18.203 / 17.125 per second** for
Mobile / Low / closing Mobile. The closing control drifted about 11%, so the
middle Low run establishes neither a reliable benefit nor a regression. Held-lit
dispatch medians were 56.548 / 60.955 / 65.156 ms. No option or default changed.
See the [exact comparison receipt](evidence/2026-10-31-mobile-efficiency/fire-quality-receipt.json).

There is an additional method limitation: the first control had 45 seconds of
prewarm; later runs had 10 seconds. `MotionEvidenceScenario::Begin` validates
and resumes the live Keeper route, without resetting its simulation. The Android
adapter resets timing/events at arm, not world/actor state. A strict frame join
therefore does not establish equal cross-run starting scenes. Before another
comparison, use equal preparation and a fresh acknowledged checkpoint after
thermal preparation, then verify the recorded starting state and workload. GPU
clocks/power remain unobserved; neither thermals nor warmup alone is proven to
explain the drift. Do not average the controls into a claimed saving.

A **candidate to evaluate**, not an admitted policy, is an explicit shorter local
fire-light influence range with a smooth distance fade: retain visible flames,
nearby RT-occluded illumination and full-range/full-quality mode, while spending
fewer queries on distant weak contributions. This intentionally changes distant
lighting and needs current whole-frame and moving visual evidence. It must not
turn shadowed lights into unoccluded lights or remove scene objects. Exact range
and UI integration are not selected by this document.

Common skinning remains a separate secondary opportunity. Broad AS rewriting
has a weak case in the measured sub-millisecond stage. Existing rejected opacity,
player lookup, inverse-transform, mirror-removal and first-blocker trials are
not queued for unchanged repeats. Texture LOD and reconstruction remain unproven
mechanisms with their own appearance and ownership requirements.

## Later checkpoint: water-contact strategy selection

The [light-range and waterfall follow-up](evidence/2026-10-31-mobile-efficiency/WATERFALL_AND_LIGHT_RANGE.md)
corrected preparation and compiled the private range candidate, but thermal
admission stopped both candidate timing attempts. Source is restored and there
is no light-range gain claim. The same pass found a more specific historical
lead: commit `e18f17b4` (10 October) introduced shared water-contact droplets
whose transmissive material can select GenericDielectric for the whole frame.
That is later than the original August waterfall, so those two additions should
not be treated as one historical feature cost.

All three retained current baseline routes hit their maximum dispatch on the
first approach GenericDielectric frame beside the waterfall. Later wet footsteps
can continue generating droplets. Neither the exact active contributor count nor
water pixel coverage is recorded, and torch/camera state changes alongside the
strategy. Next isolate splash treatment with full water/lighting retained before
inferring a causal regression or choosing a lower-cost selectable presentation.

That proposed comparison has now completed: [splash refraction](evidence/2026-10-31-mobile-efficiency/SPLASH_REFRACTION.md).
Active contact counts confirm the strategy trigger. Simpler spray improves the
selected crossing, but closing-control drift and missing active-contact visual
coverage prevent admission. The owner now explicitly prioritizes a playable
phone default using measured reduced/OFF choices, while retaining full quality
and existing saved preferences; this remains unfinished.
