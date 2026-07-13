# Textures

The first environment PBR batch is live under `polyhaven/mobile_1k/`.

- Five exact CC0 Poly Haven sources: `medieval_wall_02`, `cobblestone_floor_08`, `mossy_stone_wall`, `damp_sand`, and `rusty_metal_04`.
- Each source folder retains its 1K JPG diffuse, OpenGL normal, and packed AO/roughness/metal (`arm`) maps.
- The retained derivation sources are three 512 x 512, five-layer RGBA arrays: `diff-array-512.rgba`, `normal-array-512.rgba`, and `arm-array-512.rgba`.
- Android packages strict KTX2 arrays using ASTC 6x6 for diffuse/ARM and ASTC 4x4 for normals. The GPU payload is about 2.38 MiB instead of 15 MiB raw.
- Runtime layer order is dry stone, wet cobblestone, mossy stone, damp ground, aged metal.
- Windows retains the raw RGBA fallback because desktop ASTC support is not assumed. Android reports an explicit unsupported diagnostic if its required ASTC formats are unavailable.
- Regenerate the KTX2 derivatives with `tools/compile-material-textures.ps1` and validate their exact VkFormat headers before use.

Texture rules:

- Use commercial-safe high-quality PBR textures.
- Record source and license in `ASSET_LICENSES.md`.
- Prefer wet stone, aged masonry, puddles, metal edge wear, torch/fire emissive masks, and rough historical materials.
- Keep compressed derivatives capability-checked and visually compared against the retained source arrays.
