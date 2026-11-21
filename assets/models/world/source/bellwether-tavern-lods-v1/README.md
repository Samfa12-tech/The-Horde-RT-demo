# Bellwether tavern detail candidates

The B01 exports use metres and identity transforms in the registered town position. All three share the same 494-triangle architecture. It includes the roof, chimney, foundation, approach ramp, fixed opening frames, room floor, hearth and candle. The [authoring receipt](authoring-receipt.json) pins the tavern Blender master, the window glass and hinged door shared by the three levels, and each candidate.

| Furnishings | Chair and table geometry | Triangles |
| --- | --- | ---: |
| `lod0` | Original timber parts and bevels | 1,012 |
| `lod1` | Original part bounds with square edges | 276 |
| `lod2` | Merged seat and top boards, full-height posts and legs, simplified framing | 144 |

The native import check loads all four GLBs with their exact manifest budgets. Every architecture and near-furniture triangle matches an oriented triangle in the current town export on a 0.1 mm coordinate grid. The south doorway and window apertures stay open in the architecture. All three furniture levels preserve the same bounds, timber finish, seat and tabletop heights. Khronos glTF Validator reported zero warnings and errors on all four GLBs. Two Blender 5.2 exports produced byte-identical GLBs, manifests and receipts.

These are source-review assets. Runtime switching still needs visibility and residency rules that preserve interior contributors through the door and glass, including shadows and reflections, and allow a player to turn back before a transition completes.

The [Blender recipe](../../../../../tools/author-bellwether-tavern-lods.py) takes the repository root and a new output directory. The existing `horde_rt_bellwether_shell_tests` executable accepts that output directory as an optional argument to run the candidate checks alongside its ordinary town tests.
