# Android RT Device Compatibility Record

Last updated: 2026-07-22

This is a living compatibility record for the Horde Lantern RT Android build. It separates direct project evidence from reports and hardware-based predictions. A device is not marked as working solely because its GPU advertises Vulkan or hardware ray tracing.

**Primary development platform:** Samsung Galaxy S26 Ultra (`SM-S948B`). This is the main Android development device and has locally tested success on the current project route.

## Evidence status

- **Locally tested confirmation** - the project was installed and exercised on the named device, with the relevant Vulkan/RT and presentation evidence recorded.
- **User-reported + screenshot evidence** - another person supplied a device result and a preserved screenshot. This is strong evidence for that device/configuration, but is not a local test by this project owner.
- **User-reported** - a report without a preserved diagnostic screenshot or report file.
- **Vendor/SoC inference** - the GPU family is advertised as supporting hardware ray tracing, but this project has not yet confirmed the Android driver extensions and scene presentation.
- **Unverified candidate** - a plausible device that still needs the project's capability probe.

The working gate is the project's presentable Vulkan RT path: acceleration structures, ray-query support, ray-tracing pipeline support, buffer device address, ASTC materials, device creation, and a real RT-produced frame reaching the Android swapchain. The Android bridge currently enables the RT device path only when `RtMode::RayTracingPipeline` is selected; a ray-query-only driver is therefore not enough. See [`RayTracingRequirements.cpp`](../src/vulkan/raytracing/RayTracingRequirements.cpp), [`android_probe_bridge.cpp`](../android/app/src/main/cpp/android_probe_bridge.cpp), and [`android/README.md`](../android/README.md).

## Confirmed evidence

### Samsung Galaxy S26 Ultra - `SM-S948B`

- **Status:** Locally tested confirmation - works; primary development platform.
- **Evidence type:** Direct local Android device testing: installation, strict ASTC selection, honest RT swapchain presentation, showcase traversal, lifecycle checks, and warm timing measurements.
- **Reported hardware:** Android 16, Qualcomm Adreno 840.
- **Performance evidence:** The project record reports the recommended 75% render tier passing the complete showcase route, with every required zone's median of three 120-frame windows below 13.7 ms at thermal status 3.
- **2026-07-22 foundation refresh:** A local Full gate reconfirmed strict ASTC, honest RT presentation, the 13-waypoint replay, Home/resume recovery, all 12 deterministic scene-only captures, and 75% checkpoint medians of 9.623 ms opening / 7.909 ms worst bend / 7.173 ms skylight / 8.691 ms green / 10.526 ms lich at thermal status 0. The separate report-only 100% opening result was 17.228 ms.
- **2026-07-22 Showcase Alpha 0.1.2 publication:** The exact stable-key-signed `0.1.2-alpha.1` APK (`versionCode 3`, SHA-256 `c9a26c79d4881230d2fd18b3bcbe4c1543032bccea760ade581f9a9fdcbf72b6`) installed locally, loaded all 17 SFX, selected strict environment/lich ASTC, honestly presented before and after Home/resume, and rejected Debug capture automation. A separate Debug checkpoint/replay/capture gate completed all 13 waypoints and 12 captures; 75% medians were 13.929 / 11.210 / 10.288 / 12.103 / 13.845 ms for opening / worst bend / skylight / green / lich at thermal status 0, with report-only 100% opening at 23.353 ms. Evidence type: direct local exact-release smoke plus automated Debug regression evidence.
- **Source:** [`ALPHA_ANDROID_REFRESH_VALIDATION_2026-07-16.md`](ALPHA_ANDROID_REFRESH_VALIDATION_2026-07-16.md), [`HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`](HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md), [`FOUNDATION_VALIDATION_2026-07-22.md`](FOUNDATION_VALIDATION_2026-07-22.md), [`SHOWCASE_ALPHA_0_1_2_RELEASE_VALIDATION_2026-07-22.md`](SHOWCASE_ALPHA_0_1_2_RELEASE_VALIDATION_2026-07-22.md).
- **Qualification:** This is the main development platform and strongest current compatibility reference. It confirms this exact model/driver configuration, not every phone using a similar GPU.

### Google Pixel 8 - Marc's device

- **Status:** Unsupported on the reported device.
- **Evidence type:** User-reported + screenshot evidence; **not locally tested** by this project owner.
- **Preserved evidence:** [`horde-pixel8-marc-2026-07-22.png`](validation/horde-pixel8-marc-2026-07-22.png).
- **Screenshot-reported hardware:** GPU `Mali-G715`; Vulkan API `1.4.343`; driver `54.3.0`.
- **Observed result:** `RT mode: Unsupported`; `RT scene presented: no`.
- **Missing required extensions:**
  - `VK_KHR_acceleration_structure`
  - `VK_KHR_ray_tracing_pipeline`
  - `VK_KHR_ray_query`
