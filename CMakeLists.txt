cmake_minimum_required(VERSION 3.22)

project(HordeLanternRT
    VERSION 0.1.0
    DESCRIPTION "Native Vulkan hardware ray tracing capability probe"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HORDE_RT_BUILD_SCAFFOLD_CODE "Build the non-rendering Phase 0 scaffold library" ON)
option(HORDE_RT_BUILD_CAPABILITY_PROBE_EXECUTABLE "Build the standalone command line Vulkan capability probe" ON)
option(HORDE_RT_BUILD_WINDOWS_DIAGNOSTIC_WINDOW "Build the Win32 diagnostic shell" ON)
option(HORDE_RT_BUILD_ANDROID_CAPABILITY_PROBE "Build the Android native Vulkan capability probe library" ON)

set(HORDE_RT_SOURCE_FILES
    src/vulkan/DeviceCapabilities.cpp
    src/vulkan/RtCapabilityReport.cpp
    src/vulkan/VulkanContext.cpp
    src/vulkan/raytracing/RayTracingRequirements.cpp
    src/vulkan/raytracing/AccelerationStructures.cpp
    src/vulkan/raytracing/RayTracingPipeline.cpp
    src/vulkan/raytracing/ShaderBindingTable.cpp
    src/vulkan/raytracing/TinyRtScene.cpp
)
find_package(Vulkan REQUIRED)

message(STATUS "Vulkan SDK found: ${Vulkan_VERSION}")

set(HORDE_RT_APP_FILES
    src/app/HordeLanternApp.cpp
    src/ui/DiagnosticOverlay.cpp
)

add_library(horde_rt_probe_core STATIC
    ${HORDE_RT_SOURCE_FILES}
)

target_include_directories(horde_rt_probe_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(horde_rt_probe_core PUBLIC cxx_std_20)
target_link_libraries(horde_rt_probe_core PUBLIC Vulkan::Vulkan)

if(HORDE_RT_BUILD_SCAFFOLD_CODE)
    add_library(horde_rt_scaffold STATIC
        ${HORDE_RT_APP_FILES}
    )

    target_include_directories(horde_rt_scaffold
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_features(horde_rt_scaffold PUBLIC cxx_std_20)
    target_link_libraries(horde_rt_scaffold PUBLIC horde_rt_probe_core)
    target_compile_definitions(horde_rt_scaffold PUBLIC HORDE_RT_VULKAN_SDK_AVAILABLE=1)
endif()

if(HORDE_RT_BUILD_CAPABILITY_PROBE_EXECUTABLE)
    add_executable(horde_rt_capability_probe
        src/platform/windows/CapabilityProbeMain.cpp
    )

    target_include_directories(horde_rt_capability_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(horde_rt_capability_probe PRIVATE
        horde_rt_probe_core
    )

    set_target_properties(horde_rt_capability_probe PROPERTIES
        OUTPUT_NAME "horde_rt_capability_probe"
    )
endif()

if(HORDE_RT_BUILD_WINDOWS_DIAGNOSTIC_WINDOW AND WIN32)
    add_executable(horde_rt_diagnostic_window
        src/platform/windows/DiagnosticWindowMain.cpp
        src/platform/windows/DiagnosticWindow.cpp
    )

    target_include_directories(horde_rt_diagnostic_window PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(horde_rt_diagnostic_window PRIVATE
        horde_rt_probe_core
    )

    set_target_properties(horde_rt_diagnostic_window PROPERTIES
        OUTPUT_NAME "horde_rt_diagnostic_window"
    )
endif()

if(HORDE_RT_BUILD_ANDROID_CAPABILITY_PROBE AND ANDROID)
    add_library(horde_rt_capability_probe_android SHARED
        src/platform/android/CapabilityProbeAndroidMain.cpp
    )

    target_include_directories(horde_rt_capability_probe_android PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(horde_rt_capability_probe_android PRIVATE
        horde_rt_probe_core
        log
        android
    )

    set_target_properties(horde_rt_capability_probe_android PROPERTIES
        OUTPUT_NAME "horde_rt_capability_probe_android"
    )
endif()

message(STATUS "Horde Lantern RT: Vulkan capability probe targets are available for Windows CLI, diagnostic shell, and Android native.")
