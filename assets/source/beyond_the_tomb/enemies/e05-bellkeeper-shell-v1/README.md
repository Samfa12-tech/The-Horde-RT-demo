# E05 Bellkeeper: original modular armour shell

Source/mechanism-art proposal for The Horde's single Drowned Abbey Bellkeeper. This is a restrained iron helmet, gorget, fixed back/side cuirass, four shoulder lames, and paired independently hinged front breast shutters. It is not a completed boss or gameplay implementation.

## Canon and proposal boundary

The guardian is the H2 heavy humanoid E05, one singular boss. An actual light path redirected by the placeable reflector must expose its armour before a valid attack; ordinary attacks, direct held light, and item possession alone cannot substitute. The package does not implement or validate that causal path. Receiver placement, exact geometry, exposure timing, attack windows and balance remain blockout proposals. No symbol, new lore, glowing core, weapon or add-wave behaviour is introduced by this shell.

## Included source

- `bellkeeper_fitted_rigged_shell.blend`: editable original shell parts, actual helper-bone pivots/sockets, the inspected Meshy body rig, packed 1K maps, inspection lights and camera. Twenty shell mesh objects remain separate and are rigidly weighted 1.0. The unrigged authoring blend is retained in the source archive and can be regenerated using the included builder.
- `bellkeeper_modular_shell.glb`: shell-only portable binary, with all maps embedded. It excludes the body geometry.
- `textures/`: original locally authored 1K base colour, normal and ORM maps.
- `source/`: deterministic Blender authoring and direct validation scripts.
- `previews/`: closed, open, three-quarter, side, close-up and fresh-import review renders.
- `evidence/`: measured counts, bounds, mesh checks, socket matrices and source lineage.

The fit-reference body is a Meshy-generated and locally normalized derivative. Its separate provider originals and receipts are maintained in the body archive. It is not relabeled as original shell geometry.

## Measured baseline

The shell has 2,832 triangles across 20 independently selectable mesh objects, one opaque iron material, and three 1024×1024 maps. The accepted pre-rig body has 9,072 triangles and one separate 1K material: 11,904 triangles / two materials combined. This is the lower B02 source pilot. It is not an engine, GPU, memory, actor-count or frame-time certification, and it does not constitute an accepted 24k comparison.

Shell-only closed bounds in Blender metres are approximately X ±0.454405, Y -0.230396 to +0.226000, Z 1.091928 to 2.052500. The inner body is exactly 2m tall with feet at Z=0. The shell's helmet raises the assembled top to about 2.0525m.

## Axes and mechanism

Blender is +Z up, -Y forward, +X character left. Normal GLB export is +Y up and +Z forward. Both pivots are at Blender Y=-0.123, Z=1.38, with X=+0.25 for left and X=-0.25 for right. The authored Empty hinge axis is local +Z.

- Closed inspection: both shutter angles 0°
- Open inspection: left +95°, right -95°
- The inner edges swing forward; the sternum and upper abdomen remain unlit
- Four small fixed cleats connect the fixed cuirass to the hinge barrels

Frames 1 and 60 are inspection states only. They do not propose combat timing. The standalone GLB contains separate named left/right hinge animation channels; both must be applied together to reproduce the paired open source state. The editable blend drives them together in its timeline.

Named source sockets include `ChestMount`, `HeadMount`, `Shoulder.L/R`, `ShutterHinge.L/R`, `ReflectedPathReceiver_PROPOSAL`, and `ExposureCentre_PROPOSAL`. Receiver/exposure sockets are invisible metadata proposals, not validated puzzle geometry. The included rigged blend uses the inspected returned rig. Its actual helper bones are ChestMount, HeadMount, Shoulder.L/R, E05_Shutter.L/R, ReflectedPathReceiver_PROPOSAL, and ExposureCentre_PROPOSAL. Hinge pose rotation is local Z, with the same ±95° open state. No mesh relies on bone-parented Empty offsets. The armature has 32 bones before any later motion-trial grip helper.

## QA and known limitations

Direct independent GLB parsing and fresh Blender import verify finite positions, nondegenerate triangles, UVs within 0–1, embedded images, one shell material, source mesh manifoldness and the shell triangle budget. Closed/open inspection renders show distinct exposure with shoulder/axilla space in the reference pose. The rigid-bone closed state differs from the authored rest by less than 2.4e-7m, and open states match the expected pivot rotations within 1.7e-7m. These measurements and static images do not prove dynamic collision clearance, armour-safe strikes, deformation, native RT import or runtime reflected-light behaviour.

The local game-dev CLI and Khronos validator were unavailable. These are direct Blender/static checks, not game-dev certification. The exported GLB is intentionally uncompressed; the exporter reports unavailable optional Draco support. It also reports its standard shared-map sampler warning; fresh import preserves the expected maps.

The core's two small rear-neck holes were closed locally before rigging. A small local nape colour/cloth seam remains documented as a source-pilot limitation. This is visible in the standalone body evidence; armour is not used as proof that the source defect is absent.

## Provenance and rights

All shell geometry, original iron pixels, and shell authoring code were created locally for this task. No third-party model geometry or stock texture was copied into the shell, and no paid provider call was made for it. The original shell/maps retain project-owner rights and receive no new blanket public licence. The included Meshy-generated E05 v03 body has explicit Meshy attribution and a conservative CC BY 4.0 notice because the account plan was not independently verified; see RIGHTS.md for terms, scope and modification history. No runtime code, repository commit, merge, public release or deployment is included.

## Native reader boundary

The unchanged native-reader investigation identifies meshes[0]/skins[0] as the current geometry/skin path. The included editable scene is intentionally not a joined engine export. The motion trial must create and validate a separate joined single-mesh/single-skin derivative. Bone-display Icosphere is a Blender importer helper, not source GLB geometry; exclude it from exports and counts. The existing native character path does not establish generic E05 body/shell PBR rendering.

## Reproduction

Run build_shell.py against the archived accepted body GLB and core-fit-final-v01.json to regenerate unrigged authored parts. Run bind_shell_rig.py against that blend, the archived provider rigged_character.glb and rig-bindings-v03.json to create the fitted editable rig. The exact core and rig hashes are in evidence. The included .blend is self-contained for inspection/editing; provider originals remain in their separate archive.
