# Bounded staged-primary Mobile RT experiment

Status: Diagnostic/Mobile and Shipping/Mobile-policy RTX image matrices pass
their unchanged pixel gates; capture-loop timing comparisons fail. **No warm
Shipping or phone performance acceptance, and no promotion.** Android candidate
compilation/static inspection passes. The optional pass observer now has real RTX
submission-owned runtime evidence; exact phone admission now **FAILS** the
unchanged image/counter gate. Phone Shipping timing is held, not assumed.
This is the single resumption record. Read its next unfinished step instead of
restarting the completed omission investigations or rebuilding unchanged artifacts.

## Identity and hypothesis

- Control source: `7ab8607e3f3297444dc17fcac9344b29b8d8ccce`, engineering branch.
- Candidate source/artifact hashes: retained in the
  [first-candidate evidence](evidence/2026-10-01-staged-primary-v1/README.md).
  Builds used the dirty worktree at this slice; the runtime source fingerprint,
  exact executables/APK/modules and capture identities are retained. A subsequent
  reviewed commit does not retroactively turn these into clean-commit builds.
- Scope: one candidate iteration, two dense RT-pipeline raygen passes. No work
  queues, temporal reuse, reconstruction, sample reduction or geometry changes.
- Hypothesis: removing primary traversal/material decode from the continuation
  shader may reduce mixed-work/compiler/register costs. Shader size is **not**
  register, spill, occupancy or causal evidence. The second pass still executes
  every accepted opaque/glass/shadow/atmosphere path and light sample.
- Primary metric: matched warm `WholeFrameCycle` median in ms (lower), with GPU
  total, p95 and per-pass timings. Target33.333ms is an outcome to test, not a
  promised saving. With estimated9–11ms serial CPU/other allowance, GPU needs
  roughly22–24ms. Existing9ms stripped-primary results are not a fixed floor.
- Existing measurements/results are linked in the
  [feasibility record](ENGINEERING_1_6_1_MOBILE_RT_FEASIBILITY_2026-09-30.md).
  Preserve rejected first-blocker, opaque-secondary and omission results. Never
  add their savings or repeat them to manufacture a forecast.

## Proposed smallest split (before implementation)

1. Generate the **same** primary ray, mask, hit and material decode with hardware
   `rayQueryEXT` inside `vkCmdTraceRaysKHR`. Store the decoded primary hit.
2. Load it and execute the unchanged primary shading, physical dielectric
   continuation, transparent shadow transport, fire/mist and output programme.

Opaque receivers in a GenericDielectric frame still receive glass-filtered light;
primary opacity does not permit selecting OpaqueFast for their shadows. Preserve
the existing frame-owned strategy decision, all budgets and diagnostic failures.
Pipeline/compute parity and Shipping/Diagnostic parity remain separate gates.
This first candidate targets RayTracingPipeline only; compute is not certified
by it and must not silently select a different execution organisation.

Lossless record proposal:32 scalar32-bit words /128bytes. Keep every FP32 hit,
material and accepted dielectric spawn field, not a newly inverted transform or
quantised normal. Derive hit from the existing negative primitive miss sentinel;
use the otherwise unused instance high bit for the spawn-guard boolean (Vulkan
instance custom index is24bit). Three bounded SSBO pages avoid assuming a
309MB single-descriptor range. Check device range/index/allocation limits;
unsupported prototype limits are an explicit failure, never a quality fallback.

At the previously recorded75% phone extent1080×2235 (2,413,800pixels):

| Quantity | Estimate, not hardware measurement |
| --- | ---: |
| Logical records |308,966,400bytes /294.65MiB |
| Full write plus read per frame |617,932,800bytes |
| Logical rate at30FPS |18.54GB/s |

Actual memory requirements/alignment/selected memory flags must be recorded.
Logical traffic is not DRAM traffic (cache/write combining may differ). Repeated
camera arithmetic, an additional dispatch, shader-write→shader-read barrier and
these pages may erase any compiler benefit. No positive saving estimate is justified.
Normal Shipping modules/catalog and default selection stay unchanged. Candidate
modules, selection and identity must be explicitly investigation-only.

## Finite validation/run matrix

Run once per exact artifact; repeat only for a documented validity problem.

1. Host: lossless layout/packing, miss/guard/opaque cases, page bounds/overflow,
   resource cleanup/resize ownership; compile/validate/disassemble actual modules.
   Shipping candidate must also contain no diagnostic atomics/binding22.
