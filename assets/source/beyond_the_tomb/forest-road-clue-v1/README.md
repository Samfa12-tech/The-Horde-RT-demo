# Original F03 road clue masonry

These two original meshes were authored locally for Horde Lantern RT on
2026-10-23. The human-readable [recipe](recipe.json) and standard-library Python
[generator](../../../../tools/prepare-forest-road-clue.py) are the source masters.
There is no downloaded mesh, stock source, image, provider job, paid generation,
Blender import or external geometry licence. This receipt assigns no blanket
CC0, MIT or other third-party/open-source licence to project-authored geometry.
Project distribution and the separately licensed existing masonry texture remain
recorded by the integration owner in [ASSET_LICENSES.md](../../../../ASSET_LICENSES.md).

Only the two generated GLBs and their generic native-loader manifest belong in
[the runtime directory](../../../models/world/runtime/forest-road-clue-v1).
Keep this README, recipe and generator out of game packages.

## Authored geometry and material

Units are metres, up is +Y, and both objects have their bottom at Y=0 with the
body centred on X/Z=0. Each GLB has one scene, one implicitly identity node,
one mesh and one indexed triangle primitive. Face-split vertices retain hard
edges, authored unit normals, UV0 and unit tangents with exact W=+1. The
closed-component check welds positions across face seams; vertex indices are
deliberately separate at normal/UV seams.

The kerb is an original closed 0.26 m X by 0.65 m Z by 0.14 m Y box. The
waystone is a closed tapered rectangular body, 1.05 m high, 0.50 by 0.27 m at
the bottom and 0.32 by 0.18 m at the top. Its front is -Z. Three short raised
stone score bars are centred at Y=0.570, 0.675 and 0.780 m; their respective
widths are 0.160, 0.135 and 0.110 m. Each is 0.022 m high along the tapered
front plane, extends 0.016 m outward normal to that plane and penetrates
0.003 m into the body. Each bar is a complete closed box. These overlapping
closed solids avoid coplanar contact faces; they are not a Boolean union.
All four waystone components share its sole primitive/material.

UV0 uses planar face charts at **1 repeat per metre** in each direction,
including the tapered faces and bars. Vertical face V follows world up projected
into that face; horizontal faces use horizontal charts. The tangent and
bitangent follow increasing U and V. No baked illumination or extra textures
are present.

Both GLBs contain the same sole opaque material named `HordeRoadMasonry`:
white linear base factor with alpha 1, roughness factor 1, metallic factor 0,
emissive factor [0,0,0], no transmission extension (glTF default 0), no texture,
image or sampler, and `doubleSided=false`. Integration maps this ordinary
material to the existing licensed `MedievalWall02` shared atlas family. This
recipe introduces no atlas layer or material-specific shading path.

| Runtime file | Bytes | Vertices | Indices | Triangles | Closed components |
| --- | ---: | ---: | ---: | ---: | ---: |
| `horde-road-kerb.glb` | 2,596 | 24 | 36 | 12 | 1 |
| `horde-road-waystone.glb` | 6,252 | 96 | 144 | 48 | 4 |
| `asset.manifest.json` | 960 | - | - | - | - |

The manifest uses the same schema-1 fields as the existing forest-rock assets.
Its limits apply independently to a loaded source GLB: 96 vertices, 144 indices,
48 triangles, one primitive and one material. `maxTextureLayersPerKind=1` is the
loader's minimum positive capacity; neither GLB carries texture payloads. Eight
placed kerbs plus one waystone contribute 144 triangles before any unrelated
scene geometry. The two distinct GLBs together contain 60 source triangles and
the runtime directory totals 9,808 bytes.

Nominal bounds are kerb [-0.13,0,-0.325] to [0.13,0.14,0.325] m and waystone
[-0.25,0,-0.135] to [0.25,1.05,0.135] m. The exact stored float32 accessor bounds
are:

```json
{
  "horde-road-kerb.glb": {
    "min": [-0.12999999523162842, 0.0, -0.32499998807907104],
    "max": [0.12999999523162842, 0.14000000059604645, 0.32499998807907104]
  },
  "horde-road-waystone.glb": {
    "min": [-0.25, 0.0, -0.13500000536441803],
    "max": [0.25, 1.0499999523162842, 0.13500000536441803]
  }
}
```

## Exact provenance (SHA-256)

| Source or runtime file | SHA-256 |
| --- | --- |
| `recipe.json` | `df7451569bdedd4c2d67237836c07c9369e12b529fc44f0c293d0587b670ba6d` |
| `tools/prepare-forest-road-clue.py` | `86b78348ab736f02b4b882f7ceb12ab9202af9362e80777169ce6b5e43adc32d` |
| `asset.manifest.json` | `d69c31f7fe62864dcdcb76ed054dc2316fe6e63f2b40c45b9cffc9e54b68b0d5` |
| `horde-road-kerb.glb` | `7bf6a0bafb61d0c32c25ca8f7e730db5d17a72582b1d12097a381be2d29632bc` |
| `horde-road-waystone.glb` | `7229e9920b0d5892a5f480722fa894a16a667c86059d29967d2a031e3f6edb1b` |

The derivative transformation is direct construction from the listed dimensions:
six outward quads per solid, two triangles per quad, separate face attributes,
little-endian float32 position/normal/UV/tangent streams, uint16 indices and
four-byte-aligned glTF 2.0 JSON/BIN chunks. No geometry compression, scale,
node transform or nondeterministic metadata is applied.

## Reproduce and validate

From the repository root with Python 3.12, generate to a new or empty directory:

```powershell
python tools/prepare-forest-road-clue.py --output-directory <new-or-empty-directory>
```

Omitting the output argument selects the runtime directory. Generation refuses
to overwrite existing files. Verify admitted bytes without writing:

```powershell
python tools/prepare-forest-road-clue.py --verify
```

On 2026-10-23 both commands passed with Python 3.12.10. A fresh temporary directory
regeneration produced byte-identical SHA-256 hashes for all three runtime files.
Every generation also compares two in-memory exports. The decoded-buffer checks
cover GLB framing, buffer/accessor ranges, exact counts and bounds, finite
attributes, unit perpendicular normal/tangent bases, exact tangent handedness,
outward face-normal agreement, nondegenerate triangles/UVs, physical UV scale,
UV-derived tangent orientation, and closed consistently wound components after
position welding. Each closed component has eight welded corners, eighteen
undirected edges, twelve triangles and positive signed volume.
Independent analytical component volumes agreed within 2e-8 cubic metres.
Five in-memory malformed copies were rejected: reversed winding, a nonfinite
normal, tangent W=0, a degenerate UV triangle and doubled UV metre scale.

This is local source/GLB structural evidence. Native importer integration,
packaging admission, RT-produced presentation, matched image inspection and
owner visual acceptance are separate integration checks. No phone operation,
heavy build, app launch, performance or visual acceptance is claimed here.
Audio/haptic manual revalidation required: NO; these are silent static meshes.
