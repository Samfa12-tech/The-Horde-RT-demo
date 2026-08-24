# Windows RT Device Compatibility Record

Last updated: 2026-08-25

This is the living compatibility record for the Horde Lantern RT Windows build. A GPU is marked as working only when the project selects `RayTracingPipeline`, creates the required Vulkan RT path, and presents RT-produced frames through the swapchain. DirectX feature level, vendor marketing, or merely launching the executable are not sufficient on their own.

The Android record remains in [`ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md).

## Evidence status

- **Locally tested confirmation** - the project owner ran the named build on the exact machine and retained capability and/or benchmark evidence.
- **User-reported + report evidence** - another tester supplied the generated capability or benchmark report.
- **User-reported** - a result without a preserved generated report.
- **Vendor/GPU-family inference** - the hardware should support RT, but this project has not confirmed the Vulkan driver path.
- **Unverified candidate** - plausible hardware still requiring the project's capability probe and a successfully presented RT frame.

## Compatibility summary

| Device / GPU | Status | Build and render tier | Key result |
|---|---|---|---|
| NVIDIA GeForce RTX 5050 Laptop GPU | Works - locally tested | `1.5.2`, 100%, `1232x803` | Exact published ZIP; `RayTracingPipeline`; honest swapchain presentation |
| HP EliteBook 845 14-inch G11 / AMD Radeon 780M integrated graphics | Works - locally tested | `0.1.3-alpha.1`, 100%, `1484x991` | `RayTracingPipeline`; 1,838 measured frames; 10.222 ms median / 97.828 FPS; 59.650 FPS 1% low |

## Confirmed evidence

### HP EliteBook 845 14-inch G11 - AMD Radeon 780M integrated graphics

- **Status:** Locally tested confirmation - works, including the complete benchmark route.
- **Tester/source:** Project owner.
- **Date:** 2026-08-03.
- **Build:** `0.1.3-alpha.1`; embedded shader `59bf3ce8503c`.
- **System:** HP EliteBook 845 14-inch G11 Notebook PC; Windows 11 Enterprise 64-bit; AMD Ryzen 7 8840U; 16 GB RAM.
- **GPU:** `AMD Radeon(TM) Graphics`, Radeon 780M integrated GPU; vendor `0x1002`, device `0x1900`.
- **Memory reported by DxDiag:** 417 MB dedicated graphics memory plus 7,822 MB shared memory.
- **Drivers:** Vulkan report `2.0.302`; Windows display driver `32.0.11020.10002` dated 2024-09-27; WDDM 3.2.
- **Vulkan:** API `1.3.280`.
- **RT result:** `RayTracingPipeline`; `VK_KHR_acceleration_structure`, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_ray_query`, `VK_KHR_buffer_device_address`, and `VK_KHR_deferred_host_operations` all present; corresponding acceleration-structure, pipeline, ray-query and buffer-device-address features enabled.
- **Presentation integrity:** RT scene presented through the swapchain, and RT was presented on every measured benchmark frame.
- **Benchmark configuration:** 100% render scale; internal and presentation extent `1484x991`; FIFO present mode; 2/2 laps; 26/26 waypoints; 1,838 measured frames.
- **Overall performance:** 10.251 ms average; 10.222 ms median (`97.828 FPS` from median); 13.299 ms P95; `59.650 FPS` 1% low.
- **Slowest recorded zone by average:** `transmission-threshold`, 12.392 ms average and 15.110 ms P95.
- **Evidence:** [`validation/horde-elitebook-845-g11-radeon-780m-benchmark-2026-08-03.json`](validation/horde-elitebook-845-g11-radeon-780m-benchmark-2026-08-03.json) and [`validation/horde-elitebook-845-g11-radeon-780m-hardware-2026-08-03.md`](validation/horde-elitebook-845-g11-radeon-780m-hardware-2026-08-03.md).
- **Qualification:** This confirms the exact HP EliteBook, driver and OS configuration. It is strong evidence that the Windows demo can run very well on at least one modern RDNA 3 integrated GPU, but it does not certify every Radeon 780M laptop or OEM driver package.

This is a notably stronger result than a launch-only test: the machine completed the same deterministic two-lap, 26-waypoint hardware-RT benchmark used on the dedicated RTX target, at full render scale and with successful RT presentation on every measured frame.

### NVIDIA GeForce RTX 5050 Laptop GPU

- **Status:** Locally tested confirmation - works; original Windows validation target.
- **Date:** 2026-07-17 benchmark validation.
- **Build:** `0.1.1-alpha.1`; embedded shader `2efd80c12f09`.
- **Vulkan / RT:** Vulkan `1.4.341`; `RayTracingPipeline`; MAILBOX present mode.
- **Benchmark configuration:** 100% render scale; internal and presentation extent `1232x803`; 2/2 laps; 26/26 waypoints; 1,838 measured frames; honest RT presentation on every measured frame.
- **Overall performance:** 7.827 ms average; 6.216 ms median; 16.508 ms P95; 20.896 FPS 1% low.
- **Evidence:** [`IN_APP_BENCHMARK_WINDOWS_VALIDATION_2026-07-17.md`](IN_APP_BENCHMARK_WINDOWS_VALIDATION_2026-07-17.md).
- **Qualification:** This remains the dedicated-GPU reference. Its older benchmark build and different presentation mode/extent mean the figures should not be treated as a controlled GPU ranking against the Radeon 780M result.

#### Showcase Alpha 0.1.5 exact packaged smoke - 2026-08-23

- **Build/artifact:** `0.1.5-alpha.1`; exact published Windows ZIP SHA-256 `631b9f01a4d348e18733c989ebacc9c32ce9005ac9498e49e8757a0a36411166`; itch build `#1908330`.
- **Package isolation:** Extracted to `reports/release-smoke/windows-0.1.5-20260823-203142/` and launched using only packaged files.
- **Identity:** Product version `0.1.5-alpha.1`.
- **RT result:** NVIDIA GeForce RTX 5050 Laptop GPU; internal extent `1232x803`; `RayTracingPipeline`; `RT scene status: Presented via swapchain`; `RT scene presented: yes`.
- **Lifecycle:** The main window closed normally with exit code 0.
- **Qualification:** This confirms the exact published package, asset layout, Vulkan RT selection, and honest presentation on the dedicated Windows reference. It is not a fresh sustained benchmark or proof for another GPU/driver.

#### Showcase Alpha 1.5.2 exact packaged smoke - 2026-08-25

- **Build/artifact:** `1.5.2`; exact published Windows ZIP SHA-256 `fd929f1972c4587c6720013eb0586934ab72924c5f8f9c50ec8576a23a57690d`; itch build `#1913191`.
- **Package isolation:** Extracted to `reports/release-smoke/windows-1.5.2-20260825-074305/` and launched using only packaged files.
- **Identity:** File and product version `1.5.2`.
- **RT result:** NVIDIA GeForce RTX 5050 Laptop GPU; `RayTracingPipeline`; `RT scene presented: yes` after successful swapchain presentation.
- **Lifecycle:** The hidden validation window received `WM_CLOSE` and exited normally with code 0; no game process was left running.
- **Qualification:** This confirms the exact published package, asset layout, Vulkan RT selection, and honest presentation. It is not a fresh sustained benchmark, a new artistic review, or proof for another GPU/driver.

## Interpretation rules

1. `COMPLETE` is benchmark-integrity evidence, not a promised minimum frame rate.
2. Compare GPUs cautiously when build, driver, window extent, present mode, display topology or thermal/power conditions differ.
3. Record the exact laptop or desktop model where possible; mobile GPUs and integrated GPUs can vary materially with power limits, memory configuration and OEM drivers.
4. A Windows device is compatible only after the Vulkan capability report selects the required RT path and an RT-produced frame reaches successful presentation.
5. Do not infer support from DirectX 12 or D3D feature level alone; this project uses native Vulkan RT.

## Evidence update template

```text
### <Exact system model> - <GPU>

- Status: <works / unsupported / partial / pending>
- Evidence type: <locally tested confirmation / user-reported + report evidence / user-reported / vendor/GPU-family inference>
- Tester/source: <name or source, if appropriate>
- Date: <YYYY-MM-DD>
- Build: <version and shader identity>
- System: <OS, CPU, RAM>
- GPU/driver: <GPU, vendor/device IDs, Vulkan and OS driver versions>
- RT result: <mode, required extensions/features, scene presented yes/no>
- Performance: <render scale, extent, present mode, frames, average/median/P95/1% low>
- Attachments: <repo-relative generated report or validation record>
- Qualification: <what this proves and what it does not prove>
```
