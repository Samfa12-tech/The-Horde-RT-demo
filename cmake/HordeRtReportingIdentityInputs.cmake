include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/HordeRtBuildIdentity.cmake")
function(horde_rt_attach_reporting_identity target root platform abi)
    set(inputs CMakeLists.txt VERSION version-code-map.json
        tools/build-identity/Refresh.cmake
        src/platform/windows/HordeLanternRT.manifest.in
        src/platform/windows/HordeLanternRT.rc.in)
    # Native code/build roots only. No assets, reports, home directory, arbitrary
    # checkout/untracked inventory or parked owner inputs are scanned. The
    # concrete header/build/shader manifest is declared at configure time.
    file(GLOB_RECURSE headers CONFIGURE_DEPENDS LIST_DIRECTORIES FALSE
        "${root}/src/*.h" "${root}/src/*.hpp" "${root}/src/*.inl"
        "${root}/third_party/pocket-audio-core/include/*.h"
        "${root}/third_party/pocket-audio-core/src/*.h")
    file(GLOB_RECURSE build_shader_inputs CONFIGURE_DEPENDS LIST_DIRECTORIES FALSE
        "${root}/cmake/*.cmake" "${root}/shaders/*.rgen" "${root}/shaders/*.rchit"
        "${root}/shaders/*.rmiss" "${root}/shaders/*.comp" "${root}/shaders/*.glsl")
    list(APPEND inputs ${headers} ${build_shader_inputs})
    foreach(source IN LISTS HORDE_RT_CORE_RELATIVE_SOURCES)
        list(APPEND inputs "src/${source}")
    endforeach()
    foreach(consumer IN ITEMS "${target}" horde_rt_probe_core horde_gameplay_simulation pocket_audio_native_pcm)
        if(TARGET "${consumer}")
            get_target_property(sources "${consumer}" SOURCES)
            get_target_property(directory "${consumer}" SOURCE_DIR)
            if(EXISTS "${directory}/CMakeLists.txt")
                file(RELATIVE_PATH relative "${root}" "${directory}/CMakeLists.txt")
                if(NOT relative MATCHES "^\\.\\.(/|$)")
                    list(APPEND inputs "${relative}")
                endif()
            endif()
            foreach(source IN LISTS sources)
                if(source MATCHES "\\$<")
                    continue()
                endif()
                if(NOT IS_ABSOLUTE "${source}")
                    set(source "${directory}/${source}")
                endif()
                file(RELATIVE_PATH relative "${root}" "${source}")
                if(NOT relative MATCHES "^\\.\\.(/|$)" AND NOT IS_ABSOLUTE "${relative}" AND
                    relative MATCHES "\\.(cpp|c|h|hpp)$")
                    list(APPEND inputs "${relative}")
                endif()
            endforeach()
        endif()
    endforeach()
    if(NOT abi MATCHES "^[A-Za-z0-9_.+-]+$")
        set(abi unknown)
    endif()
    set(minimum_scale "${HORDE_RT_MIN_RENDER_SCALE_PERCENT}")
    if(NOT minimum_scale MATCHES "^[0-9]+$")
        set(minimum_scale unspecified)
    endif()
    set(actor_worker 0)
    if(HORDE_RT_ACTOR_WORKER_EXPERIMENT)
        set(actor_worker 1)
    endif()
    set(windows_acceptance 0)
    if(HORDE_RT_WINDOWS_ACCEPTANCE)
        set(windows_acceptance 1)
    endif()
    horde_rt_attach_build_identity(TARGET "${target}" REPO_ROOT "${root}" INPUTS ${inputs}
        DEFINITIONS "platform=${platform}" "abi=${abi}" "minimumRenderScale=${minimum_scale}" "actorWorker=${actor_worker}" "windowsAcceptance=${windows_acceptance}")
endfunction()
