# Bellwether livestock

The original resting pig, shelf hen and ground hen are separate editable
collections in the Blender master. Each head has an editable pivot. The
pen, closed gate, shelter and care
props use terrain-fitted footings. The dry path branches off the established
kitchen garden approach southward, away from the locked grate and privy.

Run `tools/author-bellwether-stock.py` with Blender 5.2 in background mode.
Pass the repository root, the town GLB at `a0263262`, and a fresh output
directory after the argument separator. The exact generated layout and
runtime digests are recorded in the receipt. The town export preserves its
material and primitive counts and uses the same static solids for player
contact and rays. `stock-heads.glb` holds the two pivot-local head meshes.
The two hens share the same head mesh at separate mounts. The pen keeps the
moving heads away from the player's collision path.
