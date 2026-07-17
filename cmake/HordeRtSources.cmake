# Shared native renderer sources. Keep platform entry points and UI shells in
# their platform-specific targets; this list is consumed by Windows and Android.
set(HORDE_RT_PACKAGE_VERSION "0.1.1-alpha.1")

set(HORDE_RT_CORE_RELATIVE_SOURCES
    gameplay/ShowcaseBenchmark.cpp
    vulkan/DeviceCapabilities.cpp
    vulkan/RtCapabilityReport.cpp
    vulkan/VulkanContext.cpp
    vulkan/raytracing/RayTracingRequirements.cpp
    vulkan/raytracing/PresentableTinyRtScene.cpp
    scene/SkeletonBipedModel.cpp
)
