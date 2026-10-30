# CPU preparation evaluation - 20 October 2026

Player skinning is material on the fresh phone baseline: roughly 6 ms per frame,
with another 5 ms for Kit in the exterior views. CPU pose copying is about 0.7 ms.
The much larger existing fence wait offers potential overlap headroom, but the
current fresh simulation snapshot is created after that wait. **No CPU
optimization or execution-order change is adopted.**

The baseline is `cpu20-pipeline-a1`, starting from
`b2d548a22cc44a58c92c290018f5bf211f2a8d87` plus the measurement-only working patch.
It uses optimized Debug native code with production Shipping/Mobile shader banks,
real `RayTracingPipeline` presentation and strict ASTC on `SM-S948B`, Adreno 840,
API 37. APK SHA-256:
`01b414878a57f16a9001b1ff28394a8d2c7c5da4b5c867ecd63ebde94d7f9d5a`.
The actual baseline settings are 40%, 576 x 1188 internal / 1440 x 2971 output,
Mobile water/fire, Current shadows, Glass Off, Mist On and Dust Low. This run
precedes the separate mobile Mist Off default change.

Raw local receipts are the [build identity](../../../reports/2026-10-20-phone-performance/baseline-build.json),
[CPU analysis](../../../reports/2026-10-20-phone-performance/pipeline-a1/cpu-analysis.json),
[completed-frame resource CSV](../../../reports/2026-10-20-phone-performance/pipeline-a1/android-motion-cpu20-pipeline-a1-resources.csv),
[presentation report](../../../reports/2026-10-20-phone-performance/pipeline-a1/HordeLanternRT-benchmark-latest.json)
and [current-HAL temperatures](../../../reports/2026-10-20-phone-performance/pipeline-a1/thermals.json).
The strict analyzer accepts 704 completed frames and 703 actual display intervals;
all nine exported CPU stages join the exact completed publication and serializer
identity. Whole-route display intervals have median 75.000001 ms and p95
158.333281 ms; all 703 exceed 33.333 ms. These intervals are distinct from CPU
cycle and GPU command-buffer durations. Framework thermal status remains 0;
before-route AP/BAT/SKIN is 41.4/28.4/33.5 C and after-route is 39.1/31.4/35.1 C.
Status 0 alone does not establish fixed clocks or a sustained performance budget.

Held-view medians in milliseconds, calculated independently for each metric:

| View / frames | Skin | Player | Character | Pose copy | Upload | Fence wait | GPU | Whole CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Exterior landing / 39 | 11.136 | 5.922 | 5.201 | 0.694 | 0.450 | 137.828 | 137.713 | 154.154 |
| Lower / 62 | 6.608 | 6.608 | 0.000 | 0.775 | 0.364 | 54.392 | 54.008 | 63.739 |
| Passage / 52 | 6.399 | 6.399 | 0.000 | 0.780 | 0.362 | 66.076 | 65.934 | 75.190 |
| Repeat ascent / 40 | 10.281 | 5.181 | 5.177 | 0.684 | 0.431 | 137.276 | 136.965 | 153.332 |

Per-frame `Skin` remains exactly `PlayerSkin + CharacterSkin`; their independent
medians need not add. CharacterSkin includes Kit's inseparable clip skinning,
floor scan, bounds and static-PBR repack inside `SkinnedPbrActor::Pose` (header
lines 27-48), excluding upload. PoseCopy measures only player body/viewmodel CPU
static-PBR repacks, counts logical packed output bytes, and excludes upload.
DynamicUpload measures the actual coherent mapped memcpy or map/copy/unmap.
The October 13 normal cache is already present for the shared player palette
(`SkinnedMeshAsset.cpp:2094-2118`); Kit uses the generic per-influence normal path
at lines 1480-1508. Historical October 1/13 timings are not this baseline.

