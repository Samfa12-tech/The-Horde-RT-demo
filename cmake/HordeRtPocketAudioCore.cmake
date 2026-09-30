# Byte-exact, native-only Pocket Audio Core dependency. No build-time download.
function(horde_rt_validate_pocket_audio_core repo_root)
    set(core_root "${repo_root}/third_party/pocket-audio-core")
    set(manifest_path "${core_root}/manifest.json")
    if(NOT EXISTS "${manifest_path}" OR IS_SYMLINK "${manifest_path}")
        message(FATAL_ERROR "Pocket Audio Core manifest is missing or symlinked")
    endif()
    file(SHA256 "${manifest_path}" manifest_hash)
    if(NOT manifest_hash STREQUAL "b0bda2ea83ef811cd19ac14ad76b5b8d36ddf8725d55031d2b22107a7c3d4f28")
        message(FATAL_ERROR "Pocket Audio Core manifest pin mismatch")
    endif()
    file(READ "${manifest_path}" manifest)
    string(JSON repository GET "${manifest}" upstreamRepository)
    string(JSON commit GET "${manifest}" upstreamCommit)
    string(JSON component GET "${manifest}" component)
    if(NOT repository STREQUAL "https://github.com/Samfa12-tech/Pocket-Chordsmith"
       OR NOT commit STREQUAL "534a6e6811ce653efd5422138c5772b967263ed0"
       OR NOT component STREQUAL "packages/pocket-audio-core/native")
        message(FATAL_ERROR "Pocket Audio Core upstream identity mismatch")
    endif()

    set(expected_files
        native/CMakeLists.txt native/README.md
        native/include/pocket_audio/PcmFormat.h
        native/include/pocket_audio/PcmLoopStream.h
        native/include/pocket_audio/PcmWave.h
        native/src/PcmLoopStream.cpp native/src/PcmWave.cpp
        native/tests/PcmNativeTests.cpp
        upstream/LICENSE upstream/LICENSES.md)
    string(JSON file_count LENGTH "${manifest}" files)
    list(LENGTH expected_files expected_count)
    if(NOT file_count EQUAL expected_count)
        message(FATAL_ERROR "Pocket Audio Core file count mismatch")
    endif()
    set(declared_files)
    math(EXPR last_index "${file_count} - 1")
    foreach(index RANGE 0 ${last_index})
        string(JSON relative_path GET "${manifest}" files ${index} path)
        if(NOT relative_path IN_LIST expected_files OR relative_path IN_LIST declared_files)
            message(FATAL_ERROR "Pocket Audio Core unexpected or duplicate path: ${relative_path}")
        endif()
        list(APPEND declared_files "${relative_path}")
        set(path "${core_root}/${relative_path}")
        if(NOT EXISTS "${path}" OR IS_SYMLINK "${path}")
            message(FATAL_ERROR "Pocket Audio Core file is missing or symlinked: ${relative_path}")
        endif()
        string(JSON expected_hash GET "${manifest}" files ${index} sha256)
        string(JSON expected_bytes GET "${manifest}" files ${index} bytes)
        file(SHA256 "${path}" actual_hash)
        file(SIZE "${path}" actual_bytes)
        if(NOT actual_hash STREQUAL expected_hash OR NOT actual_bytes EQUAL expected_bytes)
            message(FATAL_ERROR "Pocket Audio Core file pin mismatch: ${relative_path}")
        endif()
    endforeach()

    # Prevent accidentally importing an app/synth or committing generated output.
    set(expected_inventory ${expected_files} manifest.json README.horde.md)
    file(GLOB_RECURSE actual_inventory LIST_DIRECTORIES FALSE
        RELATIVE "${core_root}" "${core_root}/*")
    list(SORT expected_inventory)
    list(SORT actual_inventory)
    if(NOT actual_inventory STREQUAL expected_inventory)
        message(FATAL_ERROR "Pocket Audio Core native-only inventory mismatch")
    endif()
endfunction()

function(horde_rt_add_pocket_audio_core repo_root with_tests)
    horde_rt_validate_pocket_audio_core("${repo_root}")
    set(POCKET_AUDIO_NATIVE_BUILD_TESTS "${with_tests}")
    add_subdirectory("${repo_root}/third_party/pocket-audio-core/native"
        "${CMAKE_CURRENT_BINARY_DIR}/pocket-audio-core-native")
endfunction()
