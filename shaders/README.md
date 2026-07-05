# Shaders

No production shaders are included yet.

Future Phase 1 shader work may include:

- Ray generation shader (`.rgen`).
- Miss shader (`.rmiss`).
- Closest-hit shader (`.rchit`).
- Ray-query shader path only if the device genuinely supports Vulkan ray query hardware traversal.

Do not add fake RT, SSR, baked-lighting, or compute-only path-tracing shaders as a substitute for hardware Vulkan RT.