2. Windows RTX: control/candidate pairs for the existing production-lantern,
   tinted, millimetre-closed, fire, edge-Fresnel, high-look-up and low-parry
   fixtures, plus ordinary opening. Use unchanged RGB tolerance3 and physical
   counter expectations; known control failures stay explicit. No restored buggy
   pixels or looser tolerance. Image gate precedes timed comparison.
3. Exact SM-S948B: same75%1080×2235, Shipping/Mobile, assets/backend/settings,
   music state and measurement observers. Opening, held-high and live reveal.
   Two interleaved warm ABBA blocks per workload, fixed existing sample windows,
   record order, temperature, thermal status, GPU power level/clocks and cooling.
   No cooled-versus-warm substitution. Candidate per-pass timestamps must be
   fence/submission-owned; unavailable timestamps are gaps, not zero costs.
4. Separate instrumented profiling runs at matching warm conditions if counters
   perturb timing or require replay. Never mix profiled and unprofiled medians.
   Windows evidence does not certify S26; S26 does not certify S24/S25.

## Memory profiling admission

RAM-pressure evidence: exact process/package identity, PSS/RSS/native heap,
graphics allocations and thread/resource counts; system available RAM/pressure
or reclaim indicators where exposed. Sample at workload boundaries and bounded
cadence, compare trends and record sampler overhead. Stable PSS is not proof of
low bandwidth; GPU device-local and host-visible classifications can overlap on
UMA and must not be summed as distinct physical heaps.

GPU evidence: discover the **exact** S26 driver/tool counter roster on reconnection.
Use exposed bandwidth/read/write/cache-miss/memory-stall counters with names,
units, scope and sampling method. Record unsupported extension/tool/permissions
and unavailable counters individually as gaps. No counter value is currently
known; do not assume Snapdragon/Adreno marketing establishes availability.
Registers/spills/occupancy are likewise unknown unless actually exposed.

