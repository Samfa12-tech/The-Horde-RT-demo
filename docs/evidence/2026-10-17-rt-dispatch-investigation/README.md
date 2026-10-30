# RT dispatch composition investigation — 17 October 2026

**A sparse shader-clock observer works without changing the six captured images.** Both hardware RT backends complete the unchanged ordinary unload-and-return cycle with exact frame/stage/clock/display joins. The observations point to distributed traversal/filtering, hit/material decoding, shadow visibility and surface/transport work, rather than a single cosmetic-effect bottleneck. This is diagnostic attribution with explicit sampling/observer limits, not an optimisation or production adoption.

## What was reused and what is new

The [16 October baseline stage profile](../2026-10-16-baseline-rt-profile/README.md) already establishes that dispatch dominates AS updates and transfer. Its exact stage-only APK and admitted route/analyzers are reused. The earlier resident/streamed matrix is not repeated: all four new routes stream normally, and the second pair is specifically a control for the new clock observer. The [primary-hit omission](../2026-10-01-primary-hit-reference/README.md), [all-visibility omission](../2026-10-01-all-direct-visibility/README.md), [opaque-secondary omission](../2026-10-01-opaque-secondary/README.md), [fire isolation](../2026-09-30-opening-lighting-isolation/README.md), [first-blocker](../2026-09-30-first-blocker/README.md), [primary-opacity](../2026-10-01-primary-opacity/README.md), [compiler trial](../2026-10-01-opaque-compiler/README.md) and [failed staged-primary phone admission](../2026-10-01-staged-primary-phone/README.md) were checked first. Their nonphysical removals, image failures, compiler/thermal confounds and no-go dispositions remain history; none was rerun or adopted. No previous valid per-function clock sample was found.

