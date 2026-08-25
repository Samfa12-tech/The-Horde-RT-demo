# Shared native renderer sources. Keep platform entry points and UI shells in
# their platform-specific targets; this list is consumed by Windows and Android.
set(HORDE_RT_DISPLAY_VERSION "1.5.2")
set(HORDE_RT_PACKAGE_VERSION "1.5.2")

set(HORDE_RT_CORE_RELATIVE_SOURCES
    gameplay/simulation/GameSimulation.cpp
    gameplay/ShowcaseBenchmark.cpp
    scene/assets/AssetManifest.cpp
    scene/assets/AssetValidation.cpp
    scene/assets/GltfDocument.cpp
    scene/assets/StaticMeshAsset.cpp
    vulkan/DeviceCapabilities.cpp
    vulkan/RtCapabilityReport.cpp
    vulkan/GpuFrameTimer.cpp
    vulkan/VulkanContext.cpp
    vulkan/raytracing/RayTracingRequirements.cpp
    vulkan/raytracing/RtGpuResources.cpp
    vulkan/raytracing/RtStaticMeshSlot.cpp
    vulkan/raytracing/RtTextureArrays.cpp
    vulkan/raytracing/CharacterRenderSlot.cpp
    vulkan/raytracing/PresentableTinyRtScene.cpp
    vulkan/raytracing/SimulationFrameAdapter.cpp
    scene/SkeletonBipedModel.cpp
)
