"""Exercise experimental CMake admission without Vulkan or an Android device."""
import pathlib
import subprocess
import sys
import tempfile

cmake = sys.argv[1]
repo = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="horde-region-policy-") as temporary:
    base = pathlib.Path(temporary)
    source = base / "fixture"
    source.mkdir()
    (source / "CMakeLists.txt").write_text(f'''
cmake_minimum_required(VERSION 3.22)
project(RegionPolicy NONE)
# Configure-only policy fixture: record expected target directives. Real
# Windows/Android target compilation is validated separately.
function(target_include_directories target priority visibility root)
    if(NOT priority STREQUAL "BEFORE" OR NOT visibility STREQUAL "PRIVATE" OR NOT IS_ABSOLUTE "${{root}}")
        message(FATAL_ERROR "wrong private include policy")
    endif()
endfunction()
function(target_compile_definitions target visibility definition)
    if(NOT visibility STREQUAL "PUBLIC")
        message(FATAL_ERROR "wrong consumer ABI propagation")
    endif()
    set_property(GLOBAL PROPERTY admitted_defs "${{definition}}")
endfunction()
include("{(repo / 'cmake/HordeRtRegionPages.cmake').as_posix()}")
horde_rt_attach_private_region_pages(candidate)
get_property(defs GLOBAL PROPERTY admitted_defs)
message(STATUS "admitted-definitions=${{defs}}")
''')
    private = base / "private"
    headers = private / "src/vulkan/raytracing"
    headers.mkdir(parents=True)
    for name in ("RtPipelineVariantCatalog.generated.h", "RtRayQueryVariantCatalog.generated.h"):
        (headers / name).write_text("// fixture presence only; native preflight tests validate real catalogs\n")
    abi = private / "shaders/raytracing/include/rt_scene_abi.generated.glsl"
    abi.parent.mkdir(parents=True)
    abi.write_text("uvec4 pages[8];\n")
    def case(name, args, success, witness):
        result = subprocess.run([cmake, "-S", str(source), "-B", str(base / name), *args],
            capture_output=True, text=True, timeout=20)
        output = result.stdout + result.stderr
        assert (result.returncode == 0) == success and witness in output, (name, output)
    root = f"-DHORDE_RT_REGION_PAGE_PROTOTYPE_ROOT={private.as_posix()}"
    case("production", [], True, "admitted-definitions=\n")
    case("desktop", [root], True, "HORDE_RT_REGION_PAGE_PROTOTYPE=1")
    android = [root, "-DANDROID=ON", "-DCMAKE_BUILD_TYPE=Debug", "-DHORDE_RT_DEBUG_CHECKPOINTS=ON"]
    case("android-debug", android, True, "HORDE_RT_REGION_PAGE_PROTOTYPE=1")
    case("android-release", [root, "-DANDROID=ON", "-DCMAKE_BUILD_TYPE=Release", "-DHORDE_RT_DEBUG_CHECKPOINTS=ON"],
        False, "requires Debug checkpoint-enabled builds")
    case("android-no-checkpoints", [root, "-DANDROID=ON", "-DCMAKE_BUILD_TYPE=Debug"],
        False, "requires Debug checkpoint-enabled builds")
    case("relative", ["-DHORDE_RT_REGION_PAGE_PROTOTYPE_ROOT=relative"], False, "absolute private root")
    abi.write_text("uvec4 pages[16];\n")
    case("wrong-abi", [root], False, "admitted region address table")
    abi.write_text("uvec4 pages[8];\n")
    (headers / "RtRayQueryVariantCatalog.generated.h").unlink()
    case("missing-compute", [root], False, "missing RtRayQueryVariantCatalog.generated.h")
print("Production isolation and private Windows/Android build admission:8/8 passed; no device claim.")
