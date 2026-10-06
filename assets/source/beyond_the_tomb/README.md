# Beyond the Tomb source archive

Editable source and locally reviewed candidates for the 1.7 asset plan. These directories are not registered runtime assets and are excluded by the current Android/Windows package allowlists.

| Asset-plan item | Source directory | Bounded inventory |
|---|---|---|
| A07 tree assortment | [Original tree pair](forest/trees/original-tree-pair-v1/README.txt) | Alder: 7,832 / 5,102 triangles; pine: 7,984 / 5,000 triangles; matching editable LOD sources and original 512px maps |
| A07 dead-tree variation | [Dead snag](forest/trees/dead-snag-01-v1/README.md) | Quaternius CC0 geometry with original replacement bark; 4,792 / 2,395 / 1,198 triangles; packed Blender and upstream geometry retained |
| A08 stump | [Gnarled stump](forest_dressing/stump-v01/README.md) | 3,106 triangles; packed editable Blender; three 512px maps; targeted repair evidence |
| A08/A09 ground dressing | [Original dressing](forest_dressing/original-dressing-v1/README.md) | Three rocks, fallen trunk, root/branch piece and two ferns; 4,602 triangles combined; editable Blender and generators |
| A17/T03 funerary utility | [T03 kit](funerary/t03-v01/README.md) | Displaced lid, bowl, urn base/rim shard and three candle stubs; 5,460 triangles combined; editable Blender sources |

## Enemy and skeletal source batch

- [E01 Abbey attendant](enemies/e01-abbey-attendant-v1/core/README.md): 14,122 triangles, 25 joints; canonical Idle, Walking, Attack and Dead. [Editable source](enemies/e01-abbey-attendant-v1/editable/README.md), [validation evidence](enemies/e01-abbey-attendant-v1/validation/README.md) and [original mallet source](enemies/e01-abbey-attendant-v1/original-mallet/README.md) are preserved separately. Native CPU reader/socket checks passed; game binding, RT/device performance and gameplay integration remain open. Extras are source-only.
- [E02 bell-attendant tools](enemies/e02-bell-attendant-tools-v1/README.md): original handbell (1,640 triangles) and staff (564 triangles), editable sources and shared 512px maps. No E02 character, hand fit or actions are admitted.
- [E05 Bellkeeper shell](enemies/e05-bellkeeper-shell-v1/README.md): original modular shell fitted to a Meshy-derived core/rig; 11,904 combined triangles, two 1K materials and 20 rigid armour parts. A fitted source and inspection candidate, not a heavy-gait or gameplay admission.
- [A17/T02 skeletal dressing and static E06 source](funerary/t02-skeletal-source-v1/README.md): Gord Goodwin CC0 skull, femur and humerus derivatives, coarse alternatives and a 15,585-triangle static core. Preserves exact upstream source and licence evidence. Skull openings and non-manifold vertebral topology remain documented; no production rig, actions or mechanical enemy shell.

Each subpackage has an `ARCHIVE_SHA256SUMS` roster for its sanitized public copy. Binary sources remain unchanged from the producer handoffs. No rejected bulky generation trials or unrelated private model sources are included.

Use each package's hash roster, provenance and QA. GLB, Blender, PNG/JPG and upstream binary buffers are Git LFS content. Fetch LFS objects before opening them; Git pointer text is not an asset file.

Rights: Meshy stump attribution and the applicable output licence route are recorded in [RIGHTS.md](forest_dressing/stump-v01/RIGHTS.md); the dead-tree geometry is CC0 with upstream attribution. Other geometry, maps and generators are original project work. No new blanket public licence is assigned to creator-owned outputs. See the repository's ASSET_LICENSES.md.

These are source candidates, not a complete chapter or engine-admission claim. Native import, final material binding, collision, scene placement, combined RT appearance, automatic LOD transitions, wind, and exact Android/Windows performance remain open. The dead snag has open surfaces; the original living trees/dressing use overlapping closed components. Inspect each package's visual limitations before placement. No source directory here changes the accepted 1.6.2 baseline.
