"""Prepare private sparse shader-clock observation shaders for the ordinary RT layout.

Never edits production shaders or geometry. The private observer is an ABI-changing
diagnostic build and is not a frozen-budget or production candidate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil


CAPACITY = 8192
STRIDE = 32
OFFSET = 16
RECORD_BYTES = 80


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def replace(path, old, new):
    text = path.read_text(encoding="utf-8")
    if text.count(old) != 1:
        raise RuntimeError(f"Unexpected source in {path}: {old[:80]}")
    path.write_text(text.replace(old, new), encoding="utf-8", newline="\n")


def observe_function(path, name, category_expression):
    text = path.read_text(encoding="utf-8")
    clean = re.sub(r"/\*.*?\*/|//[^\n]*", lambda m: " " * len(m[0]), text, flags=re.S)
    pattern = rf"\b(\w+)\s+{name}\s*\([^;{{}}]*\)\s*\{{"
    matches = list(re.finditer(pattern, clean))
    if len(matches) != 1:
        raise RuntimeError(f"Expected one definition of {name} in {path}")
    match = matches[0]
    begin = match.end()
    depth, end = 1, begin
    while depth:
        if end >= len(clean):
            raise RuntimeError(f"Unterminated definition of {name}")
        if clean[end] == "{":
            depth += 1
        elif clean[end] == "}":
            depth -= 1
        end += 1
    body = text[begin:end - 1]
    body_clean = clean[begin:end - 1]
    returns = list(re.finditer(r"\breturn\s+([^;]+);", body_clean))
    if not returns:
        raise RuntimeError(f"No value return in {name}")
    for result in reversed(returns):
        expression = body[result.start(1):result.end(1)]
        body = body[:result.start()] + (
            f"{{ {match[1]} clockResult = {expression}; "
            "rtClockSwitch(clockPreviousCategory); return clockResult; }") + body[result.end():]
    body = f"\n    uint clockPreviousCategory = rtClockSwitch({category_expression});" + body
    if name == "traceScene":
        marker = "    if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)"
        if body.count(marker) != 1:
            raise RuntimeError("trace/decode boundary changed")
        body = body.replace(marker, "    rtClockSwitch(2u);\n" + marker)
    path.write_text(text[:begin] + body + text[end - 1:], encoding="utf-8", newline="\n")


CLOCK_GLSL = r"""
// Private observation only. Units are implementation-defined clock ticks.
// Each sampled invocation has exclusive nested categories; never infer GPU %.
layout(buffer_reference, std430, buffer_reference_align=16) buffer ClockSamples { uvec4 v[]; };
bool rtClockSelected = false;
uint rtClockCategory = 0u;
uvec2 rtClockLast;
uvec2 rtClockTicks[8];
uint rtClockSwitch(uint category) {
    uint previous = rtClockCategory;
    if (rtClockSelected) {
        uvec2 now = clockRealtime2x32EXT();
        uvec2 delta = uvec2(now.x-rtClockLast.x, now.y-rtClockLast.y-uint(now.x<rtClockLast.x));
        uvec2 old = rtClockTicks[previous];
        uint low = old.x+delta.x;
        rtClockTicks[previous] = uvec2(low,old.y+delta.y+uint(low<old.x));
        rtClockLast=now;
    }
    rtClockCategory=category;
    return previous;
}
void rtClockBegin() {
    uvec2 pixel=HORDE_RT_PIXEL_ID.xy;
    rtClockSelected=rtInstances.clockControl.z!=0u && all(equal(pixel%32u,uvec2(16u)));
    if(rtClockSelected) {
        for(uint i=0u;i<8u;++i)rtClockTicks[i]=uvec2(0u);
        rtClockCategory=0u;
        rtClockLast=clockRealtime2x32EXT();
    }
}
void rtClockEnd(uint hitClass) {
    if(!rtClockSelected)return;
    rtClockSwitch(0u);
    uint columns=(HORDE_RT_PIXEL_EXTENT.x+15u)/32u;
    uint index=(HORDE_RT_PIXEL_ID.y/32u)*columns+HORDE_RT_PIXEL_ID.x/32u;
    if(index>=rtInstances.clockControl.w)return;
    ClockSamples samples=ClockSamples(rtInstances.clockControl.xy);
    for(uint i=0u;i<4u;++i)samples.v[index*5u+1u+i]=uvec4(rtClockTicks[2u*i],rtClockTicks[2u*i+1u]);
    samples.v[index*5u]=uvec4(rtInstances.clockControl.z,HORDE_RT_PIXEL_ID.xy,hitClass);
}
"""


def catalog_rows(source):
    headers = [
        ("src/vulkan/raytracing/RtPipelineVariantCatalog.generated.h", "shipping_mobile_"),
        ("src/vulkan/raytracing/RtRayQueryVariantCatalog.generated.h", "rayquery_compute_shipping_mobile_"),
    ]
    result = {}
    pattern = re.compile(r'"([a-z_]+)", "src/vulkan/raytracing/variants/[^\"]+", "([0-9a-f]{64})", "([0-9a-f]{64})", (\d+), 0, false')
    for relative, prefix in headers:
        text = (source / relative).read_text(encoding="utf-8")
        for match in pattern.finditer(text):
            key = match.group(1)
            if key.startswith(prefix):
                result[key] = {"spirvSha256": match.group(2), "includeSha256": match.group(3),
                               "words": int(match.group(4))}
    expected = {
        "shipping_mobile_opaque_fast", "shipping_mobile_generic_dielectric",
        "rayquery_compute_shipping_mobile_opaque_fast", "rayquery_compute_shipping_mobile_generic_dielectric",
    }
    if set(result) != expected:
        raise RuntimeError(f"Base Shipping/Mobile catalog rows incomplete: {sorted(result)}")
    return result


def shader_metadata_layout(source):
    """Return the base metadata shape, deriving ordinary sizes from the ABI files."""
    generated_path = source / "shaders/raytracing/include/rt_scene_abi.generated.glsl"
    generated = generated_path.read_text(encoding="utf-8")
    abi_path = source / "src/vulkan/raytracing/RtSceneAbi.def"
    abi = json.loads(abi_path.read_text(encoding="utf-8")) if abi_path.is_file() else None
    values = re.search(r"\bRtInstanceMetadata\s+values(?:\[(\d*)\])?\s*;", generated)
    pages = re.search(r"\buvec4\s+pages\[(\d+)\]\s*;", generated)
    if not values:
        raise RuntimeError("Generated GLSL has no RtInstanceMetadata storage array")
    record = None
    capacity = None
    if abi is not None:
        capacity = int(abi["capacities"]["instanceMetadata"])
        record = next((row for row in abi["records"] if row["name"] == "RtInstanceMetadata"), None)
        if record is None or int(record["size"]) <= 0:
            raise RuntimeError("RtSceneAbi.def has no valid RtInstanceMetadata record size")
        record_bytes = int(record["size"])
    else:
        record_bytes = None

    fixed_count = int(values.group(1)) if values.group(1) else None
    if pages:
        if fixed_count is None:
            raise RuntimeError("Flat-page GLSL must have an explicit metadata capacity")
        if capacity is not None and capacity != fixed_count:
            raise RuntimeError("Flat-page GLSL capacity disagrees with RtSceneAbi.def")
        if record_bytes is None:
            struct_match = re.search(r"struct\s+RtInstanceMetadata\s*\{([^}]+)\}", generated, re.S)
            body = struct_match.group(1) if struct_match else ""
            fields = re.findall(r"^\s*uint\s+\w+\s*;", body, re.M)
            if not fields or len(fields) != len(re.findall(r"\buint\s+\w+\s*;", body)):
                raise RuntimeError("Flat-page metadata record size cannot be derived from its GLSL fields")
            record_bytes = len(fields) * 4
        return {"layout": "flat-pages", "metadataCapacity": fixed_count,
                "metadataRecordBytes": record_bytes, "pageCount": int(pages.group(1)),
                "controlOffset": fixed_count * record_bytes + int(pages.group(1)) * 16}

    if fixed_count is not None:
        raise RuntimeError("Fixed metadata array without a page table is not an enrolled source layout")
    if capacity is None or record_bytes is None:
        raise RuntimeError("Ordinary-layout preparation requires RtSceneAbi.def")
    return {"layout": "ordinary", "metadataCapacity": capacity,
            "metadataRecordBytes": record_bytes, "pageCount": 0,
            "controlOffset": capacity * record_bytes}


def prepare(source, output, *, hearth_shadow_split=False):
    repo = Path(__file__).resolve().parents[1]
    source, output = source.resolve(), output.resolve()
    reports = (repo / "reports").resolve()
    if not output.is_relative_to(reports) or output == reports or output.exists():
        raise RuntimeError("Choose a new, non-existing directory under reports")
    layout = shader_metadata_layout(source)
    if hearth_shadow_split and layout["layout"] != "ordinary":
        raise RuntimeError("The hearth shadow split requires the ordinary RT layout")
    generated = source / "shaders/raytracing/include/rt_scene_abi.generated.glsl"
    generated_text = generated.read_text(encoding="utf-8")
    if "clockControl" in generated_text or (source / "dispatch-clock-probe.json").exists():
        raise RuntimeError("Source already contains a prepared clock observer; use its receipt with the compiler")
    if layout["layout"] == "ordinary":
        expected = "    RtInstanceMetadata values[];\n} rtInstances;"
        replacement = (f"    RtInstanceMetadata values[{layout['metadataCapacity']}];\n"
                       "    uvec4 clockControl;\n} rtInstances;")
    else:
        expected = f"    uvec4 pages[{layout['pageCount']}];"
        replacement = expected + "\n    uvec4 clockControl;"
    if generated_text.count(expected) != 1:
        raise RuntimeError(f"Generated ABI does not match the enrolled {layout['layout']} layout")
    base_catalog = catalog_rows(source)
    shader_files = sorted(p for p in (source / "shaders/raytracing").rglob("*") if p.is_file())
    base_shader_hashes = {p.relative_to(source).as_posix(): sha256(p.read_bytes()) for p in shader_files}
    base_shader_digest = sha256(json.dumps(base_shader_hashes, sort_keys=True, separators=(",", ":")).encode())
    abi_files = {}
    for relative in ("src/vulkan/raytracing/RtSceneAbi.def", "src/vulkan/raytracing/RtSceneAbi.generated.h",
                     "shaders/raytracing/include/rt_scene_abi.generated.glsl"):
        path = source / relative
        if path.is_file():
            abi_files[relative] = sha256(path.read_bytes())
    if layout["layout"] == "ordinary" and set(abi_files) != {
            "src/vulkan/raytracing/RtSceneAbi.def", "src/vulkan/raytracing/RtSceneAbi.generated.h",
            "shaders/raytracing/include/rt_scene_abi.generated.glsl"}:
        raise RuntimeError("Ordinary preparation requires both source and generated RT ABI files")
    private = output / "source"
    private.mkdir(parents=True)
    # This is a compiler overlay, not a repository clone. In particular, do not
    # traverse reports/ or copy local builds, assets, or VCS metadata.
    shutil.copytree(source / "shaders/raytracing", private / "shaders/raytracing")
    shaders_readme = source / "shaders/README.md"
    if shaders_readme.is_file():
        shutil.copy2(shaders_readme, private / "shaders/README.md")
    (private / "tools").mkdir()
    for relative in ("tools/compile-raygen.ps1", "tools/raygen-variants.json",
                     "tools/raygen-variant-budgets.json"):
        target = private / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source / relative, target)
    cpp = private / "src/vulkan/raytracing"
    cpp.mkdir(parents=True)
    for relative in ("src/vulkan/raytracing/RtPipelineVariantCatalog.generated.h",
                     "src/vulkan/raytracing/RtRayQueryVariantCatalog.generated.h",
                     "src/vulkan/raytracing/RtSceneAbi.def"):
        if (source / relative).is_file():
            shutil.copy2(source / relative, private / relative)
    shutil.copytree(source / "src/vulkan/raytracing/variants", cpp / "variants")
    include = private / "shaders/raytracing/include"
    replace(include / "rt_scene_abi.generated.glsl", expected, replacement)
    clock_glsl = CLOCK_GLSL
    shadow_category = "3u"
    dust_category = "7u"
    categories = ["other", "ray traversal/filter", "hit decode/material", "shadow visibility",
                  "surface/transport", "fire volume", "mist/electricity", "dust"]
    if hearth_shadow_split:
        clock_glsl += "\nbool rtClockDirectFireShadow = false;\n"
        shadow_category = "rtClockDirectFireShadow ? 7u : 3u"
        dust_category = "6u"
        categories[3] = "other shadow visibility"
        categories[6:] = ["mist/electricity/dust", "direct fire-emitter shadow visibility"]
        fire_shadow_call = "        vec3 lightTransmittance = areaLightTransmittance(h, lightPosition, primaryReceiver);"
        replace(include / "rt_lighting.glsl", fire_shadow_call,
                "        rtClockDirectFireShadow = true;\n" + fire_shadow_call +
                "\n        rtClockDirectFireShadow = false;")
    (include / "rt_dispatch_clock.glsl").write_text(clock_glsl, encoding="utf-8", newline="\n")
    replace(include / "rt_frame.glsl", '#include "rt_scene_abi.glsl"',
            '#include "rt_scene_abi.glsl"\n#include "rt_dispatch_clock.glsl"')
    replace(include / "rt_frame.glsl", "    vec2 uv =", "    rtClockBegin();\n    vec2 uv =")
    replace(include / "rt_frame.glsl", "    imageStore(outputImage,",
            "    rtClockEnd(primary.hit ? uint(primary.instance)+1u : 0u);\n    imageStore(outputImage,")
    for entry in ("minimal.rgen", "minimal.comp"):
        replace(private / "shaders/raytracing" / entry, "#version 460",
                "#version 460\n#extension GL_EXT_buffer_reference : require\n"
                "#extension GL_EXT_buffer_reference_uvec2 : require\n"
                "#extension GL_EXT_shader_realtime_clock : require")
    for file, name, category in [
        ("rt_hit_decode.glsl", "traceScene", "1u"),
        ("rt_lighting.glsl", "visibilityMask", shadow_category),
        ("rt_lighting.glsl", "boundedShadowTransmittanceMask", shadow_category),
        ("rt_dielectric_common.glsl", "shadePrimary", "4u"),
        ("rt_fire.glsl", "integrateFireEmitters", "5u"),
        ("rt_atmosphere.glsl", "lichGroundMist", "6u"),
        ("rt_atmosphere.glsl", "staffElectricity", "6u"),
        ("rt_indoor_dust.glsl", "composeIndoorDust", dust_category),
    ]:
        observe_function(include / file, name, category)

    marker = {
        "schema": 2,
        **layout,
        "recordBytes": RECORD_BYTES,
        "capacity": CAPACITY,
        "stride": STRIDE,
        "offset": OFFSET,
        "samplesOffset": layout["controlOffset"] + 16,
        "categories": categories,
        "baseShaderSourceSha256": base_shader_digest,
        "baseShaderFiles": base_shader_hashes,
        "baseAbiFiles": abi_files,
        "baseShippingModules": base_catalog,
    }
    if hearth_shadow_split:
        marker["profile"] = "hearth-shadow-split"
    (private / "dispatch-clock-probe.json").write_text(json.dumps(marker, indent=2) + "\n", encoding="utf-8")
    private_hashes = {p.relative_to(private).as_posix(): sha256(p.read_bytes())
                      for p in (private / "shaders/raytracing").rglob("*") if p.is_file()}
    receipt = {
        "schema": 2,
        "sourceRoot": str(source),
        "method": "32x32 stratified sparse exclusive device-clock ticks; no effect omissions",
        "layout": layout["layout"],
        "baseShaderSourceSha256": base_shader_digest,
        "baseShaderFiles": base_shader_hashes,
        "baseAbiFiles": abi_files,
        "baseShippingModules": base_catalog,
        "privateShaderFilesBeforeCompile": private_hashes,
        "markerSha256": sha256((private / "dispatch-clock-probe.json").read_bytes()),
        "notProductionBudgetAdmission": True,
    }
    (output / "source-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return private


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--hearth-shadow-split", action="store_true",
                        help="separate direct fire-emitter shadows from other shadow queries")
    args = parser.parse_args()
    print(prepare(args.source, args.output, hearth_shadow_split=args.hearth_shadow_split))
