cmake_minimum_required(VERSION 3.22)

project(HordeLanternRT
    VERSION 0.1.0
    DESCRIPTION "Native Vulkan hardware ray tracing capability probe"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HORDE_RT_BUILD_CAPABILITY_PROBE_EXECUTABLE "Build the standalone command line Vulkan capability probe" ON)
option(HORDE_RT_BUILD_WINDOWS_DIAGNOSTIC_WINDOW "Build the Win32 diagnostic shell" ON)

include(cmake/HordeRtSources.cmake)
set(HORDE_RT_SOURCE_FILES)
foreach(source IN LISTS HORDE_RT_CORE_RELATIVE_SOURCES)
    list(APPEND HORDE_RT_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/${source}")
endforeach()
find_package(Vulkan REQUIRED)

message(STATUS "Vulkan SDK found: ${Vulkan_VERSION}")

add_library(horde_rt_probe_core STATIC
    ${HORDE_RT_SOURCE_FILES}
)

target_include_directories(horde_rt_probe_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(horde_rt_probe_core PUBLIC cxx_std_20)
target_link_libraries(horde_rt_probe_core PUBLIC Vulkan::Vulkan)

if(HORDE_RT_BUILD_CAPABILITY_PROBE_EXECUTABLE)
    add_executable(horde_rt_capability_probe
        src/platform/windows/CapabilityProbeMain.cpp
        src/ui/DiagnosticOverlay.cpp
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
        src/ui/DiagnosticOverlay.cpp
    )

    target_include_directories(horde_rt_diagnostic_window PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(horde_rt_diagnostic_window PRIVATE
        horde_rt_probe_core
    )
    target_compile_definitions(horde_rt_diagnostic_window PRIVATE
        HORDE_RT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    set_target_properties(horde_rt_diagnostic_window PROPERTIES
        OUTPUT_NAME "horde_rt_diagnostic_window"
    )
endif()

message(STATUS "Horde Lantern RT: Vulkan capability probe and Windows diagnostic targets are available; Android builds from android/.")
