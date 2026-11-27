# Bellwether relic stall

The stall occupies the mapped S6 service site west of the square. A clay
lean-to covers a side counter, stock chest and wick. The tall back of the
display faces the square and hides the dealer along the sampled east and
south-east approaches. A weighing sign marks the opening around its south
end. The ground and final standing marks use the admitted chapter terrain.

The editable Blender source and [authoring receipt](authoring-receipt.json)
describe the exact mesh and generated reference points. The 1,176 solid
triangles extend the existing town's opaque primitives. The original town
vertices, indices, material rows and walkable mesh retain their original
bytes. The added solids are used for rendering and collision.

Use [the Blender recipe](../../../../../tools/author-bellwether-relic-stall.py)
with the exact town GLB from commit `e63e0a5b` and a fresh output directory.
Its pinned input SHA-256 is recorded in the receipt. The asset recipe also
emits a standalone patch for inspection, a merged town GLB, the manifest,
and the generated marks for the motor and wick.
