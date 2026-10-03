# Approved collapse atlas admission checkpoint

Two approved CC0 material families append to the existing shared prop atlas:
layer10 `Boulder01Rock` (Poly Haven/Rico Cilliers), layer11 `MedievalWall02`
(Poly Haven/Rob Tuytel). Existing Meshy licence/attribution is retained; neither
CC0 row relicenses older assets. Official [asset licence](https://polyhaven.com/license)
and [Boulder01](https://polyhaven.com/a/boulder_01)/[MedievalWall02](https://polyhaven.com/a/medieval_wall_02)
pages were checked on2026-10-03. This is source/texture admission, not native
Runtime acceptance or publication permission.

Checked exact JPEG/PNG hashes, all six lossless1024RGBA decoded pixel streams,
opaque alpha and retained CC0 snapshot hash. Compilation/`ktx validate` passed
all eight outputs. Independently verified original10layers x11mips unchanged
in all six expanded arrays, identical DFD primaries/transfer and both unchanged
single-layer emissive hashes. Negative checks reject wrong layer/format/mip/
compression/transfer contracts and a changed legacy payload byte; those tests
were in memory and did not alter sources or preserved arrays.

An initial compile rejected mixed unspecified/BT709 PNG primaries before runtime
publication. Optional `-AssignPrimaries bt709` labels declared source primaries,
without conversion; subsequent old-payload/DFD comparisons passed. No normal
filter, exposure, quality or scene-light retune was used. The failed log and
preserved10layer arrays remain outside source for review.

`atlas-growth.json` contains actual before/after KTX2 hashes, sizes, DFD hashes
and payload totals. `compiler-provenance.json` binds exact tools, KTX4.4.2,
input receipt, formats/semantics and licensing. Android array payload grows
5,301,408B (about5.06MiB); Windows grows33,554,424B (about32MiB).

Next action: native importer/renderer and closed-package checks use these exact
12layer arrays, then physically inspect the approved collapse and measure its
cost. Driver GPU allocation/residency, AS/geometry cost, native image correctness
and owner Runtime acceptance remain open. This slice performed no native build,
GPU/phone/Gradle operations, signing, commit or publication.
