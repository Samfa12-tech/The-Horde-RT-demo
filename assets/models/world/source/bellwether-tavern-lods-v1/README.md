# Bellwether tavern detail candidates

The B01 exports use metres and identity transforms in the registered town position. All three share the same 494-triangle architecture. It includes the roof, chimney, foundation, approach ramp, fixed opening frames, room floor, hearth and candle. The [authoring receipt](authoring-receipt.json) pins the tavern Blender master, the window glass and hinged door shared by the three levels, and each candidate.

| Furnishings | Chair and table geometry | Triangles |
| --- | --- | ---: |
| `lod0` | Original timber parts and bevels | 1,012 |
| `lod1` | Original part bounds with square edges | 276 |
| `lod2` | Merged seat and top boards, full-height posts and legs, simplified framing | 144 |

The native import check loads all four GLBs with their exact manifest budgets. Every architecture and near-furniture triangle matches an oriented triangle in the current town export on a 0.1 mm coordinate grid. The south doorway and window apertures stay open in the architecture. All three furniture levels preserve the same bounds, timber finish, seat and tabletop heights. Khronos glTF Validator reported zero warnings and errors on all four GLBs. Two Blender 5.2 exports produced byte-identical GLBs, manifests and receipts.

## Native comparison

Windows Debug can load a fixed tier into the chapter's existing static RT geometry before it builds the BLAS. It replaces only the registered furniture triangles under the original timber material. The authored collision is built from the original town geometry, and the window, door and light configuration are shared across tiers.

From the repository root, use a different output directory for each take. Omit the detail flag for the original mesh, select `lod1` or `lod2` for a comparison, and add `--require-rayquery-compute` to repeat the take on the other hardware RT backend.

```powershell
$hordeDebug = '.\build\presets\windows-x64-debug\Debug\HordeLanternRT.exe'
& $hordeDebug --development-rescue-journey --capture-hidden --capture-showcase .\build\bellwether-approach-lod2 --development-checkpoint bellwether-south-approach --bellwether-furniture-detail lod2
```

The window view uses `bellwether-tavern-window`. The staffed view uses `bellwether-staffed-room` with the corresponding local `--chapter-resident-fixture`. The comparison manifest records the selected lower tier and its triangle count. In the staffed room the reported static vertex and index payloads together fell by 135,040 bytes at `lod1` and 153,520 bytes at `lod2` on both backends.

Twelve presented Windows frames at 960 x 540 on an RTX 5050 Laptop GPU used matched cameras, shader bundles and presentation settings within each pair. In the staffed room, 386 pixels changed at lod1 on each backend and 716 to 717 pixels changed at lod2. At the south approach, the far tier changed 132 pixels on each backend, confined to the visible room. The RayTracingPipeline window pair with Glass On was pixel-identical. These are full-frame comparisons. Individual primary, shadow and reflection contributions still need dedicated validation.

Runtime switching still needs visibility and residency rules that preserve interior contributors through the door and glass, including shadows and reflections, and allow a player to turn back before a transition completes. Fixed-tier payload totals do not measure resident memory, transitions or mobile frame rate.

The [Blender recipe](../../../../../tools/author-bellwether-tavern-lods.py) takes the repository root and a new output directory. The existing `horde_rt_bellwether_shell_tests` executable accepts that output directory as an optional argument to run the candidate checks alongside its ordinary town tests.
