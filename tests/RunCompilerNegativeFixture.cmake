if(NOT DEFINED CXX_COMPILER OR NOT DEFINED CXX_COMPILER_ID OR NOT DEFINED SOURCE_DIRECTORY OR NOT DEFINED FIXTURE)
    message(FATAL_ERROR "Compiler-negative fixture requires CXX_COMPILER, CXX_COMPILER_ID, SOURCE_DIRECTORY, and FIXTURE.")
endif()

if(NOT CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Compiler-negative fixture is currently defined for the Windows MSVC Host gate.")
endif()

set(object_file "${CMAKE_CURRENT_BINARY_DIR}/torch_failure_terminology_legacy.obj")
execute_process(
    COMMAND "${CXX_COMPILER}" /nologo /std:c++20 /EHsc "/I${SOURCE_DIRECTORY}/src" /c "${FIXTURE}" "/Fo${object_file}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_stdout
    ERROR_VARIABLE compile_stderr
)

file(REMOVE "${object_file}")
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
