"""Preserve verified CC0 originals and losslessly decode approved 1K atlas inputs."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

from PIL import Image

parser = argparse.ArgumentParser()
parser.add_argument("--boulder-source", type=Path, required=True)
args = parser.parse_args()
repo = Path(__file__).resolve().parent.parent
original = args.boulder_source.resolve()
preserved = repo / "assets/models/world/source/collapsed-entry/polyhaven-original"
output = repo / "assets/textures/props/source/collapsed-entry"
preserved.mkdir(parents=True, exist_ok=True)
output.mkdir(parents=True, exist_ok=True)
receipt = json.loads((original / "download-receipt.json").read_text(encoding="utf-8-sig"))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

for item in receipt:
    source = original / item["path"]
    if source.stat().st_size != item["bytes"] or sha(source) != item["sha256"]:
        raise RuntimeError("Original source receipt mismatch: " + item["path"])
    target = preserved / item["path"]
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
shutil.copyfile(original / "download-receipt.json", preserved / "download-receipt.json")
license_sha = "0b7a23e05e933f7f28ffab64ac1302e215f44eeb69abbfac4f2c4fd331101be5"
if sha(original / "polyhaven-license.html") != license_sha:
    raise RuntimeError("Preserved CC0 licence snapshot changed")
shutil.copyfile(original / "polyhaven-license.html", preserved / "polyhaven-license.html")

maps = []
families = [
    ("boulder01", "Boulder01Rock", preserved / "textures", "boulder_01_diff_1k.jpg",
     "boulder_01_nor_gl_1k.jpg", "boulder_01_arm_1k.jpg", .42, "https://polyhaven.com/a/boulder_01"),
    ("medieval-wall02", "MedievalWall02", repo / "assets/textures/polyhaven/mobile_1k/medieval_wall_02",
     "diff.jpg", "normal.jpg", "arm.jpg", .34, "https://polyhaven.com/a/medieval_wall_02"),
]
for prefix, material, directory, base, normal, orm, strength, asset_url in families:
    for category, filename in zip(("base-color", "normal", "orm"), (base, normal, orm)):
        source = directory / filename
        target = output / (prefix + "-" + category + ".png")
        with Image.open(source) as image:
            if image.size != (1024, 1024):
                raise RuntimeError("Approved texture is not 1K: " + filename)
            decoded = image.convert("RGBA")
            decoded.save(target, format="PNG", compress_level=9, optimize=False)
            with Image.open(target) as check:
                if check.convert("RGBA").tobytes() != decoded.tobytes():
                    raise RuntimeError("PNG conversion changed decoded texels")
        maps.append({"material": material, "category": category, "source": source.relative_to(repo).as_posix(),
                     "sourceSha256": sha(source), "sourceBytes": source.stat().st_size,
                     "file": target.name, "sha256": sha(target), "bytes": target.stat().st_size,
                     "resolution": [1024, 1024], "format": "RGBA8 PNG", "decodedPixelsPreserved": True,
                     "transfer": "sRGB" if category == "base-color" else "linear",
                     "normalScale": strength, "assetSource": asset_url, "license": "CC0 1.0"})

report = {"schema": "horde.collapse-texture-inputs.v1", "materialOrder": ["Boulder01Rock", "MedievalWall02"],
          "atlasLayerOrder": [10, 11], "maps": maps, "normalConvention": "glTF/OpenGL +Y",
          "ormChannels": {"R": "occlusion", "G": "roughness", "B": "metallic"},
          "metallicFactor": 0, "roughnessFactor": 1, "imageRetouchOrResizing": False,
          "licenseSource": "https://polyhaven.com/license", "licenseSnapshotSha256": license_sha,
          "licenseSnapshot": (preserved / "polyhaven-license.html").relative_to(repo).as_posix(),
          "originalSourceReceipt": (preserved / "download-receipt.json").relative_to(repo).as_posix(),
          "licensingStatus": "CC0 reuse and project-authored architecture; native Runtime acceptance pending"}
(output / "input-receipt.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps({"receipt": str(output / "input-receipt.json"), "maps": len(maps),
                  "encodedInputBytes": sum(m["bytes"] for m in maps)}))
