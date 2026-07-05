# Technical Requirements

## Non-negotiable rendering rule

This project is **RT or nothing**.

The renderer must use native Vulkan hardware ray tracing. It must not use browser rendering, Godot/Unreal as the first renderer path, Three.js, Babylon.js, WebGPU, raster-only rendering, screen-space reflections, baked lighting, fake RT effects, or compute-only path tracing as a substitute for the core proof.

## Phase 0A completed scope

- Real Vulkan probe implemented on Windows and wired into Android native shell:
  - creates a Vulkan instance,
  - enumerates physical devices,
  - enumerates required extensions,
  - queries feature chain fields through `vkGetPhysicalDeviceFeatures2`,
  - evaluates `RayTracingPipeline`, `RayQuery`, or `Unsupported`.
- Adds an Android native activity target that reuses the same core and writes both text/JSON reports.
- Writes both:
  - `reports/vulkan_capability_report.txt`
  - `reports/vulkan_capability_report.json`

## Phase 0A required report fields

```text
Backend: Vulkan
RT mode: RayTracingPipeline / RayQuery / Unsupported
GPU name
Vendor ID
Device ID
Driver version
Vulkan API version
VK_KHR_acceleration_structure: yes/no
VK_KHR_ray_tracing_pipeline: yes/no
VK_KHR_ray_query: yes/no
VK_KHR_buffer_device_address: yes/no
VK_KHR_deferred_host_operations: yes/no
Internal render resolution
FPS / frame time
```

## Vulkan extension interpretation

- `VK_KHR_acceleration_structure` is required.
- `VK_KHR_ray_tracing_pipeline` is the preferred path.
- `VK_KHR_ray_query` is accepted only when it is a genuine hardware-ray-query path.
- `VK_KHR_buffer_device_address` is required for practical RT support.
- `VK_KHR_deferred_host_operations` is required for RT pipeline probing.

## Feature structs to query

For each physical device, this project now queries and evaluates:

- `VkPhysicalDeviceAccelerationStructureFeaturesKHR`
- `VkPhysicalDeviceRayTracingPipelineFeaturesKHR`
- `VkPhysicalDeviceRayQueryFeaturesKHR`
- `VkPhysicalDeviceBufferDeviceAddressFeatures`

## RT mode rules

### RayTracingPipeline

- `VK_KHR_acceleration_structure`
- `VK_KHR_ray_tracing_pipeline`
- `VK_KHR_buffer_device_address`
- `VK_KHR_deferred_host_operations`
- `VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure`
- `VkPhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline`
- `VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress`

### RayQuery

- `VK_KHR_acceleration_structure`
- `VK_KHR_ray_query`
- `VK_KHR_buffer_device_address`
- `VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure`
- `VkPhysicalDeviceRayQueryFeaturesKHR::rayQuery`
- `VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress`

### Unsupported

- Neither mode is fully satisfied by required extensions/features.

## Android native status

Android native wiring exists in this slice as:

- CMake Android target `horde_rt_capability_probe_android`.
- NativeActivity bootstrap that runs the probe and writes:
  - `files/reports/vulkan_capability_report.txt`
  - `files/reports/vulkan_capability_report.json`
- Toast-style diagnostic text from the native side.

Next small task: run and validate this target on the Galaxy S26 Ultra with `adb run-as` report extraction and confirm unsupported-mode diagnostics remain explicit.

## Unsupported-device behaviour

When unsupported, report:

- backend attempted (`Vulkan`)
- GPU/driver/version context when available
- exact missing extension and feature diagnostics
- `RT mode: Unsupported`
- do not pretend to be rendering-capable.
