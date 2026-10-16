"""Build a private, sparse shader-clock observer; never edit production shaders.

The input must be the already-verified region-page source. No effect is omitted.
Experimental modules are observations, not frozen-budget production candidates.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


def replace(path, old, new):
    text = path.read_text(encoding="utf-8")
    if text.count(old) != 1:
        raise RuntimeError(f"Unexpected source in {path}: {old[:60]}")
    path.write_text(text.replace(old, new), encoding="utf-8", newline="\n")


def observe_function(path, name, category):
    text = path.read_text(encoding="utf-8")
    # Blank comments without changing offsets; braces/returns in comments are not code.
    clean = re.sub(r"/\*.*?\*/|//[^\n]*", lambda m: " " * len(m[0]), text, flags=re.S)
    pattern = rf"\b(\w+)\s+{name}\s*\([^;{{}}]*\)\s*\{{"
    matches = list(re.finditer(pattern, clean))
    if len(matches) != 1:
        raise RuntimeError(f"Expected one definition of {name}")
    match = matches[0]
    begin = match.end()
    depth, end = 1, begin
    while depth:
        if clean[end] == "{": depth += 1
        if clean[end] == "}": depth -= 1
        end += 1
    body = text[begin:end-1]
    body_clean = clean[begin:end-1]
    returns = list(re.finditer(r"\breturn\s+([^;]+);", body_clean))
    if not returns:
        raise RuntimeError(f"No value return in {name}")
    # Evaluate the original expression before restoring the caller's category.
    for r in reversed(returns):
        expression = body[r.start(1):r.end(1)]
        body = body[:r.start()] + (f"{{ {match[1]} clockResult = {expression}; "
            "rtClockSwitch(clockPreviousCategory); return clockResult; }") + body[r.end():]
    body = f"\n    uint clockPreviousCategory = rtClockSwitch({category}u);" + body
    if name == "traceScene":
        marker = "    if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)"
        if body.count(marker) != 1: raise RuntimeError("trace/decode boundary changed")
        body = body.replace(marker, "    rtClockSwitch(2u);\n" + marker)
    path.write_text(text[:begin]+body+text[end-1:], encoding="utf-8", newline="\n")


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


def prepare(source, output):
    root = Path(__file__).resolve().parents[1]
    source, output = source.resolve(), output.resolve()
    if not output.is_relative_to(root / "reports") or output.exists():
        raise RuntimeError("Choose a new reports directory; no overwrite")
    if "uvec4 pages[8];" not in (source / "shaders/raytracing/include/rt_scene_abi.generated.glsl").read_text():
        raise RuntimeError("Expected the admitted flat region-page ABI")
    shutil.copytree(source, output / "source")
    private = output / "source"
    # Keep already-admitted variants for unselected policies; replace only the two
    # actual Shipping/Mobile pairs after compiling the private observation.
    include = private / "shaders/raytracing/include"
    replace(include / "rt_scene_abi.generated.glsl", "    uvec4 pages[8];",
            "    uvec4 pages[8];\n    uvec4 clockControl;")
    (include / "rt_dispatch_clock.glsl").write_text(CLOCK_GLSL, encoding="utf-8")
    replace(include / "rt_frame.glsl", '#include "rt_scene_abi.glsl"',
            '#include "rt_scene_abi.glsl"\n#include "rt_dispatch_clock.glsl"')
    replace(include / "rt_frame.glsl", "    vec2 uv =", "    rtClockBegin();\n    vec2 uv =")
    replace(include / "rt_frame.glsl", "    imageStore(outputImage,",
            "    rtClockEnd(primary.hit ? uint(primary.instance)+1u : 0u);\n    imageStore(outputImage,")
    for entry in ("minimal.rgen", "minimal.comp"):
        replace(private / "shaders/raytracing" / entry, "#version 460",
                "#version 460\n#extension GL_EXT_shader_realtime_clock : require")
    for file, name, category in [
        ("rt_hit_decode.glsl", "traceScene", 1),
        ("rt_lighting.glsl", "visibilityMask", 3),
        ("rt_lighting.glsl", "boundedShadowTransmittanceMask", 3),
        ("rt_dielectric_common.glsl", "shadePrimary", 4),
        ("rt_fire.glsl", "integrateFireEmitters", 5),
        ("rt_atmosphere.glsl", "lichGroundMist", 6),
        ("rt_atmosphere.glsl", "staffElectricity", 6),
        ("rt_indoor_dust.glsl", "composeIndoorDust", 7),
    ]:
        observe_function(include / file, name, category)
    receipt = {"baseHead": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root).decode().strip(),
        "source": str(source), "method": "32x32 stratified sparse exclusive device-clock ticks; no effect omissions",
        "categories": ["other", "ray traversal/filter", "hit decode/material", "shadow visibility",
                       "surface/transport", "fire volume", "mist/electricity", "dust"],
        "productionBudgetSha256": hashlib.sha256((root / "tools/raygen-variant-budgets.json").read_bytes()).hexdigest(),
        "notProductionBudgetAdmission": True,
        "files": {str(p.relative_to(private)): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in (private / "shaders").rglob("*") if p.is_file()}}
    (output / "source-receipt.json").write_text(json.dumps(receipt, indent=2), encoding="utf-8")
    (private / "dispatch-clock-probe.json").write_text(json.dumps({"schema": 1, "recordBytes": 80,
        "capacity": 8192, "stride": 32, "offset": 16, "categories": receipt["categories"]}), encoding="utf-8")
    return private


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(prepare(args.source, args.output))
