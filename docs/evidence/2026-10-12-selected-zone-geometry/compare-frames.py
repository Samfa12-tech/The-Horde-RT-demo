"""Read-only full-resolution comparisons; no image editing or quality changes."""
import json
import sys
from pathlib import Path
from PIL import Image

root = Path(sys.argv[1]).resolve()
final = root / "docs/evidence/2026-10-12-selected-zone-geometry"
parent = root / "docs/evidence/2026-10-12-ground-stake-traversal"
pairs = []
for backend in ("pipeline", "query"):
    pairs.append((f"pullup-{backend}-parent-to-final",
                  parent / f"pullup-{backend}-corrected.png",
                  final / f"pullup-{backend}.png"))
for view in ("pullup", "bend"):
    pairs.append((f"{view}-backend-comparison",
                  final / f"{view}-pipeline.png",
                  final / f"{view}-query.png"))
result = []
for label, left_path, right_path in pairs:
    with Image.open(left_path) as left_source, Image.open(right_path) as right_source:
        left, right = left_source.convert("RGB"), right_source.convert("RGB")
        if left.size != right.size:
            raise ValueError(f"Unmatched dimensions for {label}")
        changed = 0
        maximum = 0
        for a, b in zip(left.get_flattened_data(), right.get_flattened_data()):
            if a != b:
                changed += 1
                maximum = max(maximum, *(abs(x - y) for x, y in zip(a, b)))
        result.append({"comparison": label, "left": str(left_path.relative_to(root)),
                       "right": str(right_path.relative_to(root)), "width": left.width,
                       "height": left.height, "changedPixels": changed,
                       "maxAbsoluteChannelDifference": maximum})
print(json.dumps(result, indent=2))
