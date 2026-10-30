"""Private, reproducible page-address shader feasibility; never edits runtime shaders."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import re
import subprocess


def replace(path, old, new, count=1):
    text = path.read_text(encoding="utf-8")
    if text.count(old) != count:
        raise RuntimeError(f"Unexpected source for {path.name}: {old[:50]}")
    path.write_text(text.replace(old, new), encoding="utf-8", newline="\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--coalesced", action="store_true")
    parser.add_argument("--uniform-pages", action="store_true")
    parser.add_argument("--compact-symbols", action="store_true")
    parser.add_argument("--flat-table", action="store_true")
    parser.add_argument("--correct-player-attribution", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    out = args.output.resolve()
    if not out.is_relative_to(root / "reports") or out.exists():
        raise RuntimeError("Use a new private directory under reports; no overwrite allowed.")
    out.mkdir(parents=True)
    private = out / "source"
    shutil.copytree(root / "shaders", private / "shaders")
    (private / "tools").mkdir()
    for name in ("compile-raygen.ps1", "raygen-variants.json", "raygen-variant-budgets.json",
                 "GenerateRtPipelineVariantCatalog.ps1"):
        shutil.copy2(root / "tools" / name, private / "tools" / name)
    abi = private / "shaders/raytracing/include/rt_scene_abi.generated.glsl"
    replace(abi, "    RtInstanceMetadata values[];", "    RtInstanceMetadata values[26];\n    RtGeometryPage pages[4];")
    replace(abi, "layout(std430, set = 0, binding = 11)", """// Experimental private page table; legacy metadata prefix is unchanged.
struct RtGeometryPage { uvec4 vertexAndIndexAddress; uvec4 primitiveAddressAndCounts; };
layout(buffer_reference, std430, buffer_reference_align=16) readonly buffer PageVertices { StaticRtVertex values[]; };
layout(buffer_reference, std430, buffer_reference_align=4) readonly buffer PageIndices { uint values[]; };
layout(buffer_reference, std430, buffer_reference_align=16) readonly buffer PagePrimitives { RtPrimitiveMetadata values[]; };
layout(std430, set = 0, binding = 11)""")
    replace(abi, "} rtInstances;", """} rtInstances;
#define RT_PAGED(i) (((i).flags & 16u) != 0u)
#define RT_PAGE(i) rtInstances.pages[(i).reserved1]
#define RT_PRIMITIVE(i,g) (RT_PAGED(i) ? PagePrimitives(RT_PAGE(i).primitiveAddressAndCounts.xy).values[g] : rtPrimitives.values[(i).primitiveBase + (g)])
#define RT_INDEX(i,t) (RT_PAGED(i) ? PageIndices(RT_PAGE(i).vertexAndIndexAddress.zw).values[t] : rtStaticIndices.values[t])
""")
    for entry in ("minimal.rgen", "minimal.comp"):
        replace(private / "shaders/raytracing" / entry, "#version 460", "#version 460\n#extension GL_EXT_buffer_reference : require\n#extension GL_EXT_buffer_reference_uvec2 : require")
    hit = private / "shaders/raytracing/include/rt_hit_decode.glsl"
    replace(hit, "    if (instance.geometryRole == kRtGeometryRolePlayerWorldBody)", """    if (RT_PAGED(instance))
    {
        PageVertices page = PageVertices(RT_PAGE(instance).vertexAndIndexAddress.xy);
        v0 = page.values[i0]; v1 = page.values[i1]; v2 = page.values[i2];
    }
    else if (instance.geometryRole == kRtGeometryRolePlayerWorldBody)""")
    replace(hit, "rtPrimitives.values[instanceMetadata.primitiveBase + geometryIndex]", "RT_PRIMITIVE(instanceMetadata, geometryIndex)")
    for suffix in ("", " + 1u", " + 2u"):
        replace(hit, f"rtStaticIndices.values[triangleIndex{suffix}]", f"RT_INDEX(instanceMetadata, triangleIndex{suffix})")
    lighting = private / "shaders/raytracing/include/rt_lighting.glsl"
    text = lighting.read_text(encoding="utf-8")
    import re
    text, count = re.subn(r"rtPrimitives\.values\[\s*instanceMetadata\.primitiveBase \+ geometryIndex\]", "RT_PRIMITIVE(instanceMetadata, geometryIndex)", text)
    if count != 2:
        raise RuntimeError("Lighting query material lookup count changed")
    lighting.write_text(text, encoding="utf-8", newline="\n")
    if args.coalesced:
        # Three conditional scalar index loads plus a separate vertex branch
        # repeat the address decision. Select the entire triangle in one place.
        text = hit.read_text(encoding="utf-8")
        start = text.index("void loadPbrTriangle(")
        end = text.index("HitInfo traceScene(", start)
        text = text[:start] + """void loadPbrTriangle(RtInstanceMetadata instance, uint triangleIndex, uint vertexOffset,
                     out StaticRtVertex v0, out StaticRtVertex v1, out StaticRtVertex v2)
{
    if (RT_PAGED(instance))
    {
        RtGeometryPage addresses = RT_PAGE(instance);
        PageVertices vertices = PageVertices(addresses.vertexAndIndexAddress.xy);
        PageIndices indices = PageIndices(addresses.vertexAndIndexAddress.zw);
        v0 = vertices.values[vertexOffset + indices.values[triangleIndex]];
        v1 = vertices.values[vertexOffset + indices.values[triangleIndex + 1u]];
        v2 = vertices.values[vertexOffset + indices.values[triangleIndex + 2u]];
    }
    else
    {
        uint i0 = vertexOffset + rtStaticIndices.values[triangleIndex];
        uint i1 = vertexOffset + rtStaticIndices.values[triangleIndex + 1u];
        uint i2 = vertexOffset + rtStaticIndices.values[triangleIndex + 2u];
        if (instance.geometryRole == kRtGeometryRolePlayerWorldBody)
        {
            v0 = rtWorldPlayerVertices.values[i0]; v1 = rtWorldPlayerVertices.values[i1]; v2 = rtWorldPlayerVertices.values[i2];
        }
        else if (instance.geometryRole == kRtGeometryRolePlayerViewmodel)
        {
            v0 = rtViewmodelVertices.values[i0]; v1 = rtViewmodelVertices.values[i1]; v2 = rtViewmodelVertices.values[i2];
        }
        else
        {
            v0 = rtStaticVertices.values[i0]; v1 = rtStaticVertices.values[i1]; v2 = rtStaticVertices.values[i2];
        }
    }
}

