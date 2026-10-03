"""Verify frozen native viewmodel evidence; does not certify visual quality or motion."""
import hashlib
import json
import sys
from pathlib import Path

from PIL import Image, ImageChops

root, output = map(Path, sys.argv[1:3])
if output.exists():
    raise RuntimeError("Use a new output path; preserve previous evidence")
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
names = ["player-body-grips"] + ["player-viewmodel-" + suffix for suffix in
    ["grips", "forward", "downward-cut", "upward-slice", "look-up", "look-down", "lantern-high", "lantern-low"]]
rows = []
controls = None
for name in names:
    folder = root / "captures" / name
    manifest = json.loads((folder / "capture-manifest.json").read_text(encoding="utf-8-sig"))
    assert manifest["complete"] and manifest["source"] == "rt-storage-image"
    assert manifest["sceneOnly"] and not manifest["overlaysIncluded"]
    assert manifest["executionBackend"] == "RayTracingPipeline"
    assert len(manifest["captures"]) == 1
    actual_controls = {key: manifest[key] for key in ["device", "presentation", "selectedRtPipelineBundle",
        "settlingFrames", "fixedAnimationTimeSeconds"]}
    if controls is None:
        controls = actual_controls
    assert actual_controls == controls, name
    capture = manifest["captures"][0]
    assert capture["checkpoint"] == name and capture["honestlyPresentedRtFrame"]
    assert capture["camera"] == capture["requestedCamera"], name
    path = folder / capture["file"]
    assert digest(path) == capture["pngSha256"]
    visibility = capture["visibility"]
    if name.startswith("player-viewmodel-"):
        masks = visibility["instanceMasks"]
        assert masks[4] == 0x10 and masks[20] == 0x40 and not any(masks[10:17]), name
        assert visibility["diagnostics"]["available"] and visibility["primaryPixels"]["player"] > 0
    rows.append(dict(checkpoint=name, camera=capture["camera"], pngSha256=digest(path),
        manifestSha256=digest(folder / "capture-manifest.json"),
        primaryPlayerPixels=visibility["primaryPixels"]["player"],
        transportOverflowCount=manifest["dielectricDiagnostics"]["transportOverflowCount"],
        unclosedVolumeCount=manifest["dielectricDiagnostics"]["unclosedVolumeCount"]))

before_folder = root.parent / "2026-09-23-player-admission/candidate/player-body-grips"
after_folder = root / "captures/player-body-grips"
before = json.loads((before_folder / "capture-manifest.json").read_text(encoding="utf-8-sig"))
after = json.loads((after_folder / "capture-manifest.json").read_text(encoding="utf-8-sig"))
for key in ["device", "presentation", "source", "sceneOnly", "overlaysIncluded", "executionBackend",
            "settlingFrames", "fixedAnimationTimeSeconds"]:
    assert before[key] == after[key], key
assert before["captures"][0]["camera"] == after["captures"][0]["camera"]
paths = [folder / manifest["captures"][0]["file"] for folder, manifest in
         [(before_folder, before), (after_folder, after)]]
images = [Image.open(path).convert("RGB") for path in paths]
assert images[0].size == images[1].size
delta = ImageChops.difference(*images)
maximum = max(channel[1] for channel in delta.getextrema())
different = sum(max(pixel) > 1 for pixel in delta.getdata())
fraction = different / (images[0].width * images[0].height)
passed = maximum <= 3 and fraction <= .001
report = dict(schema=1, scope="Frozen RT ownership/pose evidence, not art acceptance, motion, glass correctness, performance or backend parity",
    controls=controls, captures=rows,
    worldBodyRegression=dict(beforePngSha256=digest(paths[0]), afterPngSha256=digest(paths[1]),
        maximumChannelDifference=maximum, pixelsDifferentByMoreThanOne=different,
        differentFraction=fraction, maximumAllowedChannelDifference=3,
        maximumAllowedFractionDifferentByMoreThanOne=.001, passed=passed))
output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps(report["worldBodyRegression"], indent=2))
sys.exit(0 if passed else 1)
