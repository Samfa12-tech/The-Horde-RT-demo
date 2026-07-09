cmake_minimum_required(VERSION 3.22.1)

project(horde_rt_probe_android)

add_library(horde_rt_probe_android
    SHARED
    android_probe_bridge.cpp
    ../../../../../src/ui/DiagnosticOverlay.cpp
    ../../../../../src/vulkan/DeviceCapabilities.cpp
    ../../../../../src/vulkan/RtCapabilityReport.cpp
    ../../../../../src/vulkan/VulkanContext.cpp
    ../../../../../src/vulkan/raytracing/RayTracingRequirements.cpp
    ../../../../../src/vulkan/raytracing/AccelerationStructures.cpp
    ../../../../../src/vulkan/raytracing/RayTracingPipeline.cpp
    ../../../../../src/vulkan/raytracing/ShaderBindingTable.cpp
    ../../../../../src/vulkan/raytracing/TinyRtScene.cpp
    ../../../../../src/vulkan/raytracing/PresentableTinyRtScene.cpp
)

target_include_directories(horde_rt_probe_android PRIVATE
    ../../../../../src
)

target_compile_features(horde_rt_probe_android PRIVATE cxx_std_20)

find_library(VULKAN_LIBRARY vulkan)
find_library(ANDROID_LOG_LIBRARY log)
find_library(ANDROID_LIBRARY android)

if (NOT VULKAN_LIBRARY)
    message(FATAL_ERROR "Vulkan loader library not found for Android.")
endif()

if (NOT ANDROID_LOG_LIBRARY)
    message(FATAL_ERROR "Android log library not found for Android")
endif()

if (NOT ANDROID_LIBRARY)
    message(FATAL_ERROR "Android library not found for Android")
endif()

target_link_libraries(horde_rt_probe_android
    ${VULKAN_LIBRARY}
    ${ANDROID_LOG_LIBRARY}
    ${ANDROID_LIBRARY}
)
