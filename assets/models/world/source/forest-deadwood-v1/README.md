# Prepared deadwood source recovery

This revision preserves the two supplied archives and reuses the original log
and roots already present in the forest-dressing source package. Four candidate
GLBs have passed the actual Horde native importer after separately hashed Blender
tangent export: fallen log544 triangles, root tangle548, stump3106, dead snag
LOD2 1198. Native scale is metres/Y-up; source bounds and hashes are in each
`.tangent-receipt.json`. Nothing here authorizes a new texture layer or generation.

`tangent-recipe.py` rejects an existing output path. Run headless Blender with
`--python-exit-code 1`; it does not save/replace an editable master or backup.
All archival bytes remain unchanged. See [rights](RIGHTS.md) and the custody
receipt before selecting runtime derivatives. Candidate payloads, original maps,
receipts and editable masters stay separate from the runtime package roster.

The selected revision is now `models/world/runtime/forest-deadwood-v1`: ten
placements (three logs, three roots, two stumps, two sparse coarse snags),11884
triangles. Existing ForestBark supplies opaque dielectric shading, including cut
wood; original painted bark/end-grain/stump maps stay here. Standing snags remain
upright while fallen pieces follow ground normals. Every low perimeter is surveyed
against the native terrain and buried30mm; exact triangles extend nonwalkable
contact without changing ground support or earlier grave/rock collision.
The real terrain check caught a bank-normal-rotated snag crossing the trail;
upright placement fixes it without shrinking clearance or source scale.

`tools/prepare-forest-deadwood-runtime.py --output-dir <fresh revision>` reproduces
the closed five-file runtime selection, verifying Blender receipt hashes before
copying. It rejects existing directories and preserves masters/backups. Native
Pipeline/QueryCompute views and package checks supplement import evidence;
phone/performance/owner art acceptance remain separate. No extra material row,
texture family, optical feature or actor is introduced.
