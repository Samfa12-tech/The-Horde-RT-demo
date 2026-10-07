"""Verify bounded prop-atlas growth against preserved bytes, not renderer behavior."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

MAGIC = b"\xabKTX 20\xbb\r\n\x1a\n"
FORMATS = {"base-color.windows.ktx2": 43, "normal.windows.ktx2": 37, "orm.windows.ktx2": 37,
           "base-color.android.ktx2": 166, "normal.android.ktx2": 157, "orm.android.ktx2": 165,
           "emissive.windows.ktx2": 43, "emissive.android.ktx2": 166}


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def texture(path, layers, vk_format):
    data = path.read_bytes()
    require(data[:12] == MAGIC and len(data) >= 80 + 11 * 24, f"invalid KTX2: {path.name}")
    fields = struct.unpack_from("<9I", data, 12)
    fmt, type_size, width, height, depth, count, faces, levels, compression = fields
    require((fmt, type_size, width, height, depth, count, faces, levels, compression) ==
            (vk_format, 1, 1024, 1024, 0, layers, 1, 11, 0), f"KTX2 contract differs: {path.name}: {fields}")
    images = []
    for level in range(levels):
        offset, length, uncompressed = struct.unpack_from("<3Q", data, 80 + 24 * level)
        require(length == uncompressed and offset >= 80 + levels * 24 and offset + length <= len(data),
                f"invalid mip payload: {path.name}/{level}")
        size = max(1, 1024 >> level)
        per_layer = size * size * 4 if fmt in (37, 43) else ((size + (3 if fmt == 157 else 5)) //
                    (4 if fmt == 157 else 6)) ** 2 * 16
        require(length == per_layer * layers, f"actual payload differs from format geometry: {path.name}/{level}")
        images.append(data[offset:offset + length])
    descriptor_offset, descriptor_size = struct.unpack_from("<2I", data, 48)
    descriptor = data[descriptor_offset:descriptor_offset + descriptor_size]
    require(len(descriptor) == descriptor_size and descriptor_size >= 16 and descriptor[13] == 1 and
            descriptor[14] == (2 if fmt in (43, 166) else 1), f"actual primaries/transfer differs: {path.name}")
    return dict(bytes=len(data), sha256=digest(data), mipPayloadBytes=sum(map(len, images)),
                dataFormatDescriptorSha256=digest(descriptor)), images


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--before", required=True, type=Path)
    parser.add_argument("--after", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    rows = []
    for name, fmt in FORMATS.items():
        emissive = name.startswith("emissive")
        old, old_mips = texture(args.before / name, 1 if emissive else 13, fmt)
        new, new_mips = texture(args.after / name, 1 if emissive else 14, fmt)
        require(old["dataFormatDescriptorSha256"] == new["dataFormatDescriptorSha256"], f"DFD metadata changed: {name}")
        if emissive:
            require(old["sha256"] == new["sha256"], f"emissive fallback changed: {name}")
        else:
            for level, (old_image, new_image) in enumerate(zip(old_mips, new_mips)):
                require(old_image == new_image[:len(old_image)], f"existing layers changed: {name}/mip{level}")
        rows.append(dict(file=name, before=old, after=new, oldLayerMipPayloadsByteIdentical=True,
                         mipPayloadGrowthBytes=new["mipPayloadBytes"] - old["mipPayloadBytes"],
                         containerGrowthBytes=new["bytes"] - old["bytes"]))
    manifest = json.loads((args.after / "asset.manifest.json").read_text(encoding="utf-8-sig"))
    require(manifest["layerCounts"] == {"baseColor": 14, "normal": 14, "orm": 14, "emissive": 1}, "manifest layer counts differ")
    require(manifest["layerOrder"][-4:] == ["collapsed-entry/Boulder01Rock", "collapsed-entry/MedievalWall02",
                                            "rag-torch-player/RagTorch_Atlas", "player-sword-scabbard/LeatherIron"],
            "new layer order differs")
    before_manifest = json.loads((args.before / "asset.manifest.json").read_text(encoding="utf-8-sig"))
    require(manifest["layerOrder"][:13] == before_manifest["layerOrder"], "old manifest order changed")
    for row in rows:
        category, platform, _ = row["file"].split(".")
        category = {"base-color": "baseColor"}.get(category, category)
        require(manifest[platform][category]["sha256"] == row["after"]["sha256"], "manifest output hash differs")
    targets = {}
    for platform in ("windows", "android"):
        group = [row for row in rows if f".{platform}." in row["file"]]
        targets[platform] = {key: sum(row[key] for row in group) for key in ("containerGrowthBytes", "mipPayloadGrowthBytes")}
        targets[platform]["beforeMipPayloadBytes"] = sum(row["before"]["mipPayloadBytes"] for row in group)
        targets[platform]["afterMipPayloadBytes"] = sum(row["after"]["mipPayloadBytes"] for row in group)
    result = dict(schema=1, status="pass", beforeLayers=13, afterLayers=14, resolution=1024, mipLevels=11,
                  emissiveLayers=1, rows=rows, platformBudgets=targets,
                  disclosure="Actual encoded/container and format-derived upload payload bytes, not measured GPU allocation/residency/performance.")
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "pass", "platformBudgets": targets}))


if __name__ == "__main__":
    main()
