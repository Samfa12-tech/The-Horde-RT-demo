"""Decode only native private RT bytes from the temporary isolated-ray probe.

No source image edits, screenshot resampling, or production acceptance override.
Records contain actual native committed candidates and the uploaded triangle.
"""
import argparse
import hashlib
import gzip
import json
import math
from pathlib import Path
import struct

FIELDS = """px py pz nx ny nz dx dy dz t instance material primitive open hit
tir total roughness ior attenuation throughputR throughputG throughputB flags transmission
spawnX spawnY spawnZ guarded minimumNormalBias
queryX queryY queryZ queryDX queryDY queryDZ
objectX objectY objectZ objectDX objectDY objectDZ
baryU baryV queryMinimum rawDistance geometryIndex
v0X v0Y v0Z v1X v1Y v1Z v2X v2Y v2Z
certified directionNormalDot slot""".split()


def read_native(path):
    data = path.read_bytes() if path.exists() else gzip.decompress(path.with_suffix(path.suffix + ".gz").read_bytes())
    width, height = struct.unpack("<II", data[:8])
    if (width, height) != (1080, 2235) or len(data) != 8 + width * height * 4:
        raise ValueError("Unexpected capture extent or length")
    return data, width, height


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    root = args.root
    marker = json.loads((root / "marker-analysis.json").read_text())
    targets = marker["markerCoordinates"]["interfaceBudget"] + marker["markerCoordinates"]["certifiedBudgetReason2"]
    if len(targets) != 81 or len({tuple(p) for p in targets}) != 81 or len(FIELDS) != 59:
        raise ValueError("Target or field contract changed")
    data, width, height = read_native(root / "path-native.rgba")
    baseline, _, _ = read_native(root / "control-native.rgba")
    marked, _, _ = read_native(root / "marker-native.rgba")
    paths = []
    for row, pixel in enumerate(targets):
        records = []
        for slot in range(5):
            record = {}
            for i, name in enumerate(FIELDS):
                offset = 8 + 4 * (row * width + slot * 118 + i * 2)
                value = struct.unpack("<f", data[offset:offset + 3] + data[offset + 4:offset + 5])[0]
                if not math.isfinite(value):
                    raise ValueError(f"Nonfinite record row={row} slot={slot} field={name}")
                record[name] = value
            if record["slot"] != slot or record["hit"] != 1.0:
                raise ValueError(f"Selected five-hit path no longer reproduced: {row}/{slot}")
            records.append(record)
        paths.append({"recordRow": row, "selectedPixel": pixel,
                      "classification": "interfaceBudget" if row == 0 else "certifiedBudgetReason2",
                      "records": records})
    differences = 0
    max_delta = 0
    coordinates = []
    # Reserved output records are declared diagnostic-only; compare all remaining
    # native pixels with the output-marker control, without relaxing any game gate.
    for y in range(81, height):
        for x in range(width):
            pos = 8 + 4 * (y * width + x)
            if data[pos:pos+4] != marked[pos:pos+4]:
                differences += 1
                max_delta = max(max_delta, max(abs(a-b) for a,b in zip(data[pos:pos+4], marked[pos:pos+4])))
                if len(coordinates) < 20:
                    coordinates.append([x, y])
    print(json.dumps({"schema": 1, "investigationOnly": True,
        "module": "Diagnostic/Mobile/GenericDielectric pipeline only; compute unchanged",
        "nativeSha256": hashlib.sha256(data).hexdigest(),
        "controlSha256": hashlib.sha256(baseline).hexdigest(),
        "markerSha256": hashlib.sha256(marked).hexdigest(),
        "extent": {"width": width, "height": height}, "reservedRows": 81,
        "nonrecordPixelComparison": {"reference": "marker-native.rgba",
            "changedPixels": differences, "maximumDelta": max_delta, "firstCoordinates": coordinates},
        "paths": paths}, indent=2, allow_nan=False))


if __name__ == "__main__":
    main()
