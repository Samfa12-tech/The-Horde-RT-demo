# Ray Tracing Requirements

This folder evaluates whether a Vulkan device can support the project's RT modes.

Preferred mode:

- `RayTracingPipeline` when `VK_KHR_acceleration_structure`, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_ray_query`, `VK_KHR_buffer_device_address`, `VK_KHR_deferred_host_operations`, and the required feature structs are present. Ray query is mandatory because the active raygen shader uses it for primary, shadow, and bounce traversal.
- The world, held torch, animated skeleton, and swinging procedural sword are four TLAS instances. Torch/sword remain on mask `0x02`; world/enemy remain on `0x01`.
- Android selects strict ASTC KTX2 material arrays only after format/extent/filter/transfer checks. Windows retains the raw RGBA8 material fallback.

Acceptable labelled mode:

- `RayQuery` when it genuinely uses Vulkan hardware ray traversal and the device exposes `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`, `VK_KHR_buffer_device_address`, and required feature structs.

Unsupported mode:

- Anything else.

Fake fallbacks are not allowed.
