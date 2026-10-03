# A1 presentation retirement: accepted compatibility boundary

3 October 2026. The owner explicitly chose to retain compatibility with admitted
drivers without swapchain-maintenance1 and to keep the teardown-proof limitation
open. This is a bounded owner deferral, not completion of A1's all-path requirement.
Neither maintenance1 nor present-wait becomes a mandatory hardware requirement.

Per-swapchain-image presentation semaphores correct steady-state reuse on both
platforms. Optional KHR/EXT swapchain-maintenance1 is queried and enabled through
matching instance/device dependencies and the actual feature. Enabled drivers use
per-image presentation fences, bounded waits and retained ownership after an
unsuccessful completion wait. Unsupported drivers retain the existing conventional
device-idle retirement path, which lacks a universal presentation-engine completion
proof for recreation, surface loss/replacement and final shutdown.

The [Khronos semaphore guidance](https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/swapchain_semaphore_reuse.adoc)
explicitly identifies this unextended shutdown gap. Its same-image reacquisition
proof applies to ordinary reuse. The [swapchain recreation sample](https://docs.vulkan.org/samples/latest/samples/api/swapchain_recreation/README.html)
also describes deferred predecessor retirement after a successor presentation and
reacquisition on the same surface. That cannot guarantee final shutdown, a lost
surface or Android native-surface replacement. Bounded successor retention is a
possible separate improvement for ordinary recreation, not a full-contract fix.
Present-wait is optional and cannot wait a retired swapchain; OUT_OF_DATE also
prevents it from supplying a universal fallback. Idle waits, sleeps and empty
graphics submissions do not add the missing synchronization scope. Retaining
unresolved Vulkan children and then destroying their device/surface is not legal
cleanup. No such purported proof or mandatory extension was introduced.

Current exact evidence:

- RTX5050 Laptop GPU actually enabled KHR maintenance fences in the candidate's
  genuine RT capture/resize runs. [Exact host evidence](ENGINEERING_1_6_2_RTX_VALIDATION.md)
  records successful runs and zero synchronization messages. This certifies those
  observed runs; it cannot establish universal unextended retirement correctness.
- The older SM-S928B S24 evidence is genuine hardware RayQueryCompute/OpaqueFast
  RT presentation and TLAS-refresh/Home-resume evidence. Its saved finite receipts
  do not record maintenance1/present-wait feature enablement. Actual completion-mode
  support is therefore unknown from those receipts. The S24 TLAS fix is a separate
  instance-definition correctness issue. [Exact device record](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md)
  and [finite receipts](evidence/2026-10-02-tlas-instance-refresh/README.md).
- S25 remains unverified. Model, SoC or another device's extension evidence cannot
  certify its RT backend or presentation-completion features. No new device probe
  was performed by this source reviewer. Current phone allocation/testing remains
  lead-owned and cannot substitute for S24/S25 evidence.
- Lead rechecked actual SM-S948B candidate APK
  `b4e063afed5ea46be1467870a0d96adbb1a7e731e6bda8f978c2d4401f45e8ee`
  hardware-Compute run185943: KHR extension advertised=no, EXT advertised=yes,
  swapchainMaintenance1 feature queried=yes, EXT presentation fences actually
  enabled. Sealed summary SHA256
  `6da4d397694de3b0e7622dd2d17737e52f52f668ba4e5bb7ad511d98807c29dd`.
  This is exact S26 feature/enablement evidence; it supplies no S24/S25 claim.

Keep A1 partially complete and disclose this compatibility boundary in candidate
closeout. Future exact-device evidence must record advertised extension, feature,
actually enabled retirement mode and observed lifecycle results separately. Such
tests improve compatibility evidence but do not remove the specification proof gap.
