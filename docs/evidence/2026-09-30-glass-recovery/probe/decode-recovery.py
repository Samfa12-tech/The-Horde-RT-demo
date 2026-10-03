"""Decode investigation-only output-image records; never edits source images.

RGBA files start with little-endian uint32 width/height. A field occupies two
pixels: RGB of the first are low bytes; R of the second is the high byte. The
native capture already normalises outputRedBlueSwap. Android screenshots do
not preserve these bytes through display colour management.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from PIL import Image

FIELDS = """px py pz nx ny nz dx dy dz t instance material primitive open hit
tir total roughness ior attenuation throughputR throughputG throughputB flags
transmission""".split()
TARGETS = {
    "lantern-glass-production": [(0, (346, 1200)), (1, (340, 1046))],
    "glass-edge-fresnel": [(2, (638, 1216))],
    "player-viewmodel-lantern-high-look-up": [(3, (193, 1537)), (4, (427, 1520))],
}
MARKERS = {"opaque": (255, 0, 0, 255), "miss": (0, 255, 0, 255),
           "certifiedBudget": (255, 255, 0, 255),
           "independentBudget": (0, 0, 255, 255)}


def decode(path, targets, slots):
    original = path.read_bytes()
    if path.suffix == ".png":
        image = Image.open(path).convert("RGBA")
        data = struct.pack("<II", *image.size) + image.tobytes()
    else:
        data = original
    width, height = struct.unpack("<II", data[:8])
    if len(data) != 8 + width * height * 4 or width < slots * 50:
        raise ValueError(f"Invalid native capture dimensions: {path}")
    image = Image.frombytes("RGBA", (width, height), data[8:])
    counts = dict((colour, count) for count, colour in image.getcolors(width * height))
    paths = []
    for row, pixel in targets:
        records = []
        for slot in range(slots):
            record = {}
            for i, name in enumerate(FIELDS):
                offset = 8 + 4 * (row * width + slot * 50 + i * 2)
                record[name] = struct.unpack("<f", data[offset:offset + 3] +
                                             data[offset + 4:offset + 5])[0]
            records.append(record)
        paths.append({"recordRow": row, "selectedPixel": pixel, "records": records})
    return {"file": path.name, "sha256": hashlib.sha256(original).hexdigest(),
            "width": width, "height": height,
            "markerCounts": {name: counts.get(colour, 0)
                             for name, colour in MARKERS.items()}, "paths": paths}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--position-probe", action="store_true")
    parser.add_argument("--windows-record", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    targets = {"player-viewmodel-lantern-high-look-up": [(3, (193, 1537))]} if args.position_probe else TARGETS
    captures = [decode(args.root, [(0, (328, 842))], 5)] if args.windows_record else [
        decode(args.root / (scene + ".rgba"), rows, 13 if args.position_probe else 5)
        for scene, rows in targets.items()]
    result = {"schema": 1, "investigationOnly": True, "captures": captures}
    text = json.dumps(result, indent=2, allow_nan=False) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8", newline="\n")
    else:
        print(text, end="")


if __name__ == "__main__":
    main()
