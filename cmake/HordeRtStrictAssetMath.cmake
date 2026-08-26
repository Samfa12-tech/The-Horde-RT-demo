function(horde_rt_enable_strict_asset_math repo_root)
    get_filename_component(asset_math_repo_root "${repo_root}" ABSOLUTE)
    set(asset_math_sources
        "${asset_math_repo_root}/src/scene/assets/GltfDocument.cpp"
        "${asset_math_repo_root}/src/scene/assets/StaticMeshAsset.cpp"
    )

    if(MSVC)
        set(asset_math_options "/fp:strict")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
        set(asset_math_options "-fno-fast-math" "-ffp-contract=off")
    else()
        message(FATAL_ERROR
            "No strict, non-contracted asset-transform arithmetic policy is defined for "
            "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}.")
    endif()

    # These source properties deliberately apply to every target in the calling
    # directory that recompiles either production implementation unit. Keeping
    # the policy per-source avoids changing unrelated renderer hot-path math.
    foreach(asset_math_source IN LISTS asset_math_sources)
        if(NOT EXISTS "${asset_math_source}")
            message(FATAL_ERROR "Strict asset-transform source is missing: ${asset_math_source}")
        endif()
        set_property(SOURCE "${asset_math_source}" APPEND PROPERTY
            COMPILE_OPTIONS ${asset_math_options})
    endforeach()

    message(STATUS
        "Strict non-contracted asset-transform arithmetic enabled for "
        "GltfDocument.cpp and StaticMeshAsset.cpp: ${asset_math_options}")
endfunction()
