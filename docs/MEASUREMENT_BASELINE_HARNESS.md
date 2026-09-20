# 1.6.0 source-baseline measurement harness

This branch is **not a release** and must not replace the public 1.6.0 artifacts.
It starts from exact published runtime source
`57c81b635a6c10e2772283639026936adac80f8b` solely to admit the same production
lantern benchmark workloads used by the 1.6.1 engineering candidate.

The original signed APK cannot select these cases. Any later binary from this
branch must be identified as a locally rebuilt source-baseline with a harness
backport, retaining its own source/diff, artifact hash, renderer/shader identity,
build settings, workload, thermal history and report. Never describe it as the
unmodified published binary or silently compare its numbers to another scale.

## Shared core

- Five lantern workloads and the unchanged original route use the same enum,
  camera constants, production `GameSimulation` checkpoint/reward import APIs,
  hand/hinge resolution and pendulum states as engineering source `167ce8b`.
- The four stationary cases freeze authored state using zero simulation delta.
  The extreme-pose case is not live motion. The reveal advances at 1/60 second.
- Lantern cases repeat the same 600-frame state sequence for warm-up and
  measurement. The original route retains two 1,838-frame laps.
- Default route text/JSON retain the baseline schema-1 fields and timing
  aggregation. Lantern reports add explicit workload/policy identity and cannot
  claim full-route traversal.
- Renderer, embedded shaders, asset bytes, gameplay implementation and physics
  are untouched. Do not import the new renderer/telemetry code to manufacture
  comparability. The old renderer retains its actual 1.6.0 instrumentation;
  it must not be labelled as the new diagnostic-free Shipping policy.

Fresh MSVC 19.44 / Visual Studio 2022 x64 Release configure and targeted builds
passed on September 20. Both `horde_rt_lantern_benchmark_tests` and the existing
`horde_rt_showcase_benchmark_smoke` passed (2/2). The former checks exact workload
names/policies, production-state ownership, finite transforms, identical lap
state sequences and all live reveal phases; the latter retains the old route's
timing/report/presentation-failure checks. Build/test logs are local under
`reports/harness-core-20260920/` and `C:\Dev\tmp\horde-160-harness-build/Testing/`.

## Isolated Android selector

The opt-in `-PhordeSourceBaseline=true` registers `assembleBaseline` and
`testBaselineUnitTest`, inheriting the old Release/RelWithDebInfo renderer with
checkpoints OFF. It uses only the development certificate, is non-debuggable,
has package `com.samfa12.hordelanternrt.baseline` and version suffix
`-source-baseline`. The opt-in skips production signing environment reads.
Without it the variant does not exist. Native benchmark reports explicitly say
`source-baseline (57c81b6 + lantern harness)` rather than merely `1.6.0`.

```powershell
$env:HORDE_VALIDATION_UNSIGNED = '1'
# From android/:
.\gradlew.bat -PhordeSourceBaseline=true :app:assembleBaseline `
  :app:testBaselineUnitTest --tests com.samfa12.hordelanternrt.BaselineBenchmarkIntentPolicyTest
# After exact package proof and an authorised install, using the verified device:
adb -s VERIFIED_SERIAL shell am start -W --user 0 `
  -n com.samfa12.hordelanternrt.baseline/com.samfa12.hordelanternrt.MainActivity `
  -a com.samfa12.hordelanternrt.action.BASELINE_BENCHMARK `
  --es horde.benchmark.workload lantern-held-high-v1
```

The six workload names are allowlisted in Java and again in native code. Other
builds reject the action/native selector. Debug-extra mixtures and concurrent
requests are rejected. A pending request waits for actual RT readiness and is
cleared on pause. No persisted rendering setting is changed. Native staging,
zero-delta/fixed-step advancement and cancellation cleanup remain on the owning
render thread. The final measurement frame cannot grant the gameplay finale
unlock. A completed/cancelled benchmark no longer owns the scene and therefore
cannot reset later gameplay on Home.

This deliberately reuses the original report UI and COPY/SAVE facility; there
is no new baseline report exporter or modern completion telemetry. Capture the
visible text, verify workload, completion, timestamp, 600 measured frames, exact
settings and real presentation. Keep the original schema-1 CPU timing boundary;
never compare it directly with candidate GPU distributions. Pending/failed or
interrupted runs are not results. Startup-update prompts are suppressed while
the harness is pending/running, and the normal benchmark feedback suppression
is retained.

Fresh four-ABI Android build and all 3 focused baseline Intent-policy tests
passed. Non-opt-in task inspection confirms the baseline variant is absent.
`tools/check-source-baseline-package.ps1` checks identity/development signing,
four exact native models, checkpoints OFF, unmodified baseline renderer/source,
all 51 packaged asset entries byte-equal to the published APK, stripped/package
ARM64 equality, and exact old raygen module containment with external SPIR-V
validation/disassembly. The old generic module has 56,191 words/32 atomics/3
ray-query initialization sites; legacy has 123,311 words/5 atomics/23 sites.
Those diagnostic atomics are deliberately preserved for an honest 1.6.0 baseline.

Real RT presentation, interrupted-scene cleanup and device measurement remain
pending. Build/test existence is not a phone or performance pass. Publication
and production signing are not authorised.

Audio/haptic manual revalidation required: **NO** for this shared measurement
core. Normal gameplay feedback and assets are unchanged; platform wiring must
preserve the existing benchmark feedback suppression.
