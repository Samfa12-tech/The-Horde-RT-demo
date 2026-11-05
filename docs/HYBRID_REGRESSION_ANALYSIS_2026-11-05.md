# Horde partial-hybrid regression: attribution and bounded follow-up

5 November 2026. This follows the first actual-assets experiment. Its sealed
28-artifact manifest, binaries and source ZIP are unchanged and hash-verified.
No worktree, release, merge, publication or artifact removal was performed.
Sam subsequently explicitly authorised committing this experiment. The earlier
denial and uncommitted status remain historical facts in the sealed first report.

## What the original traces actually establish

Original RTX 5050 Laptop GPU cohort: Shipping/Mobile, Pipeline, 960×540,
scale 1.0, fixed simulation, 180 owning completed/presented frames per run,
first 60 excluded, RT/Hybrid/Hybrid/RT per checkpoint.

| Checkpoint | Full RT GPU ms | Partial hybrid GPU ms | GPU delta | CPU trace/copy-record delta |
|---|---:|---:|---:|---:|
| Forest | 1.2984 | 1.4867 | +0.1882 ms / +14.50% | +0.0327 ms |
| Niche | 1.7497 | 1.9600 | +0.2103 ms / +12.02% | +0.0332 ms |
| Portal | 1.6353 | 1.8207 | +0.1854 ms / +11.34% | +0.0338 ms |

Values are means of per-run medians. Whole GPU intervals cover AS, raster,
shading and transfer; CPU scopes measure command preparation/recording. The
roughly 33 microseconds of extra host recording is not an explanation for
0.19–0.21 ms of GPU execution. Do not add CPU/GPU medians or interpret their
reciprocals as gameplay FPS. The approximately 5.95 ms CPU cycles include
presentation waiting. Static CPU and GPU stage names are not interchangeable.

Each candidate frame uploads 90,064 versus 87,200 scene bytes: +2,864 bytes,
matching 11 draw records × 80 bytes plus 31 TLAS rows × 64 bytes. There are two
additional upload calls. Upload time is not separately isolated, and the closing
niche control has CPU upload variation. The existing CPU TlasUpdateRecord scope
mostly describes the original TLAS work; the extra TLAS build command is recorded
inside the hybrid trace/copy scope. Near-equal original TLAS CPU medians do not
show that the extra GPU AS build is free.

## Source-confirmed added work, with causality limits

| Source fact | What remains unmeasured |
|---|---|
| Every candidate record also builds a remaining-primary TLAS, in BUILD mode, after normal scene AS work. All 31 rows are supplied; owner 21 is masked out. | Its GPU duration, driver BVH treatment and contribution to the net regression. |
| 11 raster draws submit 751,868 triangles / 2,255,604 nonindexed vertex references each frame, including geometry later clipped or occluded. | Actual vertex/fragment invocations, early depth behaviour, cache reuse and overdraw counters. Submitted references are not hardware pipeline-statistics measurements. |
| RGBA32UI visibility is 16 bytes/pixel plus 4-byte D32 depth: nominal 10,368,000 bytes of attachments at 960×540 before alignment. Resolve performs a full-resolution logical visibility read (8,294,400 bytes). Clear/store and raster writes add work. | Physical external-memory traffic, compression, tile residency, cache behaviour and bandwidth stalls. These payload calculations are not measured DRAM bytes. |
| The adapter still calls the remaining-scene primary ray query for every pixel reaching primary shading. Raster coverage only supplies a closer distance and removes owner-21 traversal from that query. All secondary transport remains. | How much owner-21 primary traversal was saved, and how much frame cost belongs to secondary lighting/optics. This is not the primary-ray elimination of a complete forward renderer. |
| The original raster shaders recompute camera origin/walking offsets and orientation in the vertex shader, and camera origin again in the fragment shader. | Whether the driver hoists uniform calculations or whether this ALU is a significant GPU cost. |
| Opaque raygen grows from 557,556 to 573,728 bytes and 30,940 to 31,733 SPIR-V instructions (+2.56%). Static ray-query-initialisation sites remain 27. | GPU ISA/register use, occupancy, executed instruction/ray counts or instruction-cache stalls. Static site counts are not dynamic ray budgets. |
| Generic raygen grows from 241,268 to 282,284 bytes and 13,888 to 16,212 SPIR-V instructions. | The measured scenes use OpaqueFast. Generic footprint cannot explain their observed slowdown; no Generic performance conclusion is made. |

