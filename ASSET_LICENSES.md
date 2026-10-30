# Asset Licenses

## Original F03 road masonry clue (1.7 development)

`tools/prepare-forest-road-clue.py` authors two small closed static GLBs from
numeric geometry for this project: a kerb and a tapered waystone with three
raised scored marks. The recipe and authorship record remain under
`assets/source/beyond_the_tomb/forest-road-clue-v1/`; runtime consumes only the
three-file `assets/models/world/runtime/forest-road-clue-v1/` roster. No external
model, generation service or paid input was acquired. Applicable user/service
rights remain unchanged; no blanket MIT, CC0 or third-party licence is assigned
to the original geometry. Placements reuse the existing Poly Haven CC0
MedievalWall02 material and maps, whose separate attribution remains below.
The same terrain-fitted triangles supply ordinary RT geometry and nonwalkable
contact in the existing consolidated world owner. No new material, texture
layer, light, transparency, instance, save field or road interaction is added.
Native image/grounding and physical-device acceptance remain separate.

## Original tomb-floor masonry chips (1.7 development)

`src/scene/TombFloorDebris.h` authors six small closed fractured-stone meshes in
metres, placed beneath the existing broken right roof shoulder. The deterministic
recipe is original Horde geometry, using the existing shared static PBR path and
CC0 Poly Haven MedievalWall02 material/texture family documented below. No new
third-party model, texture layer, generated/downloaded asset, licence change or
paid production is involved. These are static cosmetic pieces; no kick/contact,
collision or save state is introduced. Disabled construction omits their geometry.


## Generic distant forest bird candidate (1.7 development)

`assets/audio/filmcow/forest_distant_bird.wav` derives from the existing private
FilmCow Recorded SFX archive's exact `distand bird.wav` (source SHA-256
`eeec1e8ebae3af6395640fb0086cb2ebf006724efbb4859e93fb9c5b4a9de8a4`).
FilmCow's custom royalty-free project-use terms apply, as for the existing
showcase/equipment subsets below: personal/commercial project use is permitted;
national-government, law-enforcement and SPLC/CAHN-designated hate-group uses
are prohibited. This is **not CC0**; do not claim authorship or redistribute the
private original as a standalone sound library. Official source/terms:
https://filmcow.itch.io/filmcow-sfx . Voluntary credit: FilmCow Royalty Free Sound
Effects Library - FilmCow.

Only one mono 48 kHz PCM16 runtime derivative and its compact provenance
manifest enter packages. `tools/prepare-forest-bird.py` reproduces the inspected
FFmpeg 9 conversion without gain, pitch, crop, filters or looping; original
source remains private and unchanged. Runtime SHA-256:
`811c2a1d10a565615dd786ef4b99fda5dd0e4465aa121e36ad9158531d6902ff`.
It is an 8.288-second **generic distant bird candidate**, not an identified
nocturnal species or complete forest soundscape. Listening, night suitability,
mix and exact-device acceptance remain pending. This repository-native recipe
does not claim canonical game-dev CLI package admission.

## Prepared original woodland rocks (1.7 development)

The low boulder, upright crag and split shelf are the three original Horde
masters in the supplied forest-dressing package. Their editable scene, maps,
authoring scripts, original-authorship notice and all 59 hash-verified payloads
remain in `assets/models/world/source/forest-understory-v1/original-package/`.
Separate `forest-rocks-v1` runtime derivatives add authored tangents, preserving
source triangles, UVs, normals and embedded image pixels. The runtime placements
reuse the admitted Poly Haven Boulder01Rock PBR family; that CC0 attribution
remains below. The original slate maps are preserved in source, rather than
overwritten or represented as the reused material. Native render and collision
consume the same world-baked triangles. No new model generation, lights,
transparency, material rows or texture layers are introduced.

## Original Bellwether distant shell (Horde1.7 development chapter)

The source-only `assets/models/world/source/chimney-smoke-study-r1/` derivative
opens the existing B01 flue using the same original geometry and material owners.
Its editable Blender scene and inspection lights are authoring evidence, excluded
from runtime packages. No third-party geometry/texture or admitted smoke effect
is added; the v4 runtime remains unchanged. The source README and manifest retain
exact provenance, regeneration and the rejected shader study's boundary.

`assets/models/world/runtime/bellwether-shell-v4/` contains original closed Horde
geometry authored with Blender5.2 from the reviewed connected-map footprints.
The editable master, exact map/terrain/export hashes and reversible foundation
adjustments are in `assets/models/world/source/bellwether-shell-v4/`.
The owner-authorised public chapter task covers these original authored masses;
no third-party model, new generation, light or texture is included in the GLB.
Runtime reuses the existing MedievalWall02 and TombEarthenware material/texture
owners. Their individual provenance below remains unchanged. The preserved v1/v2
source iterations and v2 export stay intact outside new runtime packages.
Revision4 adds bounded solid portal/step/tower/shutter detail to the same
nine closed footprints; no building interior, new light or texture is admitted. This asset notice
does not apply the code licence to third-party texture/audio/model sources.

## Generic closed dielectric fixture

- `assets/models/props/runtime/dielectric-fixture/closed-glass-lod0.runtime.glb`
- Original deterministic test geometry generated in-repository by
  `tools/generate-dielectric-fixture.py`; no third-party source or licence is involved.

The running RT scene uses the five Poly Haven material sets, the Hotstrike Studio skeleton derivative, the CC0 Meshy placeholder lich, the FilmCow sound subset, the DRAGON-STUDIO water loop, the user-selected Pixabay chest/torch cues, and the production Meshy 7 sword, torch, player, reward chest, and reward lantern recorded below. Source/high assets stay outside release packages; only audited runtime GLBs, manifests, WAV derivatives, 1K platform texture arrays, and this attribution record are distributed.

1.6.2 Keeper flank lighting reuses two instances of the existing production medieval hand-torch runtime body and its admitted materials/textures/sockets. The source and runtime asset bytes, CC BY4.0 attribution and provenance below are unchanged. Generic authored stone/iron stand geometry uses the existing world material sets. No new generated or purchased asset is introduced; see [lighting evidence](docs/ENGINEERING_1_6_2_KEEPER_TORCH_LIGHTING.md).

## Asset rules

The exact owner-supplied FilmCow licence PDF and its SHA-256 are recorded in
[the licence provenance](docs/licenses/README.md). In addition to the project-use
restrictions below, that PDF prohibits claiming authorship of the sounds or
reselling them. These terms apply to both selected FilmCow subsets.

- Every asset must be commercial-safe.
- Every asset must have a source URL or source note.
- Every asset must have a license recorded before use.
- Prefer glTF/GLB for models where practical.
- Use high-quality PBR textures when visual work begins.
- Meshy-assisted assets are allowed when their source/licence route is recorded; Meshy models must be textured before export.
- Do not import untextured Meshy models and call them complete.
- When the Meshy account plan is unknown, do not claim paid-plan terms. Ship only when the underlying asset permits distribution and apply the conservative Meshy Free-plan CC BY 4.0 attribution path.

## Asset manifest

