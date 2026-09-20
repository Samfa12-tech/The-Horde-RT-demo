"""Compare the observed frozen scene, excluding the changing Android HUD band."""
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops, ImageStat

root = Path(__file__).resolve().parent
paths = [root / "baseline-a1-start.png", root / "candidate-b1-start.png"]
images = [Image.open(path).convert("RGB") for path in paths]
assert all(image.size == (1440, 3120) for image in images)
# The only benchmark overlay is in the top band. Keep all unobscured lower RT
# pixels; these are compositor screenshots, NOT raw RT-storage-image captures.
region = (0, 400, 1440, 3120)
diff = ImageChops.difference(*(image.crop(region) for image in images))
maximum = max(high for low, high in diff.getextrema())
report = {
    "source": "android-presented-compositor-screenshots",
    "workload": "lantern-held-high-v1",
    "simulationPolicy": "frozen-authored-snapshot",
    "region": region,
    "excluded": "top 400 rows, including differing benchmark HUD text",
    "pngSha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in paths},
    "maxRgbDelta": maximum,
    "meanRgbDelta": ImageStat.Stat(diff).mean,
    "existingRgbTolerance": 3,
    "withinTolerance": maximum <= 3,
    "qualification": "One presented frozen-view comparison, not full transport correctness or same-build Diagnostic/Shipping parity.",
}
print(json.dumps(report, indent=2))
