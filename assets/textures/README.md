# Textures

The first environment PBR batch is live under `polyhaven/mobile_1k/`.

- Five exact CC0 Poly Haven sources: `medieval_wall_02`, `cobblestone_floor_08`, `mossy_stone_wall`, `damp_sand`, and `rusty_metal_04`.
- Each source folder retains its 1K JPG diffuse, OpenGL normal, and packed AO/roughness/metal (`arm`) maps.
- The Android proof derives three 512 x 512, five-layer RGBA arrays: `diff-array-512.rgba`, `normal-array-512.rgba`, and `arm-array-512.rgba`.
- Runtime layer order is dry stone, wet cobblestone, mossy stone, damp ground, aged metal.
- The arrays are deliberately uncompressed first-proof data. Replace them with a mobile GPU-compressed format after the material mapping is settled.

Texture rules:

- Use commercial-safe high-quality PBR textures.
- Record source and license in `ASSET_LICENSES.md`.
- Prefer wet stone, aged masonry, puddles, metal edge wear, torch/fire emissive masks, and rough historical materials.
- Plan mobile-appropriate compression later without undermining the RT visual goal.
