# Baseline RT stage profile — 16 October 2026

**RT dispatch dominates the current phone workload.** One existing ordinary unload-and-return cycle per backend joins all1,345 completed frames to AS-stage, dispatch-stage and transfer-stage observations. Exterior dispatch medians are137–142ms; interior65–68ms. AS-stage medians are0.37–0.55ms and transfer0.11–0.12ms. Quality, shader modules, assets and the working private loading path are preserved. This attributes the dominant execution interval; it does not identify an individual light, material, traversal or filtering bottleneck, or prove smooth gameplay.

## Reuse before new testing

| Existing experiment | Reused finding / reason not to repeat |
| --- | --- |
| [30 September Shipping phone stage probe](../2026-09-30-shipping-phone/README.md), exact v3 patch and owning-frame joins | Reused its stage-parameter timestamp method and fence/submission ownership. Old opening:1,838 joined frames, AS median0.4956ms, dispatch65.6335ms. Different source, Android16,75% scale and opening scene; these are historical evidence, not current woodland timings. Its v2 missing-final-frame failure motivated an explicit regression test. |
| [1 October all-direct-visibility omission](../2026-10-01-all-direct-visibility/README.md) | Nonphysical removal of legitimate shadow/glass transport; heavy GPU cost remained. Compiler changes confounded subtraction. Not rerun or adopted. |
| [1 October staged-primary profile](../2026-10-01-staged-primary-profile/README.md) and [engineering disposition](../../ENGINEERING_1_6_1_STAGED_MOBILE_RT_2026-10-01.md) | Desktop observer evidence exists; phone visual admission failed. Do not revive this renderer or mix its profiled medians with this baseline. |
| August RT Lab /12 October Current–Higher /incremental upload observations | Different scenes, short samples or CPU/ownership evidence; they do not isolate the current ordinary-route GPU workload. Retained as history, not repeated as a new matrix. |
| [Current resident/streamed four-run control](../2026-10-16-phone-resident-streamed/README.md), APK a42a0651…14808 | Already shows expensive resident rendering and a working13.59MiB cycle. Reused both streamed held-view controls; only two new profiled routes were needed. Original failed full-route and setup-ID packets remain preserved there. |

## Exact candidate and unchanged controls

APK `f13dda30ad56536a774004ee963c621aec2396f0a25179c490f32e72294fa527` (183,654,122B), based on `a3091c10e8746bdd9297828230c9fb28e1860ed7` plus the archived source hashes/patch. This receipt does not describe the APK as a pristine later-commit build. The final commit additionally contains analysis, documentation and CI coverage. Same development signer/versionCode10; install/pullback bytes match. Original preferences remain byte-identical and the release app is untouched. Debug is stopped after both routes.

Raw deviceSM-S948B, Android17/API37, Adreno840, Vulkan1.4.295, driver2150932499. Native Debug/O2, actual Shipping/Mobile private independent-region shaders.40% scale,576×1188 internal,1440×2971 swapchain, Current shadows, water1/fire0, cap30, glassOff, mistOn, dust1. All169 asset payloads are byte-identical to the prior control, including actual shader payloads. Pipeline identities in [profile-summary.json](profile-summary.json) match exactly. Frozen budget SHA256 `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf`; no limits, samples, masks, densities, geometry, lighting, materials or streaming policy changed.18 texture layers and existing metadata/TLAS/primitive capacities remain unchanged.

The observer is opt-in: `-PhordeFrameStageProbe=true` requires the already-private region-page Shipping/Mobile Debug chapter-timing profile. Default Android/Windows builds compile out its pools, markers and CSV fields. GPU timer default endpoints remain TOP/BOTTOM. The existing three timer owners retain query availability/error handling, per-slot fences and committed submission serials. RAII cancellation clears unsubmitted recordings without cancelling submitted work. Static cutover recording forwards the same probe.

AS timestamps bracket `UpdateDynamicInstances` at AS_BUILD, including work/dependency gaps in that interval; they are not isolated refit/build counters. Dispatch brackets the unmodified RT pipeline or ray-query compute execution, including traversal and all shader work. Transfer begins after the existing shader→transfer barrier and surrounds the transfer/copy path; it is not isolated copy-only cost. Vulkan timestamps may execute at logically later stages; these intervals are not additive counters. Independent region-preparation AS work and CPU loading are not silently folded into AS-update attribution.

## Matched fixed-view results

Medians in ms; AS/dispatch/transfer are GPU execution intervals. Whole GPU and actual display interval remain distinct. Each phase holds360simulation ticks; analysis keeps60–300ticks, excludes transitions and verifies frozen camera position/angles. The near-target poses differ from the old control by at most5.2mm; lower/restored views are exact. No mid-run image readback, forced pose/readiness, second simulation or fixed-delta override. Existing120s route,30s arm and2s terminal drain limits remain unchanged.

