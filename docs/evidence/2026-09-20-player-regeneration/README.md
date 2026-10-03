# Player regeneration evidence, 2026-09-20

[Exact inputs, Blender build and two output hashes](regeneration.json).
This is offline evidence, not permission to publish or a new visual/device pass.

Two default parallel Blender 5.2 exports produced hashes
`5acab5e0255da8f262b0f2d22ca2b6838048a0b225794d259c09b704f316d1e1`
and `d51a28a14cef6bd16eadf974ad5186a7f95c142936f8f14935d9051e8d4dd6bf`.
Their JSON is identical; their only binary difference is two bytes in the gauntlet
TANGENT accessor (maximum component delta 0.00009995698928833008). The first
default export differs from tracked runtime `cd64a844…2c479d` in three tangent
components across gauntlet/near-face, not position, normal, UV, joint or weight data.
Do not call default parallel export byte-reproducible.

Two `--threads 1` exports then matched exactly; two more through the committed
validation runner reproduced the same `e8737f10…450fd` hash. It differs from the
tracked runtime partly because unique vertices are ordered differently. Comparison
after expanding indices by named primitive shows exact position/normal/UV/joint/
weight equality for all 83,325 vertices. Tangents differ in seven expanded gauntlet
components (max 0.00010001659393310547) and five near-face components
(max 0.00009995698928833008); Body/Head tangents match exactly. JSON and binary
views outside the reordered primitive data/tangents match. The generated asset passes `PlayerRenderSlot::LoadAsset`,
including semantic checks, grip derivation and boot-grounding setup.

No generated GLB was promoted, and the tracked runtime remains byte-identical.
Its current decoded counts are 41,530 unique upload vertices / 27,775 triangles:
Body 4,630 / 5,532; Gauntlet 18,527 / 8,838; Head 2,289 / 1,813;
NearFace 16,084 / 11,592. Blender processing reports 33,894 pre-export vertices.
These counts correct the stale three-part source metadata, not the source asset.

Reproduce into a **new**, unused validation directory (runner refuses collisions):

```powershell
.\tools\validate-player-regeneration.ps1 -OutputDirectory C:\Dev\tmp\horde-player-regeneration-new -BlenderExecutable 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
```

The runner writes two GLBs, processing reports, logs and a hash receipt. The GLBs
are local-only/rebuildable, not duplicated in Git. Retained local run:
`reports/player-regeneration-proof-da931ee918ba455c9a7defe85c292d0c`.
Use the report's exact Blender/input hashes before comparing byte identity.
Sampler-selection warnings occurred during export; no visual acceptance is inferred.

Next: decide and validate generated-runtime admission, including static/skinned
stream agreement and affected native RT images. Do not silently replace the
accepted runtime or weaken deterministic image gates for the tangent difference.