| Asset | Type | Source | License | Imported by | Notes |
|---|---|---|---|---|---|
| Gothic arming sword, player right-hand v01 | Source GLB plus 12,358-triangle LOD1; embedded PBR textures; 2K sidecar maps; stripped development runtime GLB and audited KTX2 arrays | Meshy-6 text-to-3D task `019f48d9-df47-7c7a-8341-20e8a11adb8b`; PBR refine task `019f48dd-bbe8-7d0c-9450-c1c13c0c7f06`; see `assets/models/weapons/meshy/gothic_arming_sword_rh_v01.METADATA.md` and `assets/models/weapons/meshy/runtime-development/runtime-budget.json` | **Pending account-plan verification. Development-only and excluded from every Windows/Android package.** Paid/private Meshy output may be commercially used under Meshy terms; free output requires CC BY 4.0 attribution. Do not ship until this is resolved. | Codex, 2026-07-10/11 and 2026-08-26 | Source GLB is 49,439 triangles. `gothic_arming_sword_rh_lod1.glb` was welded and simplified with glTF Transform/meshoptimizer to 12,358 triangles. The `pbr-sword-closeup` Debug checkpoint alone loads its stripped 474,260-byte runtime GLB and mipmapped Windows KTX2 arrays through the generic static RT route. Normal gameplay and all packaged Release builds retain the procedural sword; source-tree fallback is disabled. The skeleton is unarmed. |
| Medieval hand torch concept and LOD1 v01 | Meshy-5 preview GLB plus 1,538-triangle remesh GLB and remesh normal sidecar | Meshy-5 text-to-3D task `019f6498-a3ad-76d2-9e06-9e6ebb2c6d3b`; remesh task `019f649c-83be-7e48-bad9-767c02384b74`; see `assets/models/props/meshy/medieval_hand_torch_v01.METADATA.md` | **Staged only; not distributed.** The conservative Meshy Free-plan CC BY 4.0 attribution path will be used unless a paid-plan record is confirmed. | Codex, 2026-07-15 | The generated LOD has no complete PBR material and is intentionally excluded from Android and Windows packages. Its silhouette informed the live phone-safe procedural wooden shaft, iron cage, and layered emissive flame. Credit if later shipped: “Medieval hand torch created with Meshy.” |
| Production Gothic arming sword, right hand, 2026-08-26 | Fresh Meshy 7 Ultra source/refine lineage; 11,499-triangle runtime GLB; exact `Grip`; shared two-layer 1K PBR arrays | Preview `01a03b99-8999-7adc-8590-536691aacb87`; refine `01a03b9c-d300-78bb-adc2-8fc93a65306f`; exact prompts/settings/hashes in `assets/models/weapons/source/meshy-2026-08-26-sword-candidate-1/METADATA.md` | **CC BY 4.0, conservative Meshy Free-plan route.** The active account plan was not independently provable. Attribution: “Production Gothic arming sword created with Meshy; runtime processing by Samfa12/Codex.” | Codex, 2026-08-26 | Candidate 1 accepted; no second sword candidate generated. Runtime GLB and 1K Windows/Android PBR arrays are distributed; source/refined GLBs and 2K maps remain Git-LFS source evidence and are excluded from packages. |
| Production medieval hand torch, 2026-08-26 | Fresh Meshy 7 Ultra source/refine lineage; reviewed local neutral PBR grade; 4,999-triangle runtime GLB; exact `Grip`, `Flame`, `Light`; shared two-layer 1K PBR arrays | Candidate-1 preview `01a03b9d-02b1-7b7b-86ee-5e8c1f47af51`; refine `01a03ba0-23a8-7bd4-92bf-3f0493705c61`; rejected candidate-2 preview `01a03ba9-ae5f-79d9-92ee-08a56e9bbec8`; details in `assets/models/props/source/meshy-2026-08-26-torch-candidate-1/METADATA.md` | **CC BY 4.0, conservative Meshy Free-plan route.** The active account plan was not independently provable. Attribution: “Production medieval hand torch created with Meshy; runtime processing by Samfa12/Codex.” | Codex, 2026-08-26 | Candidate 1 geometry accepted; its pale Meshy PBR was rejected and neutrally regraded without baked light/emissive. Candidate 2 was rejected before refine because it contained prohibited authored flame geometry. Only the candidate-1 runtime body is distributed. The visible fire is now the engine-owned bounded `FireEmitter` production path: RT-visible core, depth-clipped world-space volume, deterministic flicker, and coherent coloured light/reflection state. |
| Player rag torch v01 | Original assistant-authored Blender geometry and procedural PBR maps; 5,452-triangle stripped runtime GLB; exact `Grip`, `Flame`, `Light`; appended as props atlas layer 12 with Windows RGBA8 and Android ASTC arrays | User-supplied `rag_torch_v01_blender_package.zip`, SHA-256 `0808734c9a2d664a2b4a80d9d7221dca11c9b79fa4cc868fcecf8b1cda026889`; input manifest/hash receipt at `assets/textures/props/source/rag-torch-v01/input-receipt.json`; processing evidence at `assets/models/props/runtime/player-rag-torch/processing-receipt.json` | Original assistant-authored asset supplied for the user's torch brief. No third-party or paid input. No blanket MIT, CC0, Meshy, or other licence is assigned; applicable user/service rights remain unchanged. | Codex, 2026-10-07 | Player-held torch only. No decimation or baked flame; the engine-owned fire and light use the authored sockets. Keeper flank torches retain the existing Meshy production torch (the preceding row); the reward lantern is unchanged. Original `.blend` and source GLB remain outside the repository; only the runtime GLB and source PBR maps are retained here. |
| Player sword scabbard v01 | Original assistant-authored Blender sheath mesh and procedural 1K PBR maps; 304-triangle stripped runtime GLB; one generic static-PBR primitive; props atlas layer 13 with Windows RGBA8 and Android ASTC arrays | Authored locally by Codex from measured production-sword mesh bounds; `tools/author-player-sword-scabbard.py` and `tools/generate-sword-scabbard-textures.py`; processing evidence at `assets/models/props/runtime/player-sword-scabbard/processing-receipt.json` | Original assistant-authored geometry and maps; no third-party or paid input. No blanket MIT, CC0, Meshy, or other licence is assigned; applicable user/service rights remain unchanged. | Codex, 2026-10-07 | The runtime prop follows the animated player Hips mount and uses the shared static-PBR path. Production sword-stow activation flags remain disabled pending gameplay/device/owner acceptance. Mesh is intentionally low cost and not aggressively decimated. |
| Historical-Gothic traveller/fighter, 2026-08-26/30 | Meshy 7 Ultra preview/refine/remesh/rig base character plus retained fitted sleeves and accepted Meshy 7 anatomical gauntlets; 27,775-triangle reusable PBR biped; 24 deform joints plus asset-owned `LeftGrip`/`RightGrip`; Idle/Walking only; shared three-layer 1K PBR arrays | Base-character lineage: rejected preview `01a03c89-37b7-743a-940e-9b2c798152f4`, accepted preview `01a03c8c-1b40-733b-8a8b-0255f1384c22`, refine `01a03c91-1472-765e-9a2f-d15370325975`, remesh `01a03c9d-5296-7994-8fe4-e68ecc77c7ac`, rig `01a03ca2-a736-7808-b554-4f4193ec49f4`; accepted-gauntlet tasks and exact evidence in `assets/models/player/source/meshy-2026-08-30-viewmodel-gauntlet/METADATA.md`; base evidence in `assets/models/player/source/meshy-2026-08-26-gothic-traveller-candidate-2/METADATA.md` | **CC BY 4.0, conservative Meshy Free-plan route.** The active account plan was not independently provable. Attribution: “Historical-Gothic traveller/fighter and viewmodel gauntlets created with Meshy; runtime processing and animation integration by Samfa12/Codex.” | Codex, 2026-08-26/30 | Candidate 2 remains the accepted reusable character foundation. The generated leaf gloves were replaced with a reviewed through-grip gauntlet, mirrored offline for opposite chirality; source fitted sleeves were retained and reweighted across shoulder/elbow/wrist rather than replaced by procedural tube arms. Source/high GLBs and source maps remain Git-LFS evidence and are excluded from packages. Only the bounded runtime GLB, manifests and shared 1K arrays are distributed. The integrated exact `SM-S948B` package/presentation/lifecycle evidence is recorded by the final 2026-08-30 programme validation. Owner review deliberately keeps this skinned path in named development checkpoints while normal gameplay uses block arms until the hands/gauntlets are accepted in every scenario. |
| Historical-Gothic viewmodel gauntlet/grip, 2026-08-30 | Accepted Meshy 7 standard PBR source prompted as right-handed but audited as left-handed; 4,419-triangle stripped 5K remesh; deterministic right-hand mirror; cyan audit hilt removed; integrated into the player runtime | Rejected concept `01a05058-7391-7100-8c4e-f79ba6f26edd`, rejected geometry `01a0505b-29c2-777e-aa45-b1aefff720cf`, rejected retexture `01a0505d-84bf-726b-a791-826e5fdf3359`; accepted concept `01a05063-413f-7030-adb9-45419fbe14ed`, accepted PBR geometry `01a05065-430a-724e-88cc-c569f781d2bd`, accepted remesh `01a0507d-81ee-7506-aa51-2d6246adc1c5`; exact prompts/settings/costs/hashes in `assets/models/player/source/meshy-2026-08-30-viewmodel-gauntlet/METADATA.md` | **CC BY 4.0, conservative Meshy Free-plan route.** The active account plan was not independently provable. Attribution: “Historical-Gothic viewmodel gauntlet created with Meshy; runtime processing by Samfa12/Codex.” | Codex, 2026-08-30 | Exact spend was 83 credits (630 to 547). Candidate 1 was rejected for lacking an auditable handle channel. Candidate 2 preserves distinct curled fingers and a through-grip; its actual anatomy is retained on the left and mirrored for the right. The bright-cyan training hilt and nine-vertex debris were removed deterministically. The source and 2K maps remain Git-LFS evidence and are excluded from packages. |
| Production Gothic reward chest, 2026-08-27 | Meshy 7 Ultra preview/refine/remesh lineage and deterministic rigid Blender 5.2 runtime reauthor; 4,088 triangles total; two materials; exact `ChestBase`, `ChestLid`, `Latch`, `RewardLanternHingeSocket`, `ChestLidHinge`; shared nine-layer 1K PBR arrays | Preview `01a03f69-2b15-7c6b-b17f-de4741cdafba`; refine `01a03f6c-ba91-7f43-a42a-10d2f09f9f09`; remesh `01a03f74-72ac-7e62-8b58-cd906ddc758a`; exact prompts/settings/hashes in `assets/models/props/meshy/production-gothic-chest-2026-08-27/METADATA.md` | **CC BY 4.0, conservative Meshy Free-plan route.** The active account plan was not independently provable. Attribution: “Production Gothic reward chest created with Meshy; runtime processing by Samfa12/Codex.” | Codex, 2026-08-27 | The first and only geometry candidate was accepted. Its refined PBR had no baked illumination. The remesh preserved the broad silhouette but collapsed ornament and was rejected for direct runtime use. The bounded base/lid runtime GLBs use independent rigid transforms; the lid rotates about its authored rear hinge and the semantic reward hinge target composes the 0.90-scale lantern above the floor-contact chest without base/lid intersection. Source/high files are Git-LFS evidence and excluded from packages. |
| Icon-faithful production Gothic reward lantern, 2026-08-27 | Meshy 7 Ultra preview/refine/remesh lineage and deterministic Blender 5.2 runtime reauthor; 6,048 triangles total; three materials; exact `GripRing`, `Hinge`, `LanternBody`, `LanternGlass`, `Flame`, `Light`, `FlameCore`; shared nine-layer 1K PBR arrays | Rejected preview `01a03f69-327a-7c6d-ac8d-ccdbb9fe13d7`; accepted preview `01a03f6b-e99a-7ee2-9738-d6d236f89c2b`; refine `01a03f70-0499-7047-976b-daef45aa7684`; remesh `01a03f74-7501-7164-8113-53ba917fc66d`; owner-package icon SHA-256 `bc4b237247982a0a1953c403459dc42f8ec579f7b093ec150e78d42ae07639dc`, independently recalculated from the supplied ZIP during the final 2026-08-30 audit; exact prompts/settings/hashes are in `assets/models/props/meshy/production-reward-lantern-2026-08-27/METADATA.md` | **CC BY 4.0, conservative Meshy Free-plan route.** The active account plan was not independently provable. Attribution: “Production Gothic reward lantern created with Meshy; runtime processing by Samfa12/Codex.” | Codex, 2026-08-27 | Candidate 1 was rejected before texturing for a modern boxy camping silhouette, weak tracery and fused opaque/yellow panels. Candidate 2 supplied the accepted compact pointed silhouette; its source central bulb/flame-like mass and pane fill were removed. The 9K remesh collapsed tracery and was rejected for direct runtime use. Runtime `LanternGlass` is six locally authored closed 7 mm outward components with KHR transmission 0.94, IOR 1.52, volume thickness 1.0 and attenuation distance 1.8 m. Amber contents are engine-controlled, not baked. Source/high files are Git-LFS evidence and excluded from packages. |
| Generic closed dielectric fixture | Eight-vertex, twelve-triangle closed/manifold runtime GLB with authored KHR transmission, volume and IOR | Deterministically generated in-repository by `tools/generate-dielectric-fixture.py`; no external source | Project-created test geometry; no third-party licence or attribution requirement | Codex, 2026-08-26 | Small imported static-PBR fixture for the reusable bounded dielectric transport and RT Lab controls. Positive closed/manifold and negative open/non-manifold validation fixtures are retained under `tests/fixtures/dielectric-topology`. |
| Stylized skeleton derivative, merged animations v01 | Hotstrike Studio base mesh, subsequently textured, rigged, and animated with Meshy; skinned GLB with 11 animation clips and embedded 4K texture | Original: Hotstrike Studio, https://hotstrikestudio.itch.io/free-stylized-skeleton. User-provided Meshy-processed archive: `Meshy_AI_SKM_Skeleton_Var_1_biped.zip`; see `assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.METADATA.md` | Hotstrike Studio asset licence permits use and modification in free or paid finished games and other media, but prohibits standalone resale/redistribution and asset-pack inclusion. Meshy processing is credited as **Meshy**; this release applies the Meshy Free-plan CC BY 4.0 attribution requirement conservatively, so it is safe whether the processing occurred on a free or paid plan. | Original by Hotstrike Studio; Meshy-assisted derivative supplied by user; runtime integration by Codex, 2026-07-11 | Credit: “Original stylized skeleton by Hotstrike Studio; texture, rig, and animation processing created with Meshy.” It owns the bounded two-skeleton opening encounter and uses `Idle_5`, `Walking`, `Attack`, and `Dead` through CPU skinning and dynamic RT BLAS refit with at most two pose buckets; the finale uses the separate singular lich GLB. The current RT proof uses a procedural bone material; the embedded 4K texture is retained but not sampled. Finished-game packaging is permitted; the raw derivative is also present in current public Git/LFS history. Permission to retain it in public source was requested from Hotstrike Studio on 2026-07-16 at https://itch.io/post/16578566; the request is pending and remains an explicit redistribution-permission/history-remediation gate. |
| Lich placeholder, merged animations v01 | Active Meshy-generated skinned GLB placeholder; 9,188 triangles, nine clips, embedded 2K base-colour plus a falsely duplicated emissive image; deterministic raw-KTX2/ASTC derivatives and selective violet emissive mask | User-supplied `Meshy_AI_Meshy_Merged_Animations.glb`; see `assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.METADATA.md` | **CC0.** The user's Meshy workspace shows this exact lich asset with `Change License: CC0`; retained evidence: `assets/models/enemies/meshy/lich_placeholder_source_licence.png`, SHA-256 `6094E4D9A27A25022A1426C297F069DB60F779CC77526CFE6B154421F6DB96EE`. | User-supplied Meshy output; audited by Codex, 2026-07-15/16 | Runtime final-room placeholder. Robe, body, and staff are fused into one standard biped-skinned primitive with no staff or cloth bones; visible staff/robe deformation is a known source-art limitation. The two embedded image payloads decode identically, so `tools/prepare-lich-textures.ps1` derives a selective violet staff/eye/gem mask. Forty UV-audited emissive staff vertices drive the moving staff-light sample through their real skin weights. Proposed credit: “Placeholder lich character created and animated with Meshy.” |
| Medieval Wall 02 | 1K diffuse, OpenGL normal, packed ARM; lossless RGBA PNG derivatives for static masonry | https://polyhaven.com/a/medieval_wall_02; Rob Tuytel | CC0 | Codex, 2026-07-12; collapsed-entry reuse, 2026-10-03 | Dry corridor stone, dungeon runtime array layer 0 unchanged. Approved revision-two collapsed-entry masonry reuses the same admitted source family through static prop atlas layer 11; the new PNGs preserve decoded JPEG pixels and ARM channels without retouching or resizing. |
| Boulder 01 collapsed-entry rock derivative | Original glTF/bin plus 1K diffuse, OpenGL normal and packed ARM; authored closed fractured geometry and lossless RGBA PNG derivatives | https://polyhaven.com/a/boulder_01; Rico Cilliers. Licence: https://polyhaven.com/license | CC0 1.0 | Poly Haven original; bounded Blender derivative/export by Codex, 2026-10-03 | Approved revision-two collapse uses shared static PBR atlas layer 10 (`Boulder01Rock`) with ordinary hardware-RT geometry. Original glTF/bin/JPEG maps, download receipt and CC0 snapshot are preserved in `assets/models/world/source/collapsed-entry/polyhaven-original`; exact source/PNG hashes and channel semantics are in `assets/textures/props/source/collapsed-entry/input-receipt.json`. Source assets are excluded from game packages. Native Runtime/owner visual acceptance remains separate from this licence admission. |
| Cobblestone Floor 08 | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/cobblestone_floor_08; Rob Tuytel | CC0 | Codex, 2026-07-12 | Wet stone floor base, runtime array layer 1. |
| Mossy Stone Wall | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/mossy_stone_wall; Amal Kumar | CC0 | Codex, 2026-07-12 | Mossed masonry, runtime array layer 2. |
| Damp Sand | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/damp_sand; eye-candy.xyz | CC0 | Codex, 2026-07-12 | Puddle-edge/damp-ground blend, runtime array layer 3. |
| Rusty Metal 04 | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/rusty_metal_04; Amal Kumar | CC0 | Codex, 2026-07-12 | Aged metal, runtime array layer 4. |
| FilmCow Recorded SFX showcase subset | Seventeen short UI/combat/movement WAV clips: `clicky button 1`, `button pressed 1`, `metal button 1`, `woosh 1`, `woosh 3`, `fencing hit 1`, `fencing hit 2`, `body fall 1`, `footstep dirt 9`, `footstep dirt 14`, `footstep dirt 17`, `footstep dirt 18`, `harpoon rattle`, `air duster 5`, `anvil hit 1`, `body fall with lots of bass 4`, and `body hit with grunt 3` | Local Possum Cafe archive copy of FilmCow Recorded SFX; official source and licence: https://filmcow.itch.io/filmcow-sfx | FilmCow custom royalty-free licence. Personal and commercial project use is permitted without required credit; use is prohibited for national-government projects, law-enforcement projects, and groups categorized as hate groups by SPLC or CAHN. This is not CC0. | Codex, 2026-07-16 | Imported by `tools/import-filmcow-sfx.ps1` and converted from mono 48 kHz 24-bit PCM to mono 48 kHz 16-bit PCM without changing duration. The four dirt-step sources are deterministically peak-normalized to 0.78 and the short lich hurt reaction to 0.84 during conversion; other cues preserve source amplitude. Packaged files are under `assets/audio/filmcow/`; the complete source archive is not redistributed. Voluntary credit: FilmCow Royalty Free Sound Effects Library — FilmCow. |
| FilmCow equipment cue subset | Two selected game-ready derivatives: `dagger unsheath 1.wav` and `dagger sheath 1.wav` | Owner-supplied local FilmCow archive; official source and licence: https://filmcow.itch.io/filmcow-sfx | FilmCow custom royalty-free licence, same personal/commercial project permissions and three prohibited-use categories as the showcase subset above; not CC0. Official terms rechecked 2026-10-07 after owner supplied the page. | FilmCow; deterministic runtime processing by Samfa12/Codex, 2026-10-07 | `tools/import-filmcow-sfx.ps1` converts PCM24 to mono 48 kHz PCM16, preserving duration, pitch and amplitude. Only `assets/audio/filmcow/equipment/sword_draw.wav`, `sword_sheath.wav` and their `asset.manifest.json` are packaged; source hashes and derivative hashes are in that manifest. One-shot audio follows the shared attachment edge, not repeated input or draw-start. In-game owner listening/volume acceptance remains pending; complete bank stays private. |
| Menu ambience derivatives | Filtered FilmCow `ambience - air conditioner.wav` and an owner-supplied [Metal Creaks by Irhouen](https://pixabay.com/sound-effects/film-special-effects-metal-creaks-189729/) excerpt | FilmCow project-use terms above; [Pixabay Content License](https://pixabay.com/service/license-summary/) for Irhouen, checked 8 October 2026. Adapted in-game use permitted; no standalone source/audio-library redistribution. | FilmCow / Irhouen; bounded processing and integration by Codex | Only `assets/audio/menu/{menu_room.wav,menu_chain.wav,asset.manifest.json}` ships. Four-second mono PCM16 room loop and one-second edge-faded metal-creak event (source 4.1–5.1 s); hashes, filters and levels in manifest, reproducible with `tools/prepare-menu-ambience.py`. Owner saved Room 7%/Chain 21% on 7e6662c6 at SFX 70%; the rejected gas-hiss flame candidate is removed entirely. Room is filtered indoor-air foley; final package and Windows listening remain separate gates. The supplied chain-rustling alternative is not included. The earlier Hammy01 chain is replaced at the owner’s request. Replacement character/level listening remains pending. Voluntary credit: Metal Creaks by Irhouen via Pixabay. |
| Water Dripping 364450 positional waterfall loop | 21.323-second mono 48 kHz 16-bit PCM WAV derivative with a 0.75-second cyclic crossfade | “Water Dripping” by DRAGON-STUDIO, https://pixabay.com/sound-effects/nature-water-dripping-364450/; see `assets/audio/pixabay/waterfall_loop.METADATA.md` | Pixabay Content License. Free use, adaptation, and commercial use are permitted without required attribution; standalone distribution in substantially the same form is prohibited. Licence summary: https://pixabay.com/service/license-summary/ | DRAGON-STUDIO; runtime processing and integration by Codex, 2026-08-23 | The user supplied the source MP3. The derivative is converted to mono 48 kHz PCM16, raised 4.5 dB, and crossfaded cyclically for seamless in-game looping. It is attenuated and stereo-panned from the waterfall's world position. Voluntary credit: “Water Dripping by DRAGON-STUDIO via Pixabay.” |
| Wooden Trunk Latch 1 183944 reward-chest unlock | 0.384-second mono 48 kHz 16-bit PCM WAV runtime derivative | “Wooden Trunk Latch 1” by floraphonic, https://pixabay.com/sound-effects/film-special-effects-wooden-trunk-latch-1-183944/; see `assets/audio/pixabay/chest_unlock.METADATA.md` | Pixabay Content License. Free use, adaptation, and commercial use are permitted without required attribution; standalone distribution in substantially the same form is prohibited. Licence summary: https://pixabay.com/service/license-summary/ | floraphonic; runtime processing and integration by Codex, 2026-08-28 | The Pixabay MP3 is deterministically decoded, mixed to mono, and encoded as 48 kHz PCM16 without changing duration. Only `assets/audio/pixabay/chest_unlock.wav` is packaged. Voluntary credit: “Wooden Trunk Latch 1 by floraphonic via Pixabay.” |
| Chest Opening 87569 reward-chest creak | 4.992-second mono 48 kHz 16-bit PCM WAV runtime derivative | “Chest Opening” by spookymodem (Freesound), distributed on Pixabay by freesound_community, https://pixabay.com/sound-effects/household-chest-opening-87569/; original Freesound item 202092: https://freesound.org/people/spookymodem/sounds/202092/; see `assets/audio/pixabay/chest_open.METADATA.md` | Pixabay Content License for the acquired Pixabay MP3; the identified original Freesound WAV is CC0. Free use, adaptation, and commercial use are permitted without required attribution; standalone distribution of the Pixabay content in substantially the same form is prohibited. Licence summary: https://pixabay.com/service/license-summary/ | spookymodem; Pixabay distribution by freesound_community; runtime processing and integration by Codex, 2026-08-28 | The Pixabay MP3 is deterministically decoded, mixed to mono, resampled from 24 kHz to 48 kHz, and encoded as PCM16 without changing duration. Only `assets/audio/pixabay/chest_open.wav` is packaged. Voluntary credit: “Chest Opening by spookymodem (Freesound), via Pixabay.” |
| Fire extinguishing 212651 torch-failure cue | 1.044-second mono 48 kHz 16-bit PCM WAV runtime derivative | “Fire extinguishing” by MUSICHOLDER, https://pixabay.com/sound-effects/film-special-effects-fire-extinguishing-212651/; see `assets/audio/pixabay/torch_extinguish.METADATA.md` | Pixabay Content License. Free use, adaptation, and commercial use are permitted without required attribution; standalone distribution in substantially the same form is prohibited. Licence summary: https://pixabay.com/service/license-summary/ | MUSICHOLDER; runtime processing and integration by Codex, 2026-08-28 | The user-supplied Pixabay MP3 is deterministically decoded, mixed to mono, resampled from 32 kHz to 48 kHz, and encoded as PCM16 without changing duration. Only `assets/audio/pixabay/torch_extinguish.wav` is packaged. Voluntary credit: “Fire extinguishing by MUSICHOLDER via Pixabay.” |
| Horde Lantern RT application icon | 1024 px source PNG, Windows ICO, and Android launcher PNG derivatives | Generated specifically for this project with OpenAI image generation; prompt recorded in the 2026-07-15 release-prep work | Project-created generated asset; no third-party source asset imported | Codex, 2026-07-15 | Gothic iron lantern and amber hardware-ray motif. Source and derivatives are under `assets/branding/` and Android `mipmap-*`. |

Anatomy correction, 2026-09-23: the gauntlet row's left-source/right-mirror description records the historical export, not the corrected source classification. The owner identified the source as **Right**; current investigation candidates reflect that source for Left/torch and preserve it for Right/sword, with corresponding winding and UV-corner reversal. The historical classification is withdrawn. Accepted runtime files remain unchanged pending visual and device acceptance. See [hand-orientation evidence](docs/ENGINEERING_1_6_1_HAND_ORIENTATION_2026-09-23.md). This corrects geometry provenance only; licences, attribution, source bytes and distribution permissions are unchanged.

Android runtime derivatives for the five CC0 rows are strict KTX2 arrays using ASTC 6x6 for diffuse/AO-roughness-metal and ASTC 4x4 for normals. The retained raw RGBA arrays and original 1K JPGs remain the provenance/source chain; layer order is unchanged.

Poly Haven's asset license states that its assets are CC0 and may be used commercially without required attribution: https://polyhaven.com/license. Attribution is retained here as project provenance.

## Outdoor score (1.7 sources and full main PCM)

### Beneath the Same Moon — Outdoor Exploration r2 (1.7 source admission)

Owner-supplied original arrangement and audio, approved for Horde outdoor music
work and handed off through the owner's Drive folder on 2026-10-23. **Authorised
for Horde use only**; no general redistribution licence, Core software licence
change or release authority is inferred. Exact Drive IDs, byte hashes, original
filenames and ZIP member hashes are recorded under
`assets/source/beyond_the_tomb/music/beneath-the-same-moon-outdoor-r2/`.
The two original stereo 48 kHz PCM24 FLACs, original editable source ZIP and
verbatim JSON/PCS1 scores remain source assets, excluded from game packages.
The source pack preserves the earlier forest-theme provenance and its pinned
Pocket Chordsmith rendering/finishing recipes; imported scripts were not run.

The main-only runtime derivative is pinned by
`assets/audio/music/beneath-the-same-moon/asset.manifest.json`: stereo 48 kHz PCM16,
5,120,000 frames / 106.666667 seconds, WAV SHA-256
`14fe0a94e665cf25b4fb4aa08a45e26e18990794740b670906adfecfaf6aee15`.
It preserves the full approved main without gain, trimming, normalization,
sample-rate or tempo change. The quiet mix is neither loaded nor packaged;
existing runtime dialogue ducking and user volume remain independent.

The reviewed canonical native Core update
`b77a9d0bfe3051c6e352815295164b9b80586f56` supplies explicit bounded long-PCM
support while retaining legacy defaults; Horde vendors the exact subset with
unchanged private/UNLICENSED notices. No A-H asset or legacy dawn cue is replaced.
This source/runtime admission does not establish listening with Kit, phone
performance or release acceptance. See the
[source verification](docs/evidence/2026-10-23-outdoor-music-source/README.md) and
[main integration checkpoint](docs/evidence/2026-10-23-outdoor-main/README.md).

## Supplied adaptive score (1.6.1 runtime assets and playback accepted)

`What the Dark Keeps`, supplied in `What_the_Dark_Keeps_Horde_RT_Music_Pack.zip`
SHA-256 `e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa`.
Owner-confirmed rights statement,2026-09-30: **Owner-supplied; authorised for Horde use only**.
No general permissive redistribution licence is inferred. Editable canonical JSON/
PCS1 under `assets/audio/music/what-the-dark-keeps/source/` remains the revisable
source of truth and is excluded from game packages, as is the supplied preview.
Only the hash-pinned manifest and sixteen measured stereo 48 kHz PCM16 body/tail
derivatives under that asset's `runtime/` enter Windows/Android packages.
Current native-clock/loop/transition and owner listening checks are accepted;
see the [finite current A/D delta receipt](docs/evidence/2026-10-03-music-drone/README.md).
Asset admission alone is not listening acceptance. This grant does not relicense Pocket Chordsmith/Pocket Audio source,
change existing asset licence statements or authorise production publication.
See [music integration checkpoint](docs/ENGINEERING_1_6_1_MUSIC_2026-09-30.md).
The sixteen A-H PCM body/tail prototypes under
`docs/evidence/2026-10-01-music-render/audio/` carry that same Horde-only owner grant.
They are non-runtime rendering evidence, excluded from game packages, not a
general asset library or seamless/playback acceptance. Source/tool hashes and
original unmodified-score processing are recorded in that directory. Initial
runtime copies were byte-identical; historical `-loop` filenames became
`-body` to distinguish the C/G one-shots. The separate
native-only Pocket Audio Core utility pin is documented in
`third_party/pocket-audio-core/README.horde.md`; no Chordsmith editor/synth app is
vendored and no upstream code is relicensed. Its software permission is separate
from this Horde-only score grant.

Owner-directed instrumentation studies under
`docs/evidence/2026-10-01-music-instrumentation/` carry the same **Horde-only**
score grant. They retain separate editable JSON/PCS1 candidates and actual v68
app-voice/live-FX derivatives; comparison gain changes apply only to previews.
They are excluded from runtime packages and are not a bank-replacement,
general acoustic-instrument library, listening or publication acceptance.

On October 1 the owner selected the whistle-lead instrumentation. The canonical
JSON/PCS1 and sixteen runtime derivatives now carry that palette: retained v68
actual app voices/live FX, unchanged notes/cue timing, accepted A/E PCM reused,
six remaining cues rendered once. Sparse authored bell accents remain in C/G.
No preview normalization is applied to runtime. Rights remain **Horde-only**;
that bank's historical receipt alone is not public distribution or later exact-
artifact acceptance. Subsequent owner listening and scoped A/D-only held-drone
removal are recorded separately in the current receipt above. See
`docs/evidence/2026-10-01-music-whistle-bank/README.md` for exact provenance.

## Windows report-verification SDK (1.6.1 development)

Microsoft.Web.WebView2 **1.0.4258.31**, official NuGet package, BSD-3-Clause.
Native headers/static loader only; exact archive/entry hashes and source URL in
`third_party/webview2-sdk/manifest.json`. The SDK is explicitly restored into
ignored build storage, not vendored source. Windows uses the separately installed
Evergreen WebView2 Runtime for consented anti-spam verification only; no bundled
fixed Runtime, editor/framework migration or automatic Runtime installation.
Missing Runtime leaves clear diagnostics and offline JSON fallback.
SDK licence (reproduced for the statically linked loader's binary distribution):

Copyright (C) Microsoft Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
   * The name of Microsoft Corporation, or the names of its contributors
may not be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## cgltf glTF loader

The bundled loader in `third_party/cgltf/` is cgltf by Johannes Kuhlmann,
MIT licence. The complete copyright, permission notice and disclaimer from
`third_party/cgltf/LICENSE` are distributed byte-for-byte as
`THIRD_PARTY_NOTICES/cgltf-LICENSE.txt` in Windows packages and
`assets/THIRD_PARTY_NOTICES/cgltf-LICENSE.txt` in Android APKs. Both in-app
credits identify the loader and the notice location. Package admission verifies
the complete notice bytes; this corrects future candidate packaging and does
not alter frozen 1.6.1 artifacts.

## 1.6.2 keeper and skeleton runtime cues

The owner supplied four local Pixabay MP3s. Their original bytes remain local,
outside public Git and distributed packages. The integrated game uses only the
mono PCM16/48k runtime derivatives listed with exact source/runtime hashes,
frames, edge trims, endpoint ramps and event mappings in
`assets/audio/pixabay/keeper-asset.manifest.json`. These are under the
[Pixabay Content License](https://pixabay.com/service/license-summary/) and
[full terms](https://pixabay.com/service/terms/), verified3October2026; they are
not represented as CC0. Attribution is voluntary. Do not redistribute these
files as a standalone audio library.

- Falling Bones and Rattling Bones by spookymodem (Freesound), uploaded through
  freesound_community, via Pixabay. Runtime: `skeleton_falling_bones.wav` and
  `skeleton_idle_rattle.wav`.
- Lich Demonic Voice - I Sense You! and Come Closer! by PhatPhrogStudio, via
  Pixabay. Runtime: `keeper_i_sense_you.wav` and `keeper_come_closer.wav`.

`tools/prepare-keeper-audio.py` preserves source hashes, converts format, trims
only quiet edge windows with a25ms reserve and adds short endpoint ramps. The
idle excerpt is bounded to1.25s. No gain normalization or music instrumentation
change is applied. Exact-candidate owner listening remains pending.

## 1.6.2 native night environment and authored shaft foliage

The night-storm panorama is an existing project-owned generated Briarhold asset,
created9August2026 with OpenAI built-in image generation; no new generation or
paid service was used. Its original provenance and source PNG are retained in
`assets/textures/environment/source/`. Source SHA-256:
`83c297f7373e52feee50a881d5291f8e99faab8556fbce4c041c4b63b497dd8b`.
The inspected1024x512 WebP intermediary hash is
`58dc7c09547d7ed560c75d97038556b908b50ffecd12873601b5ca9bfc814824`.

`tools/prepare-horde-environment.py` produces mipmapped512x256 RGBA8-sRGB Windows
and ASTC6x6-sRGB Android KTX2 files. Exact derivative hashes/formats/bytes are in
`assets/textures/environment/runtime/asset.manifest.json`. Package only runtime
tiers and their provenance, never source art. This is direction-based native RT
miss/reflection radiance, not baked scene illumination.

The bounded hanging sprigs are project-authored opaque leaf/stem geometry,
reusing the admitted moss material through generic material tint/normal controls.
There is no new third-party foliage texture, alpha card or generation licence.

## 1.6.2 collapse reference study (not yet runtime-admitted)

[Poly Haven Boulder01](https://polyhaven.com/a/boulder_01) by Rico Cilliers is
[CC0](https://polyhaven.com/license), verified3October2026. The owner-selected
1K glTF source and dependencies were downloaded and matched upstream byte/MD5
metadata; SHA-256 receipts are retained in the isolated source-art workspace.
The downloaded mesh has66122triangles, independently counted in Blender5.2;
the source page's124K claim is not this download's topology. An unmodified source
copy is preserved. A2200triangle welded/decimated study and project-authored
broken dressed stone/lintel appear in `docs/design/1.6.2-collapse/` for the
required owner layout/reference review. Final Form/runtime export is pending;
these studies are excluded from shipping assets.

## 1.6.2 Android Core-compatible waterfall derivative

The existing admitted Pixabay waterfall source remains unchanged. The Windows
runtime retains its full21.323479s mono loop. Android's canonical native PCM Core
accepts stereo48k PCM16 bodies of at most12s; `tools/prepare-core-waterfall-loop.py`
produces an11.5s dual-mono derivative without modifying Core or its pinned code.
A750ms head crossfade uses the real continuation after the selected body to join
the wrap without an added discontinuity. No silence, gain normalization or
resampling is introduced. Exact source/derivative hashes, frames, processing and
the shorter repetition tradeoff are recorded in
`assets/audio/pixabay/waterfall-core.manifest.json`.

The existing Pixabay licence/provenance applies to this integrated derivative;
it is excluded from Windows runtime packaging. Exact-candidate repeated-loop
listening and Android output/lifecycle acceptance remain pending. The change is
not evidence that the former MediaPlayer issue was reproduced or diagnosed.

### Original development rescue blockout (1.7 journey)

The later `assets/source/development-rescue-ground-stake-v1/` is a separate
original Blender master/GLB round-trip. It replaces the development overhead
frame with a short buried iron stake and retains the accepted shaft, lid,
coping and apron interfaces. The stake reuses existing Poly Haven CC0
Rusty Metal 04 (world layer 4); exact source-map hashes are in its source
manifest. The original rescue master and backups remain preserved. Neither
editable source directory belongs in runtime packages. No new texture family,
generation service or third-party geometry was acquired.

`assets/source/development-rescue/` contains original shaft, coping, lid and landing geometry authored locally from `src/scene/RescueBlockoutGeometry.h` through installed Blender 5.2. No third-party geometry was acquired. Editable Blender and GLB roundtrip sources remain outside runtime packaging; the renderer consumes the verified shared geometry recipe. Existing Poly Haven CC0 DryStone (world layer 0) and MossyStone (layer 2) maps retain their original attribution and exact byte identities in the source manifest. Native import and source admission are engineering checks, not artistic/device acceptance or new redistribution rights. The temporary outdoor skeleton reuses the already-admitted runtime model and its existing licence entry; it is a render workload witness, not Kit or a new enemy roster entry. Outstanding Hotstrike/Meshy public redistribution questions remain unresolved.

### Original development wooded terrain recipe (11 October 2026)

`DevelopmentWorldRoute.h`, `DevelopmentWorldGeometry.h` and `RescueJourneyGeometry.h`
retain the editable original terrain, closed banks and opaque tree silhouette
recipes. The native world upload consumes those recipes directly. The owner-supplied
original pine/alder pair below provides the bounded LOD1 forest dressing; the former
silhouette recipe remains a separate diagnostic geometry path. Kit's thirteen
exact approved WAV cuts were subsequently admitted after the owner confirmed
public repository and game-package distribution rights; see the dedicated Kit
audio entry below. This does not change the asset-specific licence notices or
establish physical playback/performance acceptance.

- **Original irregular pine and upright alder tree pair v1**: owner-authored editable
  Blender sources, original 512px PBR maps, LOD0/LOD1 GLB exports, source receipts,
  and LOD1 runtime GLBs. Supplied in `Horde-1.7-tree-pair-compact-editable.zip`; the
  exact package SHA-256 and preserved closed source roster are in
  `assets/models/world/source/forest-tree-pair-v1/`. The owner confirms these assets
  are approved for game integration and public PR. The package assigns no blanket
  public/open-source license; this entry claims no MIT, CC0, Meshy, or other
  third-party rights. The runtime admits only `horde-irregular-pine-v1-lod1.glb`
  (5,000 triangles) and `horde-upright-alder-v1-lod1.glb` (5,102 triangles), at
  authored scale 1.0. Exact bytes, hashes, bounds and source links are in
  `assets/models/world/runtime/forest-tree-pair-v1/runtime-roster.json`. Original
  bark normal/roughness and foliage/base maps remain 512px; production array inputs
  use exact nearest-neighbour 2x texel replication, with bark roughness packed as
  ORM G, and original source PNGs remain unchanged. Four base-colour families append
  at layers 14–17; mapped bark alone appends normal/ORM layer 14. The 18/15/15
  layer-count manifest and full-mip Windows RGBA8/Android ASTC outputs record exact
  hashes. Source masters and rights notices stay preserved; the repository license
  does not assign a license to these assets.
  The separate owned `forest-crown-study-r1/` source-only derivatives preserve this
  provenance and introduce no third-party geometry/textures. They are unadmitted
  study files, excluded from runtime selection and shipping; no broader licence
  or production replacement is asserted.


### 1.7 development tomb native candidates (2026-10-10)

- **T02 skull/jaw, femur and humerus**: recovered from the public immutable source archive at `93bdbd54016de6c4c7381a29b909d4049e4bf528`, `assets/source/beyond_the_tomb/funerary/t02-skeletal-source-v1`. Gord Goodwin's Simple Rigged Skeleton is **CC0-1.0**; retained source credit, licence notice, source-page check and checksum rosters accompany the recovered kit. Three selected GLBs have a runtime-only UV0 addition for the native importer; positions, topology, authored normals and source masters remain unchanged. The exact recipe and source/runtime hashes are in `models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/derivation-receipt.json`. Coarse alternatives remain source candidates, not runtime LOD admission. This is static burial dressing, not reuse of the animated enemy mesh.
- **T03 seven funerary props**: recovered original project Blender kit at immutable planning pin `d37f16381311c6e620f0c4a9d01c1cce83260dc8`, `assets/source/beyond_the_tomb/funerary/t03-v01`. Seven selected runtime GLBs are byte-exact copies (5,460 source triangles) with no imported texture bytes. Editable Blender files, recipes, authored-source provenance and source checksum rosters are retained separately. Original project geometry introduces no third-party model licence; no exclusive copyright or legal warranty is claimed. Exact-copy runtime receipt: `models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/derivation-receipt.json`.
- **Two original recessed niche modules**: deterministic headless Blender geometry authored specifically for Horde, using the already admitted masonry binding. Editable sources and derivation receipts remain outside runtime packages. They add no third-party texture/model bytes. Earlier unselected replacement skull/bone/funerary experiments are not admitted or packaged.
- **Wet contact cues**: four short Windows and Android PCM step variants use the owner-supplied “Footsteps Water 01” by aglinder. The provided MP3 is distributed on [Pixabay](https://pixabay.com/sound-effects/footsteps-water-01-73731/) under the **Pixabay Content License**; the identified [original Freesound recording](https://freesound.org/people/aglinder/sounds/265582/) states **CC0 1.0**. The supplied MP3 hash is `70f548d544985d6fa7e14463684d3792c51d87da5a987b631d51f3e589f401a9`; it stays outside the repository and packages. Four separated transient onsets are used for fixed 0.38-second cuts, fades, measured level matching and dual-mono Android conversion. The game-ready cues are combined runtime media, not standalone audio distribution. Pixabay terms permit commercial and non-commercial use and adaptation subject to prohibited uses, including standalone distribution; attribution is voluntary. `assets/audio/pixabay/water-contact.manifest.json` records source identity, cut/fade/gain/channel recipe, measured levels and runtime hashes. Owner listening approval for the revised mix remains pending.

- **Keeper death scream**: the owner-supplied “Monster Demon Voice - Death / Defeat Scream” by PhatPhrogStudio, [Pixabay item 582531](https://pixabay.com/sound-effects/film-special-effects-monster-demon-voice-death-defeat-scream-582531/), published 17 August 2026, is used under the [Pixabay Content License terms](https://pixabay.com/service/terms/), checked 10 October 2026. Adapted commercial/non-commercial game use is permitted; standalone redistribution is prohibited. This audio is not MIT-licensed. The original MP3 remains outside the public repository. Only the game-integrated mono/dual-mono PCM derivatives and provenance/processing manifest are included. Full source/runtime hashes, endpoint fades and measured levels are in `assets/audio/pixabay/keeper-death.manifest.json`; `tools/prepare-keeper-death-audio.py` reproduces them from an independently supplied source. Voluntary credit: PhatPhrogStudio via Pixabay. Exact-candidate owner listening and Android device acceptance remain pending.

The repository code licence does not override these individual asset terms. Native import and exact package hashes do not establish artistic, RT presentation or sustained-performance acceptance. Library locators identify the recovered prepared packages; they do not by themselves admit runtime use. The later-outdoor Meshy stump is excluded from this tomb slice. No private Briarhold assets or Kit recordings are transferred.

### Approved Kit English runtime cuts (11 October 2026)

The owner explicitly confirmed: **“I confirm public game/repository distribution
rights”** for the thirteen previously approved Mureka Kit cuts. This supersedes
the pending audio-rights disposition at chapter checkpoint `24eb2587`; it does
not change the separate visual-actor or unrelated asset rights. The evidence
class is owner-confirmed permission, not a newly obtained provider legal opinion.

Only the thirteen clean game-integrated PCM cuts are admitted under
`assets/audio/kit/runtime/`, with a closed source/runtime hash roster and measured
clip duration in `asset.manifest.json`. `tools/prepare-kit-audio.py` verifies the
private approved-cut package and copies byte-exact mono PCM16 24 kHz cuts without
normalization, resampling or additional mask/grate treatment. The master audio,
provider manifest/account details and prototype archives remain private and
unchanged. Runtime redistribution is covered by the owner's confirmed grant;
these audio files are not blanket MIT-licensed by the repository code licence.
No new generation, acquisition or spending occurred. Exact-candidate positional
listening, Voice/SFX balance and Android playback/lifecycle acceptance remain
pending. The visible Kit model remains unadmitted.


### Kit visual source intake (11 October 2026)

The owner expressly confirmed that public Horde repository and game-package
redistribution of the exact Meshy Warden model is covered. The copied source
`assets/models/characters/source/kit-warden-v1/briarhold-warden-1k.glb` has SHA-256
`47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e`.
This records the owner's entitlement confirmation, not an independent account
inspection or an MIT/CC0 claim. Credit: Warden/Kit source created with Meshy;
source supplied by Samfa12. The original private-project file is unchanged.
Native clip/skin intake is separate from runtime admission: no visible Kit,
Kit renderer/resource handoff is shipped by this intake. The intended non-overlapping Keeper/Kit lifecycle may reuse the dedicated character slot; that is a subsequent admission task.

### Kit same-rig step-turn authoring proof (23 October 2026)

The separate `assets/models/characters/source/kit-step-turn-proof-v1/` source-only
study contains offline-authored joint animation on the same owner-approved Warden
rig. Original geometry, weights and imported bind transforms are retained; motion
authoring is by Samfa12/Codex using Blender. The source above and its entitlement
remain the provenance; no new provider generation or blanket MIT/CC0 claim.
This editable turn study and its studio previews are unadmitted and excluded from
runtime packages. See `docs/evidence/2026-10-23-kit-step-turn-proof/README.md`.

### Kit same-rig turn runtime promotion (24 October 2026)

`assets/models/characters/runtime/kit-warden-v1/kit-warden-turn.runtime.glb`
is a byte-identical promotion of the owned native export
`assets/models/characters/source/kit-step-turn-proof-v1/native/kit-turn-proof.glb`:
2,272,652 bytes, SHA-256
`a72091cac1768f66353178247bde804af7e4b41dd482ccaad813215131583c11`.
It retains the original seven animations, mesh, materials, skin/bind hierarchy,
accessors and original binary payload, appending only the two offline-authored
joint-only turn takes. The shared runtime applies the exported yaw separately
and controls playback timing; no root yaw is embedded in those joint clips.

The existing owner-confirmed Meshy Warden entitlement and Samfa12/Codex motion
authoring provenance above apply. No new provider generation, texture processing,
rig replacement or blanket MIT/CC0 claim occurs. Original source and
`kit-warden-lod0.runtime.glb` remain unchanged in the repository. Development
packages select the promoted derivative once, reusing the same manifest and
platform texture arrays; they exclude the original duplicate, editable study,
previews, root-motion/reference data and promotion receipt. The exact-copy and
append-only checks are recorded in
`assets/models/characters/runtime/kit-warden-v1/turn-runtime-promotion.json`.
This records provenance and package selection, not a public release, physical
phone or full campaign acceptance.

### Chapter terrain and shortened entry source candidates (11 October 2026)

`assets/models/world/source/chapter-entry-return-r03/` preserves original Horde architectural authoring derived from the accepted collapse, reusing the existing Poly Haven Boulder01 rock and Boulder01/MedievalWall02 CC0 maps. `assets/models/world/source/chapter-terrain-r18/` is original Horde Blender authoring from the owner-supplied r2 heightfield, with admitted Poly Haven damp_sand/mossy_stone_wall CC0 materials. Individual texture/source hashes and derivation recipes remain with each candidate. The recovered original planning NPZ is under `assets/source/beyond_the_tomb/terrain/topo-r2/`. These sources and candidate GLBs are outside runtime package paths; native import and source validation do not grant final runtime/visual/performance acceptance. No blanket third-party licence change or new generation occurred.

The separate development runtime `assets/models/world/runtime/chapter-entry-return-v1/` now consumes the byte-identical compact r03 candidate, with a runtime manifest retaining the original per-asset ceilings and both existing atlas families. Legacy collapsed-entry files and all editable sources remain unchanged. See the [native admission and capture receipt](docs/evidence/2026-10-11-chapter-entry-runtime/README.md); terrain r18 remains a source candidate.


## Chapter forest-floor material (11 October 2026)

Forest Ground 01 by Rob Tuytel, [Poly Haven original](https://polyhaven.com/a/forrest_ground_01), is CC0 1.0 under the [asset licence](https://polyhaven.com/license). The owner authorized this source on 11 October. Original 1K PNG diffuse, OpenGL normal and packed AO/roughness/metal maps are preserved under `assets/textures/polyhaven/source/forrest_ground_01-1k-v1/`; provenance.json records provider URLs, sizes, MD5 and local SHA-256. All three provider digests were verified. These are material maps, not scene lighting or baked world shadows.

`chapter-terrain-r19` replaces the unsuitable broad mossy-masonry bank material with this forest-ground family at the published 2 m texture width. Existing damp sand remains on the trail. `tools/build-chapter-terrain-textures.ps1` records the two-layer runtime derivation in `assets/textures/props/source/chapter-terrain-maps-r03/receipt.json`: Windows 1K RGBA8 and Android strict ASTC6x6 base/ORM, ASTC4x4 normal, eleven mips. The original prop arrays are not re-encoded. Previous terrain and texture candidates remain preserved. Runtime distribution includes only the two-layer platform arrays and admitted GLB/manifest, not editable sources.

`chapter-terrain-r21` extends the original Horde landform using the owner-designated connected-region atlas and local topo at planning commit `d37f16381311c6e620f0c4a9d01c1cce83260dc8`. Its source receipt identifies both authoring scripts and the exact recovered layout/grid. The regional mesh reuses the same admitted damp-sand and Forest Ground 01 material images and platform arrays unchanged; no additional texture, external model or licence is introduced. The editable master stays outside runtime packages. Beyond the reviewed macro rectangle, coarse surrounding relief is labelled provisional continuation, not additional approved map content.

`chapter-terrain-material-r01` records a named linear base-colour-factor
adjustment for Forest Ground only. Its receipt preserves the exact r21 model
and original six texture-array hashes; no new provider content or licence is
introduced. The original manifest and reproduction recipe stay in source, and
only the admitted material sidecar accompanies runtime assets.

`chapter-terrain-r22` subdivides only the original Horde regional mesh's coarse
water-intersecting outer triangles and necessary shared edges. Added vertices
sample the unchanged owner-designated heightfield; the exact core, route,
openings, bounds, two embedded material/image payloads and existing platform
arrays are preserved. No external asset or new licence is introduced. Its
compact source receipt pins the recipes and local reproducible master/export;
the admitted runtime GLB is SHA-256
`12c0c577a67c5fc8e88fe0528a9eaa5b2ec950ba409bbffe969560b7c54ee060`.
The existing runtime manifest and Forest Ground material factor remain exact.

`chapter-terrain-r23` adds bounded bank-crest sampling to r22 without changing
the owner atlas, original vertices, grid, route/core or admitted material images.
Its documented authoring exception raises only 42 deficient NEW crest samples
using the original continuous carve profile: 40 capped at 0.50 m and two exact
outflow endcaps capped at 1.25 m. Dry samples are never lowered. No external asset
or licence is introduced. The runtime GLB is SHA-256
`c636a67911b7f5c5381c30ec7a49e5d4ff15c70fe688b368ba52efe77b537774`;
the compact source receipts pin all changes and preserve rejected trials.

### Kit preserved runtime derivative (11 October 2026)

`assets/models/characters/runtime/kit-warden-v1/` is a prepared derivative of the
exact Kit source covered by the owner confirmation above. All non-image GLB
buffer views are byte-preserved; routing thumbnails replace embedded image
payloads, and the reproducible map extraction/compilation keeps the source
masters untouched. Credit: Warden/Kit source created with Meshy; runtime
processing by Samfa12/Codex. The same asset-specific Meshy entitlement applies,
not a blanket MIT/CC0 licence. The development chapter packages the runtime
derivative and exact native maps under `assets/textures/characters/runtime/kit-warden-v1/`.
Editable masters remain separate; see the runtime README and processing receipt.

### Original Horde grave markers (1.7 development)

`assets/models/world/source/grave-markers-r03` contains a new original Blender master and derivation hashes for two plain closed stone markers, authored with `tools/author-grave-markers.py`. Runtime GLBs in `assets/models/world/runtime/grave-markers-r03` contain no third-party asset or embedded texture. Rendering reuses the admitted CC0 MedievalWall02 family; its separate licence remains unchanged. The graveyard also reuses existing T03 covers, bowls, urn and cold candle source/runtime identities and original provenance. Editable sources and failed r01 intake evidence remain outside packages; no flames, lights, loot or new texture layers are admitted.

### Original rescue grave-lid relief (12 October 2026)

`assets/source/development-rescue-lid-finish-v1` preserves the original editable
Blender master and native GLB round trip of `src/scene/RescueLidFinish.h`, authored
with `tools/author-rescue-lid-finish.py`. No external input or provider output is
included. Runtime consumes the same closed stone recipe through the shared
world-surface material transport, without packaging the editable source/GLB or
adding a texture, material row or light. Existing stone texture notices remain
separate; this original geometry does not change their licensing.

### Original supplied forest understory (1.7 development)

`assets/models/world/source/forest-understory-v1/original-package` preserves all
59 manifest payloads of the supplied original Horde forest-dressing kit,
including its seven masters, editable Blender scene, maps, scripts and
`ORIGINAL-AUTHORSHIP.txt`. The two runtime fern GLBs are byte-identical originals
with Horde runtime filenames. Geometry and foliage are project-owned original
work; no third-party model/map is included and no new CC0/MIT declaration is
made. The current owner task authorizes public Horde reuse. Fern albedo matches
the supplied tree-pair map exactly and reuses the admitted ForestAlder family.
Rocks/log/root candidates remain source-only pending separate native derivative,
material, collision and cost checks. No new texture or transparency is admitted.

### Prepared woodland deadwood (12 October2026)

Four separately hashed native tangent derivatives are admitted under
`assets/models/world/runtime/forest-deadwood-v1`: original supplied fallen log
and roots, the owner-created paid-plan Meshy stump (public Horde redistribution
confirmed), and Quaternius CC0 DeadTree_1's prepared coarse LOD2 snag. Original
stump/snag archives, notices, maps and editable sources are byte-preserved at
`assets/models/world/source/forest-deadwood-v1`; log/root masters remain in the
original forest-dressing package. Its rights/custody/derivative receipts identify
each source separately; no blanket MIT/CC0 claim applies to the Meshy stump or
original Horde assets. The compact stump is a repaired low-poly editable source,
not the separately archived high-poly Meshy master.

The reversible runtime material default shares the existing opaque dielectric
ForestBark family, including cut wood, without allocating another texture layer.
Original painted bark/end-grain/stump maps remain preserved. No new generation,
decimation, transparency, light or shader feature is added. Exact world triangles
supply nonwalkable contact; the open snag is not claimed as a closed volume.
Phone performance and final owner art acceptance remain separate gates.

### Original low trail root (18 October 2026)

`src/scene/ForestDeadwood.cpp` authors one new tapered twenty-triangle trail root
directly from numeric rings. This is original project geometry created for Horde,
not a derivative/copy of the supplied 548-triangle root tangle, imported art or
paid generation. It reuses the already admitted ForestBark material/maps and
existing static native RT owner; it adds no texture, external file or licence.
The exact final world-space RT triangles also form its explicit STEP contacts.
The ten original off-trail deadwood placements remain nonwalkable and retain
their individual provenance above. Physical performance and final visual
acceptance of the new root remain separate gates.

### Free nature recordings for the same-night exterior (18 October 2026)

`assets/audio/night-ambience/manifest.json` pins original SHA-256 values and
every prepared PCM derivative. `NOTICE.md` carries credits/source/licence links
and processing disclosures into the Android package. The originals are preserved
in ignored local staging, not redistributed as large game payloads. Prepared
derivatives are mono 24kHz PCM16 with bounded excerpts, fixed attenuation and
150ms fades; exact crop/gain/hash/levels are in the manifest and reproduced by
`tools/prepare-night-ambience.py`. No loop, pitch or noise filter is added.
Source species/time descriptions are metadata, not independent listening
acceptance. All four cuts remain unheard here; final native mix is pending.

| Prepared recording | Credit and licence | Disposition |
| --- | --- | --- |
| `runtime/tawny_owl.wav` | W.carter, [Tawny owl calling at night in Tuntorp, Brastad, Sweden](https://commons.wikimedia.org/wiki/File:Tawny_owl_calling_at_night_in_Tuntorp,_Brastad,_Sweden.ogg), [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Original already contains cuts from two locations and background-noise reduction; these prior changes are retained in the manifest. | Eight-second finite candidate in the shared forest cue, superseding the generic FilmCow bird mapping. Author identifies Strix aluco and night recording; no independent species or listening claim. |
| `runtime/evening_cricket.wav` | Gunther Tschuch, [Oecanthus pellucens song 23C](https://commons.wikimedia.org/wiki/File:Oecanthus_pellucens_song_23C.wav), [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/). **The adapted audio remains CC BY-SA 4.0**, with attribution, source/licence links and disclosed changes; no endorsement is implied. | Eight-second finite candidate in alternating quiet gaps. Author records male Italian tree cricket at 21:04; darkness is not independently established. |
| `runtime/pond_frogs_crickets.wav` | Glaneur de sons, [Nature sounds ambience in a Dordogne pond](https://commons.wikimedia.org/wiki/File:Nature_sounds_ambience_in_a_Dordogne_pond.ogg), [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/), as recorded by Commons and its 2014 licence review. | Eight-second finite mixed dusk pond candidate admitted byte-exact from preparation, only within 18 m of actual shared regional water faces in the ready same-night grounded exterior. Tomb runoff and guessed forest ponds do not admit it. Source species are unspecified; recording remains unheard and native mix/listening acceptance is pending. |
| `candidates/periodical_cicada.wav` | Kathryn M. Fontaine, John R. Cooley and Chris Simon, [Calling song of Magicicada septendecim / Audio S1](https://commons.wikimedia.org/wiki/File:Calling_song_of_Magicicada_septendecim_-_pone.0000892.s001.oga), PLOS ONE 2007, doi:10.1371/journal.pone.0000892; [CC BY 2.5](https://creativecommons.org/licenses/by/2.5/). | Prepared 5.8-second candidate, **excluded from runtime packages/playback**. North American species with no night metadata; it is not admitted as nocturnal European ambience. |

The earlier FilmCow candidate and its provenance remain preserved, but Android
no longer packages its unused WAV. No blanket CC0 or software-licence claim
applies to these separately attributed recordings. No paid generation was used.

## 1.7 finite local tomb and rope cues

`assets/audio/chapter-local-ambience/` admits two small game-integrated FilmCow
derivatives from the existing private Possum Cafe archive: the full `rope tighten
1.wav` (0.991667 s) and a one-second activity window from `ambience - dripping
water 1.wav` at source frames [5,083,200, 5,131,200). Jason Steele / FilmCow's
custom royalty-free project-use licence applies, **not CC0**; the existing
[licence record](docs/licenses/README.md) and owner-supplied
[PDF](docs/licenses/filmcow-sfx-license.pdf) retain its personal/commercial media
permission and authorship/resale/government/law-enforcement/hate-group restrictions.
Voluntary credit: FilmCow Royalty Free Sound Effects Library — FilmCow.

The manifest pins original and runtime hashes, source crop, PCM format and
licence-PDF identity. `tools/prepare-chapter-local-ambience.py` reads the original
mono 48 kHz PCM24 files without modification, converts to PCM16 and applies only
10/80 ms edge fades. The two runtime WAVs total 191,288 bytes. Originals stay
private and are excluded from packages. Purpose labels and PCM activity support
these bounded candidates; actual listening, character and mix remain unverified.
No generic rock handling is admitted as structural creaking, and no continuous
drip bed, live-collapse sound or paid generation is added.

## 1.7 finite forest foliage cue

`assets/audio/forest-foliage/` admits one bounded game-integrated derivative of
Jason Steele / FilmCow's `leaves rustle 1.wav` from the existing private archive.
The same custom royalty-free project-use licence and restrictions above apply;
this is **not CC0**. The original remains private. Voluntary credit: FilmCow
Royalty Free Sound Effects Library - FilmCow.

The full 43,283-frame mono 48 kHz PCM24 source is converted deterministically to
PCM16 with 10/80 ms edge fades, preserving duration and source level. Only
`runtime/leaf_rustle.wav` (86,610 bytes, 0.901729 s) and its hash/provenance manifest
ship. `tools/prepare-forest-foliage.py --verify` reproduces both without writing.
This is a labelled leaf-rustle candidate, not a certified natural wind recording;
actual listening, character and in-game level remain unverified. No continuous
wind bed, synthesis, provider generation or new audio layer is supplied.

## Compact torch carry r1 - source-only rejected study, 24 October 2026

`assets/models/player/source/compact-torch-carry-study-r1/` contains original
pose-intent coordinates, an editable Blender reference and its construction
recipe. It derives from the existing Gothic traveller/viewmodel and original
assistant-authored player Rag torch recorded above; their provenance, CC BY
attribution obligations where applicable, and user/service rights remain
unchanged. No additional third-party or paid input and no blanket licence is
assigned. The native-solved reference meshes do not replace those originals.

This endpoint study is rejected, source-only and excluded from every runtime
manifest/package. It supplies no admitted GLB or skeletal clip. Exact hashes and
failed native evidence are in
`docs/evidence/2026-10-24-compact-carry-study/receipt.json`.


## Private unshipped Keeper revival audition (2026-10-25)

"Short Monster Scream" by djani00 (Freesound), distributed by freesound_community
on [Pixabay](https://pixabay.com/sound-effects/horror-short-monster-scream-105026/),
was supplied privately by the owner. It is retained under the
[Pixabay Content License](https://pixabay.com/service/terms/), not relabelled CC0.
The owner MP3 SHA-256 is
`31509a2a1b39b488603f17f3ab92c3c1000e8cdf49733c7a337d7d482f84ae5f`.
A private full-duration mono 48 kHz PCM16 draft is 175,104 frames / 3.648 seconds,
350,252 file bytes (350,208 PCM bytes), SHA-256
`1e1891010f4c89e6f20ec5fba90191aa0f0999c0bd95a4df3fe36659beb9e521`.
Its conversion is an equal L/2+R/2 downmix, without amplification, trimming,
pitch or timing change. Conversion alone does not grant standalone redistribution.

Neither source nor draft is in public assets, runtime manifests, or packages.
Only an explicit Debug local-file audition can load the exact pinned draft from
`debug_audio/lich_revival.wav` within the isolated Windows application directory
or Android app-private files directory. Missing or changed files are diagnosed;
no other clip is substituted. Normal and Release behavior remains unchanged.
Actual listening, mix, encounter admission and any game-distribution decision are
still pending. Current terms (checked 2026-10-25) distinguish an integrated new
creative work from standalone content distribution; this note does not admit the
standalone audio to the public repository.

## Healing flask r1 - original source-only study, 26 October 2026

`assets/models/props/source/healing-flask-study-r1/` contains original
project-authored profile geometry, uniform opaque PBR factors, an exact `Grip`
marker, deterministic source GLB, editable Blender source and authoring recipe.
No third-party mesh/image/texture, paid generation or provider input is used.
Applicable user/service rights remain unchanged; no blanket MIT, CC0, Meshy or
other third-party licence is assigned.

The GLB SHA256 is
`ba93fd1a0242fb806c8be3972a1db1371e1aee3cd7ee4e22be447dbb7d8e95dd`.
This is an unadmitted source study, not a runtime import or a completed healing
feature. Blender studio lights/floor are preview-only and absent from the GLB.
Source files and previews are excluded from runtime manifests/packages. Static
validation and authoring previews are recorded in
`docs/evidence/2026-10-26-flask-source-study/README.md`; native hand/RT admission
and the pending joint resource decision remain open.

## Flask use/neck fit - source-only derivative, 26 October 2026

`assets/models/player/source/flask-use-study-r1/` is an editable source animation
study using the existing Gothic Traveller body/viewmodel, original admitted
sword and project-authored flask. Existing player/gauntlet/sword provenance,
attribution and user/service rights remain unchanged. No new provider, paid
generation or third-party input is used, and no blanket MIT/CC0 is assigned.

`assets/models/props/source/healing-flask-neck-fit-r2/` derives from the original
r1 flask above. It lengthens the neck for the unchanged gauntlet while retaining
the body/Grip positions, radii, material and primitive/triangle/vertex counts.
GLB SHA256 is `4a4337e81d98065c050ab896b8b0b61563feeedac55806665d2727ad14555cfa`.
The r1 source is preserved. Its editable derivative is in the separate study
Blend; no modelled asset or clip is admitted to runtime/package inventories.

`assets/models/player/source/flask-use-tick-aligned-r2/` is a separate timing
derivative of that same owned source scene. It advances only the complete-use
flask return/restore edge to53/60seconds, retaining original meshes, binds,
materials, rights and cancellation action. The original r1 source remains.
This source-only study is not a runtime/package asset or new third-party input.

Silent source previews and compact provenance/validation receipts are retained
in `docs/evidence/2026-10-26-flask-motion-study/`. They contain project assets in
an authoring scene, not captured game footage, RT/performance evidence or an
approved public demonstration. All native/physical and resource gates remain.

## Joint warning/flask runtime candidates - 26 October 2026

`assets/models/props/runtime/attack-warning-ring-v1/` is original project-authored
annulus geometry and scalar emissive PBR material. Its deterministic recipe and
source GLB are under `assets/models/props/source/attack-warning-ring-v1/`.
`assets/models/props/runtime/healing-flask-v1/` derives from the original owned
neck-fit r2 flask above, adding only required UV0 data; original geometry, material
and exact Grip are preserved. The untouched source remains under
`assets/models/props/source/healing-flask-neck-fit-r2/`; the new runtime recipe
and preservation receipt are under `assets/models/props/source/healing-flask-v1/`.

No third-party mesh, texture, audio, paid generation or provider input is used.
Applicable user/service rights remain unchanged; no blanket MIT/CC0 is assigned.
Runtime hashes are respectively
`15befb04ac511128714249c27b09a3ba96ea0c847edfbd91e80b14ef1eb7ed85` and
`e7571eb7b56b78ebe0a43946e16979f66b3b0b1f9a82613912e193300358341d`.
The two explicitly selected runtime GLBs/manifests may enter development packages
for the approved joint resource test. Source folders, recipes and validation
receipts are not runtime package inputs. These assets and their gameplay
presentation remain unadmitted for normal play or release until native visual,
lifecycle and gameplay gates pass. Resource approval is not visual acceptance.

## Flask native binding source candidate - 26 October 2026

`assets/models/player/source/flask-native-binding-r1/` is a separate derivative
of the owned tick-aligned source study and existing Gothic Traveller body above.
It preserves original geometry, binds, scales and Idle/Walking, appending only
the project-authored FlaskUse/FlaskCancel tracks. The editable Blend corrects
right-arm quaternion interpolation while preserving authored keys. Existing
player/gauntlet/sword attribution and user/service rights remain unchanged; no
new third-party, provider or paid input is used and no blanket MIT/CC0 is assigned.

Candidate GLB SHA256 is
`56d97faeb0595a625282c3798bc40b49d7ffedd9598e2d99fd8a186acf15f014`.
This source-folder candidate is excluded from Android/runtime packages and is
not normal-play or release admission. Its explicit native CPU binding, exact
receipt and remaining cancellation/RT gates are recorded in
`docs/evidence/2026-10-26-flask-native-binding/README.md`.
