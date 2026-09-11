# Samsung Galaxy S24 Ultra RayQuery capability evidence — 2026-09-11

## Result

A Samsung Galaxy S24 Ultra was tested with the current Horde Lantern RT Android build and reached the Vulkan capability diagnostics, but the playable RT scene was not attempted or presented because this device/driver exposes Vulkan ray query without `VK_KHR_ray_tracing_pipeline`.

This is the same important capability boundary already observed on the tested Galaxy S25 Ultra, and strengthens the case for the planned `RayQueryCompute` hardware-RT backend described in `FUTURE_WORK.md`.

## Screenshot-reported capability data

- Device: Samsung Galaxy S24 Ultra
- Exact Samsung model code: not captured in the supplied screenshot
- GPU: `Adreno (TM) 750`
- Vendor ID: `20803`
- Device ID: `1124406273`
- Driver version: `512.762.41`
- Driver version packed: `2150604841`
- Vulkan API version: `1.3.128`
- Backend: `Vulkan`
- Selected RT mode: `RayQuery`
- `VK_KHR_acceleration_structure`: yes
- `VK_KHR_ray_tracing_pipeline`: **no**
- `VK_KHR_ray_query`: yes
- `VK_KHR_buffer_device_address`: yes
- `VK_KHR_deferred_host_operations`: yes
- Internal render resolution: `N/A`
- Diagnostic-screen timing: `241.35 fps / 4.14 ms`
- GPU RT command-buffer time: `N/A` because GPU frame timing was not initialised
- RT scene status: `Not attempted`
- RT scene dispatch resolution: `N/A`
- RT scene presented: `no`
- Scene geometry label: `Complete Horde showcase route with sequential animated skeleton and staff-lit lich`

## Interpretation

This result does **not** show that the Galaxy S24 Ultra or Adreno 750 lacks hardware ray tracing. The device exposes Vulkan acceleration structures and `VK_KHR_ray_query`, which is a genuine Vulkan hardware ray-traversal path. The blocker for the current Horde renderer is specifically the absence of `VK_KHR_ray_tracing_pipeline`, because the current presentable frame-launch route still depends on the full ray-tracing pipeline and `vkCmdTraceRaysKHR`.

The planned compatibility programme should therefore treat this device as a strong secondary acceptance target for a real Vulkan `RayQueryCompute` backend that uses the existing BLAS/TLAS and ray-query-based Horde shading while preserving the current full `RayTracingPipeline` backend on devices such as the Galaxy S26 Ultra and Windows RTX hardware.

The `241.35 fps / 4.14 ms` value is diagnostic/UI timing only. It is **not** RT-scene performance because the Horde RT scene was never dispatched on this device.

## Evidence classification

- Evidence type: user-reported + screenshot evidence
- Date: 2026-09-11
- Current support status: unsupported by the current full-pipeline launch path; promising `RayQueryCompute` candidate
- Screenshot source: supplied by the project owner in the project conversation
- Screenshot file itself is not checked into the repository by this note
- Device-local report files shown by the app: `/data/user/0/com.samfa12.hordelanternrt/files/reports/vulkan_capability_report.txt` and `vulkan_capability_report.json`; these files were not supplied with this result

## Qualification

This evidence applies to the tested Galaxy S24 Ultra / Adreno 750 / driver `512.762.41` configuration only. Do not generalise it to every S24 firmware, every Snapdragon 8 Gen 3 device, or every Adreno 750 driver without a matching runtime probe.
