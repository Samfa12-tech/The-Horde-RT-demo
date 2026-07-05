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

set(HORDE_RT_SOURCE_FILES
    src/vulkan/DeviceCapabilities.cpp
    src/vulkan/RtCapabilityReport.cpp
    src/vulkan/VulkanContext.cpp
    src/vulkan/raytracing/RayTracingRequirements.cpp
)

find_package(Vulkan REQUIRED)

message(STATUS "Vulkan SDK found: ${Vulkan_VERSION}")

if(HORDE_RT_BUILD_SCAFFOLD_CODE)
    add_library(horde_rt_scaffold STATIC
        src/app/HordeLanternApp.cpp
        ${HORDE_RT_SOURCE_FILES}
        src/ui/DiagnosticOverlay.cpp
    )

    target_include_directories(horde_rt_scaffold
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_features(horde_rt_scaffold PUBLIC cxx_std_20)
    target_link_libraries(horde_rt_scaffold PUBLIC Vulkan::Vulkan)
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
        horde_rt_scaffold
        Vulkan::Vulkan
    )

    set_target_properties(horde_rt_capability_probe PROPERTIES
        OUTPUT_NAME "horde_rt_capability_probe"
    )
endif()

message(STATUS "Horde Lantern RT: native Vulkan RT capability probe target is available. Android integration is pending.")