The current ownership boundary prevents a mechanical relocation. Android waits
the sole fence at `android_probe_bridge.cpp:3817`, completes its owning GPU and
Diagnostic evidence at 3840, acquires an image, then samples/advances ordinary
simulation at 4337-4351 and builds value `RtSceneFrameInputs` at 4428. Windows
follows the same order at `DiagnosticWindow.cpp:6374`, 6391, 6490 and 6516.
Moving input sampling earlier changes freshness and the elapsed-time boundary;
reusing the previous frame's pose instead would add stale geometry/equipment.
Fence wait and GPU durations must not be added as independent work, and available
wait duration is not a measured speedup from an unimplemented overlap candidate.

The smallest next slice is an ownership refactor retaining the current order:

- Prepare one owned, immutable CPU bundle from the current value frame on the
  existing simulation/render thread. Retain its tick, exact animation/request,
  input publication where available, scene/surface/static-geometry generations
  and active character lane. Tick alone is insufficient: same-tick checkpoint or
  animation changes can require a fresh pose.
- Keep the solved pose/normal palette, body/viewmodel streams, sockets, final
  grips and held-item visual transforms consistent in that bundle. Existing
  `SolvedPose()` and stream references are borrowed until the next update
  (`PlayerRenderSlot.h:245-259`); const mesh operations also mutate scratch and
  rig-binding caches (`SkinnedMeshAsset.h:258-261`). Do not add worker access.
- Consume the bundle once only after the successful sole fence and generation
  validation. Mapped vertex/instance writes, Diagnostic reset/read, BLAS/TLAS
  recording, descriptor/resource retirement and submission stay with their
  existing owner. CPU-prepared, GPU-uploaded and successfully submitted states
  must remain distinct.
- Preserve pending refits across abandoned recording/failed submission, then
  acknowledge only successful queue submission. The existing character contract
  is `CharacterRenderSlot.h:114-116`; player cadence reuse must not substitute
  for upload/refit commit state. Completion evidence must retain the exact
  submitted tick/serial, including final owning drain and tokenless ownership.

Before considering overlap, test byte-identical body/viewmodel positions,
normals/tangents, UV/material fields and sockets across idle/walk/crouch/rope/stow;
same-tick changed requests and invalid-pose recovery; stale generation/lane
rejection; abandoned prepare/upload/record/submit retries; restored storage at
unchanged clip/time; and zero GPU writes before fence success. Verify successful
commit remains consume-once and failed attempts publish no current-frame evidence.
Only after this slice passes should one same-thread overlap candidate be evaluated
against fresh matched phone display/CPU/GPU evidence and input/equipment freshness.

The instrumentation's focused Release checks passed 7/9: performance evidence,
benchmark report, frame coordinator, upload observation, node-pose cache, arm
reference and skinned-character fixtures. Two pre-existing mismatches remain
visible in the preserved [eight-test CTest log](validation/cpu-preparation-release-8-ctest.log)
and [failure index](validation/cpu-preparation-release-8-failed.log):

- `horde_rt_scene_resource_inventory_tests`: checks at
  `RtSceneResourceInventoryTests.cpp:439`, 979, 1599 and 1641 reject the current
  30-material / 31-32-primitive roster and chapter indices 1,291,902 versus frozen
  1,280,514. Initialization/import inputs and these assertions were unchanged;
  the new scopes affect runtime dynamic updates only.
- `horde_rt_character_render_slot_smoke`: six normal-axis source checks at
  `CharacterRenderSlotSmoke.cpp:957-967`, one water-distance source check at
  1339-1347 and the Android tuning assignment source check at 1792-1796 fail.
  Their shaders/tests/bridge are unchanged from HEAD; the bridge already uses
  the environment-survey conditional instead of the expected one-line assignment.

The [source-inspection receipt](validation/cpu-preparation-source-proof.txt)
records matching HEAD/working blobs for those inputs, unchanged tracked shader
and asset trees, and the complete instrumentation-only scene diff. This is
inspection-based attribution, not a clean baseline test run or a pass. No old
assertion was weakened and no broad water/resource fix is part of this slice.
