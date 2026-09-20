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

Platform selection,
first-frame/lap staging, isolated development packaging, real RT presentation,
and device measurement are still pending. Source/test existence is not a phone
or performance pass. Publication and production signing are not authorised.

Audio/haptic manual revalidation required: **NO** for this shared measurement
core. Normal gameplay feedback and assets are unchanged; platform wiring must
preserve the existing benchmark feedback suppression.