- **Present but insufficient:** `VK_KHR_buffer_device_address` and `VK_KHR_deferred_host_operations` were present, but the missing RT extensions prevent the project from starting its required path.
- **Interpretation:** Mark the Pixel 8 as currently unsupported for this project. The screenshot proves the reported device/configuration; it should not be rewritten as a successful Vulkan or software-fallback result.

## Research candidates

These entries are hardware/driver candidates, not confirmed project support. Every named model still needs the project's runtime probe, followed by a real scene/presentation and performance check.

### Qualcomm Adreno candidates

Qualcomm identifies hardware-accelerated ray tracing on Snapdragon 8 Gen 2 and higher platforms. The 8s Gen 3 and 8s Gen 4 product pages also explicitly list hardware ray tracing. These are the best untested candidates because the current working reference is an Adreno 840 device.

| SoC family | Representative Android devices | Evidence status |
|---|---|---|
| Snapdragon 8 Elite Gen 5 | Samsung Galaxy S26/S26+/S26 Ultra; OnePlus 15; other 2026 flagship models using this platform | Unverified candidate; highest-priority follow-up after `SM-S948B` |
| Snapdragon 8 Elite | Samsung Galaxy S25 series and S25 Edge; Galaxy Z Fold7; OnePlus 13; Xiaomi 15/15 Ultra; ASUS ROG Phone 9; RedMagic 10; Honor Magic7 Pro; iQOO 13; Motorola Razr 60 Ultra | Unverified candidate; strong hardware inference |
| Snapdragon 8 Gen 5 | OnePlus 15R and equivalent premium devices | Unverified candidate; strong hardware inference, exact Android driver still required |
| Snapdragon 8s Gen 4 | POCO F7 and other phones explicitly using Snapdragon 8s Gen 4 | Unverified candidate; strong hardware inference |
| Snapdragon 8s Gen 3 | POCO F6; Xiaomi 14 Civi; OnePlus Ace 3V; Motorola Razr+ variants | Unverified candidate; hardware RT advertised, driver exposure still required |
| Snapdragon 8 Gen 3 | Samsung Galaxy S24 Ultra; Snapdragon-region S24/S24+; OnePlus 12; Xiaomi 14/14 Ultra; ASUS ROG Phone 8; RedMagic 9; iQOO 12; Honor Magic6 Pro; Sony Xperia 1 VI; Motorola Edge 50 Ultra | Unverified candidate; strong hardware inference |
| Snapdragon 8 Gen 2 | Samsung Galaxy S23 family; OnePlus 11; ASUS ROG Phone 7; Xiaomi 13 family; RedMagic 8 Pro/8S Pro; iQOO 11; Vivo X90 Pro+; Honor Magic5 Pro; Motorola Edge 40 Pro; Sony Xperia 1 V | Unverified candidate; strong hardware inference |

### MediaTek Immortalis candidates

Arm states that Immortalis-G715, G720, and G925 support hardware ray tracing, while warning that early Android drivers may expose ray query without the full Vulkan ray-tracing pipeline. MediaTek explicitly advertises Vulkan ray-query support on Dimensity 9300 and Vulkan ray-tracing pipeline support on Dimensity 9500.

| SoC/GPU family | Representative Android devices | Evidence status |
|---|---|---|
| Dimensity 9500 / Arm G1-Ultra | OPPO Find X9/X9 Pro; Xiaomi 17T Pro; other Dimensity 9500 flagships | Unverified candidate; best MediaTek tier |
| Dimensity 9400 / Immortalis-G925 | OPPO Find X8/X8 Pro; Vivo X200 family; equivalent Dimensity 9400 phones | Unverified candidate; strong hardware inference |
| Dimensity 9300/9300+ / Immortalis-G720 | Vivo X100/X100 Pro; Xiaomi 14T Pro; OPPO Find X7; Samsung Galaxy Tab S10 series | Unverified candidate; ray-query support is explicitly advertised, full pipeline must be checked |
| Dimensity 9200/9200+ / Immortalis-G715 | Vivo X90/X90 Pro/X90S; Xiaomi 13T Pro; OPPO Find X6; iQOO Neo8 Pro | Unverified candidate; older driver exposure makes this lower priority |

### Samsung Xclipse candidates

Samsung confirms hardware ray tracing on Xclipse 920, 940, 950, and 960 generations. The remaining risk is whether the device's shipped Android driver exposes every extension required by this project.

