if(NOT DEFINED CXX_COMPILER OR NOT DEFINED CXX_COMPILER_ID OR NOT DEFINED SOURCE_DIRECTORY OR NOT DEFINED FIXTURE)
    message(FATAL_ERROR "Compiler-negative fixture requires CXX_COMPILER, CXX_COMPILER_ID, SOURCE_DIRECTORY, and FIXTURE.")
endif()

if(NOT CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Compiler-negative fixture is currently defined for the Windows MSVC Host gate.")
endif()

set(fixture_project "$ENV{TEMP}/horde-torch-failure-legacy-fixture")
file(REMOVE_RECURSE "${fixture_project}")
file(MAKE_DIRECTORY "${fixture_project}")
file(WRITE "${fixture_project}/CMakeLists.txt"
"cmake_minimum_required(VERSION 3.22)\nproject(torch_failure_terminology_legacy LANGUAGES CXX)\nadd_executable(torch_failure_terminology_legacy \"${FIXTURE}\")\ntarget_include_directories(torch_failure_terminology_legacy PRIVATE \"${SOURCE_DIRECTORY}/src\")\ntarget_compile_features(torch_failure_terminology_legacy PRIVATE cxx_std_20)\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${fixture_project}" -B "${fixture_project}/build" -G "Visual Studio 17 2022" -A x64
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Could not configure compiler-negative fixture.\n${configure_stdout}${configure_stderr}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${fixture_project}/build" --config Debug
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_stdout
    ERROR_VARIABLE compile_stderr
)
file(REMOVE_RECURSE "${fixture_project}")
if(compile_result EQUAL 0)
    message(FATAL_ERROR "Legacy torch-failure identifiers unexpectedly compiled.\n${compile_stdout}${compile_stderr}")
endif()

set(compile_output "${compile_stdout}${compile_stderr}")
foreach(legacy_identifier IN ITEMS LanternPhase LanternSnapshot LanternSequence lanternStrength)
    string(FIND "${compile_output}" "${legacy_identifier}" identifier_offset)
    if(identifier_offset EQUAL -1)
        message(FATAL_ERROR "Compiler rejection did not identify legacy identifier ${legacy_identifier}.\n${compile_output}")
    endif()
endforeach()

message(STATUS "Legacy torch-failure identifiers were rejected by the C++ compiler as required.")