On reconnection, query the device's Perfetto data-source descriptors once and
retain the raw roster before choosing counter IDs/names. Use the advertised
producer name, units and scope, not IDs copied from another Adreno. See the
[primary GPU tracing documentation](https://perfetto.dev/docs/data-sources/gpu).
For each of GPU read/write bandwidth, cache misses, memory stalls, registers,
spills and occupancy, record `available`, `unsupported`, `permission/tool blocked`
or `not exposed`; availability requires an actual sample, not just a descriptor.
Device-wide counters are not automatically attributable solely to Horde. If a
Vulkan performance-query tool is usable, retain its exact queue/counter roster
and required profiling/replay passes separately; the
[extension specification](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_performance_query.html)
does not establish support on this phone. Do not add a generic query framework
merely to discover support. Do not root the phone or change its system settings.

The bounded RAM sidecar is `tools/capture-android-memory-samples.ps1` (PowerShell
7.5+, guarded explicitly; Windows PowerShell5.1 is unsupported). It consumes
the existing runner's exact `trial.json`; it does not install/pull back an APK,
prove GPU-counter availability or independently observe workload boundaries.
Example for an already verified Shipping/Mobile 75% trial, in a separate host
process while the workload runs:

```powershell
.\tools\capture-android-memory-samples.ps1 `
  -TrialPath <exact-trial-directory>/trial.json `
  -OutputPath <new-memory-sidecar-directory> `
  -SampleCount 6 -IntervalSeconds 30 `
  -BoundaryLabels @('warm-start', 'warm-1', 'warm-2', 'warm-3', 'warm-4', 'warm-end')
```

These are operator-declared schedule labels, not measured transitions. Sampling
interval is the delay after the preceding collection; each collection has its
own UTC start/end and duration. Preserve raw command outputs, partial receipts,
missing fields as null, source errors and PID/start-time identity failures. Use
the same cadence on control/candidate; if it perturbs timing, run memory profiling
separately and do not mix those medians with observer-free comparisons. A completed
sidecar means the scheduled collections finished, **not** all metrics passed.

Candidate overhead evidence: actual three-page allocation bytes/flags, logical
read/write bytes, per-pass plus total GPU timing, and exposed memory counters.
If traffic cannot be measured directly, report the allocation/logical arithmetic
and **unknown DRAM bandwidth cost** separately. Total-minus-pass residual is not
a pure bandwidth measurement. A traffic-only calibration, if needed, is a
separate nonvisual control, never a reduced-effect rendering candidate.

### Separate optional pass observer

`HORDE_RT_STAGED_PRIMARY_TIMING=ON` is opt-in and requires the explicit staged
candidate. Android additionally requires `hordeBenchmarkValidation=true`, the
staged shader directory and `hordeStagedPrimaryTiming=true`; ordinary Debug and
Release explicitly reset all three prototype settings OFF/empty. No normal
configuration or installed phone build was switched.

Three additional timestamps per submission delimit TOP→primary RT completion
(includes prerequisite waits), then primary→shading completion (includes the
write/read barrier). Availability, slot and exact submission serial are collected
only after the owning fence or successful final idle. Unsupported/failed/mismatched
queries remain null/error rows. A preallocated4000-row profile is separate from
the accepted benchmark ledger and its ordinary report; no allocation/file IO/JSON
occurs in frame retention. Profiled medians must not be mixed with unprofiled ABBA.

These intervals include record write/read work but do **not** separate DRAM traffic
from traversal/shading/cache/synchronisation. Subtracting their sum from total GPU
time is not an intermediate-buffer bandwidth measurement. Actual memory counters
and query overhead comparisons remain required where available.

## Decision and continuation

GO only if image/physical gates pass and matched phone results show meaningful
net savings without tails/RAM-pressure regressions. Then assess the *measured*
remaining costs before proposing a separate selective-temporal experiment.
NO-GO if absent/small savings or bandwidth/synchronisation offsets them; retain
negative evidence and stop this candidate. Inconclusive while phone/counters or
image evidence are missing. Restore normal configuration afterward. No merge,
release, publication or automatic promotion.

Skill fallback: `game-dev` is unavailable locally. The existing native harness
and immutable run receipts remain the evidence boundary; no nonexistent skill
CLI goal/run is claimed. User authorised this bounded experiment; scope is the
experimental renderer/shaders and focused platform/report hooks, not a general
telemetry or scheduling framework.

### Completed results

- Prior work preserved at pushed7ab8607; four unrelated scratch paths untouched.
- Reviewed slices `783f1be` (RAM sidecar), `0c84e40` (isolated staged candidate)
  and `ee05ea0` (focused owner CI) are pushed. Exactee05 push36804796096 and
  PR36804800378 SUCCESS: GCC/Clang/MSVC55/55 each event, focused Vulkan CPU-host
  14/14 each. Actual eight job logs inspected; synthetic merge75e1a02 has
  exact basebda1b99/headee05 parents. These validate that committed checkpoint,
  not later dirty profiling changes or physical RT.
- Exact7ab8607 push36798927158 and PR36798930661 SUCCESS; actual GCC/Clang/MSVC
  logs show54/54 on both events, Vulkan CPU-host13/13 on both. Synthetic PR merge
  e8432dc72f6c282f6af22209f27cfe11839ac7c3 has basebda1b99 and exact7ab8607 parents.
  These do not validate subsequent dirty candidate work or physical RT.
- RAM sidecar host fixtures:34 assertions PASS, including partial/unavailable
  metrics, command deadlines, retained raw failures and mid-collection PID reuse
  rejection. Fake ADB only; no new Android/device evidence. Generator transform
  fixtures:16 checks PASS. Actual Diagnostic/Mobile, Diagnostic/High and Shipping/
  Mobile two-pass modules compile/validate/disassemble; exact receipts are external
  under `C:/Dev/tmp/horde-staged-rt-20261001/`. No register inference.
- Fresh MSVC Diagnostic/Mobile Debug and Shipping/Mobile Release normal and
  candidate RT app links PASS. Each affected host selection passes3/3 (record/
  page contract, scene preflight, resource inventory). New CPU owner fault
  injection independently passes1/1 in both Diagnostic/Mobile build trees:
  partial allocation/pipeline/SBT failures, resize preparation/cancel/commit,
  descriptor-before-old-page cleanup and move/destruction exactly once. These
  fake Vulkan/resource tests do not certify real driver lifetime or shaders.
- Diagnostic/Mobile RTX5050 native capture pairs completed:13 standard plus7
  focused fixtures. All13 standard pixels pass unchanged maxRGB3/fraction0.001;
  worst maximum is3 and worst fraction0.00000579. All7 focused PNG pairs are
  byte-identical. The standard comparator's overall result is nevertheless FAIL:
  capture-loop median6.117650ms control versus10.675150ms candidate (+74.498%).
  This is retained negative **Diagnostic fresh-process Windows capture-loop**
  evidence, not matched Shipping performance or a phone saving/forecast.
- Shipping/Mobile **shader policy in the Debug capture shell** also completes
  all20 RTX image pairs:13 standard pass unchanged tolerances and7 focused PNGs
  are byte-identical. Standard capture-loop median6.446350ms control versus
  11.065550ms candidate (+71.656%); overall comparator FAIL is preserved. Release
  capture automation correctly rejects the request before Vulkan (exit2); the
  diagnostic shell is not presented as Release performance. Candidate Shipping/
  Diagnostic standard images are all byte-identical, but that comparator's
  timing gate separately FAILS (+3.657%). Neither comparison establishes backend
  parity, warm phone results, register pressure or memory bandwidth.
- Isolated Android benchmark Shipping/Mobile builds successfully for all4ABIs.
  Retained APK SHA256 `fe116182e2fbdbbecbfca84b1cd56a96d0f216fc671ccad68b5948a07887d62a`,
  application `com.samfa12.hordelanternrt.benchmark`, version9/1.6.1-benchmark,
  development debug signing (identity not inspected). Actual APK ARM64 library
  and Windows Release candidate each contain exactly8 expected modules: normal
  2raygen/2compute plus4staged raygen, all exact hashes/policy, Vulkan1.2 SPIR-V
  validation/disassembly PASS, zero atomics and no binding22. The normal closed
  four-module Shipping inspector is unchanged; these artifacts intentionally
  are **not normal Shipping-admissible** or admitted phone performance artifacts.
  Normal Debug/Release arguments explicitly reset prototype directory/default
  OFF; only the explicit benchmark property opts in. No installation/device run.
- RTX candidate at960x540 /540x960 records actual three-page allocation
  66,355,200bytes, equal to logical/padded bytes, memory flags1 (DEVICE_LOCAL).
  Logical read+write132,710,400bytes/frame is arithmetic, not measured DRAM traffic.
- Initial unsupported `--development-checkpoint opening` exited2 before Vulkan;
  error logs are retained. The existing13-checkpoint standard batch supplied
  opening instead. Completed valid captures must not be repeated for that error.
- Separate optional profile slice: MSVC Shipping/Mobile Release app and six
  affected host tests PASS. Shader modules/transport are unchanged. The Debug
  capture shell's13 standard plus production-lantern PNGs are byte-identical to
  the retained untimed candidate, including primary/secondary ownership images.
- Real RTX5050 Release observer smoke completes `lantern-held-high-v1`, one
  warm-up and one measured lap, all600 expected rows retained without missing or
  rejected queries, exact serial601–1200/epoch2/generation6 and valid whole-GPU
  samples. Actual extent1232x803 at100%; frozen snapshot, not live motion or a
  matched performance experiment. Actual pages126,630,144 allocation bytes versus
  126,629,888 logical bytes; nominal253,259,776 read+write bytes/frame, device-local
  flags1. The first smoke's missing Release skeleton asset failure is retained;
  one repaired run stages69 existing runtime assets with matching hashes, without
  rebuilding the executable or changing asset/source-distribution policy.
- Android optional-profile build succeeds4ABIs. The demonstrated armeabi-v7a
  typed-null move compile defect was fixed with typed Vulkan handles; earlier
  failed log and APK retained. One further28s incremental build follows the
  Android-only failed-idle ownership guard. Final timed APK SHA256
  `0e605deeeff693e01083790a314f0ecd23cb68e2472a8c077b9d2a084bcd5a8b`.
  It is an isolated development benchmark, not an installed/accepted phone build.
  [Observer evidence](evidence/2026-10-01-staged-primary-profile/README.md) preserves
  runtime identity, image proof, build inputs and exact profiled-binary inspection.
- Optional observer `59cb057` is pushed, exact push36807823930/PR36807828168
  SUCCESS: GCC/Clang/MSVC55/55 and focused Vulkan CPU-host15/15 on both events.
  Synthetic integration17f31fe uses exact59cb057/basebda1b99. These validate the
  committed observer checkpoint, not subsequent Android admission changes.
- Bounded Diagnostic/High RTX supplement is complete:15 pairs preserve images,
  unchanged maxRGB3/fraction0.001, two focused PNGs byte-identical. Comparator
  timing FAIL +76.918% is retained separately (candidate observer ON/control OFF,
  fresh-process Debug captures, not matched performance). Do not repeat it.
- Normal Shipping/Mobile control `8ead0a2ba632ba490027ee8806d7e135002fb09713fad14390fcd2186a5bf175`
  builds4ABIs and passes the unchanged four-module package inspector. Its70
  runtime assets exactly match both retained staged Shipping candidates.
- Diagnostic phone admission is now explicit, isolated `.debug.staged`, with
  immutable APK/Diagnostic-Mobile manifest and unchanged player assets; wrong
  policy/backend/hash/overlay/implicit selection fail closed. Candidate APK
  `db125b80210753a9b1e50bdf51cbb5a923036d13280db346b3190f13f4afc299`
  builds4ABIs. Actual ARM64 eight-module containment and Vulkan1.2 validation/
  disassembly PASS, with Diagnostic counters retained. Eleven real Gradle probes
  and26 host admission assertions (PS7/5.1) PASS; admission/character-slot CTest2/2
  PASS after updating a demonstrated stale pause-adapter assertion, not runtime
  code. [Receipts](evidence/2026-10-01-staged-primary-admission/README.md).
- Normal Diagnostic/Mobile Debug control is retained at
  `581ea4668967458c000162da9c3eb722cd5e69719f069b80aa79591cebcc066a`:
  four-ABI build PASS, staged/timing OFF in actual caches,70 runtime assets match
  staged Debug and Shipping control. This justified policy-switch build is done;
  no rebuilding either Debug APK just to change metadata.
- Owner has made the phone available and deferred music listening. ADB confirms
  authorised serial/modelSM-S948B; raw Perfetto service query is retained privately.
  Exact descriptor decoding and physical/image admission are next, not yet passes.

### Exact-phone admission checkpoint (after8fb1da4)

- Exact8fb push36810364762 / PR36810369431 green: GCC55/Clang55/MSVC56/focused
  Vulkan CPU-host15 on both events. Integrationf66a6fe joins exact8fb/bda1b99.
- Normal composite19+repair1 and actual staged20 captures completed on SM-S948B
  at75%. Same70 assets, frozen semantic state and stable player/AS ownership.
  Ten pairs fail unchanged maxRGB3 (largest116); fraction<0.000036 does not
  excuse it. All5 glass/2focused viewmodel images byte-identical. Two ordinary
  fixtures have torch+1/player−1 primary-pixel counter differences. Cumulative
  skin updates differ, not animation authority. Original control typo/final
  manifest/ASTC-log gap explicit; only missing checkpoint repaired, no repeat19.
- Production interface-budget/overflow1 and recovery80/reason2 remain on both.
- Actual intermediate308,966,400B / nominal617,932,800B read+write/frame, not DRAM
  measurement. Exact309,113B binary-safe Perfetto decode exposes only allocation
  source, no GPU counter specs in this snapshot. Hardware counters remain gaps;
  preliminary CRLF-corrupted/normalized query unaccepted.
- Apparent freeze is expected deterministic capture while RT continues; scoped
  inspected logs show no music error. Listening still deferred/open. Capture
  typo/duplicate preflight now29 PS7/5.1 assertions PASS, no native/APK rebuild.
  [Exact failure evidence](evidence/2026-10-01-staged-primary-phone/README.md).

### Next unfinished step

Review/push the capture-preflight/exact-phone failure checkpoint with fresh CI.
First one bounded normal same-APK opening/skeleton A/A capture-stability check:
sparse boundary differences need a demonstrated cause, and original control19
lacked its final manifest. This is a specific validity investigation, not a new
20-image matrix. Record exact run/next step here. Fixed24-trial Shipping ABBA and
separate RAM/profile runs stay **held by image admission**. Windows matrices,
retained APKs/counter decoding are complete; no repeat/rebuild without a new
recorded validity problem. Known control physical failures stay
open independently of execution-equivalence/performance evidence.
Keep total-frame/GPU/pass/actual allocation/logical traffic and unavailable hardware
counters separate. Record each finished trial and next unfinished row here.
The desktop negative result warrants caution, not a claim of phone behaviour;
Mobile decision remains **INCONCLUSIVE / no promotion** until exact-device evidence.

Audio/haptic manual revalidation required: **NO** for this renderer experiment;
music/feedback authority is unchanged. Separate pending music listening remains open.
