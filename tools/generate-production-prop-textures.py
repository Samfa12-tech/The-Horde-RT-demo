import pathlib

import numpy as np
from PIL import Image, ImageDraw, ImageFilter


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "assets" / "textures" / "props" / "source"
SIZE = 1024
SEED = 0x484F524445
ARRAY_INPUT = OUTPUT / "static-array-1k"


def save(name: str, values: np.ndarray) -> None:
    path = OUTPUT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(np.clip(values, 0, 255).astype(np.uint8), "RGBA").save(path)


def normal_from_height(height: np.ndarray, strength: float) -> np.ndarray:
    dx = np.gradient(height, axis=1) * strength
    dy = np.gradient(height, axis=0) * strength
    normal = np.dstack((-dx, -dy, np.ones_like(height)))
    normal /= np.maximum(np.linalg.norm(normal, axis=2, keepdims=True), 1.0e-6)
    rgba = np.empty((SIZE, SIZE, 4), dtype=np.float32)
    rgba[:, :, :3] = (normal * 0.5 + 0.5) * 255.0
    rgba[:, :, 3] = 255.0
    return rgba


def wood(rng: np.random.Generator) -> None:
    y, x = np.mgrid[0:SIZE, 0:SIZE]
    blurred = rng.normal(0.0, 1.0, (SIZE, SIZE)).astype(np.float32)
    blurred_image = Image.fromarray(np.clip(blurred * 24.0 + 128.0, 0, 255).astype(np.uint8), "L")
    blurred = np.asarray(blurred_image.filter(ImageFilter.GaussianBlur(7.0)), dtype=np.float32) / 255.0 - 0.5
    grain = np.sin(x * 0.080 + np.sin(y * 0.013) * 1.8 + blurred * 6.0)
    fine = np.sin(x * 0.31 + y * 0.006) * 0.22
    height = grain * 0.55 + fine + blurred * 0.8
    shade = np.clip(0.56 + height * 0.15, 0.28, 0.80)
    base = np.empty((SIZE, SIZE, 4), dtype=np.float32)
    base[:, :, 0] = 118.0 * shade
    base[:, :, 1] = 59.0 * shade
    base[:, :, 2] = 27.0 * shade
    base[:, :, 3] = 255.0
    save("chest-wood-base-color.png", base)
    save("chest-wood-normal.png", normal_from_height(height, 0.34))
    orm = np.empty((SIZE, SIZE, 4), dtype=np.float32)
    orm[:, :, 0] = 244.0
    orm[:, :, 1] = np.clip(182.0 - height * 13.0, 155.0, 215.0)
    orm[:, :, 2] = 4.0
    orm[:, :, 3] = 255.0
    save("chest-wood-orm.png", orm)


def iron(prefix: str, rng: np.random.Generator, soot: bool) -> None:
    noise = rng.normal(0.0, 1.0, (SIZE, SIZE)).astype(np.float32)
    noise_image = Image.fromarray(np.clip(noise * 26.0 + 128.0, 0, 255).astype(np.uint8), "L")
    broad = np.asarray(noise_image.filter(ImageFilter.GaussianBlur(4.0)), dtype=np.float32) / 255.0 - 0.5
    fine = np.asarray(noise_image.filter(ImageFilter.GaussianBlur(0.65)), dtype=np.float32) / 255.0 - 0.5
    wear = np.clip(broad * 0.55 + fine * 0.22, -0.35, 0.35)
    base = np.empty((SIZE, SIZE, 4), dtype=np.float32)
    base[:, :, 0] = 31.0 + wear * 32.0
    base[:, :, 1] = 33.0 + wear * 34.0
    base[:, :, 2] = 36.0 + wear * 37.0
    if soot:
        base[:, :, :3] *= 0.82
    base[:, :, 3] = 255.0
    image = Image.fromarray(np.clip(base, 0, 255).astype(np.uint8), "RGBA")
    draw = ImageDraw.Draw(image)
    for _ in range(46):
        start_x = int(rng.integers(0, SIZE))
        start_y = int(rng.integers(0, SIZE))
        length = int(rng.integers(12, 74))
        colour = (47, 45, 41, int(rng.integers(36, 92)))
        draw.line((start_x, start_y, min(SIZE - 1, start_x + length), start_y + int(rng.integers(-2, 3))),
                  fill=colour, width=1)
    image.save(OUTPUT / f"{prefix}-base-color.png")
    save(f"{prefix}-normal.png", normal_from_height(wear, 0.72))
    orm = np.empty((SIZE, SIZE, 4), dtype=np.float32)
    orm[:, :, 0] = 238.0
    orm[:, :, 1] = np.clip((168.0 if soot else 153.0) - wear * 27.0, 120.0, 205.0)
    orm[:, :, 2] = 232.0
    orm[:, :, 3] = 255.0
    save(f"{prefix}-orm.png", orm)


def resize_shared_array_inputs() -> None:
    sources = {
        "sword-base-color.png": ROOT / "assets/models/weapons/source/meshy-2026-08-26-sword-candidate-1/textures-2k/base-color.png",
        "sword-normal.png": ROOT / "assets/models/weapons/source/meshy-2026-08-26-sword-candidate-1/textures-2k/normal.png",
        "sword-orm.png": ROOT / "assets/textures/held-items/source/sword-orm-2k.png",
        "torch-base-color.png": ROOT / "assets/models/props/source/meshy-2026-08-26-torch-candidate-1/textures-reviewed/base-color.png",
        "torch-normal.png": ROOT / "assets/models/props/source/meshy-2026-08-26-torch-candidate-1/textures-reviewed/normal.png",
        "torch-orm.png": ROOT / "assets/textures/held-items/source/torch-orm-2k.png",
        "torch-emissive.png": ROOT / "assets/models/props/source/meshy-2026-08-26-torch-candidate-1/textures-reviewed/emissive.png",
        "player-base-color.png": ROOT / "assets/textures/player/source/base-color.png",
        "player-normal.png": ROOT / "assets/textures/player/source/normal.png",
        "player-orm.png": ROOT / "assets/textures/held-items/source/player-orm-2k.png",
    }
    ARRAY_INPUT.mkdir(parents=True, exist_ok=True)
    for name, source in sources.items():
        with Image.open(source) as image:
            image.convert("RGBA").resize((SIZE, SIZE), Image.Resampling.LANCZOS).save(
                ARRAY_INPUT / name)


OUTPUT.mkdir(parents=True, exist_ok=True)
rng = np.random.default_rng(SEED)
wood(rng)
iron("chest-iron", rng, False)
iron("lantern-iron", rng, True)
resize_shared_array_inputs()

black = np.zeros((1, 1, 4), dtype=np.uint8)
black[0, 0, 3] = 255
save("black-emissive.png", black)
print(f"Generated neutral production prop PBR sources under {OUTPUT}")
