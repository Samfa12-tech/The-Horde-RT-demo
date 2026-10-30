"""Verify the approved forest layers append after the unchanged 14-layer prop arrays."""
import argparse
import importlib.util
import json
from pathlib import Path

_spec = importlib.util.spec_from_file_location(
    "static_prop_growth", Path(__file__).with_name("validate-static-prop-atlas-growth.py"))
_growth = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_growth)

OLD_COUNTS = {
    "base-color.windows.ktx2": 14, "base-color.android.ktx2": 14,
    "normal.windows.ktx2": 14, "normal.android.ktx2": 14,
    "orm.windows.ktx2": 14, "orm.android.ktx2": 14,
    "emissive.windows.ktx2": 1, "emissive.android.ktx2": 1,
}
NEW_COUNTS = {
    "base-color.windows.ktx2": 18, "base-color.android.ktx2": 18,
    "normal.windows.ktx2": 15, "normal.android.ktx2": 15,
    "orm.windows.ktx2": 15, "orm.android.ktx2": 15,
    "emissive.windows.ktx2": 1, "emissive.android.ktx2": 1,
}
EXPECTED_LAYERS = [
    "forest-tree-pair-v1/Bark", "forest-tree-pair-v1/Pine",
    "forest-tree-pair-v1/Moss", "forest-tree-pair-v1/Alder",
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--before", required=True, type=Path)
    parser.add_argument("--after", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    rows = []
    for name, vk_format in _growth.FORMATS.items():
        old, old_mips = _growth.texture(args.before / name, OLD_COUNTS[name], vk_format)
        new, new_mips = _growth.texture(args.after / name, NEW_COUNTS[name], vk_format)
        _growth.require(old["dataFormatDescriptorSha256"] == new["dataFormatDescriptorSha256"],
                        f"format metadata changed: {name}")
        if name.startswith("emissive"):
            _growth.require(old["sha256"] == new["sha256"], f"emissive bytes changed: {name}")
        else:
            for level, (old_payload, new_payload) in enumerate(zip(old_mips, new_mips)):
                _growth.require(old_payload == new_payload[:len(old_payload)],
                                f"legacy layer payload changed: {name}/mip{level}")
        rows.append({"file": name, "before": old, "after": new,
                     "legacyLayerMipPayloadsByteIdentical": True,
                     "mipPayloadGrowthBytes": new["mipPayloadBytes"] - old["mipPayloadBytes"],
                     "containerGrowthBytes": new["bytes"] - old["bytes"]})

    manifest = json.loads((args.after / "asset.manifest.json").read_text(encoding="utf-8-sig"))
    before_manifest = json.loads((args.before / "asset.manifest.json").read_text(encoding="utf-8-sig"))
    _growth.require(manifest["layerCounts"] ==
                    {"baseColor": 18, "normal": 15, "orm": 15, "emissive": 1},
                    "forest layer counts differ from the approved per-family map presence")
    _growth.require(manifest["layerOrder"][-4:] == EXPECTED_LAYERS, "forest layer order differs")
    _growth.require(manifest["layerOrder"][:14] == before_manifest["layerOrder"],
                    "existing 14 layer names/order changed")
    for row in rows:
        category, platform, _ = row["file"].split(".")
        category = {"base-color": "baseColor"}.get(category, category)
        _growth.require(manifest[platform][category]["sha256"] == row["after"]["sha256"],
                        f"manifest runtime hash differs: {row['file']}")
    targets = {}
    for platform in ("windows", "android"):
        group = [row for row in rows if f".{platform}." in row["file"]]
        targets[platform] = {key: sum(row[key] for row in group)
                             for key in ("containerGrowthBytes", "mipPayloadGrowthBytes")}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    result = {
        "schema": 1,
        "status": "pass",
        "beforeLayerCounts": {"baseColor": 14, "normal": 14, "orm": 14, "emissive": 1},
        "afterLayerCounts": {"baseColor": 18, "normal": 15, "orm": 15, "emissive": 1},
        "appendedLayerOrder": EXPECTED_LAYERS,
        "legacyLayerMipPayloadsByteIdentical": True,
        "emissiveByteIdentical": True,
        "resolution": 1024,
        "mipLevels": 11,
        "rows": rows,
        "platformGrowth": targets,
        "disclosure": "Encoded/container and format-derived upload payload bytes only; not measured GPU allocation, residency, bandwidth, performance, or visual acceptance.",
    }
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "pass", "platformGrowth": targets}))


if __name__ == "__main__":
    main()
