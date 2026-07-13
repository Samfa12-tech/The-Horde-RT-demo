# Hardware RT scene shaders

- `minimal.rgen` is the active phone-safe ray-query shading path used inside the Vulkan ray-tracing pipeline dispatch.
- `minimal.rmiss` and `minimal.rchit` are active pipeline stages.
- `MinimalRayGenShader.inc` is generated from `minimal.rgen` and embedded by `PresentableTinyRtScene.cpp`, so the Android scene remains self-contained.

Regenerate the compiled shader and embedded include after changing `minimal.rgen`:

```powershell
.\tools\compile-raygen.ps1
```

The script compiles for Vulkan 1.2, deterministically rewrites `MinimalRayGenShader.inc`, and prints the SPIR-V size and SHA-256. The `.spv` output is an ignored intermediate, while the generated include is the reviewed source artifact.
