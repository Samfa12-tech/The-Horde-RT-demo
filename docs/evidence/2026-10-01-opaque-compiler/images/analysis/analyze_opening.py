#!/usr/bin/env python3
"""Bounded, read-only RGB diff analysis for the frozen opaque-opening pair."""
from collections import Counter, deque
from hashlib import sha256
import json
from pathlib import Path
from PIL import Image

CONTROL = Path(r"C:\Dev\tmp\horde-generic-route-profile-20260930\restoration\debug-opening\run-20260930-234438\capture-01-opening-75.png")
PROFILE = Path(r"C:\Dev\tmp\horde-opaque-retained-profile-20260930\images\diagnostic-profile\run-20261001-003239\capture-01-opening-75.png")
OUT = Path(__file__).with_name("opening-difference-analysis.json")
EXPECTED_HASHES = (
    "d3ec98a838e61194c340343ae4e293aea0ce8b39697962dbac17e134c62dbbcf",
    "b944259d62f68fece59e79af3bc0299ece880a1fb6b1883caf1149706b9003aa",
)


def digest(path):
    h = sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def components(points):
    remaining = set(points)
    found = []
    while remaining:
        seed = remaining.pop()
        q = deque([seed])
        xs = [seed[0]]
        ys = [seed[1]]
        while q:
            x, y = q.popleft()
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    if not (dx or dy):
                        continue
                    p = (x + dx, y + dy)
                    if p in remaining:
                        remaining.remove(p)
                        q.append(p)
                        xs.append(p[0])
                        ys.append(p[1])
        found.append({"pixels": len(xs), "bbox": [min(xs), min(ys), max(xs), max(ys)]})
    return sorted(found, key=lambda c: (-c["pixels"], c["bbox"][1], c["bbox"][0]))


def main():
    hashes = [digest(CONTROL), digest(PROFILE)]
    if tuple(hashes) != EXPECTED_HASHES:
        raise SystemExit(f"PNG hash mismatch; expected {EXPECTED_HASHES}, got {hashes}")
    with Image.open(CONTROL) as a0, Image.open(PROFILE) as b0:
        if a0.format != "PNG" or b0.format != "PNG":
            raise SystemExit("Expected PNG inputs")
        a = a0.convert("RGB")
        b = b0.convert("RGB")
    if a.size != b.size or a.size != (1440, 3120):
        raise SystemExit(f"Unexpected extents: {a.size}, {b.size}")
    w, h = a.size
    changed = []
    max_hist = Counter()
    signed_hist = Counter()
    row_over1 = [0] * h
    col_over1 = [0] * w
    tile_rows, tile_cols = 16, 8
    tiles = [[{"over1": 0, "different": 0, "max": 0, "sum_rgb": 0} for _ in range(tile_cols)] for _ in range(tile_rows)]
    total_abs_rgb = 0
    max_diff = 0
    over1 = 0
    for index, (pa, pb) in enumerate(zip(a.getdata(), b.getdata())):
        ds = (pb[0] - pa[0], pb[1] - pa[1], pb[2] - pa[2])
        d = max(abs(v) for v in ds)
        if not d:
            continue
        x, y = index % w, index // w
        changed.append((x, y, d, ds, pa, pb))
        max_hist[d] += 1
        signed_hist[tuple((1 if v > 0 else -1 if v < 0 else 0) for v in ds)] += 1
        total_abs_rgb += sum(abs(v) for v in ds)
        max_diff = max(max_diff, d)
        tr = min(tile_rows - 1, y * tile_rows // h)
        tc = min(tile_cols - 1, x * tile_cols // w)
        tile = tiles[tr][tc]
        tile["different"] += 1
        tile["max"] = max(tile["max"], d)
        tile["sum_rgb"] += sum(abs(v) for v in ds)
        if d > 1:
            over1 += 1
            row_over1[y] += 1
            col_over1[x] += 1
            tile["over1"] += 1
    over_points = [(x, y) for x, y, d, *_ in changed if d > 1]
    largest = sorted(changed, key=lambda p: (-p[2], p[1], p[0]))[:32]
    nonzero_rows = [i for i, n in enumerate(row_over1) if n]
    nonzero_cols = [i for i, n in enumerate(col_over1) if n]
    result = {
        "schema": 1,
        "scope": "Read-only diagnostic of the exact frozen phone opening PNG pair; image gate remains failed, no tolerance or source changes",
        "inputs": [str(CONTROL), str(PROFILE)],
        "sha256": hashes,
        "extent": [w, h],
        "summary": {
            "pixels": w * h,
            "differentPixels": len(changed),
            "pixelsDifferentByMoreThanOne": over1,
            "maximumChannelDifference": max_diff,
            "fractionOverOne": over1 / (w * h),
            "meanAbsoluteRgbDifference": total_abs_rgb / (3 * w * h),
            "maxDifferenceHistogram": {str(k): max_hist[k] for k in sorted(max_hist)},
            "signedRgbDirectionHistogram": {str(k): n for k, n in signed_hist.most_common()},
        },
        "overOneBounds": [min(nonzero_cols), min(nonzero_rows), max(nonzero_cols), max(nonzero_rows)] if over1 else None,
        "visualRegionNotes": [
            {"xy": [1167, 2617], "feature": "visually maps to the lower-right held weapon/hand guard area in the unmodified captures", "confidence": "approximate raster inspection only; no per-pixel primitive identity"},
            {"xy": [1155, 2312], "feature": "visually maps to the lower-right foreground weapon/hand region", "confidence": "approximate raster inspection only; no per-pixel primitive identity"},
            {"xy": [287, 503], "feature": "visually maps to the left foreground lantern top/glow edge", "confidence": "approximate raster inspection only; no per-pixel primitive identity"},
            {"scope": "remaining >1 pixels", "feature": "sparse isolated points span other foreground/floor/scene regions; no coherent broad silhouette or global color shift is evident", "confidence": "image-space observation, not causal attribution"},
        ],
        "overOneRowsTop": sorted(((i, n) for i, n in enumerate(row_over1) if n), key=lambda v: (-v[1], v[0]))[:24],
        "overOneColumnsTop": sorted(((i, n) for i, n in enumerate(col_over1) if n), key=lambda v: (-v[1], v[0]))[:24],
        "overOneConnectedComponents8": components(over_points),
        "tileGrid": {"rows": tile_rows, "columns": tile_cols, "tiles": tiles},
        "largestPixelWitnesses": [
            {"xy": [x, y], "maxDifference": d, "signedAfterMinusBeforeRgb": list(ds), "controlRgb": list(pa), "profileRgb": list(pb)}
            for x, y, d, ds, pa, pb in largest
        ],
        "interpretationLimit": "Spatial concentration can identify likely surfaces, but raster-space pixel differences alone cannot attribute cause to compiler arithmetic, ray traversal, sampling, or a specific shader operation.",
    }
    OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(OUT), "summary": result["summary"], "overOneBounds": result["overOneBounds"], "components": result["overOneConnectedComponents8"][:12]}, indent=2))


if __name__ == "__main__":
    main()
