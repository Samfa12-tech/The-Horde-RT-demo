"""Compile private Shipping/Mobile observation pairs without budget publication."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess


def run(args, log):
    result = subprocess.run([str(a) for a in args], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    with log.open("ab") as stream: stream.write(result.stdout)
    if result.returncode: raise RuntimeError(f"Compiler step failed: {args[0]}; see {log}")


def compile_private(folder):
    folder = folder.resolve()
    root = Path(__file__).resolve().parents[1]
    if not folder.is_relative_to(root / "reports"):
        raise RuntimeError("Private reports output required")
    source = folder / "source"
    marker = source / "dispatch-clock-probe.json"
    assert json.loads(marker.read_text())["recordBytes"] == 80
    sdk = Path("C:/VulkanSDK/1.4.350.0/Bin")
    log = folder / "compile-private.log"
    if log.exists(): raise RuntimeError("Refuse to overwrite compiler evidence")
    artifacts = []
    for material in ("opaque_fast", "generic_dielectric"):
        key = "shipping_mobile_"+material
        # Reuse the first successful bounded feasibility compilation.
        compiled = folder / ("compiled" if material == "opaque_fast" else "compiled-generic") / key
        if not compiled.exists():
            run(["pwsh", "-NoProfile", "-File", source / "tools/compile-raygen.ps1",
                 "-Variant", key, "-OutputDirectory", compiled.parent], log)
        resolved = (compiled / "minimal.rgen.resolved").read_text()
        compute = resolved.replace("#extension GL_EXT_ray_tracing : require", "")
        compute = compute.replace("#define HORDE_RT_PIXEL_ID gl_LaunchIDEXT", "#define HORDE_RT_COMPUTE_ENTRY\n#define HORDE_RT_PIXEL_ID gl_GlobalInvocationID")
        compute = compute.replace("#define HORDE_RT_PIXEL_EXTENT gl_LaunchSizeEXT",
            "#define HORDE_RT_PIXEL_EXTENT imageSize(outputImage)\nlayout(local_size_x=8,local_size_y=8,local_size_z=1) in;")
        compute_source = compiled / "minimal.comp.resolved"
        compute_source.write_text(compute, encoding="utf-8", newline="\n")
        raw, spv = compiled / "minimal.comp.raw.spv", compiled / "minimal.comp.spv"
        run([sdk / "glslangValidator.exe", "-V", "--target-env", "vulkan1.2", "-S", "comp"] +
            (["-Os"] if material == "opaque_fast" else []) + ["-o", raw, compute_source], log)
        options = ["-O"] if material == "opaque_fast" else ["--eliminate-dead-functions",
            "--eliminate-dead-code-aggressive", "--simplify-instructions", "--eliminate-dead-branches", "--cfg-cleanup"]
        run([sdk / "spirv-opt.exe", *options, raw, "-o", spv], log)
        for stage in ("rgen", "comp"):
            spv = compiled / f"minimal.{stage}.spv"
            run([sdk / "spirv-val.exe", "--target-env", "vulkan1.2", spv], log)
            assembly = compiled / f"minimal.{stage}.spvasm"
            if stage == "comp": run([sdk / "spirv-dis.exe", spv, "-o", assembly], log)
            asm = assembly.read_text()
            assert "OpReadClockKHR" in asm and "ShaderClockKHR" in asm
            assert not re.search(r"\bOpAtomic\w+|\bBinding 22\b", asm)
            if material == "opaque_fast":
                assert len(re.findall(r"\bOpFunction\b", asm)) == 1 and "OpFunctionCall" not in asm
            else:
                assert len(re.findall(r"\bOpRayQueryInitializeKHR\b", asm)) <= 3
            name = key if stage == "rgen" else "rayquery_compute_"+key
            data = spv.read_bytes()
            words = struct.unpack("<"+"I"*(len(data)//4), data)
            text = "\n".join(", ".join(f"0x{w:08x}u" for w in words[i:i+8])+"," for i in range(0,len(words),8))+"\n"
            inc = source / "src/vulkan/raytracing/variants" / (name+".inc")
            inc.write_text(text, encoding="utf-8", newline="\n")
            header = source / "src/vulkan/raytracing" / ("RtPipelineVariantCatalog.generated.h" if stage=="rgen" else "RtRayQueryVariantCatalog.generated.h")
            old = header.read_text()
            pattern = rf'("{name}", "src/vulkan/raytracing/variants/{name}\.inc", )"[0-9a-f]+", "[0-9a-f]+", \d+, 0, false'
            updated, count = re.subn(pattern, lambda m: m[1]+f'"{hashlib.sha256(data).hexdigest()}", "{hashlib.sha256(text.encode()).hexdigest()}", {len(words)}, 0, false', old)
            assert count == 1, (name,count)
            header.write_text(updated, encoding="utf-8", newline="\n")
            artifacts.append({"key":name,"sha256":hashlib.sha256(data).hexdigest(),"bytes":len(data),
                "words":len(words),"clockSites":len(re.findall(r"\bOpReadClockKHR\b",asm)),
                "querySites":len(re.findall(r"\bOpRayQueryInitializeKHR\b",asm)),"productionBudgetAdmission":False})
    (folder / "compiled-observer.json").write_text(json.dumps(artifacts,indent=2),encoding="utf-8")
    print(json.dumps(artifacts,indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder",type=Path)
    compile_private(parser.parse_args().folder)
