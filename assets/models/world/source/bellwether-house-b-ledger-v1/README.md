# House B stock ledger

An open ledger rests in the sorting tray next to the store room's oil cup.
The entry records two sacks for the tavern and points to the existing tally
board at the mill's west door. Stop beside the table and face the book to read
it. The entry is available with speech subtitles switched off.

The project-authored ledger adds 108 solid triangles to the shared town mesh
and reuses its clay, stone and char materials. The editable Blender file keeps
the binding, pages and marks as separate parts. The merged GLB uses the same
six primitives and the existing table lamp.

Regenerate from the repository root with Blender 5.2, the town GLB and
adjacent manifest from `82fc6e53`, and an unused output directory.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-house-b-ledger.py -- . <parent-town.glb> <new-output-directory>
```

The [authoring receipt](authoring-receipt.json) records the input and export
hashes and the position used for the reading prompt.
