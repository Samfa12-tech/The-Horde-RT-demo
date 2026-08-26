cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(HordeLanternRT
    VERSION 1.5.2
    DESCRIPTION "Native Vulkan hardware ray tracing technology demo"
    LANGUAGES CXX
)

set(HORDE_RT_EMBEDDED_RAYGEN "${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/raytracing/MinimalRayGenShader.inc")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HORDE_RT_EMBEDDED_RAYGEN}")
file(SHA256 "${HORDE_RT_EMBEDDED_RAYGEN}" HORDE_RT_RAYGEN_SHA256)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(HORDE_RT_BUILD_CAPABILITY_PROBE_EXECUTABLE "Build the standalone command line Vulkan capability probe" ON)
option(HORDE_RT_BUILD_WINDOWS_DIAGNOSTIC_WINDOW "Build the Win32 diagnostic shell" ON)
option(HORDE_RT_BUILD_VULKAN_TARGETS "Build targets that require a Vulkan SDK" ON)

include(CTest)

include(cmake/HordeRtSources.cmake)
include(cmake/HordeRtStrictAssetMath.cmake)
horde_rt_enable_strict_asset_math("${CMAKE_CURRENT_SOURCE_DIR}")
set(HORDE_RT_SOURCE_FILES)
foreach(source IN LISTS HORDE_RT_CORE_RELATIVE_SOURCES)
    list(APPEND HORDE_RT_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/${source}")
endforeach()
list(FILTER HORDE_RT_SOURCE_FILES EXCLUDE REGEX "/gameplay/(simulation/GameSimulation|animation/(PlayerAnimationState|PlayerIkTargets)|effects/FireEmitterState|items/(HeldItemState|HeldItemKinematics|HeldLightState))\\.cpp$")

add_library(horde_gameplay_simulation STATIC
    src/gameplay/simulation/GameSimulation.cpp
    src/gameplay/animation/PlayerAnimationState.cpp
    src/gameplay/animation/PlayerIkTargets.cpp
    src/gameplay/effects/FireEmitterState.cpp
    src/gameplay/items/HeldItemState.cpp
    src/gameplay/items/HeldItemKinematics.cpp
    src/gameplay/items/HeldLightState.cpp
)
target_include_directories(horde_gameplay_simulation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(horde_gameplay_simulation PUBLIC cxx_std_20)

if(HORDE_RT_BUILD_VULKAN_TARGETS)
    find_package(Vulkan REQUIRED)
    message(STATUS "Vulkan SDK found: ${Vulkan_VERSION}")

    add_library(horde_rt_probe_core STATIC
        ${HORDE_RT_SOURCE_FILES}
    )

    target_include_directories(horde_rt_probe_core
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_features(horde_rt_probe_core PUBLIC cxx_std_20)
    target_link_libraries(horde_rt_probe_core PUBLIC Vulkan::Vulkan horde_gameplay_simulation)
endif()

if(HORDE_RT_BUILD_VULKAN_TARGETS AND HORDE_RT_BUILD_CAPABILITY_PROBE_EXECUTABLE)
    add_executable(horde_rt_capability_probe
        src/platform/windows/CapabilityProbeMain.cpp
        src/ui/DiagnosticOverlay.cpp
    )

    target_include_directories(horde_rt_capability_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(horde_rt_capability_probe PRIVATE
        horde_rt_probe_core
    )

    set_target_properties(horde_rt_capability_probe PROPERTIES
        OUTPUT_NAME "horde_rt_capability_probe"
    )
endif()

if(HORDE_RT_BUILD_VULKAN_TARGETS AND HORDE_RT_BUILD_WINDOWS_DIAGNOSTIC_WINDOW AND WIN32)
    add_executable(horde_rt_diagnostic_window WIN32
        src/platform/windows/DiagnosticWindowMain.cpp
        src/platform/windows/DiagnosticWindow.cpp
        src/platform/windows/HordeLanternRT.manifest
        src/platform/windows/HordeLanternRT.rc
        src/ui/DiagnosticOverlay.cpp
    )

    target_include_directories(horde_rt_diagnostic_window PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(horde_rt_diagnostic_window PRIVATE
        horde_rt_probe_core
        comdlg32
        comctl32
        bcrypt
        ole32
        windowscodecs
        shell32
        winmm
    )
    target_compile_definitions(horde_rt_diagnostic_window PRIVATE
        HORDE_RT_DISPLAY_VERSION="${HORDE_RT_DISPLAY_VERSION}"
        HORDE_RT_BUILD_ID="${HORDE_RT_PACKAGE_VERSION}"
        HORDE_RT_RAYGEN_SHA256="${HORDE_RT_RAYGEN_SHA256}"
        $<$<CONFIG:Debug>:HORDE_RT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}">
    )

    set_target_properties(horde_rt_diagnostic_window PROPERTIES
        OUTPUT_NAME "HordeLanternRT"
    )
endif()

if(BUILD_TESTING)
    add_executable(horde_rt_dielectric_math_tests tests/DielectricMathTests.cpp)
    target_include_directories(horde_rt_dielectric_math_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_dielectric_math_tests COMMAND horde_rt_dielectric_math_tests)

    add_executable(horde_rt_player_animation_tests
        tests/PlayerAnimationTests.cpp
        src/scene/assets/SkinnedMeshAsset.cpp
        src/vulkan/raytracing/PlayerRenderSlot.cpp
    )
    target_include_directories(horde_rt_player_animation_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_player_animation_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_player_animation_tests COMMAND horde_rt_player_animation_tests)

    add_executable(horde_rt_fire_emitter_tests
        tests/FireEmitterTests.cpp
        src/vulkan/raytracing/FireEmitterBuffer.cpp
    )
    target_include_directories(horde_rt_fire_emitter_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_fire_emitter_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_fire_emitter_tests COMMAND horde_rt_fire_emitter_tests)

    add_executable(horde_rt_held_item_socket_tests tests/HeldItemSocketTests.cpp)
    target_include_directories(horde_rt_held_item_socket_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_sources(horde_rt_held_item_socket_tests PRIVATE
        src/scene/assets/AssetManifest.cpp
        src/scene/assets/AssetValidation.cpp
        src/scene/assets/GltfDocument.cpp
        src/scene/assets/StaticMeshAsset.cpp
        src/vulkan/raytracing/HeldItemRenderSlot.cpp
        src/vulkan/raytracing/RtStaticMeshSlot.cpp
    )
    target_link_libraries(horde_rt_held_item_socket_tests PRIVATE horde_gameplay_simulation)
    target_compile_definitions(horde_rt_held_item_socket_tests PRIVATE
        HORDE_RT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_test(NAME horde_rt_held_item_socket_tests COMMAND horde_rt_held_item_socket_tests)

    add_executable(horde_rt_development_static_asset_tests tests/DevelopmentStaticAssetTests.cpp)
    target_include_directories(horde_rt_development_static_asset_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_development_static_asset_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_development_static_asset_tests COMMAND horde_rt_development_static_asset_tests)

    add_executable(horde_rt_scene_abi_tests tests/RtSceneAbiTests.cpp)
    target_include_directories(horde_rt_scene_abi_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_sources(horde_rt_scene_abi_tests PRIVATE
        src/vulkan/raytracing/RtStaticMeshSlot.cpp
        src/vulkan/raytracing/RtTextureArrays.cpp
    )
    add_test(NAME horde_rt_scene_abi_tests COMMAND horde_rt_scene_abi_tests)

    add_executable(horde_rt_static_gltf_asset_tests tests/StaticGltfAssetTests.cpp)
    target_include_directories(horde_rt_static_gltf_asset_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_sources(horde_rt_static_gltf_asset_tests PRIVATE
        src/scene/assets/AssetManifest.cpp
        src/scene/assets/AssetValidation.cpp
        src/scene/assets/GltfDocument.cpp
        src/scene/assets/StaticMeshAsset.cpp
    )
    target_compile_definitions(horde_rt_static_gltf_asset_tests PRIVATE
        HORDE_RT_STATIC_GLTF_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/static-gltf"
        HORDE_RT_DIELECTRIC_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/models/props/runtime/dielectric-fixture"
    )
    add_test(NAME horde_rt_static_gltf_asset_tests COMMAND horde_rt_static_gltf_asset_tests)

    if(WIN32)
        find_program(HORDE_RT_POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
        add_test(NAME horde_rt_scene_abi_generator_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RtSceneAbiGeneratorTests.ps1)
        add_test(NAME horde_rt_raygen_compiler_strategy_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RaygenCompilerStrategyTests.ps1)
        add_test(NAME horde_rt_static_texture_array_tool_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/StaticTextureArrayToolTests.ps1)
        add_test(NAME horde_rt_static_preparation_lod_policy_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/StaticAssetPreparationPolicyTests.ps1 -Mode LodBudget)
        add_test(NAME horde_rt_static_preparation_skip_policy_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/StaticAssetPreparationPolicyTests.ps1 -Mode UnsafeSkip)
        add_test(NAME horde_rt_static_preparation_colliding_lod_policy_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/StaticAssetPreparationPolicyTests.ps1 -Mode CollidingLod)
        add_test(NAME horde_rt_dielectric_topology_tool_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/DielectricTopologyToolTests.ps1)
        set_tests_properties(
            horde_rt_scene_abi_generator_tests
            horde_rt_raygen_compiler_strategy_tests
            horde_rt_static_texture_array_tool_tests
            horde_rt_static_preparation_lod_policy_tests
            horde_rt_static_preparation_skip_policy_tests
            horde_rt_static_preparation_colliding_lod_policy_tests
            horde_rt_dielectric_topology_tool_tests
            PROPERTIES TIMEOUT 120)
    endif()

    add_executable(horde_rt_simulation_timing_tests tests/SimulationTimingTests.cpp)
    target_include_directories(horde_rt_simulation_timing_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_simulation_timing_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_simulation_timing_tests COMMAND horde_rt_simulation_timing_tests)

    add_executable(horde_rt_simulation_gameplay_tests tests/SimulationGameplayTests.cpp)
    target_include_directories(horde_rt_simulation_gameplay_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_simulation_gameplay_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_simulation_gameplay_tests COMMAND horde_rt_simulation_gameplay_tests)

    add_executable(horde_rt_input_mailbox_stress tests/InputMailboxStress.cpp)
    target_include_directories(horde_rt_input_mailbox_stress PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_input_mailbox_stress COMMAND horde_rt_input_mailbox_stress)

    add_executable(horde_rt_desktop_controller_input_tests tests/DesktopControllerInputTests.cpp)
    target_include_directories(horde_rt_desktop_controller_input_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_desktop_controller_input_tests COMMAND horde_rt_desktop_controller_input_tests)

    add_executable(horde_rt_gpu_timestamp_math_tests tests/GpuTimestampMathTests.cpp)
    target_include_directories(horde_rt_gpu_timestamp_math_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_gpu_timestamp_math_tests COMMAND horde_rt_gpu_timestamp_math_tests)

    add_executable(horde_rt_combat_smoke tests/SwordCombatSmoke.cpp)
    target_include_directories(horde_rt_combat_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_combat_smoke COMMAND horde_rt_combat_smoke)

    add_executable(horde_rt_showcase_route_smoke tests/ShowcaseRouteSmoke.cpp)
    target_include_directories(horde_rt_showcase_route_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_showcase_route_smoke COMMAND horde_rt_showcase_route_smoke)

    add_executable(horde_rt_showcase_gameplay_smoke tests/ShowcaseGameplaySmoke.cpp)
    target_include_directories(horde_rt_showcase_gameplay_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_showcase_gameplay_smoke COMMAND horde_rt_showcase_gameplay_smoke)

    # This must use CMake's configured compiler and generator: the shared host
    # lane configures BUILD_TESTING on Linux as well as the Windows Host gate.
    try_compile(HORDE_RT_LEGACY_TORCH_FAILURE_IDENTIFIERS_COMPILED
        "${CMAKE_BINARY_DIR}/CMakeFiles/torch_failure_terminology_legacy"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/torch_failure_terminology_legacy.cpp"
        CMAKE_FLAGS
            "-DCMAKE_CXX_STANDARD:STRING=20"
            "-DCMAKE_CXX_STANDARD_REQUIRED:BOOL=ON"
            "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_CURRENT_SOURCE_DIR}/src"
        OUTPUT_VARIABLE HORDE_RT_LEGACY_TORCH_FAILURE_IDENTIFIERS_OUTPUT
    )
    if(HORDE_RT_LEGACY_TORCH_FAILURE_IDENTIFIERS_COMPILED)
        message(FATAL_ERROR "Legacy torch-failure identifiers unexpectedly compiled.\n${HORDE_RT_LEGACY_TORCH_FAILURE_IDENTIFIERS_OUTPUT}")
    endif()
    foreach(legacy_identifier IN ITEMS LanternPhase LanternSnapshot LanternSequence lanternStrength)
        string(FIND "${HORDE_RT_LEGACY_TORCH_FAILURE_IDENTIFIERS_OUTPUT}" "${legacy_identifier}" legacy_identifier_offset)
        if(legacy_identifier_offset EQUAL -1)
            message(FATAL_ERROR "Compiler rejection did not identify legacy identifier ${legacy_identifier}.\n${HORDE_RT_LEGACY_TORCH_FAILURE_IDENTIFIERS_OUTPUT}")
        endif()
    endforeach()
    message(STATUS "Legacy torch-failure identifiers were rejected by the configured ${CMAKE_CXX_COMPILER_ID} compiler.")

    add_executable(horde_rt_spatial_audio_smoke tests/SpatialAudioSmoke.cpp)
    target_include_directories(horde_rt_spatial_audio_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_spatial_audio_smoke COMMAND horde_rt_spatial_audio_smoke)

    if(HORDE_RT_BUILD_VULKAN_TARGETS)
        add_executable(horde_rt_skinned_character_smoke tests/SkinnedCharacterModelSmoke.cpp)
        target_include_directories(horde_rt_skinned_character_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(horde_rt_skinned_character_smoke PRIVATE horde_rt_probe_core)
        add_test(NAME horde_rt_skinned_character_smoke COMMAND horde_rt_skinned_character_smoke)

        add_executable(horde_rt_character_render_slot_smoke tests/CharacterRenderSlotSmoke.cpp)
        target_include_directories(horde_rt_character_render_slot_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(horde_rt_character_render_slot_smoke PRIVATE horde_rt_probe_core)
        add_test(NAME horde_rt_character_render_slot_smoke COMMAND horde_rt_character_render_slot_smoke)

        add_executable(horde_rt_player_skinning_benchmark tests/PlayerSkinningBenchmark.cpp)
        target_include_directories(horde_rt_player_skinning_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(horde_rt_player_skinning_benchmark PRIVATE horde_rt_probe_core)

        add_executable(horde_rt_developer_overlay_smoke
            tests/DeveloperOverlaySmoke.cpp
            src/ui/DiagnosticOverlay.cpp
        )
        target_include_directories(horde_rt_developer_overlay_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(horde_rt_developer_overlay_smoke PRIVATE horde_rt_probe_core)
        add_test(NAME horde_rt_developer_overlay_smoke COMMAND horde_rt_developer_overlay_smoke)

        add_executable(horde_rt_showcase_benchmark_smoke tests/ShowcaseBenchmarkSmoke.cpp)
        target_include_directories(horde_rt_showcase_benchmark_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(horde_rt_showcase_benchmark_smoke PRIVATE horde_rt_probe_core)
        add_test(NAME horde_rt_showcase_benchmark_smoke COMMAND horde_rt_showcase_benchmark_smoke)
    endif()
endif()

if(HORDE_RT_BUILD_VULKAN_TARGETS)
    message(STATUS "Horde Lantern RT: Vulkan capability probe and Windows diagnostic targets are available; Android builds from android/.")
else()
    message(STATUS "Horde Lantern RT: configuring shared host gameplay tests without Vulkan targets.")
endif()
