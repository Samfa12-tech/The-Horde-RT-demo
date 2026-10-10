"""Verify preserved forest masters and texel-preserving atlas derivatives using stdlib only."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/models/world/source/forest-tree-pair-v1"
ORIGINAL = SOURCE / "textures"
DERIVED = ROOT / "assets/textures/props/source/forest-tree-pair-v1"
ROSTER = SOURCE / "SHA256-MANIFEST.json"


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decode_png(path):
    """Decode bounded 8-bit, non-interlaced PNG inputs without optional packages."""
    data = path.read_bytes()
    check(data[:8] == b"\x89PNG\r\n\x1a\n", f"not a PNG: {path.name}")
    width = height = bit_depth = color_type = interlace = None
    compressed = bytearray()
    offset = 8
    while offset < len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        check(offset + 12 + length <= len(data), f"truncated PNG chunk: {path.name}")
        if kind == b"IHDR":
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(">IIBBBBB", payload)
            check(compression == filtering == 0 and interlace == 0 and bit_depth == 8,
                  f"unsupported PNG encoding: {path.name}")
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break
        offset += length + 12
    channels_by_type = {0: 1, 2: 3, 4: 2, 6: 4}
    check(color_type in channels_by_type, f"unsupported PNG color type: {path.name}")
    channels = channels_by_type[color_type]
    stride = width * channels
    decoded = zlib.decompress(compressed)
    check(len(decoded) == height * (stride + 1), f"PNG decoded size mismatch: {path.name}")
    pixels = bytearray(height * stride)
    source_offset = 0
    for y in range(height):
        filter_type = decoded[source_offset]
        source_offset += 1
        row = y * stride
        prior = row - stride
        for x in range(stride):
            value = decoded[source_offset + x]
            left = pixels[row + x - channels] if x >= channels else 0
            up = pixels[prior + x] if y else 0
            upper_left = pixels[prior + x - channels] if y and x >= channels else 0
            if filter_type == 1:
                value += left
            elif filter_type == 2:
                value += up
            elif filter_type == 3:
                value += (left + up) // 2
            elif filter_type == 4:
                estimate = left + up - upper_left
                distances = (abs(estimate - left), abs(estimate - up), abs(estimate - upper_left))
                predictor = left if distances[0] <= distances[1] and distances[0] <= distances[2] else (
                    up if distances[1] <= distances[2] else upper_left)
                value += predictor
            elif filter_type != 0:
                raise AssertionError(f"unknown PNG filter {filter_type}: {path.name}")
            pixels[row + x] = value & 255
        source_offset += stride
    return width, height, color_type, channels, bytes(pixels)


def to_rgba(image):
    width, height, color_type, channels, pixels = image
    if color_type == 6:
        return width, height, pixels
    output = bytearray(width * height * 4)
    for index in range(width * height):
        source = index * channels
        target = index * 4
        if color_type == 0:
            red = green = blue = pixels[source]
            alpha = 255
        elif color_type == 2:
            red, green, blue = pixels[source:source + 3]
            alpha = 255
        else:
            red = green = blue = pixels[source]
            alpha = pixels[source + 1]
        output[target:target + 4] = bytes((red, green, blue, alpha))
    return width, height, bytes(output)


def check_nearest_2x(source, derived, label):
    source_width, source_height, source_pixels = source
    width, height, pixels = derived
    check((width, height) == (source_width * 2, source_height * 2),
          f"2x image dimensions differ: {label}")
    for y in range(source_height):
        for x in range(source_width):
            src = (y * source_width + x) * 4
            for dy in range(2):
                for dx in range(2):
                    dst = ((y * 2 + dy) * width + x * 2 + dx) * 4
                    check(pixels[dst:dst + 4] == source_pixels[src:src + 4],
                          f"nearest 2x derivative changes source texel: {label}")


def main():
    roster = json.loads(ROSTER.read_text(encoding="utf-8-sig"))
    listed = {entry["path"]: entry for entry in roster}
    evidence_root = SOURCE / "evidence"
    package_payloads = {path.relative_to(SOURCE).as_posix()
                        for path in SOURCE.rglob("*") if path.is_file() and
                        path.name != "SHA256-MANIFEST.json" and
                        not path.is_relative_to(evidence_root)}
    check(package_payloads == set(listed),
          "the 48 preserved source masters no longer match their original closed package roster")
    for relative, entry in listed.items():
        path = SOURCE / relative
        check(path.stat().st_size == entry["bytes"] and sha(path) == entry["sha256"],
              f"preserved source package payload changed: {relative}")

    expected_receipts = {"pine-lod1-tangent-receipt.json", "alder-lod1-tangent-receipt.json"}
    receipt_dir = evidence_root / "runtime-tangents"
    check({path.name for path in receipt_dir.iterdir() if path.is_file()} == expected_receipts,
          "tracked tangent evidence differs from the two-derivative receipt roster")
    for receipt_path in receipt_dir.iterdir():
        receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
        check(receipt["status"] == "pass" and receipt["geometryUnchanged"] and
              receipt["embeddedImagePixelsUnchanged"],
              f"tangent derivation receipt failed: {receipt_path.name}")

    pairs = {
        "bark-base-color.png": "horde-bark-original-basecolor-512.png",
        "pine-base-color.png": "horde-pine-original-basecolor-512.png",
        "moss-base-color.png": "horde-moss-original-basecolor-512.png",
        "alder-base-color.png": "horde-alder-original-basecolor-512.png",
        "bark-normal.png": "horde-bark-original-normal-512.png",
    }
    for output_name, source_name in pairs.items():
        source = to_rgba(decode_png(ORIGINAL / source_name))
        derived = to_rgba(decode_png(DERIVED / output_name))
        check(source[:2] == (512, 512), f"original map dimensions changed: {source_name}")
        check_nearest_2x(source, derived, output_name)

    rough_width, rough_height, rough_type, rough_channels, rough_rgb = decode_png(
        ORIGINAL / "horde-bark-original-roughness-512.png")
    check((rough_width, rough_height, rough_type, rough_channels) == (512, 512, 2, 3) and
          all(rough_rgb[index] == rough_rgb[index + 1] == rough_rgb[index + 2]
              for index in range(0, len(rough_rgb), 3)),
          "original bark roughness must remain a 512x512 achromatic RGB PNG")
    roughness = rough_rgb[0::3]
    orm = to_rgba(decode_png(DERIVED / "bark-orm-512.derived.png"))
    check(orm[:2] == (512, 512), "derived bark ORM is not 512x512")
    for index in range(512 * 512):
        channel = index * 4
        check(orm[2][channel:channel + 4] == bytes((255, roughness[index], 0, 255)),
              "bark ORM did not pack source roughness into glTF G with neutral AO/metallic")
    check_nearest_2x(orm, to_rgba(decode_png(DERIVED / "bark-orm.png")), "bark ORM")
    print("Forest source masters and texel-preserving derivatives passed using Python stdlib only.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"Forest texture source check failed: {exc}", file=sys.stderr)
        raise
