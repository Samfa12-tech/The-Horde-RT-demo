# 1.6.1 hardware RayQuery backend integration

Development evidence, not a released build or Galaxy-family certification.
The complete audit, music, reporting and final-candidate matrix remain open.

## Architecture and scope

The pipeline backend remains preferred when its real requirements are met.
RayQuery-only devices select `RayQueryCompute`: fixed 8x8 GLCompute invocation
groups execute the same per-pixel GLSL using `rayQueryEXT` against the same real
BLAS/TLAS, materials, lights, glass/water transport and output image. Presentation
uses the existing swapchain copy. There is no software traversal, raster lighting,
screen-space substitute, reduced render scale or alternate physical shading.

The shared scene and pipeline bundle own both strategies for the selected backend.
Compute uses compute-stage descriptors, push constants and barriers, two compute
pipelines, and no miss/hit modules or SBT. Pipeline-only entrypoints are neither
loaded nor required by compute. One frame in flight and existing completion-owned
resource/evidence rules are unchanged.

Selection requires Vulkan 1.2, AS/RQ/BDA features, AS/RQ/deferred-host-operations
extensions, and a verified graphics+compute+present queue on the probed GPU/driver/API
tuple. Promoted BDA/SPIR-V/float-controls extension names are not required on 1.2;
raw advertised extension facts stay separate. A different presentable GPU no longer
silently inherits the probed device's capabilities. The tuple is not a device UUID.

Raw `rtMode`, selected `executionBackend`, and `rtScene.presented` are separate facts.
Selection is persisted before logical-device/pipeline creation, with presentation
false. Only a successful RT-produced swapchain presentation sets it true.

## Targeted implementation evidence

- Shader extraction `0418132` preserved all eight existing raygen artifact hashes.
- Backend identity/preflight `4c25b87`; eight frozen compute variants `aceba50`;
  dual compiled providers and final-binary containment `9b2fddb`;
  device requirements/core-1.2 policy `1391024`.
- Native scene/bundle/platform integration accepted at `99b8565` after lead review,
  independent source review and the targeted evidence below. Captures retain their
  original pre-commit provenance rather than being relabelled with the later SHA.
- Fresh affected MSVC Debug tests: 7/7, 11.03 s. Release: 7/7, 4.40 s.
  These cover requirements, bundle contracts/lifetime, scene preflight/inventory,
  canonical evidence and benchmark serialization. Subsequent expanded missing-feature
  selector coverage passed Debug 1/1. Selected-before-present regression observed
  RED then passed Debug 1/1 and Release 1/1 (0.89 s). Both final Windows configurations
  linked after that diagnostics fix; Android final build evidence is below.
- Independent review caught and corrected selection publication being delayed until
  first presentation, and Android selection occurring before actual queue verification.
  Final bounded re-review reported no remaining Critical/Important/Minor source findings.
- No current local implementation commit has fresh remote CI evidence yet.

## Shader evidence

The final Shipping/High Windows binary scanned at SHA-256
`eab95f919b31f2480be1a61c6138c3a2b9d9bf2b7f6c0ab38e35723eb6cf6397`
contained exactly two RayGenerationKHR and two GLCompute modules. All four passed
`spirv-val`/`spirv-dis`, contained real ray queries and had zero diagnostic atomics
and no binding 22. This is final-binary containment, not Shipping runtime performance.

Compute Shipping opaque: 489,428 bytes, 1 function, 0 calls, 23 query sites.
Compute Shipping generic: 217,180 bytes, 59 functions, 192 calls, 3 query sites.
Diagnostic variants intentionally retain binding 22 and 5/32 diagnostic atomics.
The scanner's duplicate, missing, forbidden-policy, unknown-module and wrong-stage
adversarial controls passed. Mobile/High opaque aliases remain legitimate same-stage
byte aliases; semantic policy identities stay distinct.

## Bounded Windows comparison

Retained local evidence: `reports/rayquery-native-paired-20260913/`.
Exact EXE SHA-256:
`b25249e800cc4f238001b2d61fabedbd2cb5795a702fecc69a9900951efaa99d`.
This is `9b2fddb` plus the recorded native WIP, before review-only diagnostic fixes;
do not label it as a clean later commit.

Both default pipeline and forced compute `lantern-held-high` captures exited 0,
honestly presented, and persisted the correct execution backend. RTX 5050 Laptop,
Diagnostic/High, 960x540, render scale 100%, twelve settling frames. No matched
performance conclusion is drawn from these short initialization/capture runs.

