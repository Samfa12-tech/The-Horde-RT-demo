#!/usr/bin/env python3
"""Generate the original 1K PBR source maps for the player sword scabbard."""

from __future__ import annotations

import pathlib
import math
import random
from PIL import Image


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "assets/textures/props/source/sword-scabbard"
SIZE = 1024


def save_image(path: pathlib.Path, image: Image.Image) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, format="PNG", optimize=False)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    rng = random.Random(20261007)
    base = Image.new("RGB", (SIZE, SIZE))
    orm = Image.new("RGB", (SIZE, SIZE))
    roughness = Image.new("L", (SIZE, SIZE))
    metallic = Image.new("L", (SIZE, SIZE))
    height = [[0.0] * SIZE for _ in range(SIZE)]
    base_pixels = base.load()
    orm_pixels = orm.load()
    roughness_pixels = roughness.load()
    metallic_pixels = metallic.load()
    for py in range(SIZE):
        v = py / (SIZE - 1)
        for px in range(SIZE):
            u = px / (SIZE - 1)
            grain = rng.gauss(0.0, 1.0)
            fiber = 4.0 * math.sin(v * math.tau * 37.0 + grain * 0.45)
            if u < 0.735:
                value = grain * 2.3 + fiber
                base_pixels[px, py] = (round(45 + value),
                                        round(20 + value * 0.62),
                                        round(13 + value * 0.42))
                orm_pixels[px, py] = (248, 212, 0)
                roughness_pixels[px, py] = 212
                metallic_pixels[px, py] = 0
                height[py][px] = grain * 0.0012 + fiber * 0.00025
            elif u > 0.765:
                value = grain * 2.0 + 2.0 * math.sin(v * math.tau * 24.0)
                base_pixels[px, py] = (round(70 + value), round(64 + value), round(54 + value))
                orm_pixels[px, py] = (248, 112, 255)
                roughness_pixels[px, py] = 112
                metallic_pixels[px, py] = 255
                height[py][px] = 0.005 * math.sin(v * math.tau * 24.0)
            else:
                base_pixels[px, py] = (31, 20, 15)
                orm_pixels[px, py] = (215, 225, 0)
                roughness_pixels[px, py] = 225
                metallic_pixels[px, py] = 0

    normal = Image.new("RGB", (SIZE, SIZE))
    normal_pixels = normal.load()
    for py in range(SIZE):
        ym = max(0, py - 1)
        yp = min(SIZE - 1, py + 1)
        for px in range(SIZE):
            xm = max(0, px - 1)
            xp = min(SIZE - 1, px + 1)
            nx = -(height[py][xp] - height[py][xm]) * 40.0
            nz = -(height[yp][px] - height[ym][px]) * 40.0
            length = math.sqrt(nx * nx + 1.0 + nz * nz)
            normal_pixels[px, py] = (
                round((nx / length + 1.0) * 127.5),
                round((1.0 / length + 1.0) * 127.5),
                round((nz / length + 1.0) * 127.5),
            )

    save_image(OUT / "sword-scabbard-base-color.png", base)
    save_image(OUT / "sword-scabbard-normal.png", normal)
    save_image(OUT / "sword-scabbard-orm.png", orm)
    save_image(OUT / "sword-scabbard-roughness.png", roughness)
    save_image(OUT / "sword-scabbard-metallic.png", metallic)


if __name__ == "__main__":
    main()