| SoC/GPU family | Representative Android devices | Evidence status |
|---|---|---|
| Exynos 2600 / Xclipse 960 | Galaxy S26/S26+ Exynos-region variants | Unverified candidate |
| Exynos 2500 / Xclipse 950 | Galaxy Z Flip7; other devices using Exynos 2500 | Unverified candidate |
| Exynos 2400/2400e / Xclipse 940 | Galaxy S24/S24+ Exynos-region variants; Galaxy S24 FE | Unverified candidate |
| Exynos 2200 / Xclipse 920 | Galaxy S22/S22+/S22 Ultra Exynos-region variants | Unverified candidate; older driver risk |

## Tablets and handhelds

The following devices are plausible because they use one of the candidate SoCs:

- OnePlus Pad 2 - Snapdragon 8 Gen 3.
- Lenovo Legion Tab 2025 - Snapdragon 8 Gen 3.
- RedMagic Nova gaming tablet - Snapdragon 8 Gen 3.
- Xiaomi Pad 6S Pro - Snapdragon 8 Gen 2.
- Samsung Galaxy Tab S10/S10+/S10 Ultra - Dimensity 9300+.
- AYN Odin 2 - Snapdragon 8 Gen 2.

These are renderer candidates only. The Android activity is portrait-first (`sensorPortrait`), so tablet/handheld layout and controls need separate validation even if Vulkan RT works.

## Not currently considered likely

Do not treat the following as likely compatible without a successful real-device probe:

- Snapdragon 8 Gen 1 and 8+ Gen 1.
- Snapdragon 7-series and 7+ Gen 2/Gen 3.
- Google Tensor generations.
- Dimensity 9000/9000+, 8200, 8300, and ordinary Mali-G-series parts without Immortalis.
- Samsung Xclipse 530 / Exynos 1480.
- Older Adreno 600-series, PowerVR, or older Mali GPUs.

## Evidence update template

Add one block per new result rather than changing a prediction into an implied fact:

```text
### <Device and exact model code>

- Status: <works / unsupported / partial / pending>
- Evidence type: <locally tested confirmation / user-reported + screenshot evidence / user-reported / vendor/SoC inference>
- Tester/source: <name or source, if appropriate>
- Date: <YYYY-MM-DD>
- Hardware: <GPU, Vulkan API, driver version>
- RT result: <mode, required extensions/features, scene presented yes/no>
- Performance: <render scale, thermal state, frame-time/FPS evidence, or N/A>
- Attachments: <repo-relative report, screenshot, log, or validation bundle>
- Qualification: <what this proves and what it does not prove>
```

## Research sources

- [Android NDK stable APIs - Vulkan runtime capability guidance](https://developer.android.com/ndk/guides/stable_apis)
- [Qualcomm Snapdragon device finder](https://www.qualcomm.com/smartphones/device-finder)
- [Qualcomm Snapdragon 8 Gen 2](https://www.qualcomm.com/smartphones/products/8-series/snapdragon-8-gen-2-mobile-platform)
- [Qualcomm Snapdragon 8s Gen 3](https://www.qualcomm.com/smartphones/products/8-series/snapdragon-8s-gen-3-mobile-platform)
- [Qualcomm Snapdragon 8s Gen 4](https://www.qualcomm.com/smartphones/products/8-series/snapdragon-8s-gen-4-mobile-platform)
- [Qualcomm Snapdragon 8 Elite Gen 5](https://www.qualcomm.com/smartphones/products/8-series/snapdragon-8-elite-gen-5)
- [Arm Immortalis-G715 Vulkan ray-tracing overview](https://developer.arm.com/community/arm-community-blogs/b/mobile-graphics-and-gaming-blog/posts/arm-immortalis-g715-developer-overview)
- [MediaTek Dimensity 9300](https://www.mediatek.com/dimensity-9300)
- [MediaTek Dimensity 9400](https://www.mediatek.com/products/smartphones/mediatek-dimensity-9400)
- [MediaTek Dimensity 9500](https://www.mediatek.com/products/smartphones/mediatek-dimensity-9500)
- [Samsung Exynos 2200](https://semiconductor.samsung.com/processor/mobile-processor/exynos-2200/)
- [Samsung Exynos 2400](https://semiconductor.samsung.com/processor/mobile-processor/exynos-2400/)
- [Samsung Exynos 2500](https://semiconductor.samsung.com/processor/mobile-processor/exynos-2500/)
- [Samsung Exynos 2600](https://semiconductor.samsung.com/processor/mobile-processor/exynos-2600/)
