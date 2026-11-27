"""Prepare a private RT shader copy that records which production rays hit B01 furniture."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil


def replace(path, old, new, count=1):
    source = path.read_text(encoding="utf-8")
    if source.count(old) != count:
        raise ValueError(f"Unexpected {path.name} source for {old[:50]}")
    path.write_text(source.replace(old, new), encoding="utf-8", newline="\n")


def witness_hit(path, hit, category, count=1):
    source = path.read_text(encoding="utf-8")
    pattern = rf"( *HitInfo {hit} = traceScene\([^;]+;)"

    def add_witness(match):
        indent = " " * (len(match[1]) - len(match[1].lstrip()))
        return match[1] + f"\n{indent}WITNESS_HIT({hit}, {category});"

    observed, matches = re.subn(pattern, add_witness, source)
    if matches != count:
        raise ValueError(f"Unexpected {path.name} trace site count for {hit}: {matches}")
    path.write_text(observed, encoding="utf-8", newline="\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    private = args.output.resolve()
    # The copied generators publish files inside this new private tree.
    if not private.is_relative_to(root / "build") or private.exists():
        parser.error("Use a new private directory under the repository build directory.")
    shutil.copytree(root / "shaders", private / "shaders")
    (private / "tools").mkdir()
    for name in (
        "compile-raygen.ps1", "raygen-variants.json", "raygen-variant-budgets.json",
        "GenerateRtPipelineVariantCatalog.ps1", "GenerateRayQueryComputeVariants.ps1",
    ):
        shutil.copy2(root / "tools" / name, private / "tools" / name)
    (private / "src/vulkan/raytracing/variants").mkdir(parents=True)

    include = private / "shaders/raytracing/include"
    frame = include / "rt_frame.glsl"
    lighting = include / "rt_lighting.glsl"
    transport = include / "rt_dielectric_transport.glsl"
    common = include / "rt_dielectric_common.glsl"
    witness = """#if HORDE_RT_VARIANT_INSTRUMENTATION == HORDE_RT_INSTRUMENTATION_DIAGNOSTIC
uint furnitureRayBits = 0u;

void recordFurnitureRay(uint instance, uint category)
{
    // This ID is the registered split B01 furniture instance, shared by all tiers.
    if (rtInstances.values[instance].stableObjectId == 0x42303146u)
        furnitureRayBits |= category;
}

#define WITNESS_INSTANCE(instance, category) recordFurnitureRay(uint(instance), category)
#define WITNESS_HIT(result, category) if ((result).hit) WITNESS_INSTANCE((result).instance, category)
#else
#define WITNESS_INSTANCE(instance, category)
#define WITNESS_HIT(result, category)
#endif
"""
    replace(frame, '#include "rt_diagnostics.glsl"\n', '#include "rt_diagnostics.glsl"\n' + witness)
    witness_hit(frame, "primary", "1u")
    for name, bit in (("reflectedHit", "4u"), ("bounceHit", "4u"), ("transmittedHit", "8u")):
        witness_hit(common, name, bit, count=2 if name == "transmittedHit" else 1)
    witness_hit(transport, "reflectedHit", "4u", count=2)
    witness_hit(transport, "nextHit", "witnessTir ? 4u : 8u", count=2)
    tir = "if (!thinWall && dot(idealTransmission, idealTransmission) < 0.25)"
    replace(transport, tir, """#if HORDE_RT_VARIANT_INSTRUMENTATION == HORDE_RT_INSTRUMENTATION_DIAGNOSTIC
        bool witnessTir = !thinWall && dot(idealTransmission, idealTransmission) < 0.25;
#endif
        """ + tir, count=2)

    replace(lighting, "    result.entering = rayQueryGetIntersectionFrontFaceEXT(query, true);",
            "    WITNESS_INSTANCE(result.instance, 2u);\n"
            "    result.entering = rayQueryGetIntersectionFrontFaceEXT(query, true);")
    blocked = """    if (rayQueryGetIntersectionTypeEXT(query, true) !=
        gl_RayQueryCommittedIntersectionNoneEXT)
        return vec3(0.0);"""
    replace(lighting, blocked, blocked.replace("        return vec3(0.0);", """    {
        WITNESS_INSTANCE(rayQueryGetIntersectionInstanceCustomIndexEXT(query, true), 2u);
        return vec3(0.0);
    }"""))
    unblocked = """    return rayQueryGetIntersectionTypeEXT(query, true) ==
        gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;"""
    replace(lighting, unblocked, """    if (rayQueryGetIntersectionTypeEXT(query, true) ==
        gl_RayQueryCommittedIntersectionNoneEXT)
        return 1.0;
    WITNESS_INSTANCE(rayQueryGetIntersectionInstanceCustomIndexEXT(query, true), 2u);
    return 0.0;""")
    output = "    if ((outputMode & 1u) != 0u)"
    replace(frame, output, """#if HORDE_RT_VARIANT_INSTRUMENTATION == HORDE_RT_INSTRUMENTATION_DIAGNOSTIC
    // Preserve the high nibble so the normal shading result stays live.
    uint red = uint(round(clamp(color.r, 0.0, 1.0) * 255.0));
    color.r = float((red & 0xf0u) | furnitureRayBits) / 255.0;
#endif
""" + output)

    # Private instrumentation has no production SPIR-V size budget. Keep the
    # standard compiler, structural invariants and exact atomic counts.
    budgets_path = private / "tools/raygen-variant-budgets.json"
    budgets = json.loads(budgets_path.read_text())
    for budget in budgets["budgets"]:
        if budget["key"].startswith("diagnostic_") and budget["key"].endswith("_generic_dielectric"):
            for metric, added in (("selectionMerges", 10), ("functions", 1), ("functionCalls", 8)):
                budget["max"][metric] += added
    budgets_path.write_text(json.dumps(budgets, indent=2) + "\n")
    receipt = {
        "encoding": "furniture-ray-red-low-nibble-v1",
        "bits": {"primary": 1, "shadow": 2, "reflection": 4, "transmission": 8},
        "sources": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in (frame, lighting, transport, common)},
    }
    (private / "furniture-ray-witness.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(private)


if __name__ == "__main__":
    main()
