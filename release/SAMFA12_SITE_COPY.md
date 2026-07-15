# Horde Lantern RT - initial showing alpha

Horde Lantern RT is a short native Vulkan hardware-ray-tracing technology demo running on both Android phone hardware and Windows RTX. Walk a torch-lit gothic ruin, inspect wet stone and material responses, and face one animated skeleton while the same real RT presentation path drives the scene.

## This alpha proves

- Native Vulkan BLAS/TLAS acceleration structures.
- A ray-tracing pipeline, shader binding table, and `vkCmdTraceRaysKHR` frame dispatch.
- Honest swapchain presentation of the RT-produced image.
- Phone-safe `rayQueryEXT` shading inside raygen rather than a raster fallback.
- One shared scene direction across Android and Windows.
- A 50-100% RT render-resolution slider, defaulting to 100%, on both downloads.

## Downloads

- Android: signed APK for compatible Vulkan-RT phones. The first validated target is Samsung `SM-S948B`.
- Windows: portable x64 zip for compatible Vulkan hardware-RT GPUs. The first validated target is an NVIDIA GeForce RTX 5050 Laptop GPU.
- Source and issue tracking: https://github.com/Samfa12-tech/The-Horde-RT-demo

## Important compatibility note

This demo is RT or nothing. Unsupported devices receive a clear diagnostic report; they do not receive a raster, browser, or fake-ray-tracing fallback.

## Alpha scope

This is the initial torch-corridor and material-gallery showing. It includes a closed starting chamber, one deep arch/skeleton reveal, a clear skylight pane, and a compact lantern-cage flame. The broader framed-mirror, stained-transmission, and final guided-crypt composition remain later showcase slices.

## Credits

- Environment materials: Poly Haven, CC0.
- Sound effects: FilmCow Royalty Free Sound Effects Library.
- Original stylized skeleton: Hotstrike Studio; texture, rig, and animation processing created with Meshy.
- Full provenance and licence details are linked from the download page and included in the Windows zip.
- Meshy attribution route used for this release: conservative Free-plan **CC BY 4.0** credit for the processed skeleton derivative.
