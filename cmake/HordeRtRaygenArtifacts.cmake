# Frozen shader artifacts are tracked as configure dependencies only.  Runtime
# pipeline selection deliberately remains on the two compatibility includes.
function(horde_rt_configure_raygen_artifacts repository_root)
    set(HORDE_RT_RAYGEN_COMPATIBILITY_ARTIFACTS
        "${repository_root}/src/vulkan/raytracing/MinimalRayGenShader.inc"
        "${repository_root}/src/vulkan/raytracing/MinimalLegacyRayGenShader.inc"
        PARENT_SCOPE)
    set(HORDE_RT_RAYGEN_VARIANT_ARTIFACTS
        "${repository_root}/src/vulkan/raytracing/variants/diagnostic_high_generic_dielectric.inc"
        "${repository_root}/src/vulkan/raytracing/variants/diagnostic_high_opaque_fast.inc"
        "${repository_root}/src/vulkan/raytracing/variants/diagnostic_mobile_generic_dielectric.inc"
        "${repository_root}/src/vulkan/raytracing/variants/diagnostic_mobile_opaque_fast.inc"
        "${repository_root}/src/vulkan/raytracing/variants/shipping_high_generic_dielectric.inc"
        "${repository_root}/src/vulkan/raytracing/variants/shipping_high_opaque_fast.inc"
        "${repository_root}/src/vulkan/raytracing/variants/shipping_mobile_generic_dielectric.inc"
        "${repository_root}/src/vulkan/raytracing/variants/shipping_mobile_opaque_fast.inc"
        PARENT_SCOPE)
    set(HORDE_RT_RAYGEN_ARTIFACT_AUTHORITIES
        "${repository_root}/tools/raygen-variant-catalog.json"
        "${repository_root}/tools/raygen-variants.json"
        "${repository_root}/tools/raygen-variant-budgets.json"
        "${repository_root}/tools/compile-raygen.ps1"
        PARENT_SCOPE)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${repository_root}/src/vulkan/raytracing/MinimalRayGenShader.inc"
        "${repository_root}/src/vulkan/raytracing/MinimalLegacyRayGenShader.inc"
        "${repository_root}/src/vulkan/raytracing/variants/diagnostic_high_generic_dielectric.inc"
        "${repository_root}/src/vulkan/raytracing/variants/diagnostic_high_opaque_fast.inc"
        "${repository_root}/src/vulkan/raytracing/variants/diagnostic_mobile_generic_dielectric.inc"
        "${repository_root}/src/vulkan/raytracing/variants/diagnostic_mobile_opaque_fast.inc"
        "${repository_root}/src/vulkan/raytracing/variants/shipping_high_generic_dielectric.inc"
        "${repository_root}/src/vulkan/raytracing/variants/shipping_high_opaque_fast.inc"
        "${repository_root}/src/vulkan/raytracing/variants/shipping_mobile_generic_dielectric.inc"
        "${repository_root}/src/vulkan/raytracing/variants/shipping_mobile_opaque_fast.inc"
        "${repository_root}/tools/raygen-variant-catalog.json"
        "${repository_root}/tools/raygen-variants.json"
        "${repository_root}/tools/raygen-variant-budgets.json"
        "${repository_root}/tools/compile-raygen.ps1")
endfunction()

function(horde_rt_attach_raygen_artifacts target_name)
    target_sources(${target_name} PRIVATE ${HORDE_RT_RAYGEN_VARIANT_ARTIFACTS})
    set_source_files_properties(${HORDE_RT_RAYGEN_VARIANT_ARTIFACTS} PROPERTIES HEADER_FILE_ONLY TRUE)
endfunction()
