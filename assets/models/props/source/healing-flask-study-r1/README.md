# Original healing-flask source study r1

**Source-only, unadmitted.** This is an original flattened pilgrim-flask shape,
not a runtime manifest or a completed healing feature. The single opaque PBR
material suggests worn pewter. A 14 mm recessed mouth replaces the sealed first
draft so a later drinking pose will not visibly pour through a cap. It supplies
no glass, liquid, light, detachable stopper or new texture.

The generated GLB is 9,292 bytes: 242 vertices, 384 triangles, one primitive and
one material. Bounds are 114 mm wide, 164 mm high and 56 mm deep. `Grip` is an
exact-case meshless marker at `[0,0.065,0]` metres in the GLB's +Y-up/+Z-front
convention. Its identity rotation is provisional for hand-fit review, not proof
of alignment with the existing weapon hand. No hand/actor rig is embedded.

- `recipe.json`: editable profile, material, units, grip and 400-vertex/400-triangle
  source budget. These are asset budgets, not permission to raise renderer caps.
- `healing-flask-study-r1.glb`: deterministic source mesh, absent from runtime rosters.
- `healing-flask-study-r1.blend`: editable import plus labelled authoring studio.
  Floor/cameras/lights are preview-only and absent from the GLB.
- `authoring-preview.py`: new-only Blender authoring scene and three views. It
  imports GLB and saves Blender source; it never invokes the glTF image exporter.

From the repository root, create the GLB only when its destination is absent:

```powershell
python tools/author-healing-flask-study.py
python tools/author-healing-flask-study.py --verify
```

The second command is read-only and proves exact GLB regeneration. The first
refuses existing files. Do not delete owner files to make either command run.
The preview script likewise refuses existing Blend or preview destinations;
ordinary review opens the retained source rather than rerunning authoring.
No existing asset was normalized or replaced. The optional `game-dev` CLI is
absent here; no CLI canonical package or normalization is claimed. This uses
the project's existing original-source authoring route.

See the [review/validation evidence](../../../../../docs/evidence/2026-10-26-flask-source-study/README.md).
Khronos validation has zero errors/warnings and one expected informational
`NODE_EMPTY` for `Grip`. Static closure/winding/normals and byte regeneration
pass. Blender previews establish source silhouette/mouth only. Native loader,
RT appearance, held/drink joins, clearance, interruption, upgrade presentation,
touch controls and actual campaign/device acceptance remain unproved. The
approved backend remains OFF pending its joint resource/presentation gates.

Original project-authored geometry and PBR factors; no external mesh/image,
paid generation or provider input. User/service rights remain unchanged; no
blanket CC0/MIT or third-party licence is assigned. See `ASSET_LICENSES.md`.
Source data stays out of shipping manifests/packages.
