# Local investigation only: no production module/catalog is replaced or edited.
set(HORDE_RT_STAGED_PRIMARY_SHADER_DIR "" CACHE PATH "Explicit selected-policy staged-primary investigation artifacts")
option(HORDE_RT_STAGED_PRIMARY_DEFAULT "Explicitly select staged-primary investigation instead of normal rendering" OFF)
option(HORDE_RT_STAGED_PRIMARY_TIMING "Investigation-only per-pass timestamp observer (separate profiled runs)" OFF)

function(horde_rt_attach_staged_primary target_name repo_root)
    if(HORDE_RT_STAGED_PRIMARY_TIMING AND NOT HORDE_RT_STAGED_PRIMARY_DEFAULT)
        message(FATAL_ERROR "Per-pass profiling requires the explicitly selected staged-primary investigation.")
    endif()
    if(HORDE_RT_STAGED_PRIMARY_DEFAULT AND "${HORDE_RT_STAGED_PRIMARY_SHADER_DIR}" STREQUAL "")
        message(FATAL_ERROR "Staged-primary selection requires explicit investigation shader artifacts.")
    endif()
    if("${HORDE_RT_STAGED_PRIMARY_SHADER_DIR}" STREQUAL "")
        return()
    endif()
    if(NOT EXISTS "${HORDE_RT_STAGED_PRIMARY_SHADER_DIR}/StagedPrimaryShaders.generated.h")
        message(FATAL_ERROR "Compile/validate the selected staged-primary shader artifacts before configuring this investigation build.")
    endif()
    target_sources(${target_name} PRIVATE "${repo_root}/src/vulkan/raytracing/experimental/StagedPrimaryPass.cpp")
    if(HORDE_RT_STAGED_PRIMARY_TIMING)
        target_sources(${target_name} PRIVATE "${repo_root}/src/vulkan/raytracing/experimental/StagedPrimaryTiming.cpp")
    endif()
    target_include_directories(${target_name} PRIVATE "${HORDE_RT_STAGED_PRIMARY_SHADER_DIR}")
    # Class layout must agree in all consumers; selected shader policy remains
    # private to the native provider and is asserted by the candidate component.
    target_compile_definitions(${target_name} PUBLIC HORDE_RT_STAGED_PRIMARY_EXPERIMENT=1
        HORDE_RT_STAGED_PRIMARY_DEFAULT=$<BOOL:${HORDE_RT_STAGED_PRIMARY_DEFAULT}>
        HORDE_RT_STAGED_PRIMARY_TIMING=$<BOOL:${HORDE_RT_STAGED_PRIMARY_TIMING}>)
    message(STATUS "INVESTIGATION staged-primary modules: ${HORDE_RT_STAGED_PRIMARY_SHADER_DIR}; selected=${HORDE_RT_STAGED_PRIMARY_DEFAULT}")
endfunction()
