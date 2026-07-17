# Ray Tracing Requirements

This folder evaluates whether a Vulkan device can support the project's RT modes.

Preferred mode:

- `RayTracingPipeline` when `VK_KHR_acceleration_structure`, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_ray_query`, `VK_KHR_buffer_device_address`, `VK_KHR_deferred_host_operations`, and the required feature structs are present. Ray query is mandatory because the active raygen shader uses it for primary, shadow, and bounce traversal.
- The current showcase assembles 18 TLAS instances: world; one selected skeleton/lich slot; held or dropped torch and sword; torso plus reusable articulated arms/lower body; reflection/shadow-only head; and sliding finale roof. Preserve the documented per-purpose masks in the scene source rather than collapsing traversal domains.
- Android selects strict ASTC KTX2 environment arrays and strict ASTC 6x6 lich textures only after format/extent/filter/transfer checks. Windows retains executable-relative raw RGBA8 environment and lich data.

Acceptable labelled mode:

- `RayQuery` when it genuinely uses Vulkan hardware ray traversal and the device exposes `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`, `VK_KHR_buffer_device_address`, and required feature structs.

Unsupported mode:

- Anything else.

Fake fallbacks are not allowed.
