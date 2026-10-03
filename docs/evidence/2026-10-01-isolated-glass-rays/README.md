# Current isolated-lantern ray witness

Result: the remaining one closed interface-budget event and 80 reason-2
"certified recovery" events on SM-S948B are **real fifth-interface truncations**,
not duplicate candidates or near-surface spawn repeats. No production fix or
correctness acceptance is claimed. The existing four-interface limit, materials,
geometry, ray masks, diagnostics and image tolerances remain unchanged.

## Exact scope and method

Detached source `71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95`, with the retained
investigation patches; Android Debug/Diagnostic/Mobile/RayTracingPipeline,
strict ASTC, 75% / native1080x2235 on exact SM-S948B/R5GL219SZGK,
Android16/Adreno840/driver2150932499. Existing named frozen checkpoint
`lantern-glass-production`; not a live-motion or Shipping timing run.
No accepted player geometry, mounting, animation, ownership or presentation changed.

The existing native output-image capture is read back after the owning GPU
completion and retained as LE uint32 width/height plus normalized RGBA8 bytes.
Only the authorised Debug app's private `files/reports/isolated-transport.rgba`
is read with `run-as`; remote/local SHA-256 matches. Android screen captures are
not used to decode numeric records. Gzip files are lossless roundtrip-verified.
No personal media, stable-app data, APK or full native library is archived here.

| Stage | Existing phone harness run | Purpose / result |
| --- | --- | --- |
| Normal transport + native readback | 20261001-031518 | Reproduces one budget event, 80 reason2, mask2 |
| Output-only marker | 20261001-032259 | One blue `(346,1200)`, 80 yellow; only81 native pixels change |
| Selected actual-ray records | 20261001-034218 | 81 current rays, five hits each, 59 fields per hit |
| Normal DebugC11 restored | 20261001-034808 | Opening/capture/Home-resume PASS; exact install/pullback |

All four focused harness runs pass their benchmark/capture/Home-resume checks.
That is not a glass-correctness gate. Each recorded frame is valid, RT-presented
and completion-owned; the control/marker/path frames have different submission
serials and simulation ticks. All41 diagnostic values nevertheless match.
The selected checkpoint remains authored/frozen; frame identity is not fabricated.

The marker-only capture is byte-identical to normal control at every unmarked
native pixel. The larger path probe is byte-identical to the marker image outside
its81 reserved record rows. Those rows still execute every original ray, shading
operation and counter; only their final colour write is withheld to avoid record
write races. This is an instrumentation-stability check, **not** a relaxed
production image gate. No geometry is hidden to obtain visual acceptance.

Only the temporary Diagnostic/Mobile Generic pipeline module is changed by the
shader probes; compute stays normal. Path APK `3277fe81…9062c20d` packages pipeline
module `a1f236da…0079add`, 66,396 words, two static query-initialization sites and
41 static atomics. Actual packaged/stripped modules pass `spirv-val`/`spirv-dis`.
Static query/atomic counts are not dynamic workload counts. Full APK/module hashes,
compiler statistics, manual patches and exact package checks are retained.

## Proven cause

All80 reason2 paths are:

`enter pane A → TIR inside A → exit A → enter distinct pane B → exit B`.

The fifth candidate is a genuine B exit, with volume open/certified, no TIR since
entering B and positive direction-normal dot. Captured native candidate distances
are0.003091–0.005438. Seventy-nine final segments connect opposing parallel pane
faces; the other, pixel `(347,1201)`, crosses distinct adjacent faces12→13.
Component pairs are3→4(30),4→1(18),5→0(21),3→2(11), under the documented GLB grouping.

The separate event `(346,1200)` enters A, internally reflects twice, exits A,
then genuinely enters another pane at primitive27, native distance0.1305838.
This is not an exact or near duplicate of the preceding candidate.

Independent offline verification matches all405 captured triangle vertices
exactly to the72-triangle current LanternGlass GLB, SHA-256
`34a2522f2027d3fb04b77480cc929d36c5a19c6e0f33bf3fa0f0ae0959c99ec4`.
Double-precision intersections over those72 triangles select each captured
candidate as nearest, with no tied nearest hit under the documented tolerances.
Maximum raw-distance residual is4.07e-7; this corroborates the hardware hits but
does not emulate GPU/BVH rounding. A separate lead plane/Gram-system check of
the distinguishing entry and adjacent-face exit passes, with interior margins
0.12773 and0.05094. Calculation tolerances do not alter renderer image tolerance.

At interface index4 the existing shader checks its limit before processing that
candidate. The reason2 branch zeros the transmitted contribution and increments
the certified-recovery counter. In these observed paths it is therefore an
intentional finite-budget truncation, **not successful physical recovery**.
The one closed-volume path retains the honest failure counter. Mid-path throughput
is nonzero; final pixel contribution beyond this boundary has not been measured.
Neither suppressing the counters nor relabelling recovery establishes correctness.

## Remaining gate and recovery

Keep Phase4 open: bounded-transport policy and final radiance/image impact remain
unresolved. Do not raise budgets, remove diagnostic failures, change glass/materials
or geometry, use scalar glass, or restore known buggy pixels. The demonstrated
numerical fixes remain accepted; this result does not reopen them or player work.
Live-motion failures, physical-shadow/live acceptance, Shipping/Diagnostic parity
and backend pixel parity remain separate. No compute, RTX, S24 or S25 inference.

Normal DebugC11 `c11ff703…0daf0fe` is restored after the probes; the separate
Shipping A/B/B/A profile uses immutable benchmark APKs and is not this evidence.
Audio/haptic manual revalidation required: NO (unchanged semantic inputs/playback).

Recompute native records with `decode-current-paths.py <this-directory>`; it reads
the losslessly compressed captures too. `offline-validation/analyze_paths.py`
accepts this directory as its first argument and pins the existing runtime GLB.
On another checkout, repoint only its GLB path, retaining the exact SHA pin.
Reconstruction of temporary probes uses the retained manual patch/headers and
existing embedded-shader generator; the custom investigation catalog must not be
promoted into the production tree. The packaged SPIR-V is retained independently.
Initial readback-signature/stale-adapter build failures and green reruns are kept.
Three incomplete curation attempts remain external and recoverable; no original
evidence or user scratch was overwritten. `sha256-manifest.json` excludes itself
and binds the exact final archive, including lossless captures and actual modules.
