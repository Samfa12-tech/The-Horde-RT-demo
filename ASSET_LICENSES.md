# Asset Licenses

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

## Supplied adaptive score (1.6.1 runtime assets and playback accepted)

`What the Dark Keeps`, supplied in `What_the_Dark_Keeps_Horde_RT_Music_Pack.zip`
SHA-256 `e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa`.
Owner-confirmed rights statement,2026-09-30: **Owner-supplied; authorised for Horde use only**.
No general permissive redistribution licence is inferred. Editable canonical JSON/
PCS1 under `assets/audio/music/what-the-dark-keeps/source/` remains the revisable
source of truth and is excluded from game packages, as is the supplied preview.
Only the hash-pinned manifest and sixteen measured stereo48kHz PCM16 body/tail
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

`assets/source/development-rescue/` contains original shaft, coping, lid and landing geometry authored locally from `src/scene/RescueBlockoutGeometry.h` through installed Blender 5.2. No third-party geometry was acquired. Editable Blender and GLB roundtrip sources remain outside runtime packaging; the renderer consumes the verified shared geometry recipe. Existing Poly Haven CC0 DryStone (world layer 0) and MossyStone (layer 2) maps retain their original attribution and exact byte identities in the source manifest. Native import and source admission are engineering checks, not artistic/device acceptance or new redistribution rights. The temporary outdoor skeleton reuses the already-admitted runtime model and its existing licence entry; it is a render workload witness, not Kit or a new enemy roster entry. Outstanding Hotstrike/Meshy public redistribution questions remain unresolved.


### 1.7 development tomb native candidates (2026-10-10)

- **T02 skull/jaw, femur and humerus**: recovered from the public immutable source archive at `93bdbd54016de6c4c7381a29b909d4049e4bf528`, `assets/source/beyond_the_tomb/funerary/t02-skeletal-source-v1`. Gord Goodwin's Simple Rigged Skeleton is **CC0-1.0**; retained source credit, licence notice, source-page check and checksum rosters accompany the recovered kit. Three selected GLBs have a runtime-only UV0 addition for the native importer; positions, topology, authored normals and source masters remain unchanged. The exact recipe and source/runtime hashes are in `models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/derivation-receipt.json`. Coarse alternatives remain source candidates, not runtime LOD admission. This is static burial dressing, not reuse of the animated enemy mesh.
- **T03 seven funerary props**: recovered original project Blender kit at immutable planning pin `d37f16381311c6e620f0c4a9d01c1cce83260dc8`, `assets/source/beyond_the_tomb/funerary/t03-v01`. Seven selected runtime GLBs are byte-exact copies (5,460 source triangles) with no imported texture bytes. Editable Blender files, recipes, authored-source provenance and source checksum rosters are retained separately. Original project geometry introduces no third-party model licence; no exclusive copyright or legal warranty is claimed. Exact-copy runtime receipt: `models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/derivation-receipt.json`.
- **Two original recessed niche modules**: deterministic headless Blender geometry authored specifically for Horde, using the already admitted masonry binding. Editable sources and derivation receipts remain outside runtime packages. They add no third-party texture/model bytes. Earlier unselected replacement skull/bone/funerary experiments are not admitted or packaged.
- **Wet contact cues**: four short Windows and Android PCM step variants use the owner-supplied “Footsteps Water 01” by aglinder. The provided MP3 is distributed on [Pixabay](https://pixabay.com/sound-effects/footsteps-water-01-73731/) under the **Pixabay Content License**; the identified [original Freesound recording](https://freesound.org/people/aglinder/sounds/265582/) states **CC0 1.0**. The supplied MP3 hash is `70f548d544985d6fa7e14463684d3792c51d87da5a987b631d51f3e589f401a9`; it stays outside the repository and packages. Four separated transient onsets are used for fixed 0.38-second cuts, fades, measured level matching and dual-mono Android conversion. The game-ready cues are combined runtime media, not standalone audio distribution. Pixabay terms permit commercial and non-commercial use and adaptation subject to prohibited uses, including standalone distribution; attribution is voluntary. `assets/audio/pixabay/water-contact.manifest.json` records source identity, cut/fade/gain/channel recipe, measured levels and runtime hashes. Owner listening approval for the revised mix remains pending.

- **Keeper death scream**: the owner-supplied “Monster Demon Voice - Death / Defeat Scream” by PhatPhrogStudio, [Pixabay item 582531](https://pixabay.com/sound-effects/film-special-effects-monster-demon-voice-death-defeat-scream-582531/), published 17 August 2026, is used under the [Pixabay Content License terms](https://pixabay.com/service/terms/), checked 10 October 2026. Adapted commercial/non-commercial game use is permitted; standalone redistribution is prohibited. This audio is not MIT-licensed. The original MP3 remains outside the public repository. Only the game-integrated mono/dual-mono PCM derivatives and provenance/processing manifest are included. Full source/runtime hashes, endpoint fades and measured levels are in `assets/audio/pixabay/keeper-death.manifest.json`; `tools/prepare-keeper-death-audio.py` reproduces them from an independently supplied source. Voluntary credit: PhatPhrogStudio via Pixabay. Exact-candidate owner listening and Android device acceptance remain pending.

The repository code licence does not override these individual asset terms. Native import and exact package hashes do not establish artistic, RT presentation or sustained-performance acceptance. Library locators identify the recovered prepared packages; they do not by themselves admit runtime use. The later-outdoor Meshy stump is excluded from this tomb slice. No private Briarhold assets or Kit recordings are transferred.
