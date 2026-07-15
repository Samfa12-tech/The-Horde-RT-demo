# Medieval hand torch v01 - staged Meshy study

## Generation

- Meshy model: Meshy 5.
- Preview task: `019f6498-a3ad-76d2-9e06-9e6ebb2c6d3b` (20 credits).
- Remesh task: `019f649c-83be-7e48-bad9-767c02384b74` (5 credits).
- Requested remesh target: 1,500 triangles, 0.85 m height, bottom origin.
- Result: 1,538 indexed triangles, 1,869 vertices, position/normal/UV streams, 16-bit indices.

## Files

- `medieval_hand_torch_preview_v01.glb`: 17,654-triangle preview source.
- `medieval_hand_torch_lod1_v01.glb`: 1,538-triangle remesh candidate.
- `medieval_hand_torch_lod1_v01_normal.png`: remesh sidecar supplied by Meshy.

## Release state

The generated models have no complete PBR material and are excluded from Android and Windows packages. They are retained as a source study for the eventual measured static-GLB/PBR path. The initial alpha instead uses a phone-safe procedural wooden shaft, iron cage, and two nested emissive flame volumes in the held-torch BLAS.

If a generated torch file is later shipped, verify the Meshy account plan and retain the conservative credit “Medieval hand torch created with Meshy” unless paid-plan provenance is recorded.
