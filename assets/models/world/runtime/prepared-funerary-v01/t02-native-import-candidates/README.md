# Prepared T02 native-import candidates

These candidate-only GLBs derive from the recovered T02 source package. They append deterministic UV coordinates because the Horde static GLB loader currently requires `TEXCOORD_0`, including for texture-free materials. Geometry, indices, normals, materials, and node transforms are preserved; no texture, remesh, merge, subdivision, or master rebuild is performed. See `derivation-receipt.json` for exact source/runtime hashes. Root-owned licensing, runtime manifest admission, placement, and collision remain pending.
