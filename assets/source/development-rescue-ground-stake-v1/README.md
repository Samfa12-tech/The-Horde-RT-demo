# Grounded rescue stake and shaft source

Original, editable Blender 5.2 geometry authored from the current measured shaft
interfaces. This replaces the development overhead beam/cantilever with one short
iron stake; it does not move the accepted tomb, lid, shaft walls or safe apron.

The stake is 0.14 m square, extends from Y=1.75 to 2.73 m, and is buried 0.30 m
below the Y=2.05 m apron. The rope anchor is (-33.40, 2.65, -14.979) m. Its small
runtime attachment collar is authored separately in `RescueJourneyGeometry.h`;
the collar is not part of this Blender round-trip proof.

`source-manifest.json` records actual source texture bytes, asset hashes and the
canonical geometry recipe. Existing DryStone, MossyStone and AgedMetal material
codes use admitted Poly Haven CC0 maps. The geometry is project-created; this
does not relicense unrelated assets. No additional texture family is introduced.

Native admission and Blender export/import/re-export produce 264 vertices,
396 indices and three materials. The source GLB and round-trip GLB remain source
fixtures, outside runtime packages. Runtime geometry consumes the same measured
box/lid/apron recipe and the existing admitted texture layers.

Reproduction uses `tools/author-rescue-blockout.py` with a **new empty output
directory**, followed by the native importer and source-admission tests. The
authoring script refuses a nonempty destination and disables Blender backup
writes. Preserve this master, every historical master and all `.blend1` files.

Native import is engineering evidence. The night-time grip presentation, final
graveyard composition, phone performance and owner traversal acceptance remain
separate gates. See [the integrated evidence](../../../docs/evidence/2026-10-12-ground-stake-traversal/README.md).
