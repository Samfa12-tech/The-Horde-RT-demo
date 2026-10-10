#!/usr/bin/env python3
"""Author and Blender-roundtrip the original development rescue blockout."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_geometry(header: Path) -> tuple[list[dict], dict, dict]:
    text = header.read_text(encoding="utf-8")
    block_region = text.split("kRescueBlockoutBoxes{{", 1)[1].split("}};", 1)[0]
    blocks = []
    pattern = re.compile(r"\{\{\{([^}]+)\}\},\s*\{\{([^}]+)\}\},\s*(\d+)u\}")
    for match in pattern.finditer(block_region):
        minimum = [float(v.strip().removesuffix("f")) for v in match.group(1).split(",")]
        maximum = [float(v.strip().removesuffix("f")) for v in match.group(2).split(",")]
        blocks.append({"min": minimum, "max": maximum, "materialCode": int(match.group(3))})
    if len(blocks) != 12:
        raise ValueError(f"Expected 12 shared box records, parsed {len(blocks)} from {header}")

    def named_box(name: str) -> dict:
        match = re.search(rf"kRescueBlockout{name}\s*\{{\s*\{{\{{([^}}]+)\}}\}},\s*\{{\{{([^}}]+)\}}\}},\s*(\d+)u?\s*\}}", text)
        if not match:
            raise ValueError(f"Could not parse shared {name} bounds")
        return {"min": [float(v.strip().removesuffix("f")) for v in match.group(1).split(",")],
                "max": [float(v.strip().removesuffix("f")) for v in match.group(2).split(",")],
                "materialCode": int(match.group(3))}
    return blocks, named_box("Lid"), named_box("Landing")


BLENDER_SCRIPT = r'''import bpy, json, math, sys
from mathutils import Vector
args = json.loads(sys.argv[sys.argv.index("--") + 1])
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for material in list(bpy.data.materials):
    if material.users == 0: bpy.data.materials.remove(material)
materials = []
for name, code in (("DryStone", 0), ("MossyStone", 2)):
    color = (1,1,1,1)
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = color
        bsdf.inputs["Roughness"].default_value = 0.88
    mat["horde_existing_surface_material_code"] = code
    materials.append(mat)
names = {0:"DryStone", 2:"MossyStone"}
objects = []
def box(label, spec):
    lo, hi = spec["min"], spec["max"]
    # Blender is Z-up. Convert shared engine +Y/+Z coordinates into Blender
    # (X,-Z,Y); the glTF exporter converts them back to the engine axes.
    center = ((lo[0]+hi[0])/2, -(lo[2]+hi[2])/2, (lo[1]+hi[1])/2)
    dimensions = (hi[0]-lo[0], hi[2]-lo[2], hi[1]-lo[1])
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj=bpy.context.object; obj.name=label
    obj.dimensions=dimensions; bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    code=spec["materialCode"]; obj.data.materials.append(materials[0 if code==0 else 1])
    obj["horde_existing_surface_material_code"]=code
    uv=obj.data.uv_layers.new(name="UVMap")
    face_uv=((0,0),(1,0),(1,1),(0,1))
    for poly in obj.data.polygons:
        if len(poly.loop_indices)==4:
            for loop_index, coordinate in zip(poly.loop_indices, face_uv):
                uv.data[loop_index].uv=coordinate
    for poly in obj.data.polygons: poly.use_smooth=False
    objects.append(obj)
for i, spec in enumerate(args["boxes"]): box("BlockoutPiece_%02d"%i,spec)
box("SafetyLid",args["lid"])
box("UpperLanding",args["landing"])
# Join into one mesh while keeping material slots and named face groups.
bpy.ops.object.select_all(action="DESELECT")
for obj in objects: obj.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.object.join()
mesh=bpy.context.object; mesh.name="RescueShaftBlockout"
mesh["source"]="original Codex blockout; shared bounds in src/scene/RescueBlockoutGeometry.h"
mesh["package_policy"]="development source only; excluded from runtime packages"
bpy.ops.object.select_all(action="DESELECT"); mesh.select_set(True); bpy.context.view_layer.objects.active=mesh
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=args["blend"])
def export(path):
    bpy.ops.object.select_all(action="DESELECT"); mesh.select_set(True); bpy.context.view_layer.objects.active=mesh
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True,
        export_apply=True, export_yup=True, export_materials="EXPORT", export_cameras=False,
        export_lights=False, export_extras=True)
export(args["glb"])
# Real importer/re-export roundtrip in this same Blender process.
bpy.ops.object.select_all(action="SELECT"); bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=args["glb"])
bpy.ops.object.select_all(action="DESELECT")
for obj in bpy.context.scene.objects:
    if obj.type=="MESH": obj.select_set(True); bpy.context.view_layer.objects.active=obj
bpy.ops.export_scene.gltf(filepath=args["roundtrip"], export_format="GLB", use_selection=True,
    export_apply=True, export_yup=True, export_materials="EXPORT", export_cameras=False,
    export_lights=False, export_extras=True)
'''


MATERIAL_SOURCE_SPECS = (
    ("DryStone", 0, 0, "medieval_wall_02", {
        "diff.jpg": "c3123e0512b784dbfbb000fbe2cd7b5779c4b577ea2f221db6d59f3dbff2fd10",
        "normal.jpg": "3da467a4b25408892beda4c5d931feaf0b75a5424376f09a33b6356022a63da9",
        "arm.jpg": "b8b3186b6221ab007701d4b914b5bb3407ee17b4099dd2faaeded7d3208de0ea",
    }),
    ("MossyStone", 2, 2, "mossy_stone_wall", {
        "diff.jpg": "7240e55cfc662ea403600fc7d5143f72983fbe8098d55fc6ccae75d21421dce4",
        "normal.jpg": "e159e429269bc933743ee47051b4081261e6131e994242bf26e05ab0a0df4542",
        "arm.jpg": "c76fd89a80b95a1daa63af4b467a97117322207a04f7d59b86a5db160bbe8001",
    }),
)


def validate_material_sources(repo: Path) -> list[dict]:
    """Admit only the exact existing CC0 source maps used by these world codes."""
    texture_root = repo / "assets/textures/polyhaven/mobile_1k"
    records = []
    for name, code, layer, source_name, expected_hashes in MATERIAL_SOURCE_SPECS:
        maps = {}
        for channel, filename in (("baseColor", "diff.jpg"), ("normal", "normal.jpg"), ("orm", "arm.jpg")):
            path = texture_root / source_name / filename
            if not path.is_file():
                raise ValueError(f"missing source map for {name}/{channel}: {path}")
            actual = digest(path)
            expected = expected_hashes[filename]
            if actual != expected:
                raise ValueError(f"source map hash mismatch for {name}/{channel}: expected {expected}, got {actual}")
            maps[channel] = {"path": str(path.relative_to(repo)).replace("\\", "/"),
                             "bytes": path.stat().st_size, "sha256": actual}
        records.append({"name": name, "surfaceCode": code, "worldTextureLayer": layer,
                        "polyHavenAsset": source_name, "license": "CC0",
                        "licenseRecord": "ASSET_LICENSES.md", "sourceMaps": maps})
    return records


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--blender", type=Path, default=Path(r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"))
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()
    repo = args.repo.resolve()
    output = (args.output or repo / "assets/source/development-rescue").resolve()
    output.mkdir(parents=True, exist_ok=True)
    if not args.blender.is_file():
        raise FileNotFoundError(f"Blender executable not found: {args.blender}")
    header = repo / "src/scene/RescueBlockoutGeometry.h"
    boxes, lid, landing = read_geometry(header)
    all_boxes = [*boxes, lid, landing]
    recipe_bounds = {
        "min": [min(box["min"][axis] for box in all_boxes) for axis in range(3)],
        "max": [max(box["max"][axis] for box in all_boxes) for axis in range(3)],
    }
    # Gate all export on the existing licensed PBR inputs remaining present and
    # byte-identical to the admitted world material sources.
    texture_materials = validate_material_sources(repo)
    blender_args = {"blend": str(output / "rescue-shaft-blockout.blend"),
                    "glb": str(output / "rescue-shaft-blockout.glb"),
                    "roundtrip": str(output / "rescue-shaft-roundtrip.glb"),
                    "boxes": boxes, "lid": lid, "landing": landing}
    script = output / "_author_rescue_blockout.py"
    script.write_text(BLENDER_SCRIPT, encoding="utf-8", newline="\n")
    result = subprocess.run([str(args.blender), "--background", "--python", str(script), "--", json.dumps(blender_args)],
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    sanitized_log = result.stdout.replace(str(repo), "<repo>")
    sanitized_log = re.sub(r"C:[/\\]+Users[/\\]+[^/\\\s\"<>]+", "<user-home>", sanitized_log, flags=re.I)
    (output / "blender-run.log").write_text(sanitized_log, encoding="utf-8", newline="\n")
    if result.returncode != 0:
        print(result.stdout[-6000:], file=sys.stderr)
        return result.returncode
    script.unlink(missing_ok=True)
    # Provenance binds reused material semantics to the current runtime source
    # without importing textures, provider assets, or external archives.
    recipe_bytes = json.dumps({"boxes": boxes, "lid": lid, "landing": landing},
                              sort_keys=True, separators=(",", ":")).encode("utf-8")
    import_manifest = {
        "schema": 1,
        "asset": "development-rescue-shaft-blockout",
        "metresPerUnit": 1.0,
        "coordinateSystem": {"up": "+Y", "forward": "+Z"},
        "budgets": {"maxVertices": 400, "maxIndices": 600, "maxPrimitives": 4,
                    "maxMaterials": 2, "maxTextureLayersPerKind": 2},
        "lods": [{"name": "lod0", "maxTriangles": 200}],
        "requiredSockets": [],
        "runtimeTextureProfile": {"android": "astc", "windows": "rgba8", "mipmapped": True},
        "materialOverrides": []
    }
    (output / "asset.manifest.json").write_text(
        json.dumps(import_manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
    source_record = {
        "schema": 1,
        "asset": "development-rescue-shaft-blockout",
        "source": "Original Blender cube geometry generated from RescueBlockoutGeometry.h",
        "sourceLicense": "Project-created original geometry; no third-party source assets",
        "editableSource": "rescue-shaft-blockout.blend",
        "glb": "rescue-shaft-blockout.glb",
        "blenderRoundtripGlb": "rescue-shaft-roundtrip.glb",
        "nativeImportManifest": "asset.manifest.json",
        "unit": "metres",
        "coordinateSystem": {"up": "+Y", "forward": "+Z"},
        "geometryHeader": str(header.relative_to(repo)).replace("\\", "/"),
        "geometryRecipe": {
            "identity": "horde.scene.RescueBlockoutGeometry.v1",
            "symbols": ["kRescueBlockoutBoxes", "kRescueBlockoutLid", "kRescueBlockoutLanding"],
            "sha256": hashlib.sha256(recipe_bytes).hexdigest()
        },
        "existingMaterialCodes": texture_materials,
        "materialPaletteSource": {
            "identity": "horde.world.surface-material-codes.v1",
            "semanticReference": "src/vulkan/raytracing/PresentableTinyRtScene.cpp::enum SurfaceMaterial",
            "surfaceCodes": {"SurfaceDryStone": 0, "SurfaceMossyStone": 2}
        },
        "packagePolicy": "Development source only; no runtime package inventory references this directory.",
        "bounds": recipe_bounds,
        "files": {}
    }
    for name in ("rescue-shaft-blockout.blend", "rescue-shaft-blockout.glb",
                 "rescue-shaft-roundtrip.glb", "asset.manifest.json"):
        path = output / name
        source_record["files"][name] = {"bytes": path.stat().st_size, "sha256": digest(path)}
    (output / "source-manifest.json").write_text(json.dumps(source_record, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"output": str(output), "files": source_record["files"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
