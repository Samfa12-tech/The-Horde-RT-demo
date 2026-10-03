cmake_minimum_required(VERSION 3.20)
project(pocket_audio_native_pcm LANGUAGES CXX)

add_library(pocket_audio_native_pcm STATIC
    src/PcmLoopStream.cpp
    src/PcmWave.cpp
)
target_include_directories(pocket_audio_native_pcm PUBLIC include)
target_compile_features(pocket_audio_native_pcm PUBLIC cxx_std_20)
set_target_properties(pocket_audio_native_pcm PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(pocket_audio_native_pcm PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(pocket_audio_native_pcm PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CTest)
    option(POCKET_AUDIO_NATIVE_BUILD_TESTS "Build Pocket Audio native PCM tests" ${BUILD_TESTING})
else()
    option(POCKET_AUDIO_NATIVE_BUILD_TESTS "Build Pocket Audio native PCM tests" OFF)
endif()

if(POCKET_AUDIO_NATIVE_BUILD_TESTS)
    enable_testing()
    add_executable(pocket_audio_native_pcm_tests tests/PcmNativeTests.cpp)
    target_link_libraries(pocket_audio_native_pcm_tests PRIVATE pocket_audio_native_pcm)
    target_compile_features(pocket_audio_native_pcm_tests PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(pocket_audio_native_pcm_tests PRIVATE /W4 /permissive- /EHsc)
    else()
        target_compile_options(pocket_audio_native_pcm_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME pocket_audio_native_pcm_tests COMMAND pocket_audio_native_pcm_tests)
endif()
