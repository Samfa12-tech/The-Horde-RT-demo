# Bellwether mill footway and crossing

The footway branches from the dry part of the river causeway and climbs beside the mill's eastern wall. A 1.44 m wide stone deck reaches the wheel bay, then follows the west bank to the north crossing. The 7 m span is 2.8 m wide and descends 3.85 m to a fenced service turnout. A standing player can visit and return while the wheel is stopped or running. The existing shallow platform remains the route for the breathing-rig trial.

The town GLB uses the same five primitives and four materials. The deck, retaining walls, posts and abutments supply both ray geometry and native contact. Four original causeway retaining triangles were clipped to open the junction. The net addition is 1,038 solid and 38 walkable triangles. The terrain GLB and regional water geometry retain their original bytes.

The shared 60 Hz player motor walked the whole route in both directions without wet steps, then reached the original shallow landing. The hidden Windows `mill-crossing` survey presented both supported views on Ray Tracing Pipeline and Ray Query Compute. It checks presentation from frozen poses. The exact ARM64 debug bundle includes the updated town and wheel, with physical-device performance still open.

## Regenerate

Use Blender 5.2 in background mode with `tools/author-bellwether-mill-crossing.py`. Pass the repository, original town GLB from checkpoint `e4f54c68` and a new output directory after `--`. The script checks the input town and surveyed terrain hashes. The editable master, output hashes and measured placement are bound by the [authoring receipt](authoring-receipt.json). Independent runs produced byte-identical runtime GLBs and manifests.
