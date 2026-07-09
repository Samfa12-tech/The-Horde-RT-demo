# Shaders

The current native Vulkan RT path uses the `raytracing/` shader set:

- Ray generation (`minimal.rgen`) with phone-safe `rayQueryEXT` primary/shadow/first-bounce work.
- Miss (`minimal.rmiss`) and closest-hit (`minimal.rchit`) pipeline stages.
- Generated embedded raygen SPIR-V source (`MinimalRayGenShader.inc`) for the self-contained Android build.

Do not add fake RT, SSR, baked lighting, or compute-only path tracing as a substitute for hardware Vulkan RT.
