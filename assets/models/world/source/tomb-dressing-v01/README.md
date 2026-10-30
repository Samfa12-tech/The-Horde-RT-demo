# Original niche source and unselected experiments

Only the two real recessed burial niche modules are selected for the development tomb. The editable `tomb-dressing-v01.blend` also preserves earlier unselected skull/bone/funerary experiments; these are not admitted, placed or packaged. The development scene instead reuses the recovered T02 CC0 bone kit and original T03 funerary props. Their source masters remain byte-preserved under `assets/source/beyond_the_tomb/funerary/`.

`tools/author-tomb-dressing.py` records the original deterministic Blender 5.2.0 recipe. It authors the entire experimental collection, not just selected runtime assets; running it does not admit its outputs. It was not rerun to replace the recovered masters. The original invocation was:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python tools/author-tomb-dressing.py
```

The niche modules have a recessed back plane, side and top returns, wall thickness, load-bearing lintel or voussoirs, and a usable shelf. They are geometry that casts/receives real RT visibility, not dark quads. The skull has open concave orbital cups and an open nasal aperture. Reusable bone assets are one femur and a three-rib bundle with vertebrae. The grouped item composes those original forms and one extinguished, nonemissive candle on a limestone plinth.

Dimensions are metres and assets are opaque static meshes. `MedievalWall02` is the existing admitted masonry family. `TombBone` and `TombWax` are original texture-free dielectric material parameters passed through the generic static GLB/PBR loader; they add no texture-array layers. The runtime manifests retain the required Android ASTC, Windows RGBA8 and mipmap policy fields for native loader compatibility; the GLBs contain no texture maps, external buffers, emissive materials or transmission. The assets remain candidates until root placement, collision, material admission and project licence records are integrated.

T03 payload recovery supersedes the earlier missing-payload observation. The exact selected runtime roster is in `tools/horde-1.7-tomb-asset-policy.ps1`; it excludes the experimental replacement family and the Blender backup. Candidate receipts retain their original admission-pending identity. Selection for an engineering package is separate from final artistic, physical RT and performance acceptance.
