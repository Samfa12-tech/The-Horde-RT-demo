"""Compare only the named geometric-shadow RTX captures against the frozen RGB gate."""

import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops


ROOT = Path(__file__).resolve().parent.parent / "windows"
OUT = Path("reports/geometric-shadow-pixels-reproduced.json")
QUALITIES = ("mobile", "high")
CONFIGS = ("baseline", "candidate", "compute")
SCENES = (
    "lantern-glass-production",
    "player-viewmodel-lantern-high-look-up",
    "player-viewmodel-lantern-low-parry",
    "glass-edge-fresnel",
    "glass-millimetre-closed",
    "glass-tinted-transport",
    "glass-fire-transport",
)
COUNTER_GROUPS = ("dielectricDiagnostics", "dielectricReasonDiagnostics")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_capture(quality, config, scene):
    folder = ROOT / quality / config / scene
    manifest_path = folder / "capture-manifest.json"
    if not manifest_path.is_file():
        return None, "manifest-missing"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return None, f"manifest-invalid:{error}"
    captures = [capture for capture in manifest.get("captures", [])
                if capture.get("checkpoint") == scene]
    if (manifest.get("complete") is not True or manifest.get("source") != "rt-storage-image" or
            manifest.get("sceneOnly") is not True or manifest.get("overlaysIncluded") is not False or
            len(captures) != 1):
        return None, "manifest-not-a-single-complete-scene-capture"
    capture = captures[0]
    image_path = folder / capture.get("file", "")
    if not image_path.is_file():
        return None, "png-missing"
    image_hash = sha256(image_path)
    if image_hash.lower() != str(capture.get("pngSha256", "")).lower():
        return None, "png-hash-mismatch"
    try:
        with Image.open(image_path) as image:
            image_size = list(image.size)
    except OSError as error:
        return None, f"png-invalid:{error}"
    expected_size = [capture.get("width"), capture.get("height")]
    if image_size != expected_size or image_size != [manifest.get("presentation", {}).get("dispatchWidth"),
                                                      manifest.get("presentation", {}).get("dispatchHeight")]:
        return None, "png-dimensions-do-not-match-manifest"
    if capture.get("honestlyPresentedRtFrame") is not True:
        return None, "rt-presentation-not-proven"
    return {
        "manifestPath": str(manifest_path),
        "pngPath": str(image_path),
        "pngSha256": image_hash,
        "size": image_size,
        "gpu": manifest.get("device", {}).get("gpuName"),
        "executionBackend": manifest.get("executionBackend"),
        "buildId": manifest.get("buildId"),
        "selectedRtPipelineBundle": manifest.get("selectedRtPipelineBundle"),
        "timing": manifest.get("timing", {}),
        "dielectricCounters": {
            group: manifest.get(group, {}) for group in COUNTER_GROUPS
        },
    }, None


def compare(before, after, label):
    left_path = Path(before["pngPath"])
    right_path = Path(after["pngPath"])
    with Image.open(left_path) as left_image, Image.open(right_path) as right_image:
        left = left_image.convert("RGB")
        right = right_image.convert("RGB")
        if left.size != right.size:
            return {"label": label, "status": "dimension-mismatch",
                    "beforeSize": list(left.size), "afterSize": list(right.size)}
        channels = ImageChops.difference(left, right).split()
        maximum = ImageChops.lighter(ImageChops.lighter(channels[0], channels[1]), channels[2])
        histogram = maximum.histogram()
        pixels_over_one = sum(histogram[2:])
        pixel_count = left.width * left.height
        peak = max((index for index, count in enumerate(histogram) if count), default=0)
        bounds_mask = maximum.point([0, 0] + [255] * 254)
        bounds = bounds_mask.getbbox() if pixels_over_one else None
        examples = []
        if bounds:
            for y in range(bounds[1], bounds[3]):
                for x in range(bounds[0], bounds[2]):
                    if maximum.getpixel((x, y)) >= max(4, peak - 2):
                        examples.append({"xy": [x, y], "before": list(left.getpixel((x, y))),
                                        "after": list(right.getpixel((x, y)))})
                        if len(examples) == 12:
                            break
                if len(examples) == 12:
                    break
    fraction = pixels_over_one / pixel_count
    return {
        "label": label,
        "status": "compared",
        "before": {"path": str(left_path), "sha256": before["pngSha256"]},
        "after": {"path": str(right_path), "sha256": after["pngSha256"]},
        "size": before["size"],
        "pixels": pixel_count,
        "pixelsOverOne": pixels_over_one,
        "fractionOverOne": fraction,
        "maximumChannelDifference": peak,
        "equivalencePassed": peak <= 3 and fraction <= 0.001,
        "boundsOverOne": bounds,
        "peakExamples": examples,
    }


