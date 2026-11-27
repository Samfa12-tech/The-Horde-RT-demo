"""Compile or verify private Shipping/Mobile dispatch-clock shader pairs."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess


MODULE_KEYS = (
    "shipping_mobile_opaque_fast",
    "rayquery_compute_shipping_mobile_opaque_fast",
    "shipping_mobile_generic_dielectric",
    "rayquery_compute_shipping_mobile_generic_dielectric",
)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def run(args, log):
    result = subprocess.run([str(arg) for arg in args], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    with log.open("ab") as stream:
        stream.write(result.stdout)
    if result.returncode:
        raise RuntimeError(f"Compiler step failed: {args[0]}; see {log}")


def verify_receipt(folder, source, marker):
    receipt_path = folder / "source-receipt.json"
    if not receipt_path.is_file():
        raise RuntimeError("Private source receipt is required")
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    if marker.get("schema") == 1 and "files" in receipt and "baseHead" in receipt:
        if marker.get("recordBytes") != 80 or marker.get("capacity") != 8192 or \
                marker.get("stride") != 32 or marker.get("offset") != 16:
            raise RuntimeError("Historical clock marker does not match its established record ABI")
        generated = source / "shaders/raytracing/include/rt_scene_abi.generated.glsl"
        generated_text = generated.read_text(encoding="utf-8")
        if "uvec4 pages[8];" not in generated_text or "uvec4 clockControl;" not in generated_text:
            raise RuntimeError("Historical receipt is not a prepared flat-page observer")
        files = receipt.get("files", {})
        if not files:
            raise RuntimeError("Historical receipt has no source file hashes")
        verified = set()
        for relative, expected in files.items():
            normalized = relative.replace("\\", "/")
            source_path = Path(normalized)
            # Older receipts also recorded docs and checked-in SPIR-V that are
            # not compiler inputs. Require the exact transitive GLSL/source set.
            if not normalized.startswith("shaders/raytracing/") or source_path.suffix.lower() not in {
                    ".glsl", ".rgen", ".comp", ".rchit", ".rmiss", ".rahit", ".rint", ".rcall"}:
                continue
            path = source / source_path
            if not path.is_file() or sha256(path.read_bytes()) != expected:
                raise RuntimeError(f"Historical prepared shader receipt mismatch: {relative}")
            verified.add(normalized)
        actual = {path.relative_to(source).as_posix() for path in (source / "shaders/raytracing").rglob("*")
                  if path.is_file() and path.suffix.lower() in {
                      ".glsl", ".rgen", ".comp", ".rchit", ".rmiss", ".rahit", ".rint", ".rcall"}}
        if not verified or verified != actual:
            raise RuntimeError("Historical shader source receipt is incomplete for the prepared tree")
        return receipt

    if receipt.get("schema") != 2 or marker.get("schema") != 2:
        raise RuntimeError("Unsupported clock source receipt/marker schema")
    layout = marker.get("layout")
    if layout not in ("ordinary", "flat-pages") or receipt.get("layout") != layout:
        raise RuntimeError("Clock source receipt/marker layout is inconsistent")
    capacity = marker.get("metadataCapacity")
    record_bytes = marker.get("metadataRecordBytes")
    page_count = marker.get("pageCount")
    control_offset = marker.get("controlOffset")
    if not all(isinstance(value, int) and value > 0 for value in (capacity, record_bytes)) or \
            not isinstance(page_count, int) or page_count < 0 or \
            control_offset != capacity * record_bytes + page_count * 16 or \
            marker.get("recordBytes") != 80 or marker.get("samplesOffset") != control_offset + 16:
        raise RuntimeError("Clock source marker offsets do not match its ABI-derived layout")
    if _sha_file(source / "dispatch-clock-probe.json") != receipt.get("markerSha256"):
        raise RuntimeError("Private clock marker is stale")

    base_root = Path(receipt.get("sourceRoot", ""))
    if not base_root.is_absolute() or not base_root.is_dir():
        raise RuntimeError("Original source root is unavailable; refusing stale-source compilation")
    base_files = marker.get("baseShaderFiles", {})
    if not base_files:
        raise RuntimeError("Clock marker has no exact base shader dependency receipt")
    for relative, expected in base_files.items():
        path = base_root / Path(relative)
        if not path.is_file() or _sha_file(path) != expected:
            raise RuntimeError(f"Base shader source changed after observer preparation: {relative}")
    abi_files = marker.get("baseAbiFiles", {})
    for relative, expected in abi_files.items():
        path = base_root / Path(relative)
        if not path.is_file() or _sha_file(path) != expected:
            raise RuntimeError(f"Base RT ABI source changed after observer preparation: {relative}")
    if layout == "ordinary" and not abi_files:
        raise RuntimeError("Ordinary marker has no source/generated ABI hashes")
    for relative, expected in receipt.get("privateShaderFilesBeforeCompile", {}).items():
        path = source / Path(relative)
        if not path.is_file() or _sha_file(path) != expected:
            raise RuntimeError(f"Private shader source changed after preparation: {relative}")
    return receipt


def _sha_file(path):
    return sha256(path.read_bytes())


def module_paths(folder):
    paths = {}
    for material in ("opaque_fast", "generic_dielectric"):
        compiled_dir = folder / ("compiled" if material == "opaque_fast" else "compiled-generic") / \
            ("shipping_mobile_" + material)
        paths["shipping_mobile_" + material] = compiled_dir / "minimal.rgen.spv"
        paths["rayquery_compute_shipping_mobile_" + material] = compiled_dir / "minimal.comp.spv"
    return paths


def verify_existing_modules(folder, source, marker):
    """Validate, but never silently trust, a preexisting compiler output set."""
    paths = module_paths(folder)
    present = [path.is_file() for path in paths.values()]
    if not any(present):
        return False
    if not all(present):
        raise RuntimeError("Preexisting observer SPIR-V set is partial; prepare a fresh report root")
    manifest_path = folder / "compiled-observer.json"
    if not manifest_path.is_file():
        raise RuntimeError("Preexisting observer SPIR-V lacks its artifact manifest; refusing reuse")
    rows = json.loads(manifest_path.read_text(encoding="utf-8"))
    rows = {row.get("key"): row for row in rows if isinstance(row, dict)}
    if set(rows) != set(MODULE_KEYS):
        raise RuntimeError("Preexisting observer artifact manifest is incomplete; refusing reuse")
    sdk = Path("C:/VulkanSDK/1.4.350.0/Bin")
    validator = sdk / "spirv-val.exe"
    disassembler = sdk / "spirv-dis.exe"
    if not validator.is_file() or not disassembler.is_file():
        raise RuntimeError("SPIR-V validation tools are required to verify preexisting modules")
    for key, path in paths.items():
        data = path.read_bytes()
        row = rows[key]
        if _sha_file(path) != row.get("sha256") or len(data) != row.get("bytes") or \
                len(data) % 4 or len(data) // 4 != row.get("words") or row.get("clockSites", 0) <= 0 or \
                row.get("productionBudgetAdmission") is not False:
            raise RuntimeError(f"Preexisting observer module provenance mismatch: {key}")
        validated = subprocess.run([str(validator), "--target-env", "vulkan1.2", str(path)],
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        disassembled = subprocess.run([str(disassembler), str(path)], stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT, text=True)
        if validated.returncode or disassembled.returncode or "OpReadClockKHR" not in disassembled.stdout:
            raise RuntimeError(f"Preexisting observer SPIR-V validation/clock check failed: {key}")
        stage = "comp" if key.startswith("rayquery_compute_") else "rgen"
        include_path = source / "src/vulkan/raytracing/variants" / (key + ".inc")
        if not include_path.is_file():
            raise RuntimeError(f"Preexisting observer module include is missing: {key}")
        words = struct.unpack("<" + "I" * (len(data) // 4), data)
        include_text = _include_text(words)
        if include_path.read_text(encoding="utf-8") != include_text:
            raise RuntimeError(f"Preexisting observer include differs from its SPIR-V: {key}")
        header_path = source / "src/vulkan/raytracing" / (
            "RtRayQueryVariantCatalog.generated.h" if stage == "comp" else "RtPipelineVariantCatalog.generated.h")
        header = header_path.read_text(encoding="utf-8")
        pattern = rf'"{re.escape(key)}", "src/vulkan/raytracing/variants/{re.escape(key)}\.inc", "([0-9a-f]{{64}})", "([0-9a-f]{{64}})", (\d+), 0, false'
        match = re.search(pattern, header)
        if not match or match.group(1) != _sha_file(path) or \
                match.group(2) != sha256(include_text.encode()) or int(match.group(3)) != len(words):
            raise RuntimeError(f"Preexisting observer catalog provenance mismatch: {key}")
        base_modules = marker.get("baseShippingModules", {})
        if base_modules and row.get("baseShippingSha256") != base_modules.get(key, {}).get("spirvSha256"):
            raise RuntimeError(f"Preexisting observer module has another base source: {key}")
    return True


def _include_text(words):
    return "\n".join(", ".join(f"0x{word:08x}u" for word in words[i:i + 8]) + ","
                     for i in range(0, len(words), 8)) + "\n"


def write_ordinary_compiled_marker(source, marker_path):
    enrolled = {
        "src/vulkan/raytracing/RtPipelineVariantCatalog.generated.h",
        "src/vulkan/raytracing/RtRayQueryVariantCatalog.generated.h",
        *("src/vulkan/raytracing/variants/" + key + ".inc" for key in MODULE_KEYS),
    }
    payload = {"schema": 1, "layout": "ordinary", "markerSha256": _sha_file(marker_path),
               "files": {relative: _sha_file(source / relative) for relative in sorted(enrolled)}}
    (source / "dispatch-clock-compiled.json").write_text(json.dumps(payload, indent=2) + "\n",
                                                           encoding="utf-8")


def compile_private(folder):
    folder = folder.resolve()
    root = Path(__file__).resolve().parents[1]
    if not folder.is_relative_to((root / "reports").resolve()):
        raise RuntimeError("Private reports output required")
    source = folder / "source"
    marker_path = source / "dispatch-clock-probe.json"
    if not marker_path.is_file():
        raise RuntimeError("Private clock marker is missing")
    marker = json.loads(marker_path.read_text(encoding="utf-8"))
    receipt = verify_receipt(folder, source, marker)
    if verify_existing_modules(folder, source, marker):
        if marker.get("layout") == "ordinary" and receipt.get("schema") == 2:
            write_ordinary_compiled_marker(source, marker_path)
        print("Validated preexisting observer SPIR-V against source receipt, compiled manifest, and generated catalogs.")
        return
    if any(path.exists() for path in (folder / "compiled", folder / "compiled-generic",
                                      folder / "compile-private.log", folder / "compiled-observer.json")):
        raise RuntimeError("Preexisting or partial compiler output exists; prepare a fresh report root")

    sdk = Path("C:/VulkanSDK/1.4.350.0/Bin")
    for tool in ("glslangValidator.exe", "spirv-opt.exe", "spirv-val.exe", "spirv-dis.exe"):
        if not (sdk / tool).is_file():
            raise RuntimeError(f"Required Vulkan shader tool is missing: {sdk / tool}")
    log = folder / "compile-private.log"
    artifacts = []
    pending = []
    base_modules = marker.get("baseShippingModules", {})
    for material in ("opaque_fast", "generic_dielectric"):
        key = "shipping_mobile_" + material
        output_root = folder / ("compiled" if material == "opaque_fast" else "compiled-generic")
        compiled = output_root / key
        run(["pwsh", "-NoProfile", "-File", source / "tools/compile-raygen.ps1",
             "-Variant", key, "-OutputDirectory", output_root], log)
        resolved = (compiled / "minimal.rgen.resolved").read_text(encoding="utf-8")
        compute = resolved.replace("#extension GL_EXT_ray_tracing : require", "")
        compute = compute.replace("#define HORDE_RT_PIXEL_ID gl_LaunchIDEXT",
                                  "#define HORDE_RT_COMPUTE_ENTRY\n#define HORDE_RT_PIXEL_ID gl_GlobalInvocationID")
        compute = compute.replace("#define HORDE_RT_PIXEL_EXTENT gl_LaunchSizeEXT",
            "#define HORDE_RT_PIXEL_EXTENT imageSize(outputImage)\nlayout(local_size_x=8,local_size_y=8,local_size_z=1) in;")
        compute_source = compiled / "minimal.comp.resolved"
        compute_source.write_text(compute, encoding="utf-8", newline="\n")
        raw, compute_spv = compiled / "minimal.comp.raw.spv", compiled / "minimal.comp.spv"
        run([sdk / "glslangValidator.exe", "-V", "--target-env", "vulkan1.2", "-S", "comp"] +
            (["-Os"] if material == "opaque_fast" else []) + ["-o", raw, compute_source], log)
        options = ["-O"] if material == "opaque_fast" else ["--eliminate-dead-functions",
            "--eliminate-dead-code-aggressive", "--simplify-instructions", "--eliminate-dead-branches", "--cfg-cleanup"]
        run([sdk / "spirv-opt.exe", *options, raw, "-o", compute_spv], log)

        for stage in ("rgen", "comp"):
            spv = compiled / f"minimal.{stage}.spv" if stage == "rgen" else compute_spv
            run([sdk / "spirv-val.exe", "--target-env", "vulkan1.2", spv], log)
            assembly = compiled / f"minimal.{stage}.spvasm"
            run([sdk / "spirv-dis.exe", spv, "-o", assembly], log)
            asm = assembly.read_text(encoding="utf-8")
            if "OpReadClockKHR" not in asm or "ShaderClockKHR" not in asm:
                raise RuntimeError(f"Clock instrumentation was optimized out of {key}/{stage}")
            if not re.search(r"OpDecorate\s+%rtInstances\s+Binding\s+11", asm):
                raise RuntimeError(f"Ordinary ABI instance metadata binding is missing in {key}/{stage}")
            if re.search(r"\bOpAtomic\w+|\bBinding 22\b", asm):
                raise RuntimeError(f"Unexpected diagnostic atomics/binding in {key}/{stage}")
            if material == "opaque_fast" and stage == "rgen" and \
                    (len(re.findall(r"\bOpFunction\b", asm)) != 1 or "OpFunctionCall" in asm):
                raise RuntimeError("Shipping/Mobile Opaque Pipeline changed its admitted inlining shape")
            if material == "generic_dielectric" and stage == "rgen" and \
                    len(re.findall(r"\bOpRayQueryInitializeKHR\b", asm)) > 3:
                raise RuntimeError("Shipping/Mobile Generic Pipeline query shape is unexpected")
            name = key if stage == "rgen" else "rayquery_compute_" + key
            data = spv.read_bytes()
            if len(data) % 4:
                raise RuntimeError(f"SPIR-V is not word-aligned: {name}")
            words = struct.unpack("<" + "I" * (len(data) // 4), data)
            include_text = _include_text(words)
            artifacts.append({"key": name, "sha256": sha256(data), "includeSha256": sha256(include_text.encode()),
                "bytes": len(data), "words": len(words),
                "clockSites": len(re.findall(r"\bOpReadClockKHR\b", asm)),
                "querySites": len(re.findall(r"\bOpRayQueryInitializeKHR\b", asm)),
                **({"baseShippingSha256": base_modules[name]["spirvSha256"],
                   "differsFromBase": sha256(data) != base_modules[name]["spirvSha256"]}
                   if name in base_modules else {}),
                "productionBudgetAdmission": False})
            pending.append((name, stage, data, include_text, artifacts[-1]))

    if not all(row["clockSites"] > 0 and (not base_modules or
               sha256(item[2]) != base_modules[item[0]]["spirvSha256"])
               for row, item in zip(artifacts, pending)):
        raise RuntimeError("Observer modules are stale, lack clock instrumentation, or differ from no base module")
    for name, stage, data, include_text, row in pending:
        (source / "src/vulkan/raytracing/variants" / (name + ".inc")).write_text(
            include_text, encoding="utf-8", newline="\n")
        header = source / "src/vulkan/raytracing" / (
            "RtPipelineVariantCatalog.generated.h" if stage == "rgen" else "RtRayQueryVariantCatalog.generated.h")
        old = header.read_text(encoding="utf-8")
        pattern = rf'("{name}", "src/vulkan/raytracing/variants/{re.escape(name)}\.inc", )"[0-9a-f]+", "[0-9a-f]+", \d+, 0, false'
        updated, count = re.subn(pattern, lambda match: match[1] +
            f'"{sha256(data)}", "{sha256(include_text.encode())}", {row["words"]}, 0, false', old)
        if count != 1:
            raise RuntimeError(f"Expected one private catalog row for {name}, found {count}")
        header.write_text(updated, encoding="utf-8", newline="\n")

    receipt_path = folder / "source-receipt.json"
    receipt["compiledObserverModules"] = artifacts
    receipt["compiledObserverCatalogHashes"] = {
        "RtPipelineVariantCatalog.generated.h": _sha_file(source / "src/vulkan/raytracing/RtPipelineVariantCatalog.generated.h"),
        "RtRayQueryVariantCatalog.generated.h": _sha_file(source / "src/vulkan/raytracing/RtRayQueryVariantCatalog.generated.h"),
    }
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    (folder / "compiled-observer.json").write_text(json.dumps(artifacts, indent=2) + "\n", encoding="utf-8")
    if marker.get("layout") == "ordinary":
        write_ordinary_compiled_marker(source, source / "dispatch-clock-probe.json")
    print(json.dumps(artifacts, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", type=Path)
    compile_private(parser.parse_args().folder)
