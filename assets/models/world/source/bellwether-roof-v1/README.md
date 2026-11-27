# Bellwether clay roof tiles

The pitched roofs of B01–B08 use a 2.4 m square repeat of hand-laid plain tiles. Twelve tiles span the ridge and sixteen courses overlap down each slope. Each tile is 194 mm wide and 244 mm long, with 94 mm of overlap. U follows the ridge. V rises from the eaves, with the lower lip of each tile facing downhill.

From the repository root, rebuild the source maps with NumPy and Pillow, then compile the runtime KTX2 arrays with the project texture compiler.

```powershell
python assets/models/world/source/bellwether-roof-v1/generate_roof_maps.py
./assets/models/world/source/bellwether-roof-v1/build_runtime_textures.ps1
```

The fixed seed and source PNG digests are in [the recipe](maps/recipe.json). The height map supplies the relief for the OpenGL +Y normal. The game loads base colour, normal and packed ambient occlusion, roughness and metalness. Both platforms use 1,024-square maps with eleven mip levels. Windows uses RGBA8. Android uses ASTC 6×6 for base colour and ORM, and ASTC 4×4 for normals. The separate Blender preview recipe renders the material under studio lights.

The editable building geometry and its UV contract are in [the roofline source](../bellwether-roofline-v1/README.md).
