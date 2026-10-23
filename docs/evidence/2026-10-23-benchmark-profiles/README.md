# Shared benchmark profile contract - 23 October 2026

Base source: `478f6c2512a2ef0aaf3e73e5ce276062091e2118`.
This checkpoint implements shared profile selection, validation and temporary
working-state restoration. It does **not** enable a standardized run in either
platform UI, establish device practicality or admit a ranked result.

## Current implementation and change

Existing `ShowcaseBenchmark` owns the versioned course and warmup/measurement
laps. `BenchmarkSummary` joins completed owning RT evidence and sanitizes typed
configuration, but validates dimensions as a percentage of output. Its recorded
per-submission scene evidence does not carry internal/output extents or DRS.
Matching start/end sizes alone therefore cannot certify a fixed-size course.
`BoundedDynamicResolution` remains an uninstantiated experiment, not an active
production setting.

`src/gameplay/BenchmarkProfile.h` adds a closed shared selection/validation
contract, with two deliberately provisional definitions:

| ID | Compiled optics / platform | Actual internal | Actual output | Effects tuple |
| --- | --- | --- | --- | --- |
| `mobile-rt-provisional-v1` | Mobile / Android | 640 x 360 | 1280 x 720 | 50%, Mobile water, Low fire, Current shadows, glass off, mist off, dust off |
| `windows-rt-provisional-v1` | High / Windows | 1920 x 1080 | 1920 x 1080 | 100%, High water/fire, Current shadows, glass/mist on, Low dust |

These are candidate contracts for practicality testing, **not official adopted
resolutions or new gameplay defaults**. Both pin `showcase-route-v1`, 16:9 camera
and output, fixed-buffer presentation preserving aspect, and DRS off. The
hardware RT backend must remain the actual selected backend throughout a run;
TraceRays and hardware RayQuery results still require distinct backend cohorts.
No raster/baked path or quality fallback is introduced. Personal preview pacing
is preserved because existing platform code excludes benchmarks from the
ordinary preview cap; this contract does not certify scanout FPS.

`BenchmarkProfileRun` leases an application-owned working tuple without touching
the confirmed/pending settings store. It prevents nested overrides, refuses
cross-platform/compiled-optics substitution, and requires matching observed
state before completion. Any intermediate mismatch latches failure even if a
later closing state matches. Complete, cancel, apply error, incomplete apply,
exception unwind and abandoned scope all restore the original tuple exactly.
The working owner must outlive the scope. A platform failure must call `Fail`.
These observations are an API boundary: passing the requested tuple back is not
proof of a GPU dispatch or surface operation.

The live local report builder now always emits
`benchmarkProfile: {id: "custom", classification: "custom-unranked", ranked: false}`.
It cannot infer a standard from coincidentally matching pixels. The report keeps
existing consent, separate hardware opt-in, aggregate-only fields, byte cap,
population gates and scanout disclaimer. No report is sent or uploaded by this
change; no leaderboard, email, backend or publication schedule is activated.

## Validation

- MSVC Release builds of the new profile test and existing summary-report,
  graphics-settings and playtest-report tests pass. Four selected CTests pass
  in 3.43 s; the profile fixture reports **162 checks, zero failures**.
- Negative cases cover internal/output drift, wrong aspect/output policy, DRS,
  effects, workload/backend/optics drift, invalid extents/NaN/infinity, rejected
  selection and a matching closing state after failure. Both platforms exercise
  exact restoration on every exit. Custom selection stays custom even when its
  tuple matches a provisional standard.
- Read-only review found an exact-double comparison rejecting a truthful float
  camera ratio after it had passed the declared aspect tolerance. The new
  regression reproduces four failed checks out of 162 before correction; those
  failures are retained. Observe now validates the measured ratio first, then
  normalizes only its local aspect copy for comparison, keeping all other
  fields exact. Float-rounded 16:9 passes and a +2e-5 drift still fails. The
  original tuple is restored without normalization. Review confirms closure.
- CTest roster dry-run: 186 registered, 150 in the existing Windows selection;
  the new native fixture is included. Existing exclusions, compiler/platform
  coverage, job cap and test deadlines are unchanged. This is not a full local
  suite or Android compilation/device result.
- Preceding exact source `478f6c25` hosted run
  [39956585719](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39956585719)
  passes all six jobs. MSVC: 13m24s total, configure 37s, build 594s, selection
  128s, **149/149** results, native report UI and 10/10 APK resolver cases pass;
  no annotations. Its result does not validate this newer source. Earlier
  explicit 15-minute cancellations remain retained; no cap increase is made.

## Remaining integration and next bounded slice

Add actual internal/output dimensions and aspect/DRS/presentation policy to
submission-owned evidence before assigning provisional profile labels to live
reports. Then wire fixed surface/buffer sizing and aspect-preserving output in
one platform owner, applying the guard around all success/cancel/error/lifecycle
paths. Existing user edits must be disabled or cancel the scoped run rather than
mutating confirmed settings beneath it. GPU/surface rollback, resize failure,
rotation/background and physical letterboxing still need platform proof; the
host tuple guard alone does not perform those operations.

Validate practical output/internal sizes and image quality on Windows and an
authorized phone before promoting any profile. Preserve version/profile/backend
cohorts, sample counts, medians/ranges and transparent outliers; random report
IDs do not count unique devices. Ranked fairness, sustained warm performance,
phone controls/rotation, natural earned journey, Debug route deadline, Kit/audio
acceptance, PR conflicts and deferred forest/smoke/flask decisions remain
separate. No phone operation, personal setting/save mutation, render change,
merge or release occurred.

Compact source/log hashes and the exact preceding CI receipt are in
`receipt.json` and `478-terminal-receipt.json`. Raw local build/CTest/roster logs
remain ignored under `reports/benchmark-profile-20261023/`; no raw telemetry or
large generated artifact is added to Git.
