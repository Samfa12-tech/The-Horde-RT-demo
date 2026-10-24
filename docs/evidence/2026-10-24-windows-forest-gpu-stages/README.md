# Windows shared-forest GPU attribution

**Shared64 stays OFF by default.** The fixed eight-run batch did not reproduce
the earlier 45% landing median gap after equalizing staging and warmup. It shows
no persistent extra per-frame AS or dispatch median cost in this prefix. Large
tails, variable unaligned clock samples and different earlier setup prevent a
causal speedup claim or a definitive explanation of the old signal. No renderer
optimization or gameplay admission follows from this result.

Base `0fbeb8b87e82104c644520e711bf26b376f515b8`, existing draft PR27 branch and
worktree `C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.7-wp2-vertical-proof`.
[Summary and source/binary identities](summary.json), [all 1,024 rows](frames.csv)
and the two native images below retain the result. Earlier evidence remains
[unchanged](../2026-10-24-normal-shared-forest/README.md).

## Narrow implementation

- Root CMake `HORDE_RT_FRAME_STAGE_PROBE` defaults OFF. Explicit developer build
  reuses existing `GpuFrameTimer` and `RtSceneGpuStageProbe`; no shader changes or
  substitute bank. Runtime collection additionally requires the explicit Debug
  hidden landing selection. Ordinary gameplay allocates no additional query pools.
- Windows owns three optional pools, passes the existing probe into both normal
  recording paths and preserves success-only submission/fence/final-idle ordering.
  Initial signalled fences collect nothing; discarded evidence still drains the
  actual graphics submission. Reset/destruction remains behind successful idle.
- The saved submitted identity and completed owner must match epoch, generation,
  attempt, record, tick, slot and submission serial. All three collection/sample
  serials and slots must match too. Missing/error timing stays optional and cannot
  turn an otherwise successful RT frame into a renderer failure.
- `--capture-chapter-landing control|stages` selects a bounded observation of the
  original landing prefix. It requires Debug, hidden showcase and reunion start,
  rejects full-route/benchmark combinations and accepts only ordinary baked or
  normal shared ownership. Its receipt says `fullRouteValidated:false`.
  The full journey gate, assertions and 150-second deadline remain; the separate
  prefix has 30 seconds/256 frames and requires reaching the original Raise phase.

The partial observation uses identical ordinary staging for both representations:
one initial scene, no candidate-only recreation and 64 additional fixed-pose
warmup frames with an assertion that gameplay ticks do not advance. Measurement
then uses the original four authoritative 60 Hz ticks/render and 128 landing
frames. This is not a natural Keeper victory or full journey acceptance.

## Fixed comparison, not a tuning loop

RTX 5050 Laptop GPU; real RayTracingPipeline, current Diagnostic High bank,
960x540 internal and output, DRS OFF, High water/fire, Higher shadows, uploaded
Mist ON. Windows preferences are unchanged; this does not change mobile Mist OFF.
The inherited build label `1.6.2` is not a 1.7 release identity.

One binary, one fixed order, no overlapping owned build or test:

1. baked without stage probes; shared without stage probes;
2. baked/shared instrumented pair;
3. shared/baked instrumented reverse pair;
4. shared without stage probes; baked without stage probes.

All rows and outliers are retained. There are 512 complete three-stage observations
and 1,024 whole-command-buffer samples. The two repeats are not 128 independent
play sessions or unique devices. Each of the eight runs has identical completed
camera ticks/poses, initial light upload, captured shader/quality identities and
one initial capture; all settings/campaign/foreground custody checks pass.

| Run | Whole GPU median / P95 ms | AS median / P95 ms | Dispatch median / P95 ms | Transfer median ms |
| --- | --- | --- | --- | --- |
| Baked, probes OFF opening |6.937 /12.904 |— |— |— |
| Shared, probes OFF opening |7.101 /13.142 |— |— |— |
| Baked, probes ON first |7.316 /15.120 |2.941 /5.848 |4.304 /9.268 |0.063 |
| Shared, probes ON first |7.065 /13.744 |2.903 /5.877 |4.038 /7.784 |0.059 |
| Shared, probes ON reverse |7.169 /13.125 |2.924 /5.902 |4.145 /7.198 |0.062 |
| Baked, probes ON reverse |7.500 /14.887 |2.874 /5.840 |4.544 /9.209 |0.063 |
| Shared, probes OFF closing |6.895 /13.918 |— |— |— |
| Baked, probes OFF closing |7.133 /14.856 |— |— |— |

