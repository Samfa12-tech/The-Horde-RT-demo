# Hardware RT scene shaders

- `minimal.rgen` is the active phone-safe ray-query shading path used inside the Vulkan ray-tracing pipeline dispatch.
- `minimal.rmiss` and `minimal.rchit` are active pipeline stages.
- `MinimalRayGenShader.inc` is generated from `minimal.rgen` and embedded by `PresentableTinyRtScene.cpp`, so the Android scene remains self-contained.

Regenerate the embedded raygen include after changing `minimal.rgen`:

```powershell
C:\VulkanSDK\1.4.350.0\Bin\glslangValidator.exe -V --target-env vulkan1.2 -Os -S rgen -o shaders\raytracing\minimal.rgen.spv shaders\raytracing\minimal.rgen
```

Then regenerate `MinimalRayGenShader.inc` from that SPIR-V binary before building. The `.spv` output is an ignored intermediate, while the generated include is the reviewed source artifact.
