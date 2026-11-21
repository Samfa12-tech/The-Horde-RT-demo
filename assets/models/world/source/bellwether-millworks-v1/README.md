# Bellwether waterwheel and sluice

The 3 m wheel from the preserved Blender source now sits in the surveyed east millrace. Its timber face turns between stone bearing piers with recessed oak journal blocks. A raised sluice spans the headrace upstream of the wheel and leaves 24 cm of clearance above the water. The sluice frame and piers are solid terrain contact geometry. The optional dry crossing and walk to the wheel bay remain open.

The wheel uses 2,180 triangles and rotates about its axle at world XYZ (137.2, -21.10, 109.35). The shared simulation starts it when the bearing repair passes the existing workbench test. It turns at 0.5 radians per active second and holds during pause. An operating saved game resumes the wheel from the same starting phase without adding a campaign save field. The handoff, rig seal trial and ordinary river landing keep their existing progression.

The wheel's two finishes use the exact admitted ChestWood and BlackIron material rows. It adds no texture array layers. With Kit, the keeper and artificer present, the Windows RT preflight measured 49 primitive records and 46 material records before aliases were applied on both backends. The existing TLAS storage budget holds the additional wheel instance. The 700 new static support and sluice triangles extend the town's existing stone and timber ranges. The [authoring receipt](authoring-receipt.json) binds the Blender assembly, measured terrain, source wheel and runtime geometry bytes.

The hidden Windows `mill-wheel` survey presented the repaired scene on both hardware RT backends. It starts from the ordinary drive-test interaction, then freezes the snapshot to inspect the wheel and sluice from the same point. The 960 × 540 survey shows the wheel pose in the native night scene. Android physical-device performance remains to be checked.

## Regenerate

Use the existing source wheel GLB and the original town GLB at commit c4af08dd. Run Blender 5.2 in background mode with tools/author-bellwether-mill-works.py. Pass the repository, source wheel GLB, original town GLB and a new output directory after the argument separator. The recipe checks their inputs against the previous receipts. It exports the editable fittings, the merged town GLB and the axle-local runtime wheel. Independent exports of the runtime files match byte for byte.
