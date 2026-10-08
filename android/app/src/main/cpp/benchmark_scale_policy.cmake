# Explicit build capability only; normal Android and all desktop builds stay 50.
set(HORDE_RT_MIN_RENDER_SCALE_PERCENT "50" CACHE STRING "Ordinary 50 or isolated benchmark 33 scale admission")
option(HORDE_RT_ANDROID_BENCHMARK_VALIDATION "Isolated development benchmark package admission" OFF)
option(HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION "Isolated benchmark actual image presentation evidence" OFF)
option(HORDE_RT_ANDROID_MOTION_VALIDATION "Isolated benchmark shared-simulation motion evidence" OFF)
if(NOT HORDE_RT_MIN_RENDER_SCALE_PERCENT STREQUAL "50" AND
   NOT HORDE_RT_MIN_RENDER_SCALE_PERCENT STREQUAL "33")
    message(FATAL_ERROR "Render scale minimum must be exactly 50 or benchmark 33.")
endif()
if(HORDE_RT_ANDROID_MOTION_VALIDATION AND
   (NOT HORDE_RT_ANDROID_BENCHMARK_VALIDATION OR
    NOT HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION OR
    NOT HORDE_RT_MIN_RENDER_SCALE_PERCENT STREQUAL "33" OR
    HORDE_RT_DEBUG_CHECKPOINTS OR HORDE_RT_DEBUG_VIEWMODEL_CANDIDATE OR
    NOT CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$"))
    message(FATAL_ERROR "Motion validation requires the isolated non-Debug Shipping/Mobile min33 benchmark with actual present timing.")
endif()
if(HORDE_RT_MIN_RENDER_SCALE_PERCENT STREQUAL "33" OR HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION)
    set(_scale_instrumentation "${HORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION}")
    set(_scale_quality "${HORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY}")
    if(NOT "${HORDE_RT_INSTRUMENTATION_OVERRIDE}" STREQUAL "")
        set(_scale_instrumentation "${HORDE_RT_INSTRUMENTATION_OVERRIDE}")
    endif()
    if(NOT "${HORDE_RT_DIELECTRIC_QUALITY_OVERRIDE}" STREQUAL "")
        set(_scale_quality "${HORDE_RT_DIELECTRIC_QUALITY_OVERRIDE}")
    endif()
    if(NOT HORDE_RT_ANDROID_BENCHMARK_VALIDATION OR HORDE_RT_DEBUG_CHECKPOINTS OR
       HORDE_RT_DEBUG_VIEWMODEL_CANDIDATE OR
       NOT CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$" OR
       NOT _scale_instrumentation STREQUAL "Shipping" OR NOT _scale_quality STREQUAL "Mobile" OR
       NOT "${HORDE_RT_STAGED_PRIMARY_SHADER_DIR}" STREQUAL "" OR
       HORDE_RT_STAGED_PRIMARY_DEFAULT OR HORDE_RT_STAGED_PRIMARY_TIMING)
        if(HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION)
            message(FATAL_ERROR "Presentation timing requires isolated non-Debug Shipping/Mobile benchmark admission, checkpoints OFF and no staged/viewmodel experiment.")
        else()
        message(FATAL_ERROR "Sub-50 scales require isolated non-Debug Shipping/Mobile benchmark admission, checkpoints OFF and no staged/viewmodel experiment.")
        endif()
    endif()
endif()
