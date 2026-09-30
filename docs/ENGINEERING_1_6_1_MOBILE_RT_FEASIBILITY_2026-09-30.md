# Mobile RT feasibility: 30 FPS is a target, not a current capability claim

September 30; control traversal restored by `f77dc4d`. Phone unavailable by owner
instruction after Debug restoration. No new phone measurements or architecture
implementation in this assessment. Preserve the accepted player/physical-glass
contract and all current failure diagnostics. Windows RTX remains a separate target.
Subsequent owner reconnection permits the bounded phone profiles below; the
unavailable-phone statement describes this assessment's original checkpoint.

## What the measurements actually say

The unchanged workload does not deliver sustained 30 FPS at 75%. There is no
evidence that small traversal-flag changes can bridge this gap: the completed
[first-blocker trial](evidence/2026-09-30-first-blocker/README.md) is rejected for
admission, with its negative result and control restoration preserved.

Do not confuse native render-cycle time with GPU time. The original Shipping
candidate route is approximately 60 ms **cycle**, with approximately 50 ms whole-RT
GPU median. Its opening is approximately 66–67 ms **GPU**. Held-high is approximately
225 ms cycle / 223 ms GPU, and live lantern approximately 237 ms cycle / 225 ms GPU.
The uncooled initial held-high control-context capture is 236.8249 ms GPU at battery
42.3–43.5 C/status2–3. Latest uncooled first-blocker controls have opening GPU medians
80.154399 and 89.4686175 ms. These are distinct runs/thermal contexts, not a pooled
sustained baseline. See [exact per-run GPU/CPU statistics](evidence/2026-09-30-shipping-phone/ab-summary.json).

New [whole-strategy ABBA](evidence/2026-09-30-generic-strategy/abba/README.md)
completes7,352 Shipping rows: ordinary OpaqueFast opening GPU70.696118/80.3609875ms
versus forced Generic164.3430425/193.3582765ms. This is an unchanged opaque scene,
not a lantern routed through the wrong shader. Different algorithms, compiler
treatment, failed image equivalence and thermal drift exclude causal/glass-only
savings. Keep production selection unchanged. Compiler treatment is now isolated
as a group on identical OpaqueFast source; the following is not recovered-cost proof.
At these two normal opening medians, the planning22.333-24.333ms live GPU budget
requires approximately66-72% GPU reduction; no new performance capability claimed.

October1 [compiler-only ABBA](evidence/2026-10-01-opaque-compiler/README.md)
completes7,352 owning Shipping rows/640opening without changing maths or runtime
selection. Normal openingGPU66.419686/72.1915605ms versus probe46.043827/51.478931ms;
route cycles60.9232/66.8472 versus44.0113/50.1984ms. The −29.64% opening contrast
is descriptive, not causal: thermal/power contexts differ and separate Diagnostic
image gate fails maximum14 against unchanged limit3. Production compiler/compute
parity guards remain RED. No promotion or lantern timing assigned to this probe.
Even these unaccepted opening medians require approximately47–57% additional GPU
reduction to the planning live22.333–24.333ms budget. Focused pixel attribution is
justified; the unchanged workload is not thereby a30FPS architecture.

October1 [all-direct-visibility ABBA](evidence/2026-10-01-all-direct-visibility/README.md)
now admits12,152 owning Shipping rows, including640 opening rows all actually
OpaqueFast. It deliberately removes **all** opaque blocker shadows and physical
glass shadow attenuation, never a Shipping candidate. Normal opening GPU medians
71.40/71.18ms versus isolate31.30/30.86ms; held-high223.18/238.45 versus56.22/56.32;
live231.91/238.32 versus56.34/56.41. The live isolate cycles are65.72/65.89ms, not
30FPS. Opening isolate cycles42.25/41.83ms, not the whole-route33.51ms statistic.
Warm contexts span36.4-43.5C/status1-3 with unequal power levels/processes. Removing
code also changes compiler footprint. Differences are descriptive, not additive
ray-cost estimates or causal production savings.