- Pipeline PNG: `f05c711feccfb4ece547fd842d809df2a7d3a96db10294cc6414bec1955fa222`.
- Compute PNG: `c74c4876319840c307cd4923ebe3e009b8c617e273690219285c519cdab4c6d9`.
- Decoded RGB differences: 240/518,400 pixels; 67 exceed one channel value;
  maximum difference 80 at (405,135). Each backend reproduces its own prior hash.
  Differences include isolated distant/window and wall-highlight pixels and glass
  edges. These are not byte-identical captures and do not pass the old maximum-three
  channel tolerance. Numerical/semantic investigation remains explicit; do not claim
  full visual parity from the small differing fraction.
- Read-only disassembly reconciliation found all 58 helper functions and 323
  post-aspect main instructions identical after ID normalization. The prologue
  differences are the launcher inputs, image-size query, signed/unsigned exact
  integer-to-float extent conversion and compute bounds guard. All diagnostic
  counters match. Stage-specific floating-point code generation or traversal edge
  ties are plausible, not proven causes. A future Diagnostic-only selected-pixel
  ray/hit/color probe can distinguish them; this is an open precision investigation,
  not a reason to change physical transport or weaken the old comparison threshold.
- Image inspection confirms the scene, held lantern/sword and current procedural
  arms are rendered. This does not accept the pending modelled-viewmodel or glass gates.

## Android and owner evidence boundary

The authorised connected device is `SM-S948B` / Android 16, not S24/S25. Android
Debug exposes `--ez horde_require_rayquery_compute true` as startup configuration;
omitted/false retains normal preference. Release ignores the private forced flag.
Surface recreation reuses the startup configuration; it does not switch a live frame.

Focused local phone run: `reports/rayquery-phone-smoke-20260913/`. APK
`1f64d3328cd9a17c4c39690523b3572201e846f67b04d9b1e724669debd8bbfa`
(87,971,004 bytes), `com.samfa12.hordelanternrt.debug`, 1.6.1-debug/code 9,
built from `1391024` plus the reviewed native integration. Local retained APK and
installed/pulled-back APK matched exactly. `assembleDebug` passed all four ABIs;
the scanned ARM64 library matched the actual APK entry at
`5092c941491e534526e50645f6d3ae552f3784a93c786038ca73c708b257ae8c`.
Diagnostic/Mobile containment found exactly four expected backend modules and
passed external validation/disassembly. The Debug intent policy's Robolectric
class passed; Release rejects the private forced flag.

Device: raw `SM-S948B`, Android 16, Adreno 840, Vulkan 1.4.295,
driver 512.842.19. Forced compute selected the exact Mobile compute pair, loaded
strict ASTC, and honestly presented twelve stable `lantern-held-high` frames.
Home/resume recreated the scene and again produced twelve stable compute frames.
A fresh process without the forced flag then preferred RayTracingPipeline and
passed the same checkpoint/ASTC/presentation check. All reported count/mask fields
matched between the two paths. The phone was returned Home after the smoke.

The phone was in portrait: swapchain 1440x2980, RT image 1080x2235 at explicitly
requested 75%, with a 1440x3120 device screenshot. Do not compare this workload
with the Windows landscape capture. Compute screenshot SHA-256
`ce254413c93fc9389bcf11fc48837b6cb50ba3009ff7deed2ea796bd23ba921b`
was inspected: scene, sword and lantern rendered, with the existing procedural
arms still present. Initial battery temperature 22.7 C, Android thermal status 0,
GPU thermal power level 0. No sustained or matched performance conclusion is made.
The initial capability report honestly retained pending GPU timing; scoped runtime
logs later contained valid GPU samples with zero reported query errors. These are
not a canonical full-route performance report.

Existing glass limitations remain: both phone paths reported five primary-volume
budget/production-pane failures at this checkpoint. This backend smoke does not
close the audit's physical-glass, performance or final image acceptance gates.

Exact S24/S25 model codes, candidate APK hashes, startup/ASTC/presentation reports
and lifecycle checks remain required on those devices. Their earlier screenshots
prove a pipeline-extension mismatch, not a downstream compute success or failure.
The full cross-device route/performance matrix waits for the complete feature set.

Audio/haptic manual revalidation required: **NO**. Backend selection, shader launch,
resource ownership and diagnostics do not change semantic audio/haptic inputs.
