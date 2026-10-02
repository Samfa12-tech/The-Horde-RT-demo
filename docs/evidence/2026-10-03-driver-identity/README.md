# Audit F14 — truthful driver identity

Parentc8cb355 plus this reviewed metadata change; intervening631f4d4 only records
the owner's glass deferral. No shader, material, quality, simulation, asset,
device-selection or playback change. Existing raw numeric `driverVersion` stays
unchanged; only its incorrectly Vulkan-decoded text is replaced.

## Contract and source

`driverVersion` is implementation-defined, not necessarily encoded like the
Vulkan API version. The report preserves its unsigned decimal value explicitly
as raw rather than maintaining guessed vendor bit decoders. A guarded
`VkPhysicalDeviceDriverProperties` query supplies the implementation's own
driver ID/name/info and four-component conformance version. Query only when the
physical device supports Vulkan1.2 or advertises `VK_KHR_driver_properties`,
and the properties2 entry point is available. Otherwise metadata is unavailable
(JSON nulls/text N/A), not fabricated zero-valued evidence. String reads are
bounded to the Vulkan arrays; JSON uses the existing control-character escaping.

References: [driverVersion encoding](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceProperties.html),
[driver properties](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceDriverProperties.html),
[core1.2 promotion](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_driver_properties.html).
The appended fields do not change the existing five-field raw device-selection
identity or CPU/GLSL ABI. The Vulkan-enabled CPU-host CI lane now explicitly
builds/runs the new behavioral report fixture; it does not query hardware in CI.

## Fresh evidence, completed once

| Check | Result |
| --- | --- |
| MSVC Debug/Release capability probe and actual game targets | Both builds PASS; [Debug](debug-build.log), [Release](release-build.log). Normal incremental reuse, not clean full builds. |
| Driver report, requirements, overlay and benchmark report contracts | Debug4/4 PASS12.39s; Release4/4 PASS4.44s. [Debug](debug-tests.log), [Release](release-tests.log). |
| Actual RTX5050 Laptop query, both configurations | Exit0, raw2559295488 preserved; ID4/nameNVIDIA/info610.47/conformance1.4.3.3; API1.4.341 separately. [Fresh Release JSON](rtx-capability.json), [text](rtx-capability.txt). This capability-only probe honestly reports presented=false. |
| Android Shipping/Mobile benchmark | Four-ABI configure/build PASS25s,15 executed/41 up-to-date. [Log](android-benchmark-build.log). No production signing variables read. |
| Exact APK player/held-prop/attribution admission | PASS; [receipt](android-package-admission.log). Accepted paired geometry is unchanged. |
| Actual stripped ARM64 Shipping modules | Both backend pairs,4 modules, all0 diagnostic atomics/no binding22, hardware ray-query capability, actual spirv-val/disassembly PASS. [Receipt](android-shipping-containment.json). Shader hashes match the previous candidate. |
| Package identity/signature | Separate benchmark package,code9/1.6.1-benchmark,SDK24/target34,all4ABIs,no debuggable marker; v2 development signature/one signer PASS. [Receipt](development-signature.txt). UNPUBLISHABLE, not production signing recovery. |

Fixture covers maximum raw/unknown-vendor and NVIDIA values, API independence,
unavailable stale metadata, present zero/empty values, JSON escaping and raw
physical-device identity matching. Checks remain active in Release.

## Immutable candidate and remaining gates

- New exact APK: `C:/Dev/tmp/horde-final-device-artifacts-20261002/f14-5c555528-shipping-mobile-benchmark.apk`.
- SHA256: `5c55552866d359dd158ef26821a9c25a533c7048b7e1884ffe38c793aa68dcd3`.
- Actual packaged ARM64 bytes match the validated stripped library:
  `ad24205c3e265602bcf973e072bfb8504211256ec2aea15a1624cb6ece2c6040`.
- Older4d8677b7/b9d69ff4 artifacts remain preserved and are not relabelled as this
  source. Rebuild was justified by changed shared Vulkan metadata inputs.

No phone install/query/playback/performance run occurred. Android driver metadata
remains exact-device-unverified. Reuse the existing finite device matrix with
this APK when device time is available; do not repeat completed renderer A/B or
Windows workload rows solely for diagnostic metadata. Inspect actual metadata
as part of that run, without assuming RTX formatting describes Qualcomm.

F14 implementation/host/native-RTX-query gate is resolved; current pushed CI
must still confirm the amended focused lane. Remaining audit work includes the
missing sanitizer lane and justified static/dynamic resource disposition, not
another whole-repository audit. Audio/haptic manual revalidation:NO; metadata
only. No merge, signing recovery, release or publication.