This rejects direct-visibility reduction **alone** as a credible30FPS plan for the
measured workload: even omitting it leaves about56ms heavy GPU before live CPU.
That residual is not a fundamental hardware lower bound or measured fixed-cost
partition; a coherent new schedule can change compiler/bandwidth behaviour too.
At the live control's235.11ms median-of-run-medians, the planning22.333-24.333ms GPU
budget requires approximately89.65-90.50% reduction. Even the nonphysical56.38ms
residual needs another56.84-60.38% reduction before history overhead **and before
restoring real shadows**. Ordinary71.29ms needs approximately65.87-68.67% reduction.

Thus prioritise a combined **native-RT** Mobile schedule: full-resolution primary
geometry and current physical glass interfaces; coherent hit/material/lighting
work separation; measured sample eligibility for physical direct lighting and
secondary radiance reuse. No raster G-buffer, cached scalar glass visibility, SSR,
silhouette downscale, or unchecked old-frame blockers. Measure a bounded primary/
hit-material lower-cost reference and the residual secondary/fire work next, then
select one implementation slice with explicit S,r,H. Preserve dynamic arms/held
props, disocclusions, shadows and mirrors as quality gates; source sample ceilings
are not runtime counts. The existing bounded-interface correctness gate remains
open independently of this performance design.

Target: at least sustained 30 FPS for ordinary and lantern-heavy gameplay at 75%,
with consistent display pacing. Recovery toward 60 is a headroom-dependent
stretch target, not a forecast. Current measured medians do not certify either
target; existing native timings exclude compositor/display/input latency.
Final acceptance needs the unchanged route, held-high and continuous live-motion
workloads in a normal uncooled sustained session, plus presentation-interval
distribution and pause/resume. A cool median or diagnostic frozen frame is insufficient.

At 30 FPS the entire steady-state cycle has 33.333 ms. The optimistic GPU-only
requirements below leave **zero** CPU/presentation margin:

| Observed context | GPU median ms | Minimum GPU reduction to 33.333 ms |
| --- | ---: | ---: |
| Initial uncooled whole route | 50.0545 | 33.41% |
| Candidate opening B1/B2, cooled context | 66.443123–67.067785 | 49.83–50.30% |
| Latest uncooled opening controls | 80.154399–89.4686175 | 58.41–62.74% |
| Candidate held-high B1/B2, cooled context | 223.1577–223.6841 | 85.06–85.10% |
| Candidate live lantern B1/B2, cooled context | 224.7309–225.109 | 85.17–85.19% |
| Initial uncooled held-high | 236.8249 | 85.92% |

Current CPU preparation is serial: both platform RenderFrame paths wait the single
frame-slot fence, then RecordTraceAndCopy calls UpdateDynamicInstances, skinning,
host uploads, BLAS/TLAS record and queue submission. AS GPU work is serial in that
command buffer; CPU recording durations are not GPU AS durations. The existing
opening stage probe measures approximately 0.496 ms AS versus 65.633 ms dispatch,
in a colder investigation-only context. It does not isolate shadow/lighting,
material evaluation, player traversal or fire composition within dispatch.

The initial uncooled route's skin mean is 8.3947 ms and other measured upload/
refit/TLAS/trace-record means total 0.6900 ms. These sum disjoint mean CPU stages,
not paired median GPU/cycle values. For a **planning assumption** of 9–11 ms serial
CPU/other cost in live gameplay, available GPU budget is only 22.333–24.333 ms.
At opening GPU66.4 ms this implies approximately 63–66% reduction; at live225 ms,
approximately 89–90%. Frozen held-high has skin median zero, so applying the live
9–11 ms assumption to it would be wrong. A 60 FPS cycle allows only16.667 ms total.

## Source-backed cost boundaries

- [Frame primary](../shaders/raytracing/include/rt_frame.glsl) issues one native
  query per in-bounds pixel: 1080x2235 = 2,413,800 at this phone extent. Primary
  geometry, silhouettes, modelled arms and body ownership must remain full75%.
