# E01 Abbey attendant: source handoff

Final body: **14,122 triangles, 17,173 exported vertices, 25 joints**. The original 24 deform joints are retained; RightGrip is an unweighted helper. There are no new articulated finger bones.

## Contents and use

- E01_AbbeyAttendant_core.glb contains exactly four clips in order: **Idle (4.0 s), Walking (1.066667 s), Attack (1.5 s), Dead (2.966667 s)**. Idle/Walking loop; Attack/Dead are one-shots.
- E01_AbbeyAttendant_source_extras.glb contains Hit_Reaction, Idle_Turn_Left, Idle_Turn_Right and Running. These are source-only. Hit_Reaction travels about 0.815 m horizontally and is rejected for in-place H1 use.
- E01_AbbeyAttendant_all_clips.glb is the portable full animation source. The editable Blend retains all eight named actions and a separately constrained mallet assembly. Default viewing uses Idle.
- The body GLB remains one mesh, one skin and one material, with three embedded 1K PNG PBR maps. The separate mallet GLB has its own identity Grip socket. Do not export the assembly mallet as a second body mesh.
- Bind/world scale is 1.7 m, +Y up, +Z forward. The provider armature retains its 0.01 conversion; RightGrip's 100 scale cancels this to give a unit world socket. Do not apply rig scale without rebaking clips and inverse binds.
- In Blender use the BLENDER bone-import heuristic for faithful re-import of this Blender export. Set imported NLA strips to local frame zero before another export; otherwise a one-frame offset can be introduced. The supplied editable Blend already has correct local timing and packed images.

## What passed

Real source bytes, material/UV lineage, scale, skin weights, accessor layouts, actual native CPU skin reader, actual held-item rigid-socket validator, fresh Blender pose review, and GLB import/export/import checks were exercised. Idle closes within 0.11 mm; Walking within 0.002 mm. Sampled core body surfaces stay about 1 mm above the floor. At 17 samples per core clip, the tool has no overlaps outside the gripping hand. Corrected Dead keeps it at least 4.07 cm above the floor. Native and Blender RightGrip placement agree within 0.005 mm; the round-trip sampled joint-position difference is about 0.0051 mm. All three texture images remain pixel-identical through round-trip.

The right hand uses a bounded local subdivision, palm-plane curl, opposed-thumb smoothing and nominal-grip contact fitting. No checked Idle vertex penetrates the nominal grip ellipse, but conservative triangle contact/overlap pairs remain inside the gripping-hand region. This is an ordinary-enemy grip, not a first-person hero-hand or collision-free hand certificate. Cuff/pocket contacts are recorded rather than broadly welded.

## Still requires game integration

This is an asset-source/native-reader candidate, **not a game-ready or performance-certified enemy**. No Vulkan RT presentation, GPU skinning, device performance, actual movement-speed footlocking, gameplay blending, collision or authoritative damage/event mapping was tested. The current legacy skeleton renderer uses untextured CPU skinning. Its default clip set expects Idle_5 and its attack timing is 2.8 s; this asset uses canonical Idle and a 1.5 s attack, so it needs explicit E01 bindings and gameplay-owned timing. Dead's authored fall displacement is retained. Source extras are not admitted runtime slots.

## Source preservation

Provider originals are unchanged. The 12K comparison source is actually 12,467 triangles; the 6K source is 6,261 and was rejected for the close-pilot face/finger silhouette. The final 14,122 count includes local grip repair. Original 2K/4K remesh-specific maps were preserved; exact map hashes proved UV/map lineage before producing 1K derivatives. Source-only roughness was not overwritten: the derivative adds a restrained boot-leather roughness floor and neutral AO. No Hotstrike skeleton geometry or other licence-sensitive model was copied.

See validation-summary.json, PRODUCTION_PROVENANCE.json, RIGHTS.md and the receipts for exact hashes, accepted prompt, task lineage and the conservative Meshy attribution route. The original locally authored mallet has its separate provenance; no blanket licence is assigned to it. No game release or performance certification is implied. Preserve the embedded AI-origin metadata and applicable attribution.