Unlike the unavailable Perfetto GPU hardware-counter source, the exact phone advertises shaderDeviceClock and shaderSubgroupClock. This observer checks and enables only device clock in its private logical device. [Scoped capability receipt](clock-capability.json). [Khronos defines the device-clock operation](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_shader_clock.html); its [GLSL definition](https://raw.githubusercontent.com/KhronosGroup/GLSL/main/extensions/ext/GL_EXT_shader_realtime_clock.txt) specifies undefined time units, 64-bit wrapping and code-motion barriers. We retain integer ticks without a nanosecond/cycle conversion. Hardware bandwidth/cache/stall/register/spill/occupancy counters remain unmeasured. Pipeline-executable-properties is advertised but was not enabled or sampled; advertisement is not measured availability.

## Exact builds, ownership and quality

Observer APK `29d2de014fbb12510e6003279c07bd19fed660a6d14b6abea2255f00a60e05e9` (183,998,970B), source a9283715d68f320627efdc32533df82aaf1c27b6 plus archived exact native inputs and generated observer modules. Control/restored APK `f13dda30ad56536a774004ee963c621aec2396f0a25179c490f32e72294fa527` is the original stage-only build. Neither is described as a pristine later-commit APK. Same development signer/versionCode10; install/pullback matches.169 asset payloads, original preference bytes and the release app are unchanged. The original stage-only APK was restored and Debug stopped after the tests. The phone disconnected after all evidence was retrieved; a final read-only device-info refresh was unavailable, with no interrupted run or new live-state claim. No signing, publication, merge or release.

SM-S948B/API37/Android17, Adreno840/Vulkan1.4.295, driver2150932499. Optimized native Debug/O2; actual private instrumented Shipping/Mobile observation modules. Saved40%/Current,576×1188 internal,1440×2971 swapchain, cap30, water1/fire0, glassOff, mistOn, dust1. The observed views use OpaqueFast; Generic modules validate/create but their full rendered transport/performance is not accepted by these OpaqueFast observations. High is not profiled. All shader samples, legitimate rays, masks, densities, effects, geometry, lighting, materials and the ordinary loading rules remain intact. No AS/descriptor owner redesign. Production source modules,18 texture layers, fixed capacities and frozen budget SHA256 `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf` are unchanged.

`hordeDispatchClockProbe=true` requires the private frame-stage/region Shipping/Mobile chapter-timing profile and a matching private shader marker. Default Android/Windows builds compile it out. The observer appends16B control plus8192×80B records to the existing coherent mapped metadata allocation. Its prefix and descriptor ownership are unchanged; the shader writes a disjoint BDA tail. The actual owning allocation increases655,424B, classified host-visible and device-local on this UMA path; those classifications must not be added. The requested tail is655,376B. There is no extra descriptor/page/actor slot or independently retired owner. GPU write→host-read synchronization precedes exact owning-fence collection. A private recording nonce checks sample freshness but does not replace the authoritative committed submission/scope identity. Cancellation, preparation, retirement and return rules remain unchanged.

Pixels at fixed32×32 cell centres (offset16) yield666 sampled invocations/frame. Nested categories charge exclusive intervals and evaluate each original return expression exactly once before restoring the caller category. Traversal includes query setup, candidate filtering and associated control work; material includes committed-hit decoding, triangle/transform and texture work. Visibility includes opaque/transparent shadow handling. Surface/transport excludes nested instrumented traversal/visibility/fire work. Short-function figures include clock/branch overhead, scheduling and divergence. Compiler/register pressure may change globally even for nonsampled pixels. **The fractions below are sampled elapsed-clock composition, not percentages of GPU dispatch, shader cycles, utilisation, baseline cost or predicted speedup.** The fixed grid may miss thin/rare features; no full-screen extrapolation is made.

## Image admission and sampled composition

Six full1440×3120 captures (three frozen checkpoints per backend) are byte-identical PNGs as well as decoded RGB-identical: zero changed pixels across26,956,800 compared pixels. Exact camera, animation time, scale, output exposure, orientation and water setting match. Captures occur outside timing. The actual bend and chamber captures were opened locally; this is preservation evidence for these views, not new complete-chapter visual acceptance. [Exact parity data](parity.json).

| Frozen view | Pipeline | QueryCompute |
| --- | --- | --- |
| Landing south | Exact | Exact |
| Keeper death retired | Exact | Exact |
| Forest second bend | Exact | Exact |

[Landing /Pipeline](landing-pipeline.png), [Keeper chamber /QueryCompute](keeper-query.png), [bend /Pipeline](bend-pipeline.png). The archive retains every original baseline/observer pair.

Settled-view samples use the same owning frames as the GPU-stage analysis:360simulation-tick holds, retaining60–300ticks and excluding phase/boundary frames. Lower/restored camera poses are exact between control/observer; approach targets differ by at most1cm with identical angles. Raw witnesses retain one exact ticket/pixel/category record set per held view/backend, independently summed against the owning CSV row.

| Backend /view | Trace/filter | Material/decode | Shadow visibility | Surface/transport | Fire | Mist/electricity | Dust | Other |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| pipeline /landing | 29.3% | 19.3% | 21.4% | 19.3% | 0.3% | 0.1% | 4.9% | 5.4% |
| pipeline /lower | 25.8% | 12.2% | 23.7% | 8.7% | 0.5% | 5.2% | 9.8% | 14.1% |
| pipeline /passage | 22.3% | 13.2% | 28.7% | 13.8% | 0.7% | 0.2% | 9.2% | 12.0% |
| pipeline /return | 29.1% | 19.2% | 21.5% | 19.5% | 0.3% | 0.1% | 4.9% | 5.5% |
| query /landing | 32.6% | 17.3% | 22.5% | 20.8% | 0.3% | 0.1% | 3.0% | 3.3% |
| query /lower | 33.9% | 14.9% | 29.3% | 11.4% | 0.7% | 3.3% | 2.6% | 3.8% |
| query /passage | 27.3% | 14.8% | 33.5% | 16.8% | 1.1% | 0.2% | 2.8% | 3.4% |
| query /return | 32.4% | 17.2% | 22.4% | 21.1% | 0.3% | 0.1% | 3.1% | 3.3% |

Exterior raw witnesses contain418–419 sampled primary hits in the aggregate static PBR lane, plus misses and held/actor geometry; interior witnesses mostly hit the procedural world lane. This supports inspecting reusable hit decoding rather than attributing the cost to one tree, light or prop. It does not identify bandwidth, cache behaviour or hardware traversal alone. The differing short-category proportions between backends reinforce the need to retain the instrumentation caveats.

## Observer/control timing and working loading path

Order is Pipeline observer, Query observer, Query control, Pipeline control. Same settings/scenes and ordinary input path; no forced pose/phase/readiness or fixed-delta override. Existing120s route,30s arm,2s final drain, adverse-counter guards and original torch180s coverage are unchanged. Zero mid-run image readbacks; private clock reads and eight small raw-witness writes are additional host observation IO, reported rather than hidden.

| Backend /view | Control whole GPU ms | Observer whole GPU ms | Observed change |
| --- | ---: | ---: | ---: |
| pipeline /landing-sweep | 142.864 | 146.657 | +2.65% |
| pipeline /lower-sweep | 68.374 | 68.543 | +0.25% |
| pipeline /passage-sweep | 66.100 | 68.076 | +2.99% |
| pipeline /repeat-ascent | 142.731 | 147.829 | +3.57% |
| query /landing-sweep | 137.281 | 142.231 | +3.61% |
| query /lower-sweep | 66.484 | 67.558 | +1.62% |
| query /passage-sweep | 65.128 | 66.979 | +2.84% |
| query /repeat-ascent | 146.709 | 142.841 | -2.64% |

No held-view median regresses by15%. Query starts match AP/BAT/SKIN within0.4/0.9/0.2°C at framework status0, ending1 in both runs. Pipeline control is warmer by3.4/3.6/2.2°C and status1 versus observer0; **Pipeline overhead is not causally bounded by this pair**. Query return control is slower than its initial landing despite the restored pose; warm drift remains. No clock/power control, causal optimisation gain or exact probe-overhead partition is claimed. These data permit conservative diagnostic work-composition conclusions, not production performance acceptance. No additional repeated thermal trials were launched.

| Run | Completed/stage/display joins | Clock joins | Actual span | Display median /p95 /max ms |
| --- | ---: | ---: | ---: | ---: |
| pipeline /observer | 665 | 665 | 69.68s | 87.50 /166.67 /170.83 |
| pipeline /control | 666 | not instrumented | 69.15s | 83.33 /162.50 /175.00 |
| query /observer | 673 | 673 | 68.90s | 83.33 /162.50 /170.77 |
| query /control | 679 | not instrumented | 68.27s | 83.33 /158.33 /183.33 |

All2,683 completed frames have actual display/stage joins; all1,338 observer frames have valid clock records. Every route unloads and restores once with zero readiness waits and14,249,664B (13.59MiB) saved. Observer full→retired→restored Pipeline161,486,656→147,236,992→161,486,656B, Query161,486,144→147,236,480→161,486,144B. Observer peaks174,962,816/174,962,304B: the same13,476,160B (12.85MiB) overlap above full. Control allocations restore their original exact values. The constant observer tail explains the allocation difference, not a changed loading benefit. The same84,219-triangle detail V/I/private-primitive/BLAS owner retires; coarse world/terrain/landmarks, shared textures/materials, collision, actors/equipment and uncertain secondary-ray contributors remain. CPU sources remain cached. This is scene-owned allocation inventory, not process RAM, whole-heap high-water or package I/O.

Actual display medians83–88ms, maxima171–183ms and260–267 catch-up overruns remain poor. GPU or clock figures are not playable FPS, scanout or latency. Complete-chapter/sustained smoothness, broader page coverage/provider-cache lifetime and lifecycle/pressure acceptance remain open.

## Static limits, checks and next step

The observer is **not a production-budget candidate**. Compared with unchanged frozen ceilings, Pipeline Mobile Opaque exceeds bytes by37,300, words9,325, instructions1,735 and loops2. Generic exceeds bytes8,692, words2,173, instructions509, loops2, functions3 and calls32. Actual opaque retains one fully inlined function/no calls/27 query sites; generic retains2 query sites. Both Pipeline/Compute observer pairs validate, have clock instructions, and no diagnostic atomics/binding22. The exact observation modules and comparison failures are preserved; no budgets/checks are raised, removed or presented as passed for this instrumented binary. [Static receipt](frozen-budget-observer-comparison.json).

Native Release2/2 and Debug2/2 timer/coordinator tests pass with new sample-bound, ticket, pixel, carry, missing-data/zero and overflow cases. Python clock3/3, stage5/5 and motion/display15/15 pass. CI now selects the actual timer/coordinator build targets/CTests as well as new clock-analysis tests. The profile guard rejects an unscoped clock build; a native pairing guard also rejects clock shaders without the matching buffer layout. Private Android four-ABI assemble/lint, default four-ABI Debug assemble/lint and286Java tests in45suites (unchanged Java task up-to-date; zero failure/error/skip), default unsigned Release assemble/lint and default Windows Shipping Release build pass. Default artifacts are not installed; unsigned Release is not signed/published. [Validation identities](validation.json). A read-only independent review found no fatal ownership/return-semantics issue; it required the sampling/overhead caveats above.

The first source-generation attempt looked for integrateFireEmitters in the wrong include and failed before compilation; the partial private directory is preserved, and the corrected generator uses its actual rt_fire.glsl definition. A local analysis command required unavailable NumPy; exact comparison was instead performed with existing Pillow without installing dependencies or changing pixels. Both failures remain recorded; neither was a failed device run or justification to weaken a gate.

**Recommended next bounded candidate:** investigate the reusable PBR hit-decoding transform work. It currently computes `transpose(inverse(objectToWorld))` for every decoded hit. The ray-query interface already offers [world-to-object transform access](https://raw.githubusercontent.com/KhronosGroup/GLSL/main/extensions/ext/GLSL_EXT_ray_query.txt). Test using that available inverse transform instead of recomputing it, preserving nonuniform/mirrored transforms, tangent frames, normal maps and all ray decisions. The exact control SPIR-V retains6 MatrixInverse instructions,12 object-to-world getters and zero world-to-object getters ([static receipt](hit-decode-hypothesis.json)); these are static sites, not dynamic cost. This is a source-backed hypothesis, not an implemented or measured speedup: extra query-data access and compiler treatment might offset the ALU saving. Require default shader-budget/structural checks, focused transform/normal regressions, matched images on both backends and warm uninstrumented dispatch/display evidence before adopting anything. Do not ship this clock observer or repeat effect-removal/failed staged-primary trials.

Current/Higher and saved Lower→Current physical acceptance remain pending. Mist28/Easy Tree/canopy and later world features stay deferred. This task does not claim a complete chapter or smooth streaming acceptance.

[Complete measured summary](dispatch-summary.json), [exact candidate](candidate.json), [raw/source packet](dispatch-clock-source-and-runs.zip), [archive receipt](receipt.json).