- [HasActiveGenericTransmission](../src/vulkan/raytracing/PresentableTinyRtScene.cpp)
  tests active instance/primitive/material metadata, not screen coverage. Any
  qualifying transmission selects GenericDielectric for the full dispatch.
  This does **not** mean every pixel follows a dielectric primary path.
- [Lighting](../shaders/raytracing/include/rt_lighting.glsl) runs bounded physical
  transparent shadow transport even for opaque receivers under that strategy.
  Their segments can cross glass; merely splitting by primary opaque/glass hit
  would be incorrect. A light inside the lantern can affect many receivers.
- Existing exact zero-contribution gates already skip fire/local/sky work.
  The local/sky batch capacity4 is not four lights per pixel: two area samples
  may be used for each, plus a separate active-fire emitter loop. Profile actual
  enabled samples; do not estimate savings by counting static SPIR-V sites.
- Fire volume is bounded analytic sphere/volume sampling, not extra RT rays.
  Prior nonphysical fire-shadow/volume isolates rank hypotheses only, not
  measured savings suitable for production.
- Restored isolated Diagnostic counter40 is primaryRewardBodyPixelCount:
  52,335 primary instance8 hits, 2.17% of native pixels. It includes opaque
  materials on that instance and excludes indirect glass/shadow work. It is
  **not** a glass-ray count, eligible-reuse mask or explanation of the full cost.

## Candidate architecture and quantified savings boundaries

Recommend evaluating a dedicated Mobile schedule, not copying a desktop ray
budget unchanged. Keep native RT primary visibility, world-space scene ownership,
physical glass, current light/material intent and honest swapchain presentation.
No raster G-buffer/proof path, SSR, captured reflections, baked/scalar shadows,
or fallback. Compute may schedule work but actual visibility remains Vulkan RT.

For each option let `S` be its measured eligible GPU cost, `F` the unaffected cost,
`r` its remaining work ratio and `H` new scheduling/history/bandwidth cost:
`G = F + S`, `Gnew = F + r*S + H`, saving = `(1-r)*S - H`.
Costs overlap: never add alternative rows' savings or transplant published RTX
speedups to Adreno. No credible net-ms forecast is possible until `S` and `H`
are measured; the following are explicit conditional predictions/upper bounds.

| Option to investigate | Saving before overhead | Important bound/acceptance |
| --- | --- | --- |
| Additional exact light/contribution or conservative reachability culling | `q*S`, for proven unnecessary fraction q | At q25%, saves5 ms only if eligible S20 ms. Existing zero gates may leave q near0. No nonzero intensity cutoff, arbitrary range truncation or proxy shadow. |
| Importance-weighted direct-light sample selection with temporal/spatial **candidate** reuse; retrace selected visibility this frame | `(1-K/N)*S`, N actual contributing samples, K selected | N2→K1 saves50% of eligible shadow work; N4→K1 saves75%; N1 saves0. Preserve proper weights/RGB physical transmission; current visibility avoids stale blockers. Variance, bias, fire flicker and fast-motion quality are gates. |
| Adaptive quarter-rate secondary sampling with validated history | `0.75*S` | If S20 ms, saves15−H ms. Even quartering **all**223–237 ms GPU would leave55.8–59.2 ms, so this alone cannot reach30. Primary glass/interfaces, TIR, sharp reflections, moving grips/shadows and disocclusions need current native evidence; no image-space reflection substitute. |
| Native-RT hit/material work split and coherent specialised queues | Measured module/dispatch tax saved minus queue/bandwidth cost | No assumed percentage. Opaque receivers still need physical transparent shadows where reachable. Primary-opaque alone is not a safe OpaqueFast classifier. A 32–64-byte full-resolution hit record alone is77.2–154.5 MB before histories, double buffering or read/write traffic. |
| CPU-only skin preparation overlap into private scratch | At most the affected measured CPU preparation, approximately8–9 ms in the cited route | No GPU reduction and not a remedy for225 ms GPU. Uploads/shared TLAS/BLAS writes remain after the owning fence. Snapshot/input timing and resource ownership require design review; do not simply increase frames in flight. |

