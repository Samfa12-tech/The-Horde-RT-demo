cmake_minimum_required(VERSION 3.22.1)

project(horde_rt_probe_android)

option(HORDE_RT_DEBUG_CHECKPOINTS "Enable debug-only showcase checkpoint and replay commands" OFF)
option(HORDE_RT_DEBUG_VIEWMODEL_CANDIDATE "Use the explicitly packaged viewmodel candidate for live Debug play" OFF)
if(HORDE_RT_DEBUG_VIEWMODEL_CANDIDATE AND (NOT HORDE_RT_DEBUG_CHECKPOINTS OR NOT CMAKE_BUILD_TYPE STREQUAL "Debug"))
    message(FATAL_ERROR "The viewmodel candidate is restricted to Debug checkpoint-enabled builds.")
endif()

get_filename_component(HORDE_RT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtVersion.cmake")
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtRaygenArtifacts.cmake")
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtRaygenPolicy.cmake")
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtStagedPrimary.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/benchmark_scale_policy.cmake")
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtSources.cmake")
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtPocketAudioCore.cmake")
horde_rt_add_pocket_audio_core("${HORDE_RT_REPO_ROOT}" OFF)
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtStrictAssetMath.cmake")
horde_rt_enable_strict_asset_math("${HORDE_RT_REPO_ROOT}")
horde_rt_configure_raygen_artifacts("${HORDE_RT_REPO_ROOT}")
set(HORDE_RT_ANDROID_CORE_SOURCES)
foreach(source IN LISTS HORDE_RT_CORE_RELATIVE_SOURCES)
    list(APPEND HORDE_RT_ANDROID_CORE_SOURCES "${HORDE_RT_REPO_ROOT}/src/${source}")
endforeach()

add_library(horde_rt_probe_android
    SHARED
    android_probe_bridge.cpp
    android_music_playback.cpp
    android_ambience_playback.cpp
    "${HORDE_RT_REPO_ROOT}/src/ui/DiagnosticOverlay.cpp"
    ${HORDE_RT_ANDROID_CORE_SOURCES}
)

target_include_directories(horde_rt_probe_android PRIVATE
    "${HORDE_RT_REPO_ROOT}/src"
    "${HORDE_RT_REPO_ROOT}/third_party"
)
horde_rt_attach_raygen_artifacts(horde_rt_probe_android)
horde_rt_apply_raygen_policy(horde_rt_probe_android Android)
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtRegionPages.cmake")
horde_rt_attach_private_region_pages(horde_rt_probe_android)
horde_rt_attach_staged_primary(horde_rt_probe_android "${HORDE_RT_REPO_ROOT}")

target_compile_features(horde_rt_probe_android PRIVATE cxx_std_20)
target_compile_definitions(horde_rt_probe_android PRIVATE
    HORDE_RT_MIN_RENDER_SCALE_PERCENT=${HORDE_RT_MIN_RENDER_SCALE_PERCENT}
    HORDE_RT_ANDROID_OUTDOOR_SCALE_PROBE=$<BOOL:${HORDE_RT_ANDROID_OUTDOOR_SCALE_PROBE}>
    HORDE_RT_DISPLAY_VERSION="${HORDE_RT_DISPLAY_VERSION}"
    HORDE_RT_PACKAGE_VERSION="${HORDE_RT_PACKAGE_VERSION}"
    HORDE_RT_BUILD_ID="${HORDE_RT_BUILD_ID}"
)
target_compile_definitions(horde_rt_probe_android PRIVATE
    HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION=$<BOOL:${HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION}>)
option(HORDE_RT_DISPATCH_CLOCK_PROBE "Private sparse per-invocation dispatch observation" OFF)
if(NOT "${HORDE_RT_REGION_PAGE_PROTOTYPE_ROOT}" STREQUAL "" AND
   EXISTS "${HORDE_RT_REGION_PAGE_PROTOTYPE_ROOT}/dispatch-clock-probe.json" AND
   NOT HORDE_RT_DISPATCH_CLOCK_PROBE)
    message(FATAL_ERROR "Private clock shader ABI requires its matching native observer.")
endif()
if(HORDE_RT_DISPATCH_CLOCK_PROBE AND (NOT HORDE_RT_FRAME_STAGE_PROBE OR
   NOT EXISTS "${HORDE_RT_REGION_PAGE_PROTOTYPE_ROOT}/dispatch-clock-probe.json"))
    message(FATAL_ERROR "Dispatch clocks require the explicit private stage/clock shader profile.")
endif()
target_compile_definitions(horde_rt_probe_android PRIVATE
    HORDE_RT_DISPATCH_CLOCK_PROBE=$<BOOL:${HORDE_RT_DISPATCH_CLOCK_PROBE}>)
option(HORDE_RT_FRAME_STAGE_PROBE "Private chapter GPU AS/dispatch/transfer observation" OFF)
if(HORDE_RT_FRAME_STAGE_PROBE AND NOT HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING)
    message(FATAL_ERROR "Frame stage profiling requires private chapter route timing.")
endif()
target_compile_definitions(horde_rt_probe_android PRIVATE
    HORDE_RT_FRAME_STAGE_PROBE=$<BOOL:${HORDE_RT_FRAME_STAGE_PROBE}>)
if(HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING AND
   (NOT HORDE_RT_DEBUG_CHECKPOINTS OR NOT CMAKE_BUILD_TYPE STREQUAL "Debug" OR
    NOT HORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION STREQUAL "Shipping" OR
    NOT HORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY STREQUAL "Mobile"))
    message(FATAL_ERROR "Route timing requires explicit Debug Shipping/Mobile policy.")
endif()
if(HORDE_RT_ANDROID_OUTDOOR_SCALE_PROBE AND
   (NOT HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING OR HORDE_RT_ANDROID_BENCHMARK_VALIDATION OR
    HORDE_RT_DEBUG_VIEWMODEL_CANDIDATE OR NOT "${HORDE_RT_REGION_PAGE_PROTOTYPE_ROOT}" STREQUAL "" OR
    NOT "${HORDE_RT_STAGED_PRIMARY_SHADER_DIR}" STREQUAL "" OR HORDE_RT_STAGED_PRIMARY_DEFAULT))
    message(FATAL_ERROR "Outdoor scale probe requires the ordinary private Debug Shipping/Mobile route timing build.")
endif()
target_compile_definitions(horde_rt_probe_android PRIVATE
    HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING=$<BOOL:${HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING}>)
target_compile_definitions(horde_rt_probe_android PRIVATE
    HORDE_RT_ANDROID_MOTION_VALIDATION=$<BOOL:${HORDE_RT_ANDROID_MOTION_VALIDATION}>)
if(HORDE_RT_ANDROID_BENCHMARK_VALIDATION AND HORDE_RT_MIN_RENDER_SCALE_PERCENT STREQUAL "33")
    target_compile_definitions(horde_rt_probe_android PRIVATE HORDE_RT_BENCHMARK_SCALE_VALIDATION=1)
endif()
if(HORDE_RT_DEBUG_CHECKPOINTS)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(FATAL_ERROR "Android motion/checkpoint evidence is restricted to Debug builds.")
    endif()
    target_sources(horde_rt_probe_android PRIVATE
        "${HORDE_RT_REPO_ROOT}/src/gameplay/validation/MotionEvidenceScenario.cpp"
        "${HORDE_RT_REPO_ROOT}/src/telemetry/MotionEvidenceLedger.cpp")
    target_compile_definitions(horde_rt_probe_android PRIVATE
        HORDE_RT_DEBUG_CHECKPOINTS=1
    )
endif()
if(HORDE_RT_ANDROID_MOTION_VALIDATION)
    target_sources(horde_rt_probe_android PRIVATE
        "${HORDE_RT_REPO_ROOT}/src/gameplay/validation/MotionEvidenceScenario.cpp"
        "${HORDE_RT_REPO_ROOT}/src/telemetry/MotionEvidenceLedger.cpp")
endif()
if(HORDE_RT_DEBUG_VIEWMODEL_CANDIDATE)
    target_compile_definitions(horde_rt_probe_android PRIVATE HORDE_RT_DEBUG_VIEWMODEL_CANDIDATE=1)
endif()
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
    pocket_audio_native_pcm
    ${VULKAN_LIBRARY}
    ${ANDROID_LOG_LIBRARY}
    ${ANDROID_LIBRARY}
)
include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtReportingIdentityInputs.cmake")
horde_rt_attach_reporting_identity(horde_rt_probe_android "${HORDE_RT_REPO_ROOT}" Android "${ANDROID_ABI}")

# Explicit experiment only: both modes live in one production-shader Debug APK.
option(HORDE_RT_ACTOR_WORKER_EXPERIMENT "Private CPU actor serial/worker comparison" OFF)
if(HORDE_RT_ACTOR_WORKER_EXPERIMENT AND
   (NOT HORDE_RT_DEBUG_CHECKPOINTS OR NOT CMAKE_BUILD_TYPE STREQUAL "Debug" OR
    NOT HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING OR
    NOT HORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION STREQUAL "Shipping" OR
    NOT HORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY STREQUAL "Mobile" OR
    NOT "${HORDE_RT_REGION_PAGE_PROTOTYPE_ROOT}" STREQUAL "" OR HORDE_RT_STAGED_PRIMARY_DEFAULT))
    message(FATAL_ERROR "Actor worker requires isolated Debug production Shipping/Mobile route timing.")
endif()
target_compile_definitions(horde_rt_probe_android PRIVATE
    HORDE_RT_ACTOR_WORKER_EXPERIMENT=$<BOOL:${HORDE_RT_ACTOR_WORKER_EXPERIMENT}>)
