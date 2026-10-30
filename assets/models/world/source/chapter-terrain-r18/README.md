# Topo terrain r18: source candidate

Original Horde editable Blender terrain from the owner-supplied r2 heightfield. The 2.4 m trail follows the reviewed roughly 76 m route, with wider reunion/lookout pockets. Real ground extends behind and around the rope; the far perimeter and underside are closed. Nine occupied tomb volumes stay clear and three distinct rescue/skylight apertures stay open. A rounded barrow covers the shortened stair return in `../chapter-entry-return-r03/`; all 263 intersected terrain/roof rectangles have at least 0.3938 m cover.

The native importer accepts 39,618 triangles and two material groups. Actual exported positions/topology/winding match `surface.json`. Native triangle support passes 2,781 queries and 616 continuous bidirectional probe sweeps (nine-probe footprint coverage, not full capsule/headroom/collision acceptance). CPU retained support arrays occupy 2,873,928 bytes; timings in the Debug receipt are single host observations, not sustained/mobile performance.

All textures reuse admitted Poly Haven damp_sand/mossy_stone_wall CC0 maps at .42 repeats/metre. Texture hashes, recipe hash, input hash and file hashes are in `authoring-receipt.json`. Source GLB and editable Blender file remain outside runtime packages. `surface.json` is prepared correspondence evidence; live gameplay has not adopted it yet.

Reproduction (use absolute ROOT and fresh output):

```
blender --background --factory-startup --python-exit-code 1 --python tools/author-chapter-terrain.py -- --root ROOT --source-npz ROOT/assets/source/beyond_the_tomb/terrain/topo-r2/proposed_terrain_heightfield.npz --entry-audit ROOT/assets/models/world/source/chapter-entry-return-r03/authoring-audit.json --output-directory ROOT/assets/models/world/source/NEW_TERRAIN_DIR
python tools/validate-chapter-terrain.py ROOT/assets/models/world/source/NEW_TERRAIN_DIR
```

The same geometry was inspected in source revision r16. Its rounded mound removes the conspicuous box-shaped source terraces; all geometry, surface materials, route stations and roof envelopes were compared equal to r18. These Blender inspections are not native RT acceptance. Runtime admission, matching support publication/readiness, resource retirement, dressing, full capsule clearance and the required native gallery remain open. Failed r14 export and earlier geometric candidates are preserved locally.
