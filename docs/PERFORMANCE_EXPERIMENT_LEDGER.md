# Performance experiment ledger

**Reconciled 30 October 2026 against main `f0cea47b1194786e091a0c8eea169fae92779cc2`.** This is the current decision/navigation layer over retained reports, not a new benchmark or a replacement roadmap. PR27 is merged; public 1.6.2 remains frozen and 1.7 is unfinished. The [26 October owner policy](../PROJECT_DECISIONS.md#phone-performance-and-17-scope---2026-10-26) makes sustained phone optimisation ongoing, not a fixed-FPS release veto or permission to cut approved scope.

**Read this before selecting another experiment:** keep the measured exact-zero cleared-mist exit; reject unchanged selective-opacity and player-lookup repeats; close inverse-transform without adoption; defer Lower indirect and mirror-to-stone (retain the mirror); defer production FSR and complete texture LOD. The later actor-worker phone observation establishes no causal gain and normal worker remains OFF.

## Reading the evidence

- **KEEP** means retain the specifically admitted implementation or measurement support, not whole-game acceptance. **REJECT / CLOSED** means no unchanged rerun to chase a favourable number. **DEFER** needs the stated new evidence or prerequisites, not automatic resumption.
- Build/source and artifact identities below bind only their own cohorts. Short identifiers are explicitly prefixes; linked records retain full hashes. A historical `1.6.2-debug` package label can contain 1.7 development; use its source/APK hash.
- CPU render-entry-through-present cycles include waits; GPU intervals may include AS, trace/shading and transfer. Neither reciprocal GPU time nor nominal display refresh is playable FPS. Actual-present rates, fixed-step routes, generated-input motion and sustained ordinary play are separate evidence classes. Never add overlapping stage medians or interpolate between different APKs, OS versions, extents or profiles.
- **Repository evidence** means committed reports, receipts, selected CSVs or images explicitly named below. **Private/local evidence** means raw material reported as retained outside Git; this documentation pass did not access or verify those private bytes. A hash/manifest is not the image or APK itself. Missing raw PNGs cannot be reconstructed from hashes or summary statistics; do not invent replacement captures or silently upgrade availability.

## Bellwether 1.8 performance baseline

**26 November static collision storage.** The Windows native scene inventory
loaded the complete forest and town with 163,788 collision triangles before
and after the change. Retained CPU support arrays fell from 11,157,976 to
9,696,520 bytes. Static obstacle sweeps use the original triangle positions
and the same BVH. Their unused normals accounted for the entire difference.
The obstacle intake now charges additional faces and BVH nodes against the
bytes previously reserved for those normals. With the current ground and
step geometry this leaves room for 17,799 static obstacle faces. The
measurement covers retained CPU arrays. Startup peak memory, frame time
and sustained phone performance require separate measurements.

**28 November village collision inventory.** The full native forest and town
at `9a6ef6d8` retained 10,457,504 bytes for 176,049 collision triangles.
Of these, 42,225 supply ground, 20 supply steps, and 133,804 are static
obstacles. At that ground and step count, the byte budget admits 139,476
obstacle faces, leaving 5,672 for solid furnishings. Recalculate the allowance
when adding a walkable floor or steps. The figures come from
`horde_rt_scene_resource_inventory_tests` loading the combined scene.

**24 November ordinary Android arrival.** The
[seeded-lookout touch walkthrough](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#24-november-2026--android-touch-walkthrough-from-the-lookout-to-bellwether)
reached the hearth through the live campaign. The staffed room held
154 SurfaceFlinger intervals across 22.19 seconds at 33% on RayTracingPipeline.
The display interval median was 141.67 ms, with p95 at 166.67 ms and battery
temperature stable at 36.5 °C. This is a warmed ordinary-play observation.

**28 November staffed hearth dispatch.** In [two first-room phone runs](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#28-november-2026--staffed-hearth-shader-sampling-on-the-galaxy-s26-ultra), shadow visibility occupied 36.37–37.73% of sampled shader-clock ticks on the ordinary 33% Pipeline geometry layout. Most of those ticks came from pixels whose primary ray hit the shared static world. The frozen repeat-visit image matched the ordinary renderer pixel for pixel. The next experiment should distinguish local fire-emitter shadow queries from the other visibility queries on these receiving surfaces. Retain the existing shadow quality and use ordinary shader timings for the comparison.

**28 November shadow candidate primitive index — closed.** A trial moved the candidate primitive-index read inside the shared-world instance check at the staffed hearth. At the same 33% Pipeline view, trial medians were 76.30 and 76.35 ms for RT dispatch. Both took 0.20 ms longer than their adjacent controls on average. Restricting the analysis to frames bracketed by matching 1300 MHz readings also showed no repeatable gain. The frozen image matched pixel for pixel. Keep the ordinary shader. [Phone comparison and raw captures](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#28-november-2026--hearth-shadow-query-comparison)

**28 November hearth shadow attribution.** Two private observer runs on the Galaxy S26 Ultra attributed **72.85–74.53% of sampled shadow ticks** to direct fire-emitter visibility. The share was **73.45–75.36%** for pixels whose primary ray hit the shared world. The 33% Pipeline control and observer produced identical frozen RGB. This identifies a target for the next output-preserving experiment. Measure any gain with the ordinary shader and the full route. [Device record and exact samples](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#28-november-2026--direct-fire-shadow-attribution-at-the-staffed-hearth)

**28 November direct fire shadow early exit — promising private trial.** For binary fire shadows, the trial ends the hardware ray query as soon as the existing material test finds an opaque triangle. In four alternating runs at the staffed hearth, RT dispatch medians went from 75.89–76.01 ms to 61.59–61.68 ms among frames bracketed by the same 1300 MHz GPU clock and power level 0. The frozen RGB matched exactly. The trial made two returns in the longer route and the control made one. Both full-route runs reached the 240-second deadline before finishing the final hold. Keep the trial private pending production integration and sustained validation. [Device record and exact samples](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#28-november-2026--early-exit-for-direct-fire-shadow-rays)

The normal shader catalog build produced all eight variants for each backend, and fresh compilations matched the catalogs and compatibility includes. On the RTX 5050 Laptop GPU, six paired 960 × 540 images matched pixel for pixel. Each backend presented the hearth and square with the opaque shader and the hearth with the generic dielectric shader. The Windows evidence establishes frozen-image parity. Sustained route performance and the integrated shader's handset performance remain open. [Windows shader and image receipt](evidence/2026-11-28-bellwether-hearth-blocker-exit/windows-image-parity.json)


**24 November ordinary Android hearth controls.** At the same position and
graphics settings, the [stationary view comparison](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#24-november-2026--ordinary-bellwether-hearth-view-and-backend-timing)
recorded median display intervals of 141.67 ms facing the hearth, 108.33 ms
facing the doorway and 141.67 ms facing the hearth again. Native GPU window
means moved from 111.51–116.37 ms to 81.73–83.33 ms and back to 115.17 ms.
Command-record means stayed near 24–25 ms. Turning also moved the held
lantern. The trial bounds the cost for this position with both effects in
play. The separate fixed-step route uses a different camera and workload.

**INCONCLUSIVE (24 November): same-view Android RT backend comparison.**
The [fresh-process controls at the hearth](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#24-november-2026--ordinary-bellwether-hearth-view-and-backend-timing)
recorded 158.33 ms median displayed interval on the opening Pipeline run,
129.17 ms on RayQueryCompute and 116.67 ms on the closing Pipeline run.
Temperatures, Android thermal status and sampled GPU power limits differed.
The original backend preference stays in place. A backend performance
decision needs repeatable controls and equivalent ordinary-play coverage.

**23 November five-resident replay.** The private Android and Windows QA
builds share the native 60 Hz route and its twelve resident swaps. The
[Galaxy S26 Ultra run](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#23-november-2026--bellwether-five-resident-shippingmobile-round-trip)
used Shipping/Mobile Pipeline at 33% and took 159.40 seconds. Median GPU
time was 64.90 ms in the staffed tavern and 51.20 ms at the east fork. The
room medians increased from 51.06 to 68.97 ms across visits.

**23 November tavern stage attribution.** A [second handset run](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#23-november-2026--bellwether-tavern-gpu-stage-timings)
used the same route and displayed graphics settings with per-stage queries
enabled. All 2,460 frames passed the stage-to-submission join. Dispatch
medians across the three tavern visits were 50.32, 73.51 and 67.50 ms,
about 99% of each room's GPU interval. The actor roster, light count,
TLAS and BLAS counts and allocations matched across all room visits.
**DEFER (23 November): skip passage/staff-light math when inactive.**
The [original–trial–original handset comparison](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#23-november-2026--bellwether-inactive-local-light-trial)
held roster, resolution and settings fixed and joined 7,394 measured frames.
The trial had lower dispatch times, especially on the returns, but it started
cooler and the original runs diverged as the phone warmed. The GPU reported
different thermal power levels at the same point in the trial and closing
control. Eight Windows capture pairs were pixel-identical. The branch uses
the original shader bank. A repeat would need interleaved runs with comparable
GPU thermal state and clock observations before attributing a gain.

**CLOSED (24 November): first confirmed blocker in Bellwether binary shadows.**
The [Galaxy S26 Ultra comparison](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#24-november-2026--bellwether-binary-shadow-traversal-trial)
completed the two-return route in original–trial–original order with the
same 33% Shipping/Mobile RayQueryCompute workload and matched assets.
The trial had the slowest staffed-room dispatch on all three visits and
the slowest sampled display cadence. The 1.8 branch retains the original
shader bank despite a roughly 13% whole-GPU saving on the Windows route.
The [Windows captures and route timings](superpowers/plans/2026-11-07-bellwether-arrival-direction.md#binary-shadow-traversal-trial)
remain useful platform-specific evidence.

The five-resident Windows baseline uses optimized Debug code (`/O2 /Ob2`) to preserve the private
fixture gates, with Shipping/High shaders and 960 × 540 trace and output
on an RTX 5050 Laptop GPU. The executable SHA-256 is
`00eb88034fe01220921b9672c1d6dfae46bff97745fcbb70c5f92ec9d0a7bcec`.
Pipeline and RayQueryCompute each completed both returns in 156.94 seconds
with 9,416 simulation ticks and zero pending ticks at peak. They recorded
25,496 and 25,523 completed frames respectively.

| Windows RT backend | Staffed room GPU median / p95 (ms) | Staffed room native cycle median / p95 (ms) | East fork simulation per tick median / p95 (ms) |
| --- | ---: | ---: | ---: |
| RayTracingPipeline | 2.38 / 3.87 | 5.86 / 5.91 | 2.41 / 2.71 |
| RayQueryCompute | 2.28 / 3.67 | 5.86 / 5.92 | 2.40 / 2.78 |

The hidden Windows replays establish a native timing baseline with both
CPU changes present. Measuring each change's optimized-build gain still
requires matched control builds. Raw runs, including the first Windows
launch that stopped on missing private staging assets, are retained locally
under `reports/horde-bellwether-shipping-profile-20261123-100005`.

### West village timing route

The five-resident west route uses the same 60 Hz clock, four-tick catch-up
limit and successful-present join. It walks from the staffed room through
the south door, past the relic stall and gardens to the dry stock yard.
Each of two 24-second holds samples all three head instances and each
animal's gesture before the return. The east route remains its own cohort.
Use `-BellwetherWest` with the existing Windows measurement runner and its
private five-resident campaign directory, then verify the output.

```powershell
python tools/analyze-chapter-route-measurement.py --bellwether-west-roundtrip <run-directory>
```

Android uses a separate checkpoint and CSV. Keep the APK and shader digests,
graphics settings, thermal observations, and completed frame evidence with each
handset run. [Android launch and retrieval](../android/README.md#west-village-round-trip)

On 23 November the RTX 5050 Laptop GPU completed both returns on each backend
in 234.88 seconds with 14,092 simulation ticks and zero peak pending ticks.
The same optimized Debug executable used Shipping/High at 960 × 540. The
verifier joined every GPU sample to its successful present, saw one dealer
handoff each way on both laps, and observed the settled pose and all three
gestures with three submitted head instances on each visit.

| Windows RT backend | Joined frames | Room GPU median | Stock-yard GPU median / p95 |
| --- | ---: | ---: | ---: |
| RayTracingPipeline | 38,582 | 2.38 ms | 1.45 / 2.91 ms |
| RayQueryCompute | 38,496 | 2.28 ms | 1.31 / 2.78 ms |

The Windows executable SHA-256 is
`f60fd14f10b1375ad7a80e7561dc654fcf3a1d192f4bf831cf8b6499c8699757`.
The first attempt reached its second yard visit before the inherited
30,000-frame limit stopped collection at 182.72 seconds. The verified west
run uses the 240-second route clock as its bound. Raw reports and exact
hashes are under `reports/horde-bellwether-west-timing-20261123-130910`.

The 28 November repeat used the House B ledger revision `508cd600` on the
same RTX 5050 Laptop GPU and hidden optimized Debug Shipping/High route
at 960 × 540. Each backend completed 14,092 simulation ticks and two
returns in 234.876 seconds. The current route uses four fire lights in the
staffed hearth and two in the stock yard. The 23 November runs used three
and one respectively, with different shader modules. The figures below
establish a current baseline for the fuller village.

| Windows RT backend | Joined frames | Hearth GPU median / p95 | Stock-yard GPU median / p95 |
| --- | ---: | ---: | ---: |
| RayTracingPipeline | 38,242 | 2.58 / 3.90 ms | 1.72 / 3.46 ms |
| RayQueryCompute | 37,027 | 3.22 / 4.87 ms | 2.22 / 3.88 ms |

The verifier checked all three livestock heads on each yard visit and the
resident swap in both directions on each lap. The hearth held 45 TLAS
instances and up to 487,491,392 device-local bytes. Successful present
records bind the completion samples to their frames. Display pacing and
physical Android performance require their own captures.
[Windows receipt](evidence/2026-11-28-bellwether-west-windows/receipt.json)

The signed five-resident ARM64 Debug APK at `542cd184` has SHA-256
`b4a2bd990d4b942cee31edc96deb52341b1331d77e93141c051b3be6384c66b2`.
It includes the west checkpoint with Shipping/Mobile and GPU-stage recording,
the same 25 resident files and 13 voice files, and 602 passing Android tests
with six skipped. The phone was disconnected. Physical timings remain open.

## Closed or retained rendering experiments

**23 November Bellwether held-item clearance — KEEP for 1.8.** One carry reach search reuses the camera support check and sampled heights at the 64 authored tree roots. Each candidate reach still checks the same solids, trunk bounds, terrain slope and current tavern door pose. The cache ends with the search.

On the RTX 5050 Laptop GPU, the hidden Windows Debug RayTracingPipeline replay at 960 × 540 ran control → candidate → control with the same shaders, resident assets and 60 Hz route. The square's median / p95 simulation time per advanced tick was 54.48 / 56.27 ms, 10.80 / 11.42 ms, and 53.35 / 57.03 ms in run order. Each replay advanced 5,232 ticks and completed two returns. Elapsed time was 101.03, 87.25 and 99.32 seconds. Peak pending simulation ticks fell from 1,436 and 1,341 in the controls to 10 in the candidate. All three final room images were byte-identical.

The control and candidate executable hashes are `0c2b87bcda29a646cb380ee44446f1e95377fc0ce762b9649fb8ae9c26cbe5d4` and `a83bb18bc2e9a0f71a20747de31ced891f8f89fd0fbf1b0834870c92ac94aab2`. The five affected Windows checks passed, including a clear carry sweep between two obstructed trunk probes using the same query. Local measurements are under `reports/horde-bellwether-carry-query-20261123-093301`. Android and optimized-build gains remain to be measured.

**23 November Bellwether player vertex upload — KEEP for 1.8.** The shared skinning loop writes the player body and first-person pose directly into their initialized static-PBR streams. The pose cadence, socket checks and static vertex data are preserved. The upload regression check compared all vertex bytes for both meshes across twelve walking, reverse and crouching poses.

The hidden Windows Debug doorway replay ran control → candidate → control on the RTX 5050 Laptop GPU. All three runs used RayTracingPipeline, 960 × 540 dispatch and output, the same shaders and resident assets, and the same 60 Hz simulation. The first five seconds were excluded. Across the four route phases, all 4,355 measured frames skinned both streams. The candidate lowered the median combined player skin and pose-copy cost by 18.6–21.8% versus each control. In the staffed tavern, the combined median / p95 was 28.00 / 29.82 ms, 22.02 / 23.75 ms, and 27.11 / 28.61 ms in run order. The corresponding whole-cycle medians were 30.80, 24.84 and 29.90 ms. The room GPU medians stayed within 5.11–5.16 ms.

The room resource peaks matched, both returns completed on every run, and the final screenshots had identical SHA-256 `e54ed99a66ba76b8a2d5f7919f29915882ad4e33a3bdcad9a67893fc1b58b3db`. The control and candidate executables were `129ef87db16ff34d646c170a79d5b18665fb9559a1d96d0e73cd40ed1780984c` and `0c2b87bcda29a646cb380ee44446f1e95377fc0ce762b9649fb8ae9c26cbe5d4`. Local evidence is retained in `reports/horde-player-direct-skin-20261123-090941`. Optimized builds, handset timings and sustained play need their own measurements.

**1 November Mobile-water quality reduction — KEEP, scoped to the waterfall hitch:** [Controlled comparison and pause handoff](evidence/2026-10-31-mobile-efficiency/MOBILE_WATER.md). One APK/process, three conditioning routes then A/B/B/A, 2,844 actual-presentation joins. Spray Off improves the predefined water window by 10.0–35.7% against both controls; median intervals 91.667 ms become 62.500/54.167 ms. Whole-route rates 16.707/16.691/16.353/15.924 images/s show no broad FPS gain; closing drift −4.682% and one whole-route p95 almost +15% are borderline. Six active-contact stills inspected, not pixel parity. Mobile now omits optional droplets while water/ripples remain; High and saved tier values are preserved. Private probe removed. Final normal-policy APK completed 793 RT frames, including 28 active-contact frames on OpaqueFast. Native Android fresh/reset default aligned to the approved 33%. Four CTests, 24 Java tests, both platform builds and Android lint pass locally; no new hosted CI/LFS downloads. Phone restored/released; owner-requested pause. Future direct-light scaling is a recommendation only, not another started experiment.

**31 October splash-refraction comparison (cleanup1 November00:00UTC):** [Exact cohort and decision](evidence/2026-10-31-mobile-efficiency/SPLASH_REFRACTION.md). Same APK A/B/B/A,3,140 joined frames; authored droplets versus opaque spray. Whole-route actual rates19.100/18.295/17.958/17.527 images/s; predefined waterfall window14.195/18.409/17.341/13.061. B avoids the generic-transmission switch, but closing control drifts8.234% and the3% route-wide keep gate fails. **NOT ADMITTED:** runtime probe removed, only immutable water-contact telemetry retained. Separate visual route had zero active droplets, so appearance remains unvalidated. Owner reports spray is not noticeable; approved phone-default investigation may test contact droplets Off with full water/high-quality retained, but no playable preset or saved-choice change is established. Clock/power traces and appropriate active-contact capture remain necessary; no unchanged immediate cohort rerun.

**31 October 23:11 light-range/waterfall checkpoint:** [Bounded probe and attribution](evidence/2026-10-31-mobile-efficiency/WATERFALL_AND_LIGHT_RANGE.md). Mobile-only 6-8 m fire-light fade compiled in 16 variants, all eight High payloads unchanged; candidate timing never admitted (hot start, then bounded cooldown failure). **UNMEASURED / NOT ADMITTED**, source restored, no unchanged immediate retry. Equal warmup plus fresh native checkpoint acknowledgement corrected the preceding preparation mismatch. Three existing baseline traces / 2,396 joined frames put the largest waterfall-area dispatch spike on the first GenericDielectric frame (138.767-192.911 ms; actual inter-present 150.000-204.167 ms). Source identifies active transmissive splash droplets as a whole-frame strategy trigger; water coverage, contact state and torch changes remain confounded. **Next hypothesis: isolate splash material treatment**, preserving the waterfall, current lighting and full-quality option; no predicted percentage gain. No broad AS rewrite or new default follows. [Receipt](evidence/2026-10-31-mobile-efficiency/reach-waterfall-receipt.json).

**31 October history/Fire Low follow-up:** [Feature timeline and current comparison](ENGINEERING_1_7_MOBILE_COST_TIMELINE_2026_10_31.md). Same baseline APK, Mobile / Low / Mobile actual rates 19.319 / 18.203 / 17.125 images/s; closing control drift and unequal live-world prewarm prevent attribution. **INCONCLUSIVE; no new quality policy admitted.** Current Low changes flame sampling, not direct-light visibility queries. Next comparison requires equal preparation, a fresh acknowledged checkpoint and verified scene-state equivalence. July CPU averages are historical context, not current display FPS. Raw files remain private; [receipt](evidence/2026-10-31-mobile-efficiency/fire-quality-receipt.json) records hashes.

**31 October current-device checkpoint:** [S26 current baseline and scratch trial](evidence/2026-10-31-mobile-efficiency/README.md), source `a89362dc`, 33% / Shipping-Mobile / Pipeline, actual presentation timestamps. Opening-to-Keeper17.916 images/s; ordinary chapter A2/B1/A3 controls14.864 /14.865 /11.661. Closing-control drift prevents a causal gain claim. Scratch initialization runtime candidate **CLOSED, NOT ADMITTED**; source restored. GPU dispatch dominates held exterior views (~100 ms versus ~0.55 ms AS). Worker and failed480-tree candidate remain OFF; no future dense-forest or sustained-playability acceptance. Exact artifact/shader/device identities and limitations are in the linked receipt.

**Previously closed first-confirmed-blocker flags:** the [30 September decision](evidence/2026-09-30-first-blocker/DECISION.md) rejected admission after7,352 owning rows with changing thermal/power state and no demonstrated gain. Production flags were restored. Do not retry the unchanged flag variant; it is separate from selective primary opacity below.

| Experiment / hypothesis | Exact cohort and observed result | Decision and reopening condition | Evidence / raw availability |
| --- | --- | --- | --- |
| **Selective primary opacity:** let hardware accept ordinary opaque hits while retaining body/water filtering. | Control source `8c65afa6`; Shipping APK prefixes `3cb84efb` / `b9d69ff4`. SM-S948B, Pipeline, Shipping/Mobile, 75%, 1080×2235 → 1440×2980; eight interleaved route/held-high runs, 9,752 joined frames. Image equivalence passed. Route mean-of-run cycle medians was +4.19%; held-high −3.54% was thermally qualified. No repeatable net saving. | **REJECT / NO-GO.** No unchanged rebuild/tuning/repeat. Reconsider only a materially different mechanism or new contrary matched evidence with review. | [Full decision and hashes](ENGINEERING_1_6_1_PRIMARY_OPACITY_2026-10-01.md). Report in Git; binaries, images and raw logs recorded in a local private experiment directory. |
| **Primary player lookup guard:** avoid role lookup for unrelated candidates. | Guarded Shipping APK `6bca7be1…`, control `c299`; unchanged SM-S948B 75%/Mobile water+fire/Glass On/Pipeline. 1,838 owning frames: cycle median 69.6940 → 70.1879 ms; GPU 59.3083 → 59.1093 ms. One unmatched-temperature pair; no net benefit. | **REJECT; implementation restored.** Not the opacity experiment. No repetition scheduled; a different justified hypothesis and matched whole-frame evidence are needed. | [Lookup record](ENGINEERING_1_6_2_PLAYER_CANDIDATE_LOOKUP.md). Git summary; compiled proof, captures, APKs and timing retained outside repository. |
| **Lower indirect transport:** skip eligible diffuse-direction secondary rays while preserving selected reflections. | Source `2bdad132`, isolated Benchmark APK prefix `1d1e3e51`, before new Keeper lights. Shipping/Mobile 75% phone ABBA, 7,352 owning rows. Current/Lower pooled GPU means 62.107672 / 61.934068 ms (descriptive 0.28%); final Current control faster than both Lower runs; thermal headers 3 vs 2. Visible radiance is intentionally lost. | **DEFER adoption; trial closed; Current retained.** Needs useful matched benefit, accepted visible loss and current-scene rebasing/validation. Do not stack into another trial. | [Transport decision](ENGINEERING_1_6_2_OPTIONAL_TRANSPORT_DECISIONS.md), [inactive source backup](experiments/1.6.2-indirect/lower-2bd-2026-10-05/README.md). Source/summary in Git; exact raw phone inputs private. |
| **Keeper mirror → stone:** test whether replacing that mirror saves enough work. | Runtime `6468c3e4`; control APK `c17e219d…`, stone `982619a7…`. Pipeline 75%, 1080×2235 → 1440×2980, Shipping/Mobile, Glass On, Current transport/shadows, four courses/7,352 rows. Pooled GPU means mirror 69.978937 / stone 76.164385 ms under unequal thermals; final mirror control faster than either stone run. Stone still traces a real diffuse bounce. | **CLOSED: no useful repeatable saving; DEFER stone/removal/mobile option. Retain ordinary mobile mirror; desktop/High must retain it.** Not proof mirrors are free or stone causes slowdown. Reopen only a materially different justified experiment with matched evidence. This supersedes older “ongoing” labels. | [Final mirror decision](ENGINEERING_1_6_2_MIRROR_COST.md), [source identities](experiments/1.6.2-mirror/2026-10-05/source-record.json). Git summary/source digests; raw reports, screenshots and preferences private. |
| **Ordinary physical mist:** use actual source colour/strength and midpoint source visibility. | SM-S948B/Android16; Shipping/Mobile APKs `8da32be3…` / `d36cfbf6…`; Pipeline 75%, Glass On, four courses/7,352 rows. Pooled GPU means 57.117353 / 56.1244265 ms; ordered warming prevents causal-saving claim. Still and later moving approval are scoped to the admitted `2bd` scene, before new flank lights. | **KEEP admitted correction, not an optimisation claim.** New light/volume workloads need their own evidence; preserve midpoint approximation and negative probes. | [Adoption](ENGINEERING_1_6_2_MIST_ADOPTION.md), [source archive](experiments/1.6.2-mist/README.md), [chronology](ENGINEERING_1_6_2_SECOND_PASS.md). Git source/summary; raw sealed datasets private. |
| **Keeper flank lights:** measure the added lighting cost, not an optimisation. | Control `2bdad132` / APK `21d1b3ba…`; candidate `6468c3e4` / APK `e42bc8ca…`. Pipeline 75% Mobile/Glass On, 7,352 rows. Whole-course GPU mean 62.400876 → 72.086243 ms (+15.5%); finale 61.922873 → 129.911625 ms. Hot status-3 context; A2 end thermal observation absent. Geometry, lighting and mist work change together. | **KEEP the recorded cost/tradeoff visible.** Appearance approval and later publication are not isolated-cost or sustained-performance proof; the mirror result does not offset it. A new investigation must isolate a specified mechanism/current workload. | [Torch comparison](ENGINEERING_1_6_2_KEEPER_TORCH_COST.md), [later owner/release dispositions](ENGINEERING_1_6_2_SECOND_PASS.md). Git report; original reports and admission inputs private. |
| **Opaque inverse transform:** reuse hardware-query world-to-object instead of matrix inverse. | Production base `3252820f`; APKs `a49955a2…` / `fbfdf02a…`, archived patch identity in receipt. SM-S948B/Adreno840, 40% (576×1188 → 1440×2971), Shipping/Mobile shaders in O2 Debug, both RT backends. Eight valid runs/5,554 frames; small mixed timing changes, final controls cooler and unmatched. Six image pairs identical; two differ only 50 pixels by ≤1/255. | **CLOSED, NOT ADOPTED; trial restored.** No demonstrated meaningful gain; earlier private-shader gains do not transfer. Reopen only with a new justified mechanism/validity issue and matched evidence, not indefinite warming/retries. | [Production-bank experiment](evidence/2026-10-17-production-performance/README.md), [results](evidence/2026-10-17-production-performance/results.json). Git receipts/results; APKs/full images/ledgers local. |
| **Exact-zero cleared-mist exit:** skip work once density is exactly zero, without changing positive fade. | Parent `e218cfd3`; timing APKs `a49955a2…` / `d98cbc3a…`; final capture APK `d0363ad6…` is separate. SM-S948B/API37, both backends, production Shipping/Mobile, 40%, saved Mist On. Six valid runs/4,324 frames. Exact lower-tomb GPU gains: Pipeline 18.75–18.80%, Query 17.58–17.60%; 26 matched image pairs unchanged. Not completed ABBA or long thermal plateau. | **KEEP.** Applies to cleared-room backtracking, not live two-torch combat or outdoors. Every recorded interval still exceeded 33.333 ms. Revalidate if density/transport changes; no broad FPS claim. | [Decision and artifacts](evidence/2026-10-18-zero-mist-exit/README.md), [measurements](evidence/2026-10-18-zero-mist-exit/measurements.json). Git summaries and selected comparison JPG; full PNGs/APKs/ledgers local. |

## Resolution, quality and measurement cohorts — do not merge their numbers

| Question | Build/device/settings and result | Disposition / next admissible evidence | Evidence / raw availability |
| --- | --- | --- | --- |
| Historical lower-resolution route cost | Runtime `95eb0807`, APK `269ac11a…`, SM-S948B/Android16, Pipeline Shipping/Mobile/Glass On, output 1440×2980. 75/40/33% GPU medians 53.590051 / 18.486874 / 13.728281 ms; CPU 64.160860 / 28.634323 / 24.065652 ms. Single ordered warm/hot courses, 1,838 rows each; 75% was a later returned-device run. Old 50% `e339da58` APK is a different cohort. | **KEEP historical observations only.** No thermally matched causal saving, interpolation, default decision or sustained FPS. | [Mobile measurement ledger](ENGINEERING_1_6_2_MOBILE_QUALITY_MEASUREMENTS.md). Git summary/hashes; raw literals/screenshots/device logs private. |
| Actual-presentation collection and warm 50% route | Runtime `2a408613`, APK `faf35f48…`, SM-S948B/Android16. 75% collector proof: 12.881435 presented images/s over 142.608 s. Same APK later warm 50%/Glass Off: 22.364936/s over 82.1375 s, median/p95 interval 41.667/75 ms. Settings/initial thermals differ. | **KEEP collector evidence.** Not a scale-only A/B or sustained ordinary play. Reliable power and plateau still absent. | [Measurement ledger](ENGINEERING_1_6_2_MOBILE_QUALITY_MEASUREMENTS.md), [sanitized 50% analysis](evidence/2026-10-08-present-timing/scale50-warm-route01-analysis.json). Git analysis/receipt; full raw report private. |
| Matched-start 50/40/33 moving appearance | Runtime `a6439e0b`, APK `d305f4ab…`, SM-S948B/Android16, Pipeline/ShippingMobile, Mobile water+fire, Glass Off, Current shadows, Mist On/Dust Off, output 1440×2980. ~6.5 s generated-input runs: 24.288 / 31.205 / 46.171 actual-present images/s; readback overhead included. Native extents 720×1490 / 576×1192 / 475×983. | **KEEP measurement support and scoped owner acceptance of 33% appearance for further testing.** Not sustained FPS/contact/full-route acceptance. Public 1.6.2 kept 50%; later 1.7 default33 is a separate decision, not retroactive proof. | [Motion cohort](ENGINEERING_1_6_2_MOTION_PRESENTATION_2026_10_09.md), [cohort receipt](evidence/2026-10-09-motion-presentation/cohort-receipt.json). Git hashes/analysis; raw readbacks and separately recorded review clips local. |
| Mist Off / combined low quality on cleared route | 20 Oct r23 terrain; same APK within each cohort, not across cohorts. Final `9f9da695` APK `e699bfa8…`; SM-S948B, Pipeline Shipping/Mobile, 40%, Glass Off. Six valid routes/4,209 frames. Lower Mist On/Off GPU medians 53.848/54.133 ms; combined control/trial 53.992/54.419 ms. Combined: Mist Off/Fire Low/Dust Off/Current/Mobile water. | **No ≥3% useful gain; no new Fire/Dust defaults.** Keep owner's independent Mist Off decision. Individual Fire/Dust costs unmeasured; closing A6 refused cooling admission. Do not repeat cleared-room trial as a live Keeper answer. | [Quality checkpoint](evidence/2026-10-20-phone-performance/README.md), [exact cohort receipt](evidence/2026-10-20-phone-performance/quality-receipt.json). Git receipts; raw routes/logs local. |
| Live two-light Keeper Mist On/Off | Renewed R04 APK `b7e387d1…`; SM-S948B, Shipping/Mobile Pipeline, saved40% (576×1188 → 1440×2971). A1 prepares actual two-light/Mist On workload, then empty arm-file identity fails; zero timed frames. B1/B2/A2 never launch. | **INCONCLUSIVE / STOPPED BEFORE TIMING.** No saving to adopt. Further run requires corrected byte-verified host arm transport, offline proof and a newly authorised bounded device slot; no automatic retry. | [Failed comparison](evidence/2026-10-20-keeper-mist/renewed/README.md), [failed native report](evidence/2026-10-20-keeper-mist/renewed/a1-native-failed.json). Git failure/receipt; proposed unexecuted runner/raw custody local. |
| Diagnostic outdoor20/33/40% | Source base `bb507d24` plus recorded compiled hashes; APK `712f0709…`, SM-S948B/Android17, Shipping/Mobile Pipeline, landscape2981×1440 (pre-rotated swapchain1440×2981), MistOff/GlassOff/water1/fire0/shadow1/dust1. Ordered33/20/33/40/33 holds: GPU medians35.511/16.083/35.486/48.205/43.933 ms. Final33 control drifts+23.8%; clocks/power unmeasured. | **DEFER20% quality; diagnostic only, no default/adoption.** Image quality unacceptable/unadmitted. Preserve closing-control drift; needs acceptable reconstruction and matched moving/whole-frame evidence, not speed alone. | [Scale record](evidence/2026-10-22-outdoor-scale/README.md), [receipt](evidence/2026-10-22-outdoor-scale/receipt.json). Git summaries/host logs; raw reports and separate quality image/clip local. |
| Production GPU-stage attribution | Source `8c3246d0`, APK `91f2ff1d…`; SM-S948B/Android17/driver2150932499, 33% 475×980 → 1440×2971, Pipeline Shipping/Mobile, Mist Off/Mobile fire/Low dust/Current/Mobile water/Glass Off. 10 s warmup +30 s lookout hold; 626 post-warm frames. GPU medians AS0.547604 / dispatch33.814895 / transfer0.124505 ms; whole-cycle47.748230 ms. | **KEEP attribution, not a gain.** Dispatch dominates this view; no broad AS rewrite justified. Next hypothesis must isolate dispatch cost with protected quality; not whole-game/sustained acceptance. | [Stage report](evidence/2026-10-23-production-stage-phone/README.md), [raw resource CSV](evidence/2026-10-23-production-stage-phone/resources.csv), [sample selector](evidence/2026-10-23-production-stage-phone/sample-window.csv), [verifier](evidence/2026-10-23-production-stage-phone/verify-samples.py). These selected raw rows are in Git; complete manifests/APK/analyzer inputs remain local. |
| Actor CPU worker | Source `958f5182`, isolated APK `9466ba1a…`; SM-S948B/Android17, Pipeline Shipping/Mobile, 50% 720×1486 → 1440×2971, Water1/Fire0/GlassOff/Current/MistOff/Dust1. Serial/worker routes 590/589 rows; 109 active-Kit samples each. Skin critical-path medians9.909/8.185 ms; whole-cycle213.035/208.710 ms; GPU198.328/198.843 ms. | **DEFER normal admission; no causal gain.** Differing thermal/process setup, no closing control/clocks; ~2% cycle difference below3% goal. Normal worker OFF. Needs proven initial-state fixture, equivalent warm controls, closing control and a fresh bounded lease, not pre-fence/GPU-ownership changes. | [Phone result](evidence/2026-10-28-actor-worker-phone/README.md), [receipt](evidence/2026-10-28-actor-worker-phone/receipt.json). Git aggregate/identity evidence; completed CSVs/ledgers/raw and failed logs local. |

## Feasibility and deferred directions

| Hypothesis | What is actually established | Decision / reopen condition | Evidence availability |
| --- | --- | --- | --- |
| Spatial FSR versus Linear | RTX5050 Laptop, evaluator EXE `11df83af…`; six static cases/192 timestamp rows from a fixed 960×540 capture downsampled to 75/50%, not freshly traced inputs. Post-only means: 75% Linear0.010123 vs FSR0.060966 ms; 50%0.009581 vs0.056959 ms. Two private images/4,915,200 bytes; excludes RT/final presentation. | **DEFER production FSR; KEEP Linear.** Needs useful live/mobile moving-image and whole-frame quality/cost evidence. Do not repeat the same still test or claim a 33% phone solution. | [Second-pass static feasibility](ENGINEERING_1_6_2_SECOND_PASS.md). Git summary/exact EXE hash; evaluator/source/raw GPU evidence outside checkout. |
| Footprint-aware texture LOD | Inspected Shipping material operands use literal LOD0; world/Lich resources have one mip, props11. No complete runtime candidate or device benchmark. | **DEFER.** Needs reproducible colour-correct/normal-renormalized mip assets, valid primary/deformed/secondary footprint propagation, actual compiled operands, moving visual review and matched net benefit. No silent partial default. | [Exact sampling/resource review](ENGINEERING_1_6_2_OPTIONAL_TRANSPORT_DECISIONS.md). Git conclusions; referenced original audit receipt private. |
| DRS, temporal reconstruction and vendor upscalers | Dated CPU DRS/history foundations and feasibility research are not runtime reconstruction or phone savings. GPU motion/depth/HDR/history ownership and transparency costs were not admitted by those tests. | **DEFER runtime admission.** Recheck current implementation, establish actual inputs/ownership/reset/memory and matched moving quality/cost before integration. No default change follows. | [Temporal/DRS decisions](ENGINEERING_1_6_2_TEMPORAL_DECISIONS.md), [22 October reconstruction readiness](design/HORDE_1_7_UPSCALER_READINESS_2026_10_22.md), [future investigation](PERFORMANCE_INVESTIGATION_FUTURE.md). Git source/report scope; no admitted device result to reconstruct. |

## Custody and maintenance

**Known recovery boundary:** [20 September recovery evidence](evidence/2026-09-20-lantern/README.md) commits 26 selected reports, including five complete 600-frame phone reports, invalid interruption markers, thermal traces, capture manifests and a 15-pair comparison result. Its parity PNGs are **not included**. Those image bytes are not recoverable from the Git manifests/hashes alone: recover the original local `reports/parity-20260920` directory or make separately labelled new captures before repeating the pixel check. Neither action was performed here; new captures would not be the lost original evidence.

The [roadmap source inventory](ROADMAP_SOURCES.md) and [device record](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md) cover adjacent implementation and device checks. The [historical audit](MOBILE_PERFORMANCE_AUDIT_2026-10-04.md) and [second-pass chronology](ENGINEERING_1_6_2_SECOND_PASS.md) preserve proposals, failed attempts and superseded “pending” wording. Do not treat them as a fresh task queue.

For each new decision, append the exact source/artifact/device/driver/backend/profile, workload and timing definition, run order/population, result and uncertainty, keep/reject/defer rule, raw-artifact availability and reopening condition. Link the receipt rather than copying raw private reports into Git. Preserve invalid runs as invalid. No new tests, device operations, runtime changes or release were performed for this reconciliation.
