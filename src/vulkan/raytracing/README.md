# Ray Tracing Requirements

This folder evaluates whether a Vulkan device can support the project's RT modes.

Preferred mode:

- `RayTracingPipeline` when `VK_KHR_acceleration_structure`, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_ray_query`, `VK_KHR_buffer_device_address`, `VK_KHR_deferred_host_operations`, and the required feature structs are present. Ray query is mandatory because the active raygen shader uses it for primary, shadow, and bounce traversal.
- The current showcase assembles 20 TLAS instances: world; the selected skeleton/lich slot; held or dropped torch and sword; torso plus reusable articulated arms/lower body; reflection/shadow-only head; sliding finale roof; a second skeleton slot that remains masked when the character route is singular; and dedicated waterfall instance 19. Matching skeleton poses share the first BLAS and shader route; divergent poses use the bounded second skeleton BLAS at custom index 18. Preserve the documented per-purpose masks in the scene source rather than collapsing traversal domains.
- `CharacterRenderSlot` caches one bounded frame plan for both skin/refit and TLAS routing. Skeleton wind-up/active/recovery phases map into the existing Attack clip. A parried skeleton stays stationary in gameplay but procedurally continues that clip from contact toward recovery with a bounded whole-instance lean/recoil; it is not rendered as a frozen contact pose. Player parry only recomposes the existing right arm and sword transform. These paths add no BLAS, TLAS slot, animation asset, or third pose bucket.
- `SurfaceWater = 10` is appended after every released world material ID. Primary water uses real metadata-backed geometry, IOR 1.333 refraction, Schlick reflection, Beer-Lambert absorption, and deterministic flow normals. The RT Lab width control scales waterfall instance 19 only across world Z, matching the visible lane span and shader stream centres/radii while leaving world-X film depth and floor water fixed. Refracted and High reflected opaque hits use the terminal ordinary material/direct-visibility/atmosphere shader without a second glossy bounce; Mobile keeps analytic environment reflection; interface highlights use the same active local/skylight selectors and complete world/player caster mask. Transparent filtering uses `gl_RayFlagsNoOpaqueEXT`, secondary distance is accumulated from the camera, water-on-water hits terminate without recursion, and Off skips primary water candidates.
- The lich-room ground mist is a bounded depth-clipped analytic volume with six deterministic fixed samples. It stays floor-weighted and does not add TLAS instances, texture descriptors, particles, or a ceiling plume.
- Android selects strict ASTC KTX2 environment arrays and strict ASTC 6x6 lich textures only after format/extent/filter/transfer checks. Windows retains executable-relative raw RGBA8 environment and lich data.

Acceptable labelled mode:

- `RayQuery` when it genuinely uses Vulkan hardware ray traversal and the device exposes `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`, `VK_KHR_buffer_device_address`, and required feature structs.

Unsupported mode:

- Anything else.

Fake fallbacks are not allowed.