P95 is the lower nearest sorted sample at 95%; no sample was removed. Stage
intervals are not additive counters. AS includes this frame's dynamic BLAS/TLAS
commands, not one-time asset loading. Dispatch is the existing shader-stage
timestamp interval around the real RT dispatch. Transfer starts after the existing
storage-image shader-to-transfer barrier. The whole timer covers the command
buffer. These are GPU intervals, not CPU command-record durations.

Median GPU intervals fit 16.667/20.000/33.333 ms budgets in this fixture, and whole
GPU P95 remains below 16.667 ms. Render-cycle CPU medians are approximately
51–53 ms in Debug. No scanout FPS, Shipping gameplay or Android gain is inferred.

Instrumentation overhead is **not established as zero**: baked probes-ON medians
are 7.316–7.500 ms versus 6.937–7.133 ms OFF; shared ranges overlap more. Clock
variation and ordering prevent isolating the difference as timer cost. Exact-tick
paired percentage tails are large, including opposite-direction outliers, and
are retained in the summary; median differences are not a uniform per-view gain.

Read-only NVML samples were collected roughly once/second throughout every run.
The retained active-window samples include warmup and measurement, with GPU
utilization at least 10%. Temperatures span 45–51 C and sampled graphics clocks
180–2625 MHz. Samples frequently fall between short GPU bursts; they do not
measure clocks at each dispatch or establish matched execution clocks. They are
context, not a scheduler/utilization trace. No clock, power, affinity or priority
setting was changed and no cooldown/retry loop was used. Warm data is retained,
not discarded because it cannot prove a causal gain.

Tracked RT allocations still differ by 45,372,864 device-local bytes, with both
shared masters resident and 92 versus 28 TLAS entries. New query pools are outside
that inventory. Lower allocations do not prove faster rendering or total RAM use.
One-time asset/BLAS measurements are retained separately and are not substituted
for per-frame AS cost.

## Correctness and the original signal

All six captured poses are byte-identical within each representation across its
four instrumented/uninstrumented runs: instrumentation does not change the image.
Between baked and shared, 53–196 of 518,400 pixels differ in the retained poses,
maximum channel difference 26. Inspection of the native pair shows no apparent
silhouette/contact change. This is near parity, not pixel identity or proof of
every reflection, shadow, route or device.

![Baked landing witness](a-stages-1.png)
![Shared landing witness](b-stages-1.png)

The original full-route A/B/A measured 7.976 /11.576 /8.120 ms in its landing phase.
It remains a real observation. Its candidate alone performed same-owner recreation
(scene epoch 2 versus 1), and the initial uploaded lantern source differed by about
1.61 mm in world Z after that lifecycle. It had no in-run clock samples and no
separate AS/dispatch timing. These differences do **not** prove they caused 45%.
The new prefix removes those setup differences and adds equal warmup; it cannot
retroactively attribute old samples or distinguish every setup/warmup/clock factor.

Current evidence therefore does not justify a renderer change aimed at an assumed
instancing/traversal penalty. AS medians are essentially unchanged, dispatch has
no persistent increase here, and transfer is small. The smallest justified change
is the equalized, identity-checked comparison path implemented here. Before any
normal shared admission, use identical preparation in the broader existing route
and real warm-play acceptance; do not revive closed inverse-transform/LOD trials,
add trees, lower quality or start repeated unchanged profiling cohorts.

## Validation and admission limits

Probe-ON Debug application and three focused tests pass: timer/strict identity,
frame coordinator and launch guards. After measurements, the existing Debug cache
was restored to probe OFF, rebuilt and the same three tests passed again. A normal
hidden frozen RT smoke capture passes with zero pixel differences against the
corresponding probe-build control; its executable hash is separately recorded.
See [ON](test-probe-r1.log) and [OFF](test-default-off.log) checks.

The bounded reviewer found no additional ownership/render-failure issue; its
30-versus-150-second diagnostic wording finding was corrected before capture.
Original assertions, query tests and deadlines were preserved. Active-probe
swapchain recreation/device-loss is not newly physically certified by this prefix;
normal idle/reset ordering and existing timer/coordinator fixtures remain the
supporting contract. No phone or foreground activation was attempted.

All 151 parked owner files remain hash-identical. No captures, builds or transfers
remain active after this checkpoint; exact-head hosted CI/readback is recorded in
the final handoff. No merge, release, device install or report transmission.
Audio/haptic manual revalidation required: **NO**, observational timing only.

Forest visible depth and the rejected compact-carry family remain unfinished.
Canopy, flask and effects choices stay in the [owner decision sheet](../../design/HORDE_1_7_OWNER_DECISIONS_2026_10_24.md).
They were not implemented or silently deferred by this attribution work.
