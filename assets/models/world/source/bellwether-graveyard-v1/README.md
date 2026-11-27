# Bellwether active graveyard

The T03 churchyard sits east of the B02 church. Its south opening and stone path connect the grave rows to the existing exterior service approach. The path follows the admitted hillside. The church keeps its exterior service mark, and the northern wayside path remains clear.

The Blender master retains the individual dry-stone blocks, two entry piers, nine headstones, three footstones and the path. The headstones reuse the original [round and damaged models](../grave-markers-r03/README.md). Their plain faces carry no names or plot information.

The runtime export appends 7,412 solid and 1,512 walkable triangles to the existing town GLB. The renderer and movement both use that geometry. The path and stones share the existing MedievalWall02 material family.

Run [the authoring recipe](../../../../../tools/author-bellwether-graveyard.py) in Blender 5.2 with the repository root, the pinned town GLB from commit `179eb5d9`, and a fresh output directory. The [receipt](authoring-receipt.json) lists the source and runtime hashes. The editable master stays in the source directory.
