cmake_minimum_required(VERSION 3.22.1)

project(horde_rt_probe_android)

get_filename_component(HORDE_RT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtSources.cmake")
set(HORDE_RT_ANDROID_CORE_SOURCES)
foreach(source IN LISTS HORDE_RT_CORE_RELATIVE_SOURCES)
    list(APPEND HORDE_RT_ANDROID_CORE_SOURCES "${HORDE_RT_REPO_ROOT}/src/${source}")
endforeach()

add_library(horde_rt_probe_android
    SHARED
    android_probe_bridge.cpp
    "${HORDE_RT_REPO_ROOT}/src/ui/DiagnosticOverlay.cpp"
    ${HORDE_RT_ANDROID_CORE_SOURCES}
)

target_include_directories(horde_rt_probe_android PRIVATE
    "${HORDE_RT_REPO_ROOT}/src"
)

target_compile_features(horde_rt_probe_android PRIVATE cxx_std_20)
# Keep the locally installable debug APK inspectable while avoiding an -O0
# CPU skinning path that is not representative of a shipped phone build.
target_compile_options(horde_rt_probe_android PRIVATE $<$<CONFIG:Debug>:-O2>)

# Android 15+ devices may use 16 KiB memory pages. Keep the native payload
# installable on both 4 KiB and 16 KiB devices even while this project remains
# on the locally provisioned r26 NDK. The static C++ runtime avoids packaging
# r26's 4 KiB-only libc++_shared.so alongside this aligned library.
target_link_options(horde_rt_probe_android PRIVATE
    "-Wl,-z,max-page-size=16384"
    "-Wl,-z,common-page-size=16384"
)

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
