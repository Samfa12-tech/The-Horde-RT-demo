# Exact source-baseline lantern comparison — September 20

Status: **four complete held-high measurements; observation foundation ready
for Phase 2, not a material glass optimization or physical-transport pass**.
This closes the workload-selection gap left by the original route benchmark;
it does not redefine route timing as glass timing.

## Source and artifact boundary

The public 1.6.0 APK cannot select lantern cases. A narrow source-baseline branch
`codex/horde-160-lantern-baseline` starts from published runtime source
`57c81b635a6c10e2772283639026936adac80f8b`. Shared staging/tests are `513b5d3`;
Android selection/isolation/package checks are `84104e3`. This is **not the
unmodified published APK**, nor the new diagnostic-free Shipping policy.

- A: source `84104e3f1ccb654797d017595266533bfde3374f`; development-signed,
  non-debuggable `com.samfa12.hordelanternrt.baseline`, version suffix
  `-source-baseline`, checkpoints OFF, old Release-derived RelWithDebInfo path.
  APK SHA-256 `5f92533a75a9079ac6892b7c8f89781896bdcb9420c124023139036c29c4fee2`,
  82,363,143 bytes; installed and pulled back byte-identically on **SM-S948B**.
  Native reports explicitly identify `57c81b6 + lantern harness`.
- B: clean engineering runtime source `167ce8b`, development-signed non-debuggable
  `.benchmark` / Shipping / Mobile, APK
  `02112a48aea45431f52270ab9ee82d68091497aed0816dfcc359ba4068ee24bc`.
  Its build, installed-byte parity and exact module proof are recorded in the
  [preceding route A/B](ENGINEERING_1_6_1_ANDROID_RELEASE_ABBA_2026-09-20.md).
- No stable app replacement, data clearing, production signing or publication.

The baseline's renderer, shader sources/embedded modules, simulation implementation,
asset bytes and physics are unchanged. The package checker proved all **51**
packaged asset entries equal to the published APK, correct four-ABI baseline
native models, development certificate, checkpoints OFF, non-debuggable manifest,
stripped/packaged ARM64 equality and exact old embedded raygen bytes. ARM64 hash:
`c18d06f154ef31c53879b54771453981f61ff3ab4ada2565825226d93cd6ed8d`.

| Old baseline module | Words | Atomics | Ray-query initialization sites | SPIR-V SHA-256 |
| --- | ---: | ---: | ---: | --- |
| Generic raygen | 56,191 | 32 | 3 | `e9d4fca05e8c642b6e09251a7c57253124fa6475ab5f81e34d1acb548764c23a` |
| Legacy opaque raygen | 123,311 | 5 | 23 | `870e4ea0c0b24fdcac516fec15343c1531906600d8ec28c9eaebf826a7bd75a0` |

Both old modules passed external `spirv-val`/`spirv-dis`. Keeping their diagnostic
atomics is intentional: removing them would cease to measure the actual 1.6.0
renderer. The candidate's Shipping modules contain no diagnostic atomics/binding22.
Differences are a net renderer/engine comparison, not isolated attribution to atomics.

## Workload, launch and correctness checks

`lantern-held-high-v1` uses production GameSimulation reward/held-item authority,
the same camera `(-10.65, -15.20)`, yaw −pi/2, pitch −0.30, real hinge/pendulum
state, normal renderer adapter, 100% scale / 1440x2980, Mobile water, strict ASTC,
MAILBOX and RayTracingPipeline. Each run has 600 warm-up and 600 measured frames.
This is a **frozen authored snapshot**, not a live raise or full gameplay route.
Its spatial zone is `yellow-torch-bay`, not `finale`; source bounds and both old
and new reports agree. A zone name never substitutes for the actual workload.

Fresh four-ABI baseline build and all three Java selector-policy tests passed;
the prior shared core and original route smoke passed 2/2 MSVC Release tests.
The optional baseline variant is absent without its Gradle opt-in. Java and
native code both reject unknown workloads; normal builds reject the selector.
Pending requests wait for actual RT readiness and are cleared on pause.

Phone smoke showed the real held lantern. Home interrupted the temporary scene;
normal resume/Enter the Ruin showed the normal spawn/torch scene, not retained
reward geometry. Cleanup is owned by the render thread and cleared once the
benchmark relinquishes its scene; it cannot reset later ordinary gameplay.

One repeated launch only brought an existing task forward without delivering a
new Intent. The foreground later showed ordinary gameplay/death, so that attempt
is **not measurement evidence**. No report was accepted from it. Accepted A/B
runs explicitly use `am start -S -W` on only their isolated test package, confirm
the workload HUD and retain fresh report identity/timestamps. A wait timeout is
not itself permission to restart a live run.

The baseline and candidate presented frozen images match **exactly** (maximum
RGB delta 0) over x=0..1439, y=400..3119, excluding the changing upper HUD band.
This is an Android compositor screenshot comparison, not raw RT-storage-image
capture, whole-image parity, same-build Diagnostic/Shipping parity, or proof of
all glass transport paths. Known Diagnostic glass budget/termination findings
remain open; no physical correctness gate is inferred from this match.

Independent review found the first source-baseline live-reveal path used direct
StepFixed whereas the candidate uses AdvanceFrame. Frozen cases match exactly.
The live path was aligned at `186068a` before any reveal timing; its four-ABI
rebuild, three policy tests and package/old-SPIR-V proof passed. The resulting
APK `4cbd08c2d57314697b01284ccb31b8af1cba4d99e79915356c7104e0535a3461`
was **not installed**. No live-reveal A/B is claimed for either artifact, and the
held-high numbers below belong only to the earlier installed A artifact.

