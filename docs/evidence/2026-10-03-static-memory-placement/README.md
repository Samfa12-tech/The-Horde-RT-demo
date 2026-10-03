# F08 — immutable placement, without a new transfer scheduler

Parent36f99c2. One bounded candidate changes only memory selection for immutable
world/static vertices, indices, transforms, surface codes and primitive metadata.
Mutable material/instance metadata and independently skinned world/viewmodel/
enemy geometry keep the preceding dynamic ownership route. Materials are not
immutable in the current source: per-frame tuning/emissive values are written.

CreateBuffer accepts a placement preference, selects a compatible coherent
host-visible DEVICE_LOCAL type when available and keeps the actual selected flags.
If no such compatible type exists, it retains required coherent host placement;
allocation/binding failure never retries or disguises OOM. Immutable data is
written once and immediately unmapped. A host-visible flag does not mean the
storage is outside device-local memory. No transfer command, extra persistent
allocation, per-frame traffic, shader or frame concurrency is introduced.

This adapts the audit's staging recommendation where direct host upload into
device-local storage is supported, avoiding redundant staging there. Generic
staging for devices lacking a compatible local/coherent type remains future
platform work, not implemented or claimed device-local. No performance benefit
is inferred from flags. Per-buffer heap/type export is not added to the telemetry
framework; the existing inventory still records actual allocation sizes/flags.

## Evidence

- Fresh Debug/Release Windows game and selected4/4 resource tests PASS,12.36s/
  5.78s. Tests preserve dynamic default selection, prefer the compatible local
  type, reject incompatible memory bits, retain honest host-only placement and
  fail preferred-heap OOM without an allocation retry. Staged-primary doubles
  retain their original behavior and ownership tests.
- Native RTX5050 Debugec63690f3d98cdedd158b7433eeb6e185be7e7b9e043fed8deb05d4d1822584f
  completes13 Pipeline/High960x540 captures, exit0. All13 PNG SHA256s are identical
  to the preceding dynamic-mapping candidate. No pixel tolerance is changed.
  C:/Dev/tmp/horde-static-placement-captures-20261003 retains the full result.
- Actual SDK vulkaninfo on this RTX exposes type2/heap1 host/coherent0x6 and
  type4/heap0 local/host/coherent0x7. The candidate's live initialization succeeds.
  Available heap evidence is not a per-buffer memory-type report or timing claim.
  C:/Dev/tmp/horde-static-placement-vulkaninfo.txt retains the query.
- Fresh four-ABI benchmark build PASS39s,15 executed/41 up-to-date; immutable
  Shipping/Mobile development APK SHA256
  5f05a13102a6bf2205cb9af38124139469dd7605a40e3e99217570d835b7c70e,
  retained atC:/Dev/tmp/horde-final-device-artifacts-20261002/resources-5f05a131-shipping-mobile-benchmark.apk.
  Actual ARM64 APK/library SHA256b836454af1d4a0d51b7958d816b14d31073a08d83ecb45505188ccdef344cd71
  matches; exact asset admission and development-signature checks pass. Four
  unchanged Shipping modules pass spirv-val/disassembly with zero diagnostic
  atomics/no binding22. C:/Dev/tmp/horde-resource-final-*20261003.* retains logs.

Next: coherent reviewed push/current CI, then the finite S26 candidate/control
matrix. The owner has now released S26 from Briarhold; no renewed coordination
is required. S24/S25 remain separate device boundaries. No FPS claim or phone
acceptance from these host results. Audio/haptic manual revalidation:NO; source
inputs/playback unchanged. No merge, release signing or publication.
