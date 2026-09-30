"""Decode the existing lossless output-row witness; no image/source writes."""
import hashlib
import json
import struct
import sys
from pathlib import Path
from PIL import Image

fields = """px py pz nx ny nz dx dy dz t instance material primitive open hit
tir total roughness ior attenuation throughputR throughputG throughputB flags
transmission volumeInstance volumeMaterial volumeIor certified entering
originX originY originZ epsilon outgoingX outgoingY outgoingZ""".split()
fields += [f"v{vertex}{axis}" for vertex in range(3) for axis in "xyz"]
fields += [f"m{column}{axis}" for column in range(4) for axis in "xyz"]
fields += [f"objectOrigin{axis}" for axis in "xyz"]
fields += [f"objectDirection{axis}" for axis in "xyz"]
fields += ["guarded", "spawnX", "spawnY", "spawnZ", "queryMinimum"]
fields += [f"queryOrigin{axis}" for axis in "xyz"]
fields += [f"queryDirection{axis}" for axis in "xyz"]
fields += ["minimumWidth", "geometryError", "baryU", "baryV", "rectangularEligible",
           "nextQueryMinimum", "rawQueryT", "geometryIndex"]
fields += [f"inverse{column}{axis}" for column in range(4) for axis in "xyz"]
fields += ["spawnErrorX", "spawnErrorY", "spawnErrorZ", "normalClearance", "normalError",
           "oppositeUsage", "oppositeLimit", "spawnRejection", "minimumWeightSum",
           "insetDisplacement", "insetLimit"]
fields += ["minimumNormalBias"]
assert len(fields) == 107
field_count = int(sys.argv[2]) if len(sys.argv) > 2 else 107
assert field_count in (83, 106, 107)
fields = fields[:field_count]
path = Path(sys.argv[1])
im = Image.open(path).convert("RGBA")
assert im.size == (1232, 803)
data = im.tobytes()
paths = []
for row, pixel in enumerate([(515, 569), (515, 570)]):
    slots = []
    for slot in range(9):
        record = {}
        for field, name in enumerate(fields):
            output_row = row if field < 68 else row + 2
            column = slot * 136 + field * 2 if field < 68 else slot * 80 + (field - 68) * 2
            offset = 4 * (output_row * 1232 + column)
            record[name] = struct.unpack("<f", data[offset:offset + 3] + data[offset + 4:offset + 5])[0]
        # Never discard the terminal miss or the entry that triggers rejection.
        if slot > 0 and record["queryMinimum"] == 0 and record["rawQueryT"] == 0 and record["hit"] == 0:
            break
        slots.append(record)
    paths.append({"pixel": pixel, "records": slots})
print(json.dumps({"investigationOnly": True, "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                  "file": path.name, "paths": paths}, indent=2, allow_nan=False))
