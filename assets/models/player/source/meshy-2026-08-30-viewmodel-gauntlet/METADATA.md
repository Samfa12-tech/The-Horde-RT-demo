# Historical-Gothic viewmodel gauntlet — Meshy provenance and runtime acceptance

## Source and credit record

- Date: 2026-08-30.
- Account/plan evidence: the authenticated project account was used without exposing credentials. The balance immediately before this hand study was 630 credits and the verified closing balance was 547 credits, for an exact 83-credit spend. The account plan was not independently provable, so distribution uses the conservative Meshy Free-plan CC BY 4.0 attribution route.
- Signed download URLs and authentication material are intentionally omitted. The durable evidence is the task ID, retained local source file, and SHA-256 below.

### Rejected candidate 1

- Concept task `01a05058-7391-7100-8c4e-f79ba6f26edd`: Nano Banana Pro, one 1:1 image, no multiview; 9 credits. Exact prompt: `A production 3D asset reference of one anatomically correct RIGHT human hand wearing a fitted dark brown medieval leather fingered gauntlet with a short blackened gothic cuff. Three-quarter view. The hand is closed in a natural cylindrical power grip around an invisible 36 mm handle: four distinct curled fingers, thumb crossing securely over index and middle fingers, believable knuckles and wrist transition. Entire hand and cuff visible, isolated on neutral light grey, no arm beyond cuff, no weapon, no handle, no glow, no text, no props.`
- Concept file `right-gauntlet-reference.png`: SHA-256 `4f014eb571aac331ee2c3abcb8dc11aa20ad59848cc99437f418b756155cce30`.
- Geometry task `01a0505b-29c2-777e-aa45-b1aefff720cf`: Meshy 7 standard, Ultra disabled, triangle topology, target 8,000 polygons, remesh enabled, symmetry off, texture/PBR disabled, centre origin, automatic sizing disabled, GLB plus pre-remesh retention; 20 credits.
- Geometry file `right-gauntlet-preview.glb`: SHA-256 `77fc8c49f8c65eb14c996d6aa5630ef462aa778cd7178db628cd9570df11872a`.
- Retexture task `01a0505d-84bf-726b-a791-826e5fdf3359`: Meshy 7, original UV enabled, PBR enabled, 2K, lighting removal requested, GLB; 10 credits. Exact texture prompt: `Realistic neutral historical-Gothic fingered gauntlet: dark umber brown worn leather glove, subtle leather grain and stitched seams, charcoal blackened iron knuckle plates and cuff with restrained edge wear, small dull steel rivets. No baked lighting, no torch glow, no blood, no logos, no writing. Material must remain readable under warm firelight and cool moonlight.`
- Rejection: the image-to-3D result formed a clenched fist without an auditable through-grip channel aligned with the wrist/forearm. It could not honestly prove contact around the sword, torch, or lantern grip and was rejected even though the API tasks completed.

### Accepted candidate 2

- Concept task `01a05063-413f-7030-adb9-45419fbe14ed`: Nano Banana Pro, one 1:1 image, no multiview; 9 credits. Exact prompt: `Three-quarter side production reference of one anatomically correct RIGHT hand in a fitted dark brown medieval leather fingered gauntlet and short blackened Gothic cuff, holding a plain BRIGHT CYAN cylindrical training hilt in a natural sword power grip. The cylinder runs through the fist from below the little finger to above the thumb and is aligned with the forearm, never through the palm. Four distinct curled fingers contact it; thumb crosses index and middle fingers. Whole glove, cuff and cylinder visible on grey. No blade, guard, ornament, glow or text.`
- Concept file `right-gauntlet-hilt-reference.png`: SHA-256 `36a4520f869551666a1138312f5aec3447b96ee8316bf62604211d0fed2921af`.
- Geometry/PBR task `01a05065-430a-724e-88cc-c569f781d2bd`: Meshy 7 standard, Ultra disabled, triangle topology, target 10,000 polygons, remesh enabled, symmetry off, texture and PBR enabled, 2K, lighting removal requested, centre origin, automatic sizing disabled, GLB plus pre-remesh retention; 30 credits. Exact texture prompt: `Neutral dark brown leather, blackened iron plates, and a bright cyan removable training hilt as visually separate surfaces. No baked lighting or glow.`
- Accepted source `right-gauntlet-hilt-meshy7.glb`: SHA-256 `f77867e3d0f913fab8cfef944bdc32b1ae8baa14176d2d873e8bb183e197e7fd`.
- Remesh task `01a0507d-81ee-7506-aa51-2d6246adc1c5`: triangle topology, target 5,000 polygons, no resize, centre origin, GLB; 5 credits.
- Remesh file `right-gauntlet-hilt-5k.glb`: SHA-256 `9140fc51155e952406eef507140016ff5813be39282fda08ad800237a29a1d3a`.
- Smart Topology calls were attempted before the Meshy 7 standard task, but the local MCP wrapper rejected the unsupported `remove_lighting` request before any remote task was created. No task ID and no credit charge resulted.

