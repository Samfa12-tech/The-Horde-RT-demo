# Source Layout

The `src` tree is intentionally small and honest at this stage. It is a Phase 0 scaffold, not a working renderer.

```text
src/app/                  High-level app flow.
src/platform/android/     Android native Vulkan notes and future glue.
src/platform/windows/     Windows native Vulkan notes and future Win32 entry.
src/vulkan/               Vulkan capability data, context, and reports.
src/vulkan/raytracing/    RT requirement evaluation.
src/ui/                   Diagnostic overlay text/data model.
src/scene/                Future scene code.
src/gameplay/             Future gameplay code.
```

No gameplay should be added until the real Vulkan RT capability probe is complete.
