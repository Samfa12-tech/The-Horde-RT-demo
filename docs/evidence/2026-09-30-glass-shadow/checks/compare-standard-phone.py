"""Compare captured standard and phone images without changing the established RGB gate."""

import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops


ROOT = Path(__file__).resolve().parent.parent
PREVIOUS = ROOT.parent / "2026-09-30-glass-surface"
STANDARD_CANDIDATE = ROOT / "windows/high-standard"
STANDARD_BASELINE = ROOT / "windows/high-standard-baseline"
PHONE_CANDIDATE = ROOT / "phone/candidate"
PHONE_BASELINE = PREVIOUS / "phone"
OUT = Path("reports/geometric-shadow-standard-phone-reproduced.json")
PIXEL_MAX = 3
FRACTION_OVER_ONE_MAX = 0.001


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def image_comparison(before_path, after_path, label, before_hash=None, after_hash=None):
    with Image.open(before_path) as before_image, Image.open(after_path) as after_image:
        before = before_image.convert("RGB")
        after = after_image.convert("RGB")
        if before.size != after.size:
            return {"label": label, "status": "dimension-mismatch",
                    "beforeSize": list(before.size), "afterSize": list(after.size)}
        diff = ImageChops.difference(before, after).split()
        maximum = ImageChops.lighter(ImageChops.lighter(diff[0], diff[1]), diff[2])
        histogram = maximum.histogram()
        over_one = sum(histogram[2:])
        pixel_count = before.width * before.height
        peak = max((i for i, count in enumerate(histogram) if count), default=0)
        bounds = maximum.point([0, 0] + [255] * 254).getbbox() if over_one else None
        examples = []
        if bounds:
            for y in range(bounds[1], bounds[3]):
                for x in range(bounds[0], bounds[2]):
                    if maximum.getpixel((x, y)) >= max(4, peak - 2):
                        examples.append({"xy": [x, y], "before": list(before.getpixel((x, y))),
                                         "after": list(after.getpixel((x, y)))})
                        if len(examples) >= 12:
                            break
                if len(examples) >= 12:
                    break
    fraction = over_one / pixel_count
    return {
        "label": label,
        "status": "compared",
        "beforePath": str(before_path),
        "afterPath": str(after_path),
        "beforeSha256": before_hash or sha256(before_path),
        "afterSha256": after_hash or sha256(after_path),
        "size": list(before.size),
        "pixels": pixel_count,
        "pixelsOverOne": over_one,
        "fractionOverOne": fraction,
        "maximumChannelDifference": peak,
        "equivalencePassed": peak <= PIXEL_MAX and fraction <= FRACTION_OVER_ONE_MAX,
        "boundsOverOne": bounds,
        "peakExamples": examples,
    }


def load_standard(folder):
    manifest_path = folder / "capture-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if (manifest.get("complete") is not True or manifest.get("source") != "rt-storage-image" or
            manifest.get("sceneOnly") is not True or manifest.get("overlaysIncluded") is not False or
            len(manifest.get("captures", [])) != 13):
        raise ValueError(f"invalid standard capture manifest: {manifest_path}")
    captures = {}
    for capture in manifest["captures"]:
        if capture.get("honestlyPresentedRtFrame") is not True:
            raise ValueError(f"RT presentation not proven: {manifest_path} {capture.get('checkpoint')}")
        image_path = folder / capture["file"]
        digest = sha256(image_path)
        if digest.lower() != str(capture.get("pngSha256", "")).lower():
            raise ValueError(f"PNG hash mismatch: {image_path}")
        with Image.open(image_path) as image:
            if list(image.size) != [capture.get("width"), capture.get("height")]:
                raise ValueError(f"PNG dimensions mismatch: {image_path}")
        captures[capture["checkpoint"]] = {"path": image_path, "sha256": digest,
                                            "capture": capture}
    return manifest, captures


def phone_counters(state):
    return {key: value for key, value in state.items()
            if (key.endswith("Count") or key.endswith("Mask")) and
            isinstance(value, (int, float)) and not isinstance(value, bool)}


def load_phone(folder):
    manifest_path = folder / "capture-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if (manifest.get("schema") != 2 or manifest.get("checkpointCount") != 7 or
            "ADB screencap" not in manifest.get("captureMode", "") or
            manifest.get("device", {}).get("model") != "SM-S948B" or
            manifest.get("package") != "com.samfa12.hordelanternrt.debug" or
            manifest.get("apkSha256") != manifest.get("installedApkSha256")):
        raise ValueError(f"invalid phone capture manifest: {manifest_path}")
    captures = {}
    for checkpoint in manifest["checkpoints"]:
        if (checkpoint.get("presented") is not True or checkpoint.get("sceneOnly") is not True or
                checkpoint.get("zone") != checkpoint.get("expectedZone")):
            raise ValueError(f"invalid phone checkpoint state: {manifest_path} {checkpoint.get('checkpoint')}")
        png = checkpoint["png"]
        image_path = folder / png["file"]
        digest = sha256(image_path)
        if digest.lower() != str(png.get("sha256", "")).lower():
            raise ValueError(f"phone PNG hash mismatch: {image_path}")
        with Image.open(image_path) as image:
            if list(image.size) != [png.get("width"), png.get("height")]:
                raise ValueError(f"phone PNG dimensions mismatch: {image_path}")
        state_path = folder / checkpoint["nativeStateFile"]
        state = json.loads(state_path.read_text(encoding="utf-8"))
        if (state.get("status") != "capture-ready" or
                state.get("checkpoint") != checkpoint["checkpoint"] or
                state.get("presented") is not True or
                state.get("diagnosticsAvailable") is not True):
            raise ValueError(f"invalid phone state snapshot: {state_path}")
        captures[checkpoint["checkpoint"]] = {
            "path": image_path,
            "sha256": digest,
            "state": state,
            "checkpoint": checkpoint,
        }
    return manifest, captures


