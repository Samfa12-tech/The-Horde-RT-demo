"""Author a material-only sidecar; never write a model, texture or existing output.

Run from the checkout with --output-directory NEW_DIRECTORY. To repeat after
admission, pass the preserved baseline.manifest.json through --baseline.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    with path.open("x", encoding="utf-8", newline="\n") as output:
        json.dump(value, output, indent=2)
        output.write("\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("--factor", type=float, default=0.5)
    args = parser.parse_args()
    root = args.root.resolve()
    runtime = root / "assets/models/world/runtime/chapter-terrain-v1"
    baseline = args.baseline or runtime / "asset.manifest.json"
    model = runtime / "chapter-terrain-lod0.runtime.glb"
    source = root / "assets/models/world/source/chapter-terrain-r21/chapter-terrain-r21.glb"
    if not math.isfinite(args.factor) or not 0.0 <= args.factor <= 1.0:
        parser.error("Factor must be finite and within [0, 1]")
    if args.output_directory.exists():
        parser.error("Choose a new output directory; existing sources/backups are preserved")
    model_hash = sha(model)
    if model_hash != "fb21e177b8e67fb553d4853a778603a598908812fc1603ca8a16d94d152a4854" or sha(source) != model_hash:
        parser.error("Expected byte-identical admitted r21 source/runtime GLBs")
    original = json.loads(baseline.read_text(encoding="utf-8-sig"))
    if original["asset"] != "chapter-terrain-v1" or original["materialOverrides"]:
        parser.error("Use the preserved untuned runtime baseline manifest")
    texture_paths = sorted((root / "assets/textures/props/runtime/chapter-terrain").glob("*.ktx2"))
    if len(texture_paths) != 6:
        parser.error("Expected all six unchanged platform terrain texture arrays")
    texture_hashes = {p.relative_to(root).as_posix(): sha(p) for p in texture_paths}
    candidate = json.loads(json.dumps(original))
    candidate["materialOverrides"] = [{
        "material": "ChapterForestGround",
        "baseColorFactor": [args.factor, args.factor, args.factor, 1.0],
    }]
    # All input admission checks precede creation. Outputs use exclusive writes.
    args.output_directory.mkdir(parents=True, exist_ok=False)
    (args.output_directory / "baseline.manifest.json").write_bytes(baseline.read_bytes())
    write_json(args.output_directory / "asset.manifest.json", candidate)
    write_json(args.output_directory / "derivation-receipt.json", {
        "scope": "Linear PBR material-only tuning; no geometry/light/shader/texture/limit changes",
        "parentSource": "assets/models/world/source/chapter-terrain-r21",
        "modelSha256": model_hash,
        "baselineManifestSha256": sha(baseline),
        "candidateManifestSha256": sha(args.output_directory / "asset.manifest.json"),
        "recipeSha256": sha(Path(__file__)),
        "baseColorFactor": candidate["materialOverrides"][0]["baseColorFactor"],
        "unchangedTextureArraySha256": texture_hashes,
        "trailFactor": "Unchanged imported glTF value",
        "ownerVisualAcceptance": "Pending matched RT review",
    })
    print(f"Authored material-only factor {args.factor}: {args.output_directory}")


if __name__ == "__main__":
    main()
