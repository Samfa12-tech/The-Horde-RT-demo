# Current macro-terrain reference custody

Recovered without merging the planning branch. The original files in this
directory are byte-identical to commit
`d37f16381311c6e620f0c4a9d01c1cce83260dc8`. This note is current commentary and is
not part of the historical atlas.

The selected subset is the connected-region WebP/SVG, source generator, layout
JSON, historical README/manifest and its two baseline data files. Links in the
original README to other atlas sheets/registers remain references to the pinned
planning commit; those files were not needed for this terrain recovery and are
not claimed present here. Do not run the historical generator in this preserved
directory: it writes generated sheets and data beside itself. Use a new versioned
working/output directory and retain these reference bytes unchanged.

Actual map pixels have been inspected. Macro extent from its generator is
East -150..410, North -510..440 metres (561 by 951 proposed height samples).
The baseline NPZ is 4,354,922 bytes and matches the previously admitted planning
source SHA-256 `251126186fa4923f9902524ec4ac2cc76d0f7a1c82cb1feac9901cc7832bb0ba`.
It is not an LFS pointer. The generator blends this local baseline into broader
relief, shapes the river/headwaters/outflow and lake basin, and grades proposed
approaches. Preserve that macro intent while keeping accepted local interfaces.

With the existing engine transform, the macro rectangle is X -71.7..488.3 and
Z -313.8..636.2. The r19 production derivative currently samples X -65..237,
Z -110..381 and therefore cannot establish full regional coverage. Simply
clamping the smaller NPZ or scaling the current mesh is not an acceptable
reconstruction. Even the finite map rectangle must not expose an edge from a
reachable camera; a measured coarse surrounding envelope must close the view.

The owner's current instruction makes this the surrounding-land authority,
alongside the local forest map. Historical draft/preparation-only wording does
not cancel that instruction. Detailed later-dungeon interiors/gameplay are still
outside the chapter. Far landmark visibility must follow actual landform and
viewpoint; no promise that every landmark is visible through intervening hills.