The new isolate cannot supply S directly. For illustration only, assume its56.38ms
were an unaffected F and the control-minus-isolate178.74ms were eligible S, ignoring
thermal/compiler changes and H. Keeping half that work would still leave145.75ms;
quarter would leave101.06ms; one-sixteenth would leave67.55ms. These are **not
predictions**; they demonstrate why an impressive shadow contrast is insufficient.
Even quartering the entire235.11ms would leave58.78ms. Current non-dual opaque
lighting has a static ceiling2+N samples (N<=2 eligible fire emitters), not dozens
of equally important lights. One selected sample cannot be assumed to eliminate
96% of that work, nor can many-light desktop speedups be transferred to this scene.
Measured overhead and eligibility, not this illustrative partition, must determine
the proposed net-ms saving before a production architecture change.

Reservoir sample reuse is a researched native-RT lighting estimator, not permission
to cache a fake visibility scalar. Relevant primary references: [original ReSTIR](https://research.nvidia.com/publication/2020-07_spatiotemporal-reservoir-resampling-real-time-ray-tracing-dynamic-direct)
and [production scheduling research](https://research.nvidia.com/publication/2021-07_rearchitecting-spatiotemporal-resampling-production).
Their many-light scenes/desktop speedups do not predict this small-light Adreno workload.
Use only as a solver/scheduling reference; no SDK/framework import is selected.

## Next bounded profile and decision gate

1. On host/RTX now: reuse existing isolated-lantern/checkpoint and shader validation
   infrastructure to prepare current Mobile/High native witnesses and cost probes.
   Prove the remaining fifth-interface/recovery paths rather than reusing historical
   paths after numerical fixes. Keep RTX results explicitly RTX-only.
2. On phone reconnection: one exact-control75% ordinary opening/held-high/live
   sequence, existing ownership/timestamps/thermal protocol, not a full audit restart.
   Use scoped investigation-only work-category isolation or supported native GPU
   profiling to separate primary/material, direct physical visibility, dielectric
   continuation/reflection, fire volume and memory/occupancy costs. First report
   aggregate timer overhead and exact unchanged baseline. Omitted-work isolates
   are cost bounds, never Shipping candidates. Avoid diagnostic atomics in Shipping.
3. Measure actual enabled light/sample and secondary eligibility shares using a
   bounded diagnostic probe, not an expanded permanent telemetry framework. Make
   any forced-strategy comparison in an actually non-transmissive equivalent scene;
   do not route the real lantern through OpaqueFast to manufacture speed.
4. Select **one** measured architecture candidate and state S,r,H, projected net-ms
   range and excluded costs before implementation. At approximately225 ms liveGPU,
   a90% reduction is required under the live CPU assumption; even r0.1 requires
   nearly all current GPU cost to be eligible before overhead. If measured fixed
   work already exceeds the budget, report that constraint and revise the proposal,
   not visual quality or the minimum target silently.
5. Keep exact Fresnel/refraction/IOR/Beer-Lambert/TIR/physical-shadow fixtures,
   budgets, diagnostics and pixel tolerances. Any expected estimator/transport
   image change needs attributable reference evidence, not restoration of buggy
   pixels or tolerance relaxation. Validate walking/looking, attacks, lantern
   high/low, mirrors, disocclusion, fire and lifecycle continuously. Matched Shipping
   gain, display pacing and owner quality acceptance remain separate gates.

Current recommendation: retain30 FPS minimum as the design target; don't promise
unchanged visual quality plus30/60 before these measured boundaries. Small
micro-optimisations and a single quarter-rate-secondary switch are quantitatively
insufficient for the heavy scene. No Mobile architecture is promoted yet.
Phone compute and exact S24/S25 remain unverified. Audio/haptic manual revalidation
required: NO for this source/evidence assessment. Music, reporting, resize and final
candidate matrix remain in scope and may progress independently while phone is away.