def counter_deltas(before, after):
    deltas = {}
    for group in COUNTER_GROUPS:
        left = before["dielectricCounters"].get(group, {})
        right = after["dielectricCounters"].get(group, {})
        for key in sorted(set(left) | set(right)):
            a, b = left.get(key), right.get(key)
            if isinstance(a, (int, float)) and isinstance(b, (int, float)) and a != b:
                deltas[f"{group}.{key}"] = b - a
    return deltas


def main():
    captures = {}
    manifest_status = []
    comparisons = []
    for quality in QUALITIES:
        for config in CONFIGS:
            for scene in SCENES:
                capture, issue = load_capture(quality, config, scene)
                captures[(quality, config, scene)] = capture
                manifest_status.append({"quality": quality, "config": config, "checkpoint": scene,
                                        "status": "valid" if issue is None else issue,
                                        "manifest": capture["manifestPath"] if capture else str(
                                            ROOT / quality / config / scene / "capture-manifest.json")})

        for scene in SCENES:
            baseline = captures[(quality, "baseline", scene)]
            candidate = captures[(quality, "candidate", scene)]
            compute = captures[(quality, "compute", scene)]
            if baseline and candidate:
                comparison = compare(baseline, candidate,
                                     f"RTX/{quality}/correctness-AB/{scene}")
                comparison["counterDeltasCandidateMinusBaseline"] = counter_deltas(baseline, candidate)
                comparison["backendPair"] = [baseline["executionBackend"], candidate["executionBackend"]]
                comparisons.append(comparison)
            else:
                comparisons.append({"label": f"RTX/{quality}/correctness-AB/{scene}",
                                    "status": "incomplete"})
            if candidate and compute:
                comparison = compare(candidate, compute,
                                     f"RTX/{quality}/pipeline-compute/{scene}")
                comparison["counterDeltasComputeMinusCandidate"] = counter_deltas(candidate, compute)
                comparison["backendPair"] = [candidate["executionBackend"], compute["executionBackend"]]
                comparisons.append(comparison)
            else:
                comparisons.append({"label": f"RTX/{quality}/pipeline-compute/{scene}",
                                    "status": "incomplete"})

    report = {
        "pixelTolerance": {"maximumChannelDifference": 3, "maximumFractionOverOne": 0.001},
        "note": ("Correctness AB and pipeline-compute backend parity are separate comparisons. "
                 "An AB difference is not automatically a regression: geometric shadow changes may "
                 "affect lit opaque surfaces. PNG equivalence is not a physical-transport proof."),
        "validManifestPolicy": "complete single rt-storage-image scene capture, hash and dimensions verified, RT presentation proven",
        "manifestStatus": manifest_status,
        "comparisons": comparisons,
    }
    OUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": str(OUT), "validManifests": sum(
        item["status"] == "valid" for item in manifest_status),
        "comparisonsPassed": sum(item.get("equivalencePassed") is True for item in comparisons),
        "comparisonsFailed": sum(item.get("equivalencePassed") is False for item in comparisons),
        "comparisonsIncomplete": sum(item["status"] == "incomplete" for item in comparisons)}, indent=2))


if __name__ == "__main__":
    main()
