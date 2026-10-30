# Flask neck fit r2 - source only

This derivative preserves r1's body/Grip positions, radii, material, topology
and budgets, extending only the upper neck rows by 35 mm. Its overall height is
199 mm. The unchanged neck diameter fits the existing clenched gauntlet while
placing the fist farther from the face. The r1 source remains byte-identical.

The source GLB is 9,292 bytes, 242 vertices, 384 triangles, one opaque material
and one primitive, with no new texture/light. Its SHA256 is
`4a4337e81d98065c050ab896b8b0b61563feeedac55806665d2727ad14555cfa`.
The editable mesh is included in the separate player flask-use Blend; this
directory contains the deterministic recipe and source GLB, not a runtime roster.

```powershell
python tools/author-healing-flask-study.py --recipe assets/models/props/source/healing-flask-neck-fit-r2/recipe.json --output assets/models/props/source/healing-flask-neck-fit-r2/healing-flask-neck-fit-r2.glb --verify
```

Exact regeneration/static geometry checks pass; Khronos reports zero errors or
warnings, with the expected informational empty `Grip`. Neither this result nor
the [motion source study](../../../player/source/flask-use-study-r1/README.md)
admits runtime import, hand/RT clearance or a capacity increase. Source folders
remain outside package inventories. Original project-authored derivative;
user/service rights unchanged, no third-party or paid input.