| Backend /view | AS-stage | Dispatch-stage | Transfer-stage | Whole GPU | Actual display |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pipeline /landing | 0.553 | 141.533 | 0.122 | 142.219 | 158.33 |
| Pipeline /lower chamber | 0.377 | 68.096 | 0.121 | 68.592 | 75.00 |
| Pipeline /certified passage | 0.376 | 65.935 | 0.115 | 66.424 | 75.00 |
| Pipeline /restored landing | 0.538 | 141.803 | 0.121 | 142.469 | 158.33 |
| Query /landing | 0.530 | 136.772 | 0.117 | 137.423 | 154.17 |
| Query /lower chamber | 0.369 | 65.741 | 0.114 | 66.226 | 75.00 |
| Query /certified passage | 0.371 | 64.795 | 0.108 | 65.295 | 75.00 |
| Query /restored landing | 0.530 | 136.926 | 0.114 | 137.577 | 154.17 |

Exact statistics, p95/maxima, poses and module hashes are in the JSON/raw packet. Against the prior unprofiled streamed control, whole-GPU held medians change−0.29% to+0.35% Pipeline,−1.46% to+0.003% Query. These are separate observations, not a causal overhead estimate or optimisation gain: current starts are6–7°C cooler and Query framework status0 versus the earlier1. Current Pipeline AP39.2→40.0°C, Query41.4→42.2°C; both framework status0 throughout. No clock/power control, backend ranking or sustained thermal acceptance is claimed.

The read-only Perfetto v54 inventory advertises `android.gpu.memory` but no GPU hardware-counter/render-stage source. Only the scoped [GPU source inventory](gpu-data-sources.json) is published; unrelated process inventory is not included. No counter trace or developer/root configuration change was attempted. Bandwidth/cache/register pressure/occupancy and shader sub-costs remain unmeasured. This is an access/evidence limit, not a universal hardware limitation.

## Loading and playable pacing remain honestly separate

| Backend | Completed/stage/display joins | Actual span | Display median /p95 /max | Full →retired →restored device-local B | Observed peak B |
| --- | ---: | ---: | ---: | --- | ---: |
| Pipeline | 667 /667 /667 | 69.25s | 83.33 /162.50 /175.00ms | 160,831,232 →146,581,568 →160,831,232 | 174,307,392 |
| Query | 678 /678 /678 | 68.06s | 83.33 /158.33 /166.67ms | 160,830,720 →146,581,056 →160,830,720 | 174,306,880 |

Each route unloads/restores the same84,219-triangle forest detail V/I/private-primitive/BLAS owner once, saves13.59MiB, restores exactly and has zero readiness waits. Observed peak is12.85MiB above full. Coarse world, terrain, landmarks, shared textures/materials, collision, actors/equipment and uncertain secondary-ray contributors remain. CPU source remains cached: no package-I/O/cache-eviction claim. Memory is scene-owned allocation inventory, not whole-process RAM or an exhaustive heap high-water.

Unload/reload±1s display maxima are83.33/91.67ms Pipeline and83.33/95.83ms Query. The AS-update interval maximum over all frames is0.982/1.510ms; dispatch maximum201.784/203.760ms; transfer maximum0.323/0.323ms. These whole-route maxima are different owning frames and must not be summed. Normal display intervals still plainly fail smooth-play expectations. Every completed frame has its actual display timestamp, all stage identities match, and there are no pending or adverse timing rows. Both cutovers and final frames are included. There is no new chapter-wide/lifecycle/secondary-ray visual acceptance claim. Prestart reunion and menu-covered end captures were opened locally, outside timing.

## Validation and disposition

- Native GPU timer/coordinator Release2/2 and Debug2/2 pass. Tests retain original defaults, exercise AS/RT/compute/transfer endpoints, unavailable/error behavior, submitted ownership and unsubmitted cancellation.
- New stage analyzer5/5 pass, including missing final row, wrong/duplicate owner, wrong tick, every missing/error/overflow stage and moving/insufficient held views. Existing strict motion/display analyzer15/15 pass unchanged. CI now runs the new analyzer tests.
- Private profiling Android: four ABIs assemble and lint pass. Default Debug: four ABIs assemble/lint and286Java tests in45suites have zero failures/errors/skips (unchanged Java test task up-to-date). Default unsigned Release: four ABIs assemble and lint pass using only the existing short generated native staging directory. No release signing/install/publication. [Exact default artifacts](validation.json).
- Default Windows Shipping Release diagnostic-window renderer builds. Initial command incorrectly named a nonexistent `HordeLanternRT` CMake target; that log is retained and the actual `horde_rt_diagnostic_window` target passes. New Gradle negative guard rejects profiling without chapter timing; the expected rejection log is retained.
- Read-only independent review found normal-path ownership sound and required the interval-label caveats above. Failed/cancelled runs with unavailable/mismatched stages are rejected, never converted to zero or pooled.

**Recommended next step:** preserve the working loading system and quality. Treat the RT/compute shader as the next bounded investigation target. Reuse these exact held views, pipeline hashes and stage records; require a concrete quality-preserving hypothesis and new measurement capability before another phone replay. The failed staged-primary and shadow-omission experiments already answer their own questions and should not be repeated. This pass does not justify per-object fixes, asset replacement, AS/descriptor redesign or production shader adoption.

Complete-chapter routes, broader streaming coverage/provider-cache lifetime, lifecycle/pressure and sustained smoothness remain open. Current/Higher phone and saved Lower→Current physical checks remain pending. Mist28, Easy Tree/canopy and later world work remain deferred. No merge or release.

[Raw/source/check archive](baseline-profile-source-and-runs.zip), [archive receipt](receipt.json), [candidate identity](candidate.json), [profile data and prior control joins](profile-summary.json).
