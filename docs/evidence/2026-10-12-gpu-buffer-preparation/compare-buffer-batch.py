import hashlib
import json
from pathlib import Path
from PIL import Image

root = Path.cwd()
candidate = root / "reports/gpu-buffer-batch-visual"
baseline = root / "docs/evidence/2026-10-12-selected-zone-geometry"

def compare(a, b):
    with Image.open(a) as source_a, Image.open(b) as source_b:
        a_pixels, b_pixels = source_a.convert("RGB"), source_b.convert("RGB")
        assert a_pixels.size == b_pixels.size
        count, maximum = 0, 0
        for x, y in zip(a_pixels.get_flattened_data(), b_pixels.get_flattened_data()):
            differences = [abs(x[i] - y[i]) for i in range(3)]
            count += int(any(differences))
            maximum = max(maximum, *differences)
        return {"changedPixels": count, "maximumChannelDifference": maximum,
                "candidateSha256": hashlib.sha256(a.read_bytes()).hexdigest(),
                "baselineSha256": hashlib.sha256(b.read_bytes()).hexdigest()}

comparisons = []
for backend in ("pipeline", "query"):
    for scene, file in (("pullup", "211-chapter-stake-pullup.png"),
                        ("bend", "202-chapter-forest-second-bend.png")):
        # Previous checked-in evidence uses short final image names.
        comparisons.append({"scene": scene, "backend": backend,
            "baseline": "d6f6dcd8 inspected native scene",
            **compare(candidate / f"{scene}-{backend}" / file,
                      baseline / f"{scene}-{backend}.png")})
    preview = candidate / f"preview-{backend}"
    manifest = json.loads((preview / "graphics-preview-capture-manifest.json").read_text())
    previous = root / f"reports/issue23-current-higher-{backend}"
    prior = json.loads((previous / "graphics-preview-capture-manifest.json").read_text())
    assert manifest["complete"] and prior["complete"]
    assert manifest["executionBackend"] == prior["executionBackend"]
    for frame in manifest["captures"]:
        earlier = next(item for item in prior["captures"] if item["name"] == frame["name"])
        assert frame["camera"] == earlier["camera"]
        assert frame["graphics"]["effective"] == earlier["graphics"]["effective"]
        assert frame["honestlyPresentedRtFrame"]
        comparisons.append({"scene": frame["name"], "backend": backend,
            "baseline": "retained issue23 compact preview evidence",
            **compare(preview / frame["file"], previous / earlier["file"])})

destination = root / "reports/gpu-buffer-batch-frame-comparisons.json"
destination.write_text(json.dumps(comparisons, indent=2) + "\n")
print(json.dumps({"comparisons": len(comparisons),
                  "unchanged": sum(row["changedPixels"] == 0 for row in comparisons),
                  "changed": [row for row in comparisons if row["changedPixels"] != 0]}, indent=2))
