cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

include(cmake/HordeRtVersion.cmake)
include(cmake/HordeRtRaygenArtifacts.cmake)
include(cmake/HordeRtRaygenPolicy.cmake)
horde_rt_validate_raygen_policy()

project(HordeLanternRT
    VERSION ${HORDE_RT_PACKAGE_VERSION}
    DESCRIPTION "Native Vulkan hardware ray tracing technology demo"
    LANGUAGES CXX
)

horde_rt_configure_raygen_artifacts("${CMAKE_CURRENT_SOURCE_DIR}")

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
if(BUILD_TESTING)
    # Resolve the runner before registering any PowerShell-backed fixture.  CTest
    # records this exact executable in its generated command line.
    find_program(HORDE_RT_POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
endif()

include(cmake/HordeRtSources.cmake)
include(cmake/HordeRtStrictAssetMath.cmake)
horde_rt_enable_strict_asset_math("${CMAKE_CURRENT_SOURCE_DIR}")
set(HORDE_RT_SOURCE_FILES)
foreach(source IN LISTS HORDE_RT_CORE_RELATIVE_SOURCES)
    list(APPEND HORDE_RT_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/${source}")
endforeach()
list(FILTER HORDE_RT_SOURCE_FILES EXCLUDE REGEX "/gameplay/(simulation/GameSimulation|animation/(PlayerAnimationState|PlayerIkTargets)|effects/FireEmitterState|interactions/(InteractionState|ChestRewardSequence|FinaleSequence)|items/(HeldItemState|HeldItemKinematics|HeldLightState|LanternPendulum))\\.cpp$")

add_library(horde_gameplay_simulation STATIC
    src/gameplay/simulation/GameSimulation.cpp
    src/gameplay/animation/PlayerAnimationState.cpp
    src/gameplay/animation/PlayerIkTargets.cpp
    src/gameplay/effects/FireEmitterState.cpp
    src/gameplay/interactions/InteractionState.cpp
    src/gameplay/interactions/ChestRewardSequence.cpp
    src/gameplay/interactions/FinaleSequence.cpp
    src/gameplay/items/LanternPendulum.cpp
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
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )
    target_compile_features(horde_rt_probe_core PUBLIC cxx_std_20)
    target_link_libraries(horde_rt_probe_core PUBLIC Vulkan::Vulkan horde_gameplay_simulation)
    horde_rt_attach_raygen_artifacts(horde_rt_probe_core)
    horde_rt_apply_raygen_policy(horde_rt_probe_core Windows)
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
    set(HORDE_RT_WINDOWS_RESOURCE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/windows")
    file(MAKE_DIRECTORY "${HORDE_RT_WINDOWS_RESOURCE_DIRECTORY}")
    configure_file(src/platform/windows/HordeLanternRT.rc.in
        "${HORDE_RT_WINDOWS_RESOURCE_DIRECTORY}/HordeLanternRT.rc" @ONLY)
    configure_file(src/platform/windows/HordeLanternRT.manifest.in
        "${HORDE_RT_WINDOWS_RESOURCE_DIRECTORY}/HordeLanternRT.manifest" @ONLY)

    add_executable(horde_rt_diagnostic_window WIN32
        src/platform/windows/DiagnosticWindowMain.cpp
        src/platform/windows/DiagnosticWindow.cpp
        src/platform/windows/WindowsGitHubReleaseUpdate.cpp
        "${HORDE_RT_WINDOWS_RESOURCE_DIRECTORY}/HordeLanternRT.manifest"
        "${HORDE_RT_WINDOWS_RESOURCE_DIRECTORY}/HordeLanternRT.rc"
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
        winhttp
        winmm
    )
    target_compile_definitions(horde_rt_diagnostic_window PRIVATE
        HORDE_RT_DISPLAY_VERSION="${HORDE_RT_DISPLAY_VERSION}"
        HORDE_RT_PACKAGE_VERSION="${HORDE_RT_PACKAGE_VERSION}"
        HORDE_RT_BUILD_ID="${HORDE_RT_BUILD_ID}"
        $<$<CONFIG:Debug>:HORDE_RT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}">
    )
    horde_rt_attach_raygen_artifacts(horde_rt_diagnostic_window)

    set_target_properties(horde_rt_diagnostic_window PROPERTIES
        OUTPUT_NAME "HordeLanternRT"
    )
endif()

if(BUILD_TESTING)
    add_executable(horde_rt_pipeline_variants_tests
        tests/RtPipelineVariantsTests.cpp
        src/vulkan/raytracing/RtPipelineVariants.cpp)
    target_include_directories(horde_rt_pipeline_variants_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(horde_rt_pipeline_variants_tests PRIVATE
        HORDE_RT_SELECTED_INSTRUMENTATION=0 HORDE_RT_SELECTED_DIELECTRIC_QUALITY=0)
    add_test(NAME horde_rt_pipeline_variants_tests COMMAND horde_rt_pipeline_variants_tests)

    add_executable(horde_rt_pipeline_variant_provider_tests
        tests/RtPipelineVariantProviderTests.cpp
        src/vulkan/raytracing/RtPipelineVariants.cpp
        src/vulkan/raytracing/RtPipelineVariantProvider.cpp)
    target_include_directories(horde_rt_pipeline_variant_provider_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(horde_rt_pipeline_variant_provider_tests PRIVATE
        HORDE_RT_SELECTED_INSTRUMENTATION=0 HORDE_RT_SELECTED_DIELECTRIC_QUALITY=0)
    horde_rt_attach_raygen_artifacts(horde_rt_pipeline_variant_provider_tests)
    add_test(NAME horde_rt_pipeline_variant_provider_tests COMMAND horde_rt_pipeline_variant_provider_tests)

    add_executable(horde_rt_pipeline_bundle_contracts_tests
        tests/RtPipelineBundleContractsTests.cpp
        src/vulkan/raytracing/RtPipelineBundleContracts.cpp
        src/vulkan/raytracing/RtPipelineVariants.cpp
        src/vulkan/raytracing/RtPipelineVariantProvider.cpp)
    target_include_directories(horde_rt_pipeline_bundle_contracts_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(horde_rt_pipeline_bundle_contracts_tests PRIVATE
        HORDE_RT_SELECTED_INSTRUMENTATION=0 HORDE_RT_SELECTED_DIELECTRIC_QUALITY=0)
    horde_rt_attach_raygen_artifacts(horde_rt_pipeline_bundle_contracts_tests)
    add_test(NAME horde_rt_pipeline_bundle_contracts_tests COMMAND horde_rt_pipeline_bundle_contracts_tests)

    function(horde_rt_add_pipeline_variant_fixture target_name instrumentation quality opaque_hash generic_hash required_key)
        set(provider_target "horde_rtpv_${instrumentation}_${quality}_obj")
        add_library(${provider_target} OBJECT src/vulkan/raytracing/RtPipelineVariantProvider.cpp)
        target_include_directories(${provider_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_compile_definitions(${provider_target} PRIVATE
            HORDE_RT_SELECTED_INSTRUMENTATION=${instrumentation}
            HORDE_RT_SELECTED_DIELECTRIC_QUALITY=${quality})
        horde_rt_attach_raygen_artifacts(${provider_target})
        add_executable(${target_name}
            tests/RtPipelineVariantFixture.cpp
            src/vulkan/raytracing/RtPipelineBundleContracts.cpp
            src/vulkan/raytracing/RtPipelineVariants.cpp
            $<TARGET_OBJECTS:${provider_target}>)
        target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        add_test(NAME ${target_name} COMMAND ${target_name})
        add_test(NAME ${target_name}_evidence
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RtPipelineVariantFixtureEvidence.ps1
                -Fixture $<TARGET_FILE:${target_name}> -ProviderObject $<TARGET_OBJECTS:${provider_target}>
                -OpaqueSha256 ${opaque_hash} -GenericSha256 ${generic_hash}
                -RequiredKey ${required_key})
    endfunction()
    horde_rt_add_pipeline_variant_fixture(horde_rt_provider_shipping_mobile_fixture 0 0
        7ebdb794794a854b6cb5c44c75dd9a9decd42f4999c44a9cfa78f13b12a13f21
        c52a143c5e0e96f7bf187a9278937a5d17534690cbc0e8e7d70f2d95ae104a01
        shipping_mobile_opaque_fast)
    horde_rt_add_pipeline_variant_fixture(horde_rt_provider_shipping_high_fixture 0 1
        7ebdb794794a854b6cb5c44c75dd9a9decd42f4999c44a9cfa78f13b12a13f21
        1be2b430ba190ae82250693ae079adf3f0712b361914b1355f386fb1f57fc5aa
        shipping_high_opaque_fast)
    horde_rt_add_pipeline_variant_fixture(horde_rt_provider_diagnostic_mobile_fixture 1 0
        d7b0722e572e63bd96e62bae85cc6a5b968ae71bf157526b513b50b56e515c3b
        d8f4c915955836f6693c5c5a073c23155944a8a3048dbfdcee6c3a0c4d8f57f7
        diagnostic_mobile_opaque_fast)
    horde_rt_add_pipeline_variant_fixture(horde_rt_provider_diagnostic_high_fixture 1 1
        d7b0722e572e63bd96e62bae85cc6a5b968ae71bf157526b513b50b56e515c3b
        2b61176f676227adfd13a880cd415478536e1403b46317ea3321dc463bebecd0
        diagnostic_high_opaque_fast)

    add_executable(horde_rt_github_release_updater_tests
        tests/GitHubReleaseUpdaterTests.cpp
        src/update/GitHubReleaseUpdater.cpp
    )
    target_include_directories(horde_rt_github_release_updater_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME horde_rt_github_release_updater_tests COMMAND horde_rt_github_release_updater_tests)

    add_executable(horde_rt_version_contract_tests tests/VersionContractTests.cpp)
    target_compile_definitions(horde_rt_version_contract_tests PRIVATE
        HORDE_RT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HORDE_RT_EXPECTED_VERSION="${HORDE_RT_PACKAGE_VERSION}"
        HORDE_RT_EXPECTED_ANDROID_VERSION_CODE=${HORDE_RT_ANDROID_VERSION_CODE}
    )
    add_test(NAME horde_rt_version_contract_tests COMMAND horde_rt_version_contract_tests)

    add_executable(horde_rt_version_contract_incremental_configure_tests
        tests/VersionContractIncrementalConfigureTests.cpp)
    target_compile_definitions(horde_rt_version_contract_incremental_configure_tests PRIVATE
        HORDE_RT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_test(NAME horde_rt_version_contract_incremental_configure_tests
        COMMAND horde_rt_version_contract_incremental_configure_tests "${CMAKE_COMMAND}")
    set_tests_properties(horde_rt_version_contract_incremental_configure_tests PROPERTIES TIMEOUT 120)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
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
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
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

    add_executable(horde_rt_production_prop_asset_tests tests/ProductionPropAssetTests.cpp)
    target_include_directories(horde_rt_production_prop_asset_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )
    target_sources(horde_rt_production_prop_asset_tests PRIVATE
        src/scene/assets/AssetManifest.cpp
        src/scene/assets/AssetValidation.cpp
        src/scene/assets/GltfDocument.cpp
        src/scene/assets/StaticMeshAsset.cpp
    )
    target_compile_definitions(horde_rt_production_prop_asset_tests PRIVATE
        HORDE_RT_PRODUCTION_PROP_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets/models/props/runtime"
    )
    target_link_libraries(horde_rt_production_prop_asset_tests PRIVATE
        horde_gameplay_simulation
    )
    add_test(NAME horde_rt_production_prop_asset_tests COMMAND horde_rt_production_prop_asset_tests)

    if(WIN32)
        find_program(HORDE_RT_PWSH_EXECUTABLE NAMES pwsh)
        find_program(HORDE_RT_WINDOWS_POWERSHELL_EXECUTABLE NAMES powershell.exe powershell)
        add_test(NAME horde_rt_scene_abi_generator_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RtSceneAbiGeneratorTests.ps1)
        add_test(NAME horde_rt_raygen_compiler_strategy_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RaygenCompilerStrategyTests.ps1)
        add_test(NAME horde_rt_raygen_variant_manifest_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RaygenVariantManifestTests.ps1)
        add_test(NAME horde_rt_raygen_variant_artifact_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RaygenVariantArtifactTests.ps1)
        add_test(NAME horde_rt_pipeline_policy_fence_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RtPipelinePolicyFenceTests.ps1
                -CmakeExecutable ${CMAKE_COMMAND})
        add_test(NAME horde_rt_pipeline_policy_execution_tests
            COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/RtPipelinePolicyExecutionTests.ps1
                -CmakeExecutable ${CMAKE_COMMAND}
                -GradleExecutable ${CMAKE_CURRENT_SOURCE_DIR}/android/gradlew.bat)
        set_tests_properties(
            horde_rt_pipeline_policy_fence_tests
            horde_rt_pipeline_policy_execution_tests
            PROPERTIES TIMEOUT 300)
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
        set(HORDE_RT_VERSION_CONTRACT_POWERSHELL_EXECUTABLE "${HORDE_RT_POWERSHELL_EXECUTABLE}")
        if(HORDE_RT_PWSH_EXECUTABLE)
            set(HORDE_RT_VERSION_CONTRACT_POWERSHELL_EXECUTABLE "${HORDE_RT_PWSH_EXECUTABLE}")
        endif()
        add_test(NAME horde_rt_version_contract_powershell_tests
            COMMAND ${HORDE_RT_VERSION_CONTRACT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/VersionContractPowerShellTests.ps1)
        add_test(NAME horde_rt_github_release_preflight_tests
            COMMAND ${HORDE_RT_VERSION_CONTRACT_POWERSHELL_EXECUTABLE} -NoProfile -File
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/GitHubReleasePreflightTests.ps1)
        if(HORDE_RT_PWSH_EXECUTABLE AND HORDE_RT_WINDOWS_POWERSHELL_EXECUTABLE)
            add_test(NAME horde_rt_version_contract_windows_powershell_tests
                COMMAND ${HORDE_RT_WINDOWS_POWERSHELL_EXECUTABLE} -NoProfile -File
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/VersionContractPowerShellTests.ps1)
        endif()
        set_tests_properties(
            horde_rt_scene_abi_generator_tests
            horde_rt_static_texture_array_tool_tests
            horde_rt_static_preparation_lod_policy_tests
            horde_rt_static_preparation_skip_policy_tests
            horde_rt_static_preparation_colliding_lod_policy_tests
            horde_rt_dielectric_topology_tool_tests
            horde_rt_version_contract_powershell_tests
            horde_rt_github_release_preflight_tests
            PROPERTIES TIMEOUT 120)
        # The compiler and manifest checks preprocess/compile raygen modules,
        # so keep their deliberate CI ceiling confined to shader gates.
        set_tests_properties(
            horde_rt_raygen_compiler_strategy_tests
            horde_rt_raygen_variant_manifest_tests
            PROPERTIES TIMEOUT 600)
        # The artifact gate additionally validates the eight matrix outputs,
        # isolated fixture bundles, and two compatibility modules. Retain a
        # finite ceiling above that complete bounded workload only.
        set_tests_properties(horde_rt_raygen_variant_artifact_tests PROPERTIES TIMEOUT 1200)
        if(HORDE_RT_PWSH_EXECUTABLE AND HORDE_RT_WINDOWS_POWERSHELL_EXECUTABLE)
            set_tests_properties(horde_rt_version_contract_windows_powershell_tests PROPERTIES TIMEOUT 120)
        endif()
    endif()

    add_executable(horde_rt_simulation_timing_tests tests/SimulationTimingTests.cpp)
    target_include_directories(horde_rt_simulation_timing_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_simulation_timing_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_simulation_timing_tests COMMAND horde_rt_simulation_timing_tests)

    add_executable(horde_rt_simulation_gameplay_tests tests/SimulationGameplayTests.cpp)
    target_include_directories(horde_rt_simulation_gameplay_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_simulation_gameplay_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_simulation_gameplay_tests COMMAND horde_rt_simulation_gameplay_tests)

    add_executable(horde_rt_lantern_reward_tests tests/LanternRewardTests.cpp)
    target_include_directories(horde_rt_lantern_reward_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_lantern_reward_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_lantern_reward_tests COMMAND horde_rt_lantern_reward_tests)

    add_executable(horde_rt_lantern_pendulum_tests tests/LanternPendulumTests.cpp)
    target_include_directories(horde_rt_lantern_pendulum_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(horde_rt_lantern_pendulum_tests PRIVATE horde_gameplay_simulation)
    add_test(NAME horde_rt_lantern_pendulum_tests COMMAND horde_rt_lantern_pendulum_tests)

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
    target_link_libraries(horde_rt_showcase_gameplay_smoke PRIVATE horde_gameplay_simulation)
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
        add_executable(horde_rt_pipeline_bundle_lifetime_tests
            tests/RtPipelineBundleLifetimeTests.cpp
            src/vulkan/raytracing/RtPipelineBundle.cpp
            src/vulkan/raytracing/RtPipelineBundleContracts.cpp
            src/vulkan/raytracing/RtPipelineVariants.cpp
            src/vulkan/raytracing/RtPipelineVariantProvider.cpp)
        target_include_directories(horde_rt_pipeline_bundle_lifetime_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(horde_rt_pipeline_bundle_lifetime_tests PRIVATE Vulkan::Vulkan)
        horde_rt_apply_raygen_policy(horde_rt_pipeline_bundle_lifetime_tests Windows)
        horde_rt_attach_raygen_artifacts(horde_rt_pipeline_bundle_lifetime_tests)
        add_test(NAME horde_rt_pipeline_bundle_lifetime_tests COMMAND horde_rt_pipeline_bundle_lifetime_tests)

        add_executable(horde_rt_scene_initialise_preflight_tests
            tests/RtSceneInitialisePreflightTests.cpp)
        target_include_directories(horde_rt_scene_initialise_preflight_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(horde_rt_scene_initialise_preflight_tests PRIVATE
            horde_rt_probe_core)
        add_test(NAME horde_rt_scene_initialise_preflight_tests
            COMMAND horde_rt_scene_initialise_preflight_tests)

        if(TARGET horde_rt_diagnostic_window)
            add_test(NAME horde_rt_final_windows_bundle_containment
                COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                    ${CMAKE_CURRENT_SOURCE_DIR}/tools/InspectRtPipelineBundleContainment.ps1
                    -TargetPath $<TARGET_FILE:horde_rt_diagnostic_window>
                    -Instrumentation $<IF:$<CONFIG:Debug>,Diagnostic,Shipping>
                    -Quality High)
            add_test(NAME horde_rt_final_windows_bundle_containment_controls
                COMMAND ${HORDE_RT_POWERSHELL_EXECUTABLE} -NoProfile -File
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/RtPipelineBundleContainmentTests.ps1
                    -Scanner ${CMAKE_CURRENT_SOURCE_DIR}/tools/InspectRtPipelineBundleContainment.ps1
                    -TargetPath $<TARGET_FILE:horde_rt_diagnostic_window>
                    -Instrumentation $<IF:$<CONFIG:Debug>,Diagnostic,Shipping>
                    -Quality High
                    -PowerShellExecutable ${HORDE_RT_POWERSHELL_EXECUTABLE})
            set_tests_properties(
                horde_rt_final_windows_bundle_containment
                horde_rt_final_windows_bundle_containment_controls
                PROPERTIES TIMEOUT 300)
        endif()

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