The similar absolute GPU penalties at the same extent/draw population suggest
fixed raster/AS/resolve overhead could matter. That is a hypothesis, not a
decomposition. No pass GPU intervals or memory/occupancy counters were captured.
The stage probe cannot simply be enabled: the hybrid helper excludes it, and its
current callback return occurs before the normal dispatch probe's End. A genuine
hybrid pass profiler needs its own complete recording/submit/completion joins.

The Khronos [subpass sample](https://docs.vulkan.org/samples/latest/samples/performance/subpasses/README.html)
illustrates why attachment lifetime and tile residency can matter on tile GPUs.
Our visibility resolve occurs outside the raster render pass in RT/compute.
An inference is that attachment traffic deserves phone profiling; the sample's
Mali measurements do not predict this phone or Horde's bandwidth.

## One bounded change tested

The archived trial generator offered an explicit --precompute-camera trial. The host computes
camera origin, forward/right/up vectors and aspect once per recorded frame.
Only raster push constants and vertex/fragment adapters change. Geometry,
triangles, primary/secondary query budgets, production shading/materials, lights,
quality and dimensions stay the same. The default generator is unchanged:
regeneration produces all six original shader modules byte-for-byte. All four
RT/compute shading adapter modules are byte-identical in the trial too.

Generated-header metadata selects the matching host push-constant interpretation.
There is no new Vulkan feature requirement. The original visibility algorithm and
nonindexed draw path are retained. This tests avoidable camera-uniform work,
not a complete renderer redesign or lower-quality approximation.

Static SPIR-V changes: vertex 388 → 274 instructions and 7,308 → 4,800 bytes;
fragment 158 → 75 instructions and 3,296 → 1,340 bytes. Sin/cos/normalise sites
are removed from these two raster modules. These are compiler-output facts,
not a GPU speedup claim.

A single new balanced cohort ran Full RT → original hybrid → camera hybrid →
camera hybrid → original hybrid → Full RT in each of the same three scenes.
Each process has 180 owning completed/presented frames; first 60 excluded.
The table below is separate from the earlier cohort and must not be pooled with
it. Values are means of per-run medians, whole GPU milliseconds.

| Checkpoint | Full RT | Original hybrid | Camera hybrid | Camera vs original |
|---|---:|---:|---:|---:|
| chapter-stone-portal-front | 1.8014 | 1.8266 | 1.8231 | -0.19% |
| chapter-niche-arch | 1.7266 | 2.0441 | 2.2485 | +10.00% |
| chapter-forest-lookout | 1.3179 | 1.4136 | 1.4464 | +2.32% |

**No repeatable GPU improvement was demonstrated; camera trial NOT ADOPTED.**
Portal's -0.19% is smaller than the per-run variation. Forest is +2.32%, niche
+10.00%; those are descriptive cohort results, not isolated causal estimates.
For example niche original run medians are 2.1272/1.9609 ms, while camera is
2.0681/2.4288 ms; the larger p95/bursts and unequal per-run behaviour prevent
attributing the difference solely to the changed camera code. There are no GPU
clock/thermal/occupancy counters to resolve that uncertainty. Smaller SPIR-V
does not establish faster execution, so this result does not justify adoption or
an unchanged retry to chase a favourable sample.

The camera trial differs from the original hybrid by mean RGB channel errors
0.00020–0.00444/255, with 0.00077–0.01775% of pixels over 8/255 in any channel.
Maximum isolated error is 73/255. The images are not pixel-identical; moving/phone
review was not performed. Production shading module hashes are identical.
The intent was equal-quality camera math; pixel metrics alone do not certify
temporal/perceptual equivalence.

Windows Release and Android arm64 **native-only** compilation passed for the
trial. The latter used :app:externalNativeBuildDebug with the explicit trial
generated directory; no new APK was assembled, installed or signed. Additional
desktop 540×960 Pipeline and 960×540 QueryCompute candidate captures passed
with the Khronos device validation layer loaded and no VUID/validation errors
or validation warnings. Loader registry/environment warnings are retained.
Both have 180 joined completed/presented frames with positive real raster work.

The two trial source files, generated shader manifest, EXE, raw captures and
delta patch are preserved separately. Canonical HybridStaticPrimaryPass.cpp and
the generator were restored byte-for-byte to the sealed first experiment.
Regeneration without the trial option was separately verified to produce all six
original modules exactly. The original test APK/EXE, 28 sealed artifacts and
18-file original source snapshot remain unchanged. The single existing Windows
build directory was reconfigured back to its original shader directory.

Exact old EXE SHA-256:
73318ed0b98a19fea8cbd34760e92a4e9b5179f62ff62fd5cfa0315a6509c4b1

Exact camera trial EXE SHA-256:
58b446b977df79a6523d46c47770017b222fe4e09327bb901239917ff3e10ef9

Raw evidence, analysis scripts, shader/source manifests and archived source:
C:/Users/sam_s/Documents/Codex/2026-11-05/task/native-hybrid/projection-trial

The two JSON summaries and archived source patch are also retained beside this
report under docs/evidence/2026-11-05-hybrid-regression in the repository.
The raw PNG/JSONL/log archives and trial executable remain local; their hashes
are recorded in the local follow-up manifest, not represented as committed raw
device evidence.

Reproduction requires the archived camera-trial.patch (or its two-file source
ZIP), reviewed/applied to the original experimental source, followed by the
archived generator with --precompute-camera and the existing explicit
Shipping/Mobile CMake/Gradle shader-directory argument. The ordinary generator
in the final checkout intentionally does not include this failed option.


## Concrete next experiment for mobile

Use one finite phone cohort comparing Full RT with the explicitly selected
partial-hybrid adapter, with completed native acknowledgements and matching
scene/extent/settings. First add optional, fence-owned GPU observations for:
original scene AS, extra remaining TLAS, raster visibility, RT/compute shading
resolve and copy. Keep the existing whole command-buffer interval and actual
presentation joins. Record timestamp support/valid bits/period, unavailable
results and shader/APK identities. Optional telemetry must never fail rendering.
Do not silently attach a prior frame's timing to the current mode.

Use staged completion boundaries and report intervals honestly. Do not assume
different GPU stages execute serially, add overlapping intervals, or let extra
barriers/query instrumentation masquerade as uninstrumented performance.
Compare with an uninstrumented whole-frame control to bound observation overhead.
The [Vulkan query specification](https://docs.vulkan.org/spec/latest/chapters/queries.html)
defines asynchronous query results/availability; the existing GpuFrameTimer and
RtFrameEvidenceCoordinator provide suitable owner/join infrastructure.

Choose actual matching locations: portal/forest plus an active actor or water/
glass view so the retained primary RT and Generic strategy are represented.
Prewarm both modes equally, then Full RT/Hybrid/Hybrid/Full RT with the same
camera/checkpoint or deterministic input route. Record actual native internal/
output dimensions, saved quality settings, device/OS/driver, clock/thermal/power
observations where available, per-run GPU and presentation median/p95 and several
minutes of ordinary behaviour. Compare equal effect and lighting budgets; do not
combine numbers across APKs, extents or thermal cohorts. Use matched images and
moving review to check raster coverage, thin geometry, water/actors and camera
motion before any keep decision.

This should answer whether the expensive portion is added raster work,
attachment/resolve traffic, remaining primary traversal, secondary shading or AS.
Only then choose an indexed/barycentric route, smaller visibility encoding,
retained/update TLAS strategy, or a direct-forward opaque slice. A direct-forward
slice must rasterise all geometry that can occlude its chosen primary pixels;
dropping the remaining query in the current partial slice would lose world/
actor/held-item occluders and be an invalid equal-quality comparison.

Do not cache the TLAS only because these captures freeze the simulation: that
would largely optimise the benchmark and say little about moving actors/props.
Do not drop lights, reflections, water, mist or resolution to manufacture a win.
The ledger's closed opacity/inverse-transform/lookup experiments are not reopened.

The present blocker is a connected, authorised RT phone and the pass-specific
fence-owned profiler/export route. There is no measured mobile benefit yet and
no basis for replacing Full RT. The current real-assets APK remains the sealed
first experiment; the camera trial is separately identified rather than silently
substituted into it.