## Deterministic source processing

- `tools/prepare-player-gauntlet-source.py` uses the authored UV/base-colour separation to identify and remove the deliberately bright-cyan training hilt. It welds coincident source positions, removes only detached islands of at most eight vertices, records the recovered grip axis/origin as validated glTF custom properties, and retains the authored glove silhouette and PBR UVs.
- Accepted stripped source `right-gauntlet-5k-stripped.glb`: SHA-256 `228e364d82075d7a7ae6e275bb463bc9e73021be83fed4d088734e424f66d400`; 4,419 triangles, 2,317 topological vertices, one retained component, 768 cyan hilt triangles removed, and nine debris vertices removed.
- Processing report `right-gauntlet-5k-stripped.glb.processing.json`: SHA-256 `099d6ddcc2e891c791f8718087cb0df7cb6144ee4ee7e27a44267751573390a3`.
- The higher-resolution stripped experiment `right-gauntlet-stripped.glb` is retained as rejected processing evidence only. It invoked a dense surface rebuild and bounded decimation, so the direct 5K Meshy remesh was preferred because it preserves the reviewed silhouette without voxel remeshing.
- Source GLBs, reference images, and 2K maps are Git-LFS evidence and are excluded from packages. Only the gauntlet geometry integrated into the bounded player runtime and the established shared 1K player texture arrays are distributed.

## Player integration and review

- Although the accepted concept prompt explicitly requested a right hand, visual/topology review found that the generated palm/thumb anatomy is left-handed. Blender 5.2 therefore preserves the authored source on `LeftHand` and mirrors it across the hand-local chirality axis for `RightHand`. This is a true offline mesh mirror, not use of one chirality on both sides. Each generated gauntlet remains rigid to its actual hand bone while the retained fitted traveller sleeves use bounded shoulder/elbow/wrist skin weights.
- The original source gloves are removed. No procedural capsule palm, sausage finger, or voxel-remeshed hand remains in the accepted runtime path.
- Current player runtime GLB SHA-256: `09d2952bea059353d15fd16367a22eb8c46d6ed82de9318656444033520636ff`; 2,793,924 bytes.
- Current runtime geometry: 27,775 triangles; 33,894 processing vertices; 37,683 upload vertices; 83,325 glTF-expanded vertices/indices; three semantic primitives. `BodyPrimaryVisible` contains 14,370 triangles, `HeadPrimaryMasked` 1,813, and `NearFacePrimaryMasked` 11,592.
- Both hands pass the final-skinned contact contract against the actual handle surface. Left/right minimum surface errors are 0.01385 mm and 0.01758 mm, with 3,518/3,565 contact samples, 24 occupied axial bins each, and 86.83/131.71 mm longitudinal contact spans.
- Boots remain grounded at the authored route floor. A deterministic 180-tick claimed-lantern wall approach retains a true camera/body clearance of 0.364639 m, finite skinning and pendulum state, 141 collision-blocked ticks, a 0.210092 m maximum skinned edge, and a 0.0162076 m² maximum projected triangle area.
- Honest native RT checkpoint review covers rest/player grips, downward cut, and upward slice. Owner subjective hand/attack acceptance remains pending on the loaded Windows candidate.
- Attribution: `Historical-Gothic viewmodel gauntlet created with Meshy; runtime processing by Samfa12/Codex.`
