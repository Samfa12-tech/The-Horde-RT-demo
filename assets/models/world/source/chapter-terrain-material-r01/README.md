# Terrain material balance r01

Material-only derivative of the preserved r21 terrain. The native GLB and
Blender master, collision/support triangles and six platform texture arrays
remain byte-identical. The original runtime manifest is retained as
`baseline.manifest.json`; the generated manifest changes only the named
`ChapterForestGround` linear base-colour factor from imported white to
`[0.5, 0.5, 0.5, 1]`. Trail sand remains unchanged.

This is a reversible development appearance default, not an owner-approved
physical albedo measurement. The RT light-attribution captures established
shared lighting and stronger upward-normal moon response; this adjustment
does not dim lights, bake shadows, change roughness or re-encode sRGB maps.
Original Horde terrain and Poly Haven's asset-specific CC0 notices remain
applicable. Editable source material is excluded from runtime packages.

Reproduce metadata into a **new** directory:

```powershell
python tools/author-terrain-material-balance.py --baseline assets/models/world/source/chapter-terrain-material-r01/baseline.manifest.json --output-directory reports/terrain-material-reproduction --factor 0.5
```

The recipe checks actual r21 bytes, all six texture payloads, finite bounds and
an untuned baseline before writing. It refuses an existing destination and
never writes model, texture or Blender files. `derivation-receipt.json` pins
inputs and output. Matched in-game appearance and fresh phone acceptance are
separate gates; source derivation alone does not prove them.
