# 1.6.1 frame-evidence integration

Local implementation accepted at `f443904cd6bc1d3c8ad72c0b6c92ef324621322b`.
This closes the preserved C2b integration, not the complete observability,
compatibility, player/viewmodel, glass, music, reporting or release programme.
The [current plan](ENGINEERING_1_6_1_PLAN.md) retains those requirements.

## Ownership and corrections

`RtFrameEvidenceCoordinator` joins recorded CPU/resource facts to an exact
successful queue submission and its owning fence or successful final device idle.
Optional identity/clock/GPU-timing failure cannot stop valid rendering. Separate
graphics ownership drains tokenless Diagnostic work without inventing evidence.
Shipping performs no Diagnostic IO. Android preserves monotonic identity floors
across context moves/restarts. Gameplay remains the existing shared authority.

Recovery review found and corrected presentation accounting, Diagnostic failure
publication, development-checkpoint simulation attribution, whole-frame entry
timing, and production command-placement coverage. The shared command-routing
tests now execute the same conditional BLAS and fixed TLAS/dependency/trace-copy
sequence owners as the scene, substituting only external Vulkan commands.

SUBOPTIMAL presentation counts as successfully presented but is benchmark-ineligible.
Windows cancels its legacy benchmark before recording any recreate-interrupted
interval, including the final frame after replay completion. Actual Diagnostic
read failure remains fatal and publishes exact-owner Error/null evidence, not stale
counters or player pixels. Diagnostic reset/read IO is separately counted and
inside WholeFrameCycle; DynamicUpload remains scene-data upload work only.

## Fresh targeted evidence

MSVC 19.44 / VS 2022, Vulkan SDK 1.4.350.0:

| Check | Actual result |
| --- | --- |
| Debug evidence/coordinator/GPU collection/scene observation/inventory/Character/checkpoint CTests | 7/7, 10.58 s |
| Release evidence/coordinator CTests | 2/2, 2.22 s |
| Debug/Release GPU collection tests | 1/1 each |
| Portable Release stage-observer/development-checkpoint tests | 2/2 |
| Final-interval benchmark cancellation regression | RED reproduced, then Debug 1/1, 3.98 s |
| Windows Debug application | Linked after final benchmark fix |
| Android externalNativeBuildDebug | All four configured ABIs, success in 24 s |
| Diff whitespace check | Passed; existing checkout line-ending advisories only |

The Android native check precedes the final platform-independent benchmark Cancel
guard; that small correction has fresh host behavior/link evidence. This is not
assemble/lint/APK/device validation. Earlier native configuration emitted CXX5304
SDK XML tooling-version warnings; the final incremental native run had no warning.

One targeted `lantern-held-high` Windows capture honestly presented pipeline RT on
the RTX 5050 Laptop GPU, exited 0, and produced a scene-only 960x540 storage-image
capture with no overlays. The image is byte-identical to the pre-review capture:

- PNG SHA-256: `f05c711feccfb4ece547fd842d809df2a7d3a96db10294cc6414bec1955fa222`.
- Retained EXE SHA-256: `4f95d5a7f74078d9f552664f994c3384d4c86f8f949c551b0ff50cb940f1683e`.
- Local evidence: `reports/c2b-recovery-fixed-20260913/`.
- Build provenance: `4c25b87` plus the reviewed C2b working changes, before the
  final Windows benchmark-admission correction. It is not relabelled as a clean
  final-commit executable or a RayQueryCompute run.

The image was inspected: the existing procedural arms remain. No dedicated
viewmodel, glass correctness/performance improvement, S24/S25 compatibility,
sustained performance or owner-feel acceptance is claimed. No phone action,
publication, signing or licensing change occurred. Remote CI has not run for
this local commit; the comprehensive final matrix remains deferred until the
complete feature set under the owner's September 13 instructions.

Audio/haptic manual revalidation required: **NO**. This slice changes observation
and benchmark validity, not simulation inputs, event identity/timing, feedback
transport, listener/source data, assets, playback or haptic routing.
