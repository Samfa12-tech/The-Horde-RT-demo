# Source provenance for reports, separate from package/version-code policy.
# Call after creating the native report-producing target:
#   include("${HORDE_RT_REPO_ROOT}/cmake/HordeRtBuildIdentity.cmake")
#   horde_rt_attach_build_identity(TARGET native_target
#       REPO_ROOT "${HORDE_RT_REPO_ROOT}"
#       INPUTS CMakeLists.txt cmake/HordeRtSources.cmake ...
#       DEFINITIONS platform=Windows optional_feature=0)
# INPUTS must enumerate the native sources, headers, embedded shader inputs and
# build scripts relevant to that target. Absolute paths inside REPO_ROOT are
# accepted. Explicit inputs also support new, deliberately selected source files.
# No checkout/untracked-file walk is performed. Assets, reports and parked owner
# files are excluded unless a caller deliberately and incorrectly lists them.
# DEFINITIONS accepts only non-path, non-secret key=value build choices. Compiler
# identity, target and configuration are added automatically. This is NOT a
# compiler-command, dependency, package, executable or APK content hash.
#
# The always-run dependency refreshes HEAD and input bytes before native target
# compilation, including when CMake configure data has not otherwise changed.
# Keep source files stable during a build, as with ordinary CMake compilation.
include_guard(GLOBAL)

function(horde_rt_attach_build_identity)
    cmake_parse_arguments(PARSE_ARGV 0 ID "" "TARGET;REPO_ROOT" "INPUTS;DEFINITIONS")
    if(ID_UNPARSED_ARGUMENTS OR ID_KEYWORDS_MISSING_VALUES OR
       NOT ID_TARGET OR NOT TARGET "${ID_TARGET}" OR NOT ID_REPO_ROOT OR NOT ID_INPUTS)
        message(FATAL_ERROR "Build identity requires an existing TARGET, REPO_ROOT and explicit INPUTS.")
    endif()
    get_target_property(_already_attached "${ID_TARGET}" HORDE_RT_BUILD_IDENTITY_ATTACHED)
    if(_already_attached)
        message(FATAL_ERROR "Build identity is already attached to ${ID_TARGET}.")
    endif()
    if(NOT ID_TARGET MATCHES "^[A-Za-z0-9_.+-]+$")
        message(FATAL_ERROR "Build identity target must be a plain CMake target name.")
    endif()
    file(REAL_PATH "${ID_REPO_ROOT}" _repo_root)
    set(_inputs)
    foreach(_input IN LISTS ID_INPUTS)
        if(IS_ABSOLUTE "${_input}")
            file(RELATIVE_PATH _relative "${_repo_root}" "${_input}")
        else()
            set(_relative "${_input}")
        endif()
        string(REPLACE "\\" "/" _relative "${_relative}")
        if(_relative MATCHES "(^|/)\\.\\.?(/|$)" OR IS_ABSOLUTE "${_relative}" OR
           _relative MATCHES "[;\n\r\"]" OR _relative MATCHES "\\]==\\]" OR _relative STREQUAL "")
            message(FATAL_ERROR "Build identity inputs must be normalized repository-relative files.")
        endif()
        list(APPEND _inputs "${_relative}")
    endforeach()
    list(REMOVE_DUPLICATES _inputs)
    list(SORT _inputs)
    foreach(_definition IN LISTS ID_DEFINITIONS)
        if(NOT _definition MATCHES "^[A-Za-z0-9_.+-]+=[A-Za-z0-9_.+:-]+$")
            message(FATAL_ERROR "Build identity definitions require plain non-path key=value tokens.")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES ID_DEFINITIONS)
    list(SORT ID_DEFINITIONS)
    find_program(_git_command NAMES git)
    if(NOT _git_command)
        set(_git_command "")
    endif()
    set(_helper "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/build-identity/Refresh.cmake")
    set(_base "${CMAKE_CURRENT_BINARY_DIR}/horde-build-identity/${ID_TARGET}")
    set(_include "${_base}/$<CONFIG>")
    set(_output "${_include}/reporting/BuildIdentityStamp.generated.h")
    set(_spec "${_include}/inputs.cmake")
    # Bracket quoting keeps spaces/backslashes out of CMake's string-escape path.
    foreach(_value IN ITEMS _repo_root _inputs ID_DEFINITIONS _git_command)
        if("${${_value}}" MATCHES "\\]==\\]")
            message(FATAL_ERROR "Unsupported build identity specification delimiter.")
        endif()
    endforeach()
    file(GENERATE OUTPUT "${_spec}" CONTENT
"set(ID_REPO_ROOT [==[${_repo_root}]==])
set(ID_INPUTS [==[${_inputs}]==])
set(ID_DEFINITIONS [==[${ID_DEFINITIONS}]==])
set(ID_GIT [==[${_git_command}]==])
set(ID_TARGET [==[${ID_TARGET}]==])
set(ID_CONFIGURATION [==[$<CONFIG>]==])
set(ID_COMPILER [==[${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}]==])
set(ID_SYSTEM [==[${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}]==])
")
    add_custom_target("${ID_TARGET}_build_identity"
        COMMAND "${CMAKE_COMMAND}" "-DID_SPEC=${_spec}" "-DID_OUTPUT=${_output}" -P "${_helper}"
        BYPRODUCTS "${_output}"
        COMMENT "Refreshing declared native build provenance for ${ID_TARGET}"
        VERBATIM)
    add_dependencies("${ID_TARGET}" "${ID_TARGET}_build_identity")
    target_include_directories("${ID_TARGET}" PRIVATE "${_include}")
    set_property(TARGET "${ID_TARGET}" PROPERTY HORDE_RT_BUILD_IDENTITY_ATTACHED TRUE)
endfunction()
