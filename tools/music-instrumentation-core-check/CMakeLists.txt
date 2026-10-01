cmake_minimum_required(VERSION 3.24)
project(HordeInstrumentationCoreCheck LANGUAGES CXX)

# Isolated audition check: do not configure the unfinished renderer worktree.
get_filename_component(HORDE_REPO "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include("${HORDE_REPO}/cmake/HordeRtPocketAudioCore.cmake")
horde_rt_add_pocket_audio_core("${HORDE_REPO}" OFF)
add_executable(horde_music_instrumentation_core_check
    check.cpp
    "${HORDE_REPO}/src/audio/MusicPcmStream.cpp"
    "${HORDE_REPO}/src/audio/MusicPcmWave.cpp")
target_include_directories(horde_music_instrumentation_core_check PRIVATE "${HORDE_REPO}/src")
target_link_libraries(horde_music_instrumentation_core_check PRIVATE pocket_audio_native_pcm)
target_compile_features(horde_music_instrumentation_core_check PRIVATE cxx_std_20)
if(MSVC)
    # SimulationSnapshot transitively includes the deliberately aligned RT ABI.
    # C4324 reports that required padding, not an audition-memory defect.
    target_compile_options(horde_music_instrumentation_core_check PRIVATE /W4 /WX /wd4324 /permissive- /EHsc)
else()
    target_compile_options(horde_music_instrumentation_core_check PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Reuse the existing real-WAV and transactional-bank suites against an isolated
# staged palette. No changes to their contracts or the game's build graph.
set(HORDE_MUSIC_TEST_ASSETS "" CACHE PATH "Optional staged assets root for existing native fixtures")
if(HORDE_MUSIC_TEST_ASSETS)
    enable_testing()
    foreach(fixture IN ITEMS wave asset_bank)
        if(fixture STREQUAL "wave")
            set(test_source MusicPcmWaveTests.cpp)
        else()
            set(test_source MusicPcmAssetBankTests.cpp)
        endif()
        set(test_target horde_music_staged_${fixture}_tests)
        add_executable(${test_target} "${HORDE_REPO}/tests/${test_source}"
            "${HORDE_REPO}/src/audio/MusicPcmWave.cpp"
            "${HORDE_REPO}/src/audio/MusicPcmStream.cpp"
            "${HORDE_REPO}/src/audio/MusicPcmAssetBank.cpp")
        target_include_directories(${test_target} PRIVATE "${HORDE_REPO}/src")
        target_compile_definitions(${test_target} PRIVATE HORDE_RT_MUSIC_ASSET_DIR="${HORDE_MUSIC_TEST_ASSETS}")
        target_link_libraries(${test_target} PRIVATE pocket_audio_native_pcm)
        target_compile_features(${test_target} PRIVATE cxx_std_20)
        if(MSVC)
            target_compile_options(${test_target} PRIVATE /W4 /WX /wd4324 /permissive- /EHsc)
        else()
            target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()
        add_test(NAME ${test_target} COMMAND ${test_target})
    endforeach()
endif()
