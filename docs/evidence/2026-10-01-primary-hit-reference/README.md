# Native primary-hit reference: a cost boundary, not an optimisation

Exact SM-S948B/R5GL219SZGK, Adreno840; uncooled Shipping/Mobile
RayTracingPipeline,75%,1080x2235 internal/1440x2980 presentation, strict ASTC,
MAILBOX. Detached source71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95 plus the
retained two-file source recipe. Normal control APK a6329657…d033 and reference
40d0f759…cf9e are `.benchmark`, development-signed, installed/pulled back exactly.
No stable-app/data change, owner signing or publication. Normal control restores
before C2 and remains installed afterward; runner returns Home. Normal DebugC11
remains installed separately. All accepted player presentation is unchanged.

The temporary reference preserves camera/ray/primary mask and native traceScene,
then consumes base/normal for a deliberately nonphysical surface colour. It omits
primary radiance, reflection/refraction, fire volume, mist and electricity.
Unused hit attributes can compile away: this is **not full primary/material cost,
a fundamental hardware floor, additive decomposition or Shipping candidate**.
No diagnostic atomics are added. No production shader changed.

## Completed matched order and exact measurements

C1→P1→P2→C2; each block runs ordinary route, held-high then continuous live
reveal. P1/P2 share process29797. Twelve strict analyses admit12,152 owning
completed/presented/GPU-valid rows with no rejection/cancellation. All640opening
rows actually use OpaqueFast. Native cycle is entry-through-present-return,
not compositor/display pacing or input latency. GPU timestamps includeAS/RT/copy.

Each cell below is native cycle median / GPU command median / GPU p95, in ms.
Opening uses its160 actual rows, **not the whole-route median**. Held/live600 each.

| Workload | C1 normal | P1 reference | P2 reference | C2 normal |
| --- | --- | --- | --- | --- |
| Ordinary opening | 78.151 /66.463 /73.540 | 18.416 /7.500 /8.383 | 17.938 /7.551 /8.329 | 83.245 /72.151 /80.906 |
| Held-high | 224.499 /222.574 /234.678 | 11.237 /9.570 /10.026 | 12.054 /10.506 /11.661 | 226.987 /225.267 /241.985 |
| Live reveal | 234.379 /222.857 /238.889 | 20.201 /9.216 /9.279 | 20.358 /9.239 /9.283 | 245.831 /234.848 /248.093 |

The [final aggregate](primary-hit-abba-final-reviewed-20261001.json) reports
descriptive median-of-run statistics and ordered pairs, never a causal gain.
All320reference opening GPU intervals and2400reference held/live intervals are
within33.333ms; **none** of their normal-control counterparts is. Removing
physical shading is not meeting the real renderer's target. Live reference
cycles remain20.20–20.36ms (~49median-derived FPS), not60FPS display acceptance.

Context is post-launch, outside/not aligned to measured rows. C1 opening begins
24.0C and ends31.4C/status0 (after an offline gap, not a sustained warm control).
P1/P2 run38.9–39.8C/status1–2; C2 runs39.1–43.1C/status2, with power levels0–7
across its sequence. Different power/process/order and compiler footprints
exclude causal/additive cost assignments. Full per-run contexts are retained.

## CPU projection correction and shader proof

The frozen parser's `openingVsHeavyRouteStages.*.cpuStages` and
`liveRevealZoneStages.cpuStages` are null because it selected legacy `report.zones`,
which has timing summaries but no CPU stages. They are **not zero or unavailable
CPU cost**. Original F8E2 parser/F5B7 aggregate and their outputs remain frozen.
The [supplement](supplemental-completed-zone-cpu-20261001.json) explicitly joins
raw report hashes and actual `completedFrameEvidence.zones`, with160/600 valid
samples for all13CPU stages. Opening player skin medians8.06–8.63ms; live8.54–8.72ms;
held-high frozen skin0. CPU record timings are not GPU AS timings.

Eight actual embedded SPIR-V modules extracted from the sealed APKs independently
val/dis PASS, zero **all** atomic opcodes (including no-result), zeroOpImageRead
and no diagnostic binding22. Four Shipping/Mobile pipeline/compute keys change;
other12variants remain unchanged. All53APK render-asset payloads match exactly.
Opaque pipeline125253→12967words and23→1static query-init sites are compiler
footprints, **not dynamic ray counts or time partitions**. Compute modules are
build/containment proof only; this phone timing is pipeline-only.

Frozen strict parser negative fixtures pass8/8synthetic cases; fixtures are not
physical-device evidence. Initial module-audit comparison-expression failures
and successful retry2 remain labelled separately. The offline verifier reruns
all12raw ledgers and8module val/dis checks and reproduces the canonical aggregate.
Supplemental replay and manifest/index checks are separate retained receipts.
Large APKs/native ELF/full source/full disassemblies remain external. Eight exact
SPIR-V files use LFS. The final evidence manifest binds the retained bytes.

## Decision and remaining gates

Native primary traversal plus consumed base/normal has measured headroom. The
remaining physical radiance/schedule still needs approximately90% reduction for
live30FPS under9–11ms serial CPU assumptions; reference omission is not a saving
proposal. Even half/quarter-frequency whole radiance is quantitatively insufficient.
Next narrow opaque-secondary work and measured schedule/history overhead before
choosing one quality-preserving Mobile architecture. No resolution/quality change
or nonphysical variant is promoted. See [feasibility](../../ENGINEERING_1_6_1_MOBILE_RT_FEASIBILITY_2026-09-30.md).

Physical interface-budget/recovery loss, live glass, changed-pixel attribution,
Shipping/Diagnostic parity, pipeline/compute parity and sustained display pacing
remain open. No RTX, phone compute, S24/S25 or owner visual acceptance follows
from this reference. Audio/haptic manual revalidation required:NO (unrelated
measurement variant, feedback semantic inputs/playback unchanged).