def main():
    standard_before_manifest, standard_before = load_standard(STANDARD_BASELINE)
    standard_after_manifest, standard_after = load_standard(STANDARD_CANDIDATE)
    if set(standard_before) != set(standard_after):
        raise ValueError("standard checkpoint sets do not match")
    standard = []
    for checkpoint in standard_before:
        before, after = standard_before[checkpoint], standard_after[checkpoint]
        comparison = image_comparison(before["path"], after["path"],
                                      f"RTX/High/standard13/{checkpoint}",
                                      before["sha256"], after["sha256"])
        comparison["viewmodelGeometrySha256"] = {
            "before": before["capture"].get("viewmodelGeometry", {}).get("sha256"),
            "after": after["capture"].get("viewmodelGeometry", {}).get("sha256"),
        }
        comparison["playerBodyGeometrySha256"] = {
            "before": before["capture"].get("playerWorldBodyGeometry", {}).get("sha256"),
            "after": after["capture"].get("playerWorldBodyGeometry", {}).get("sha256"),
        }
        standard.append(comparison)

    phone_before_manifest, phone_before = load_phone(PHONE_BASELINE)
    phone_after_manifest, phone_after = load_phone(PHONE_CANDIDATE)
    if set(phone_before) != set(phone_after):
        raise ValueError("phone checkpoint sets do not match")
    phone = []
    for checkpoint in phone_before:
        before, after = phone_before[checkpoint], phone_after[checkpoint]
        comparison = image_comparison(before["path"], after["path"],
                                      f"SM-S948B/ADB-screenshot-AB/{checkpoint}",
                                      before["sha256"], after["sha256"])
        before_counters = phone_counters(before["state"])
        after_counters = phone_counters(after["state"])
        deltas = {key: after_counters.get(key, 0) - before_counters.get(key, 0)
                  for key in sorted(set(before_counters) | set(after_counters))
                  if after_counters.get(key, 0) != before_counters.get(key, 0)}
        comparison["counterDeltasCandidateMinusCBC"] = deltas
        comparison["counterValues"] = {"cbc": before_counters, "candidate": after_counters}
        comparison["provenance"] = {
            "candidateApkSha256": phone_after_manifest["installedApkSha256"],
            "cbcApkSha256": phone_before_manifest["installedApkSha256"],
            "candidateBuildIdentity": after["state"].get("buildIdentity"),
            "cbcBuildIdentity": before["state"].get("buildIdentity"),
            "gpu": after["state"].get("gpu"),
            "captureMode": phone_after_manifest["captureMode"],
            "comparisonClass": "ADB screenshot AB; not raw storage-image or backend parity",
        }
        phone.append(comparison)

    report = {
        "pixelTolerance": {"maximumChannelDifference": PIXEL_MAX,
                           "maximumFractionOverOne": FRACTION_OVER_ONE_MAX},
        "standard13": {
            "candidateManifest": str(STANDARD_CANDIDATE / "capture-manifest.json"),
            "baselineManifest": str(STANDARD_BASELINE / "capture-manifest.json"),
            "candidateBackend": standard_after_manifest.get("executionBackend"),
            "baselineBackend": standard_before_manifest.get("executionBackend"),
            "results": standard,
        },
        "phone7": {
            "candidateManifest": str(PHONE_CANDIDATE / "capture-manifest.json"),
            "baselineManifest": str(PHONE_BASELINE / "capture-manifest.json"),
            "device": phone_after_manifest.get("device"),
            "candidateApkSha256": phone_after_manifest.get("installedApkSha256"),
            "cbcApkSha256": phone_before_manifest.get("installedApkSha256"),
            "captureClass": "ADB screenshot AB, not raw backend image parity",
            "results": phone,
        },
    }
    OUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    summary = {
        "report": str(OUT),
        "standard13Pass": sum(item.get("equivalencePassed") is True for item in standard),
        "standard13Fail": sum(item.get("equivalencePassed") is False for item in standard),
        "phone7Pass": sum(item.get("equivalencePassed") is True for item in phone),
        "phone7Fail": sum(item.get("equivalencePassed") is False for item in phone),
    }
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
