"""Read-only exact raw shadow queries for the selected finale pixel."""
import hashlib
import json
from pathlib import Path
import struct
from PIL import Image

path = Path(__file__).resolve().parent.parent / 'probe/near-field/11-finale-roof.png'
fields = ('ox oy oz dx dy dz distance t instance primitive geometry front r g b '
          'interfaces depth reason px py pz material flags transmission slot').split()
with Image.open(path) as image:
    width, height = image.size
    data = image.convert('RGBA').tobytes()
rows = []
for row in range(12):
    result = {}
    for index, field in enumerate(fields):
        offset = 4 * (row * width + index * 2)
        result[field] = struct.unpack('<f', data[offset:offset+3] + data[offset+4:offset+5])[0]
    if result['slot'] == row and result['distance'] > 0 and result['reason'] in (0, 1):
        rows.append(result)
print(json.dumps(dict(investigationOnly=True, selectedPixel=[577,534],
                     file=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                     records=rows), indent=2, allow_nan=False))