## Repeated held-high results

Every run completed two case windows and 600 measured RT-presented frames at
100% / 1440x2980. All ten spatial-zone denominators agree: yellow-torch-bay 600,
the other nine zero. Each candidate has 600 exact submitted/completed identity
joins, valid CPU/GPU rows, no rejection/cancellation/outstanding samples and
compiled-out diagnostics. Fresh processes each finish at completion serial1201;
serials are not expected to be monotonic across different processes/run IDs.

| Order | Artifact | Median ms | P95 ms | Thermal status | Battery C | GPU thermal power level |
| --- | --- | ---: | ---: | --- | --- | --- |
| A1 | 1.6.0 source-baseline harness | 134.387 | 135.938 | 0–1 | 33.0–34.6 | 0–1 |
| B1 | 1.6.1 clean Shipping candidate | 130.6903 | 141.5853 | 1 | 35.0–36.1 | 0–5 |
| B2 | 1.6.1 clean Shipping candidate | 130.5626 | 139.8669 | 1–2 | 35.9–37.1 | 0–9 |
| A2 | 1.6.0 source-baseline harness | 134.578 | 145.751 | 2 | 36.7–37.7 | 0–5 |

Mean-of-run-medians: A **134.4825 ms**, B **130.62645 ms**, a descriptive
**−2.867%**. This is not a pooled median, proof of statistical significance or
attribution to a particular shader/CPU change. The two baseline medians differ
by only 0.191 ms and the two candidate medians by 0.1277 ms, but P95 and thermal/power
history vary. Both builds remain far above all 16.667/20.000/33.333 ms reference
lines: roughly 7.44 versus 7.66 median-derived FPS. This is a small net difference,
not the material glass performance improvement required later in the programme.

Candidate GPU RT-command-buffer medians are 128.9474/128.866 ms; CPU fence-wait
medians 129.2093/129.1949 ms. Player-skin means 0.0124/0.0122 ms and dynamic-upload
means 0.0036/0.0039 ms are tiny for this frozen case. The evidence points toward
GPU command work rather than CPU skinning here; it does not isolate trace time
from AS/copy work, nor generalize frozen-pose CPU caching to moving gameplay.

The approximately 13-minute accepted A–B–B–A sequence used a fresh process and
600-frame warm-up for every run, without deliberate cooling or concurrent builds.
Context windows include startup/warm-up/observation delay, not measured-only
thermal samples. Settings/workload/procedure match; thermal states are **not
identical**, and this is not a continuous-process sustained gameplay test.
The failed earlier launch and functional/cancellation smoke are excluded.

Reports and the reproduced arithmetic/pixel result are preserved in the
[recovery bundle](evidence/2026-09-20-lantern-abba/README.md). Raw reports were not
edited to fit assertions. Lead review corrected the analyzer's initial zone
assumption using `ShowcaseRoute.h` and existing evidence, restored strict legacy
field checks and fixed its result projection before the successful analysis.

## Phase 1 gate decision

The observation foundation can now support the semantic/viewmodel phases:

At closeout, the current engineering checkout freshly built and passed **3/3**
MSVC Release tests: `horde_rt_pipeline_bundle_contracts_tests`,
`horde_rt_frame_evidence_coordinator_tests`, and
`horde_rt_pipeline_bundle_lifetime_tests`. Source inspection also confirmed that
diagnostic allocation/descriptor/read/reset/barrier actions are unavailable in
Shipping. This is a focused gate check, not the final matrix or new remote CI.

- Existing 15-pair Windows High Diagnostic/Shipping captures pass the accepted
  RGB tolerance; held-glass Diagnostic counters remain available (including the
  failures that Phase 4 must investigate). See the prior lantern evidence report.
- Exact clean Android Shipping/Mobile and Windows Shipping/High artifact scans
  retain hardware queries and contain zero prohibited diagnostic atomics/binding22;
  the established Shipping runtime/tests keep Diagnostic buffer IO disabled.
- Completion-owned CPU skin/upload/BLAS-refit/TLAS-record and GPU observations
  retain the complete intended sample counts and honest timing labels.
- Exact public-APK route comparison and the clearly identified source-baseline
  held-glass comparison now record hashes, matched settings/warm-up procedure,
  natural thermal history and limitations. The audit's inaccessible public-APK
  lantern checkpoint is deliberately implemented as a narrow source harness,
  supported by unchanged renderer/assets/shader bytes and observed image parity;
  it is never relabelled as the public binary.

This closes the **measurement-foundation gate**, not glass correctness,
thermally identical causal A/B, all-lantern-view performance, the final matrix,
or the whole programme. Phase 2 is the four-way player semantic contract.

## Evidence boundaries

Local evidence: `reports/phone-lantern-abba-20260920/` in the engineering worktree.
Baseline package/static evidence: `C:\Dev\tmp\horde-160-lantern-baseline\reports\harness-core-20260920/`.
The common timing metric remains native render-entry through present return;
baseline lacks candidate completion-owned GPU distributions. Whole-run thermal
windows include startup/warm-up, not just measured frames. No cooling, lower
resolution, glass-disable, changed lighting or fake rendering is used to gain FPS.
The phone was returned Home, sampler stopped, stable settings unchanged and both
isolated test packages remain at 100%. The installed baseline is still the measured
`5f9253…c4fee2` APK, not the later live-step-aligned rebuild.

Audio/haptic manual revalidation required: **NO**. Normal feedback inputs/assets
are unchanged; the harness retains benchmark feedback suppression. Subjective
viewmodel acceptance and final cross-device/release validation remain separate.
