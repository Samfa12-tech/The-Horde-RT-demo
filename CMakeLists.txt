cmake_minimum_required(VERSION 3.22)

project(HordeLanternRT
    VERSION 0.1.0
    DESCRIPTION "Native Vulkan hardware ray tracing capability probe scaffold"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HORDE_RT_BUILD_SCAFFOLD_CODE "Build the non-rendering Phase 0 scaffold library" ON)

# Phase 0 note:
# This top-level CMake file is intentionally conservative. It creates a small
# non-rendering C++ scaffold so the repo has a clean shape, but it does not yet
# implement Vulkan instance/device creation, swapchain setup, Android glue, or
# hardware ray-tracing dispatch. Do not treat a successful CMake configure as a
# working RT renderer.

find_package(Vulkan QUIET)

if(Vulkan_FOUND)
    message(STATUS "Vulkan SDK found: ${Vulkan_VERSION}")
else()
    message(WARNING "Vulkan SDK not found. Phase 0 Vulkan implementation is not wired yet; install the Vulkan SDK before real capability-probe work.")
endif()

if(HORDE_RT_BUILD_SCAFFOLD_CODE)
    add_library(horde_rt_scaffold STATIC
        src/app/HordeLanternApp.cpp
        src/vulkan/DeviceCapabilities.cpp
        src/vulkan/RtCapabilityReport.cpp
        src/vulkan/VulkanContext.cpp
        src/vulkan/raytracing/RayTracingRequirements.cpp
        src/ui/DiagnosticOverlay.cpp
    )

    target_include_directories(horde_rt_scaffold
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_features(horde_rt_scaffold PUBLIC cxx_std_20)

    if(Vulkan_FOUND)
        target_link_libraries(horde_rt_scaffold PUBLIC Vulkan::Vulkan)
        target_compile_definitions(horde_rt_scaffold PUBLIC HORDE_RT_VULKAN_SDK_AVAILABLE=1)
    else()
        target_compile_definitions(horde_rt_scaffold PUBLIC HORDE_RT_VULKAN_SDK_AVAILABLE=0)
    endif()
endif()

message(STATUS "Horde Lantern RT: scaffold configured. Real Vulkan RT capability probing is the next task.")