""" + text[end:]
        text, count = re.subn(r"                uint i[012] = primitiveMetadata.vertexOffset \+ RT_INDEX\(instanceMetadata, triangleIndex(?: \+ [12]u)?\);\n", "", text)
        if count != 3:
            raise RuntimeError("Triangle index load count changed")
        text = text.replace("loadPbrTriangle(instanceMetadata, i0, i1, i2, v0, v1, v2);", "loadPbrTriangle(instanceMetadata, triangleIndex, primitiveMetadata.vertexOffset, v0, v1, v2);")
        hit.write_text(text, encoding="utf-8", newline="\n")
    if args.uniform_pages:
        if args.coalesced:
            raise RuntimeError("Uniform pages uses one address domain; do not combine branches")
        # Page zero is the current shared storage for every existing instance.
        # Region instances use a different page via flags bits 8-9; reserved1
        # remains available for its existing actor state, with no reinterpretation.
        replace(abi, "#define RT_PAGE(i) rtInstances.pages[(i).reserved1]", "#define RT_PAGE(i) rtInstances.pages[((i).flags >> 8u) & 3u]")
        replace(abi, "#define RT_PRIMITIVE(i,g) (RT_PAGED(i) ? PagePrimitives(RT_PAGE(i).primitiveAddressAndCounts.xy).values[g] : rtPrimitives.values[(i).primitiveBase + (g)])", "#define RT_PRIMITIVE(i,g) PagePrimitives(RT_PAGE(i).primitiveAddressAndCounts.xy).values[(i).primitiveBase + (g)]")
        replace(abi, "#define RT_INDEX(i,t) (RT_PAGED(i) ? PageIndices(RT_PAGE(i).vertexAndIndexAddress.zw).values[t] : rtStaticIndices.values[t])", "#define RT_INDEX(i,t) PageIndices(RT_PAGE(i).vertexAndIndexAddress.zw).values[t]")
        text = hit.read_text(encoding="utf-8")
        start = text.index("    if (RT_PAGED(instance))")
        end = text.index("    else if (instance.geometryRole == kRtGeometryRolePlayerWorldBody)", start)
        text = text[:start] + text[end:].replace("    else if (instance.geometryRole == kRtGeometryRolePlayerWorldBody)", "    if (instance.geometryRole == kRtGeometryRolePlayerWorldBody)", 1)
        text = text.replace("rtStaticVertices.values[i0]", "PageVertices(RT_PAGE(instance).vertexAndIndexAddress.xy).values[i0]").replace("rtStaticVertices.values[i1]", "PageVertices(RT_PAGE(instance).vertexAndIndexAddress.xy).values[i1]").replace("rtStaticVertices.values[i2]", "PageVertices(RT_PAGE(instance).vertexAndIndexAddress.xy).values[i2]")
        text = text.replace("        v0 = PageVertices(RT_PAGE(instance).vertexAndIndexAddress.xy).values[i0];", "        PageVertices vertices = PageVertices(RT_PAGE(instance).vertexAndIndexAddress.xy);\n        v0 = vertices.values[i0];").replace("PageVertices(RT_PAGE(instance).vertexAndIndexAddress.xy).values[i1]", "vertices.values[i1]").replace("PageVertices(RT_PAGE(instance).vertexAndIndexAddress.xy).values[i2]", "vertices.values[i2]")
        text = text.replace("                uint i0 = primitiveMetadata.vertexOffset + RT_INDEX(instanceMetadata, triangleIndex);", "                PageIndices indices = PageIndices(RT_PAGE(instanceMetadata).vertexAndIndexAddress.zw);\n                uint i0 = primitiveMetadata.vertexOffset + indices.values[triangleIndex];").replace("RT_INDEX(instanceMetadata, triangleIndex + 1u)", "indices.values[triangleIndex + 1u]").replace("RT_INDEX(instanceMetadata, triangleIndex + 2u)", "indices.values[triangleIndex + 2u]")
        text = text.replace("rtPrimitives.values[playerMetadata.primitiveBase + geometryIndex]", "RT_PRIMITIVE(playerMetadata, geometryIndex)")
        hit.write_text(text, encoding="utf-8", newline="\n")
    if args.flat_table:
        if not args.uniform_pages:
            raise RuntimeError("Flat table requires uniform pages")
        text = abi.read_text(encoding="utf-8")
        text = text.replace("struct RtGeometryPage { uvec4 vertexAndIndexAddress; uvec4 primitiveAddressAndCounts; };\n", "")
        text = text.replace("RtGeometryPage pages[4];", "uvec4 pages[8];")
        # Admission must reject any flags outside bits0-3 / page bits8-9.
        # No masking of corrupt/overflow page identities is permitted.
        text = text.replace("#define RT_PAGE(i) rtInstances.pages[((i).flags >> 8u) & 3u]", "#define RT_PAGE_V(i) rtInstances.pages[(i).flags >> 7u]\n#define RT_PAGE_P(i) rtInstances.pages[((i).flags >> 7u) + 1u]")
        text = text.replace("RT_PAGE(i).primitiveAddressAndCounts.xy", "RT_PAGE_P(i).xy").replace("RT_PAGE(i).vertexAndIndexAddress.zw", "RT_PAGE_V(i).zw")
        abi.write_text(text, encoding="utf-8", newline="\n")
        text = hit.read_text(encoding="utf-8")
        text = text.replace("RT_PAGE(instance).vertexAndIndexAddress.xy", "RT_PAGE_V(instance).xy").replace("RT_PAGE(instanceMetadata).vertexAndIndexAddress.zw", "RT_PAGE_V(instanceMetadata).zw")
        hit.write_text(text, encoding="utf-8", newline="\n")
    if args.compact_symbols:
        # GenericRetained preserves OpName records. New type/field names add
        # binary bytes without execution cost; concise private ABI names avoid
        # that serialization overhead without stripping diagnostics or code.
        for path in (abi, hit):
            text = path.read_text(encoding="utf-8")
            for old, new in (("RtGeometryPage", "RtPage"), ("vertexAndIndexAddress", "vi"),
                             ("primitiveAddressAndCounts", "pc"), ("PageVertices", "PV"),
                             ("PageIndices", "PI"), ("PagePrimitives", "PP"),
                             ("RtInstanceMetadataBuffer", "RtPages")):
                text = text.replace(old, new)
            if path == abi:
                text = text.replace("buffer PV { StaticRtVertex values[]; }", "buffer PV { StaticRtVertex v[]; }")
                text = text.replace("buffer PI { uint values[]; }", "buffer PI { uint v[]; }")
                text = text.replace("buffer PP { RtPrimitiveMetadata values[]; }", "buffer PP { RtPrimitiveMetadata v[]; }")
                text = text.replace(".xy).values[", ".xy).v[").replace(".zw).values[", ".zw).v[")
            else:
                text = text.replace("PV vertices =", "PV pv =").replace("PI indices =", "PI pi =")
                text = text.replace("vertices.values[", "pv.v[").replace("indices.values[", "pi.v[")
            path.write_text(text, encoding="utf-8", newline="\n")
    if args.correct_player_attribution:
        frame = private / "shaders/raytracing/include/rt_frame.glsl"
        replace(frame, "(primary.instance >= 10 && primary.instance <= 13)",
                "(primary.instance >= 10 && primary.instance <= 13 &&\n"
                "                  (rtInstances.values[primary.instance].flags & kRtInstanceFlagStaticPbr) == 0u)")
        if args.compact_symbols:
            # Short local names retain executable work, scope and diagnostic
            # fields. Avoid charging repeated serialized local OpNames for the
            # new address-based prototype's exact ownership attribution.
            for path in (hit, private / "shaders/raytracing/include/rt_lighting.glsl"):
                text = re.sub(r"\binstanceMetadata\b", "im", path.read_text(encoding="utf-8"))
                path.write_text(text, encoding="utf-8", newline="\n")
    source_head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=root,
                                 check=True, capture_output=True, text=True).stdout.strip()
    receipt = {"sourceHeadReference": source_head, "productionModified": False,
               "generatorSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "method": "private BDA pages appended to metadata, common PBR and visibility lookups", "coalesced": args.coalesced, "uniformPages": args.uniform_pages, "compactSymbols": args.compact_symbols, "flatTable": args.flat_table, "correctPlayerAttribution": args.correct_player_attribution, "files": {}}
    for path in sorted((private / "shaders").rglob("*.glsl")) + [private / "shaders/raytracing/minimal.rgen", private / "shaders/raytracing/minimal.comp"]:
        receipt["files"][str(path.relative_to(private))] = hashlib.sha256(path.read_bytes()).hexdigest()
    (out / "source-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(private)


if __name__ == "__main__":
    main()
