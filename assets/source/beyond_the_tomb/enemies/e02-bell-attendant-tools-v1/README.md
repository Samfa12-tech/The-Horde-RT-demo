# E02 Abbey bell attendant: original tool candidates

Two original, editable duty tools for the proposed E02 role in The Horde's river-fed Drowned Abbey: a modest hollow iron handbell and a short ash maintenance staff. This is a source-plus-evidence proposal. It does not admit either tool to the game or certify runtime performance.

## Included

- `abbey_bell_attendant_handbell.glb` and `.blend`: complete hollow cup, wooden handle, plain ferrule, internal suspension boss, and a separate pivoted iron clapper.
- `abbey_bell_attendant_short_staff.glb` and `.blend`: gently imperfect oval ash shaft with two plain, blunt iron end shoes.
- `textures/`: shared original 512 × 512 base-color, ORM and tangent-space normal atlas.
- `source/build_tools.py`: reproducible local construction, texture pixels, exports and source previews.
- `source/validate_tools.py`: source topology, binary glTF payload and fresh Blender import checks.
- `source/render_bell_inspection.py`: repeatable underside inspection with an explicitly added fill light.
- `previews/contact_sheet.jpg`: labeled view sheet. Individual PNGs retain higher-detail inspection views.
- `evidence/`: specifications, actual validation, exported glTF documents, texture-channel checks, environment probes and logs.
- `provenance.json` and `manifest.json`: source/rights description and file integrity.

## Measured geometry

The handbell has 1,640 exported triangles, 1,249 vertices after UV/hard-normal splits, two meshes/primitives and one opaque single-sided material. Its bounds are 0.154 × 0.154 × 0.295 metres. The editable body/handle mesh contains four closed shells; the separately pivoted clapper contains two. The bell cup is an actual thick-walled hollow mesh with an open mouth, rather than a dark painted disc.

The staff has 564 exported triangles, 654 vertices after attribute splits, one mesh/primitive and one opaque single-sided material. Its measured bounds are approximately 0.03064 × 0.02940 × 0.880 metres. It has three closed component shells: shaft and two reinforcing shoes. The overall length is 88 cm, with a longer working end above the proposed grip.

Mechanical joints intentionally overlap. Hidden bores, an articulated hinge, collision hulls and wall-thickness manufacturing tolerances are not modeled. Components can be separated by linked geometry in Blender; the clapper is already its own object. Neither export contains a body, skeleton, animation, armature or transparent geometry.

## Grip, axes and clapper reference

Each Blender master is authored in metres at unit scale. Its named root Empty is at the identity transform, not a calibrated H1 bone/socket transform. Each model is exported independently at that root.

### Handbell

- Root: `Bell_Grip` at Blender XYZ (0, 0, 0).
- The handle points Blender +Z away from the bell; the bell mouth faces -Z.
- Nominal grip cross-section: 27 × 23 mm oval; usable provisional region Z -0.030 to +0.065 m (95 mm).
- In ordinary Y-up GLB, the handle points +Y, mouth -Y and grip region is Y -0.030 to +0.065 m.
- `Bell_Clapper_Pivot` has origin Blender (0, 0, -0.059 m), or glTF (0, -0.059 m, 0). It is at rest beneath the fixed internal boss. The clapper can be rotated locally around X/Y for authoring; no allowed swing range or contact animation is asserted.

### Short staff

- Root: `Staff_Grip` at Blender XYZ (0, 0, 0).
- Shaft points +Z toward the longer working end; source Z endpoints are -0.330 and +0.550 m.
- Nominal grip cross-section: 29 × 26 mm oval; provisional grip region Z -0.050 to +0.060 m (110 mm).
- Normal GLB export maps this to +Y, endpoints Y -0.330 and +0.550 m, and grip region Y -0.050 to +0.060 m.

Blender XYZ maps to glTF X,Z,-Y. The bell's natural hanging orientation is explicitly different from the E01 mallet's +Z-toward-head convention. These are authoring references only. Actual handedness, finger curl, measured H1 socket transforms, reach and clipping must be checked during fitting.

## Material and render evidence

The tools share newly authored texture pixels with muted ash/iron tones informed by the earlier original E01 maintenance mallet. No E01 geometry, image texture, approved character or torch was copied.

Base color is sRGB. ORM is linear: R=1 neutral AO, G roughness, B metallic. Normal is linear, tangent-space OpenGL +Y, strength 0.55. Alpha is fully opaque. Texture pixels and UVs are finite; each GLB embeds all three PNGs. Both Blender masters pack their images. No baked AO or high-poly normal bake is claimed.

Preview tiles use independent framing, which is labeled on the sheet; they are not a same-scale comparison. The two underside views use a small, explicitly labeled inspection fill to expose the real cup interior and clapper. Images 09 and 10 were rendered from fresh imports of the delivered GLBs into empty Blender scenes. No engine screenshot or in-game appearance is implied.

The shared atlas is embedded independently into each portable GLB. Runtime deduplication, texture conversion/mips/ASTC, instance/material handling and draw-call effects remain engine-admission work.

## Validation result and limits

Recorded direct checks PASS for both masters: finite positions/normals/tangents, unit normals, valid indices, no degenerate geometry or UV triangles, no opposed vertex normals, UVs inside [0,1], one opaque material per export, three 512px embedded images, packed Blender images, and preserved bounds after fresh import. The editable source has closed, outward-wound manifold component shells.

GLB attribute splits naturally create disconnected edges in the raw imported topology. A separate temporary positional weld at 0.0000001 m recovers the same closed source shells for topology checking; no weld is applied to the deliverable.

The installed environment has Blender 4.3.2 but no `game-dev` CLI or Khronos validator. This is not a game-dev canonical package receipt. The optional Draco library is absent; these exports intentionally use standard uncompressed glTF with no Draco extension. Blender's shared ORM sampler warning is recorded; the checked payload and fresh-import renders contain the expected maps.

No engine/GPU/phone test, H1 hand fitting, collision, attack timing, alarm audio, clapper simulation, combat cue or gameplay integration was performed. The register's proposed higher shoulder bell/yoke silhouette is still deferred. The full E02 role remains optional after E01 works; this pair does not constitute a new approved creature or completed silhouette variant.

## Reproduce

Use Blender 4.3.2 or a compatible later build and a new output folder:

```sh
blender -b --threads 4 --python source/build_tools.py -- --output-dir /absolute/new/output
blender -b --threads 4 --python source/validate_tools.py -- --asset-dir /absolute/new/output
```

Blender source files retain editable polygon meshes and triangulation modifiers. Fixed texture seed: 26100602. Geometry and texture pixels are deterministic; file metadata, compression, saving and render sampling are not guaranteed byte-identical across software versions.

## Authorship and rights

Original geometry, procedural texture pixels and local source work for this project. No paid generation, external model, stock texture or newly licensed input was used. No new blanket public licence is assigned; existing project ownership and agreements remain the applicable context. No repository, engine code, approved character, saved asset or public deployment was modified by this kit's authoring.
