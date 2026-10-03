"""Read lossless investigation capture records; never modifies source images."""
import hashlib
import json
import struct
import sys
from pathlib import Path
from PIL import Image

FIELDS = """px py pz nx ny nz dx dy dz t instance material primitive open hit
tir total roughness ior attenuation throughputR throughputG throughputB flags
transmission volumeInstance volumeMaterial volumeIor certified entering
originX originY originZ epsilon outgoingX outgoingY outgoingZ""".split()
TARGETS = [(576, 574), (504, 577), (515, 652)]
FIELDS += [f"v{vertex}{axis}" for vertex in range(3) for axis in "xyz"]
FIELDS += [f"m{col}{axis}" for col in range(4) for axis in "xyz"]
FIELDS += [f"objectOrigin{axis}" for axis in "xyz"]
FIELDS += [f"objectDirection{axis}" for axis in "xyz"]
path = Path(sys.argv[1])
image = Image.open(path).convert("RGBA")
width, height = image.size
if (width, height) != (1232, 803):
    raise ValueError("This probe requires the exact live RTX extent.")
data = image.tobytes()
records = []
selected = list(enumerate(TARGETS))
if len(sys.argv) > 2 and sys.argv[2] == "--alternatives":
    selected = [(3 + target * 6 + probe, pixel) for target, pixel in enumerate(TARGETS) for probe in range(6)]
for row, pixel in selected:
    slots = []
    for slot in range(2 if row >= 3 else 9):
        record = {}
        for field, name in enumerate(FIELDS):
            offset = 4 * (row * width + slot * 136 + field * 2)
            record[name] = struct.unpack("<f", data[offset:offset+3] + data[offset+4:offset+5])[0]
        if record["hit"] == 0 and slot > 0:
            break
        slots.append(record)
    records.append({"recordRow": row, "pixel": pixel, "records": slots})
print(json.dumps({"investigationOnly": True, "file": path.name,
    "sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "paths": records},
    indent=2, allow_nan=False))
