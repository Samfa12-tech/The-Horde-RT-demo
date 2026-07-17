# Source Layout

The `src` tree contains the shared Vulkan RT renderer, scene import code, platform shells, collision helper, and diagnostic UI.

```text
src/platform/windows/     Interactive Win32 Vulkan RT entry and presentation.
src/vulkan/               Vulkan capability data, context, and reports.
src/vulkan/raytracing/    Presentable RT scene and requirement evaluation.
src/ui/                   Diagnostic overlay text/data model.
src/scene/                Skeleton/lich GLB animation loading and CPU skinning.
src/gameplay/             Shared route, collision, combat, lighting, audio, checkpoints, and replay.
```

The Android entry and lifecycle code live in `android/app`; renderer and scene code remain shared under `src`.
