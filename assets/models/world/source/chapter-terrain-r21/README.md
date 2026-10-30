# Regional terrain r21

Original Horde Blender 5.2 master and native export from the owner-designated
connected-region atlas plus the accepted local topo. The exact regional source
is preserved under `docs/references/horde-1.7/d37f1638/docs/design/world/dungeons`.
Its macro extent is East -150..410, North -510..440 metres. Engine transform stays
X=East+78.3, Y=Up-31.55, Z=126.2-North.

Run `tools/author-chapter-terrain.py` with Blender background/factory-startup and
absolute `--root`, `--source-npz`, `--entry-audit`, `--regional-layout` and a NEW
`--output-directory`. Inputs are the existing topo-r2 NPZ, chapter-entry-return-r03
audit and pinned baseline shared-coordinate-layout.json. Existing destinations
are refused. The failed sandbox r20 master/export and every earlier master and
backup remain untouched. Run `tools/test-chapter-regional-terrain.py` with NumPy,
then `tools/validate-chapter-terrain.py <new-directory>` and native intake.

The local core retains its exact fine vertices/triangles, route stations and
openings. Joined rings use 4/8/16 m spacing through regional relief, avoiding
global propagation of narrow shaft grid lines. Beyond the reviewed rectangle,
coarse provisional land continues past the unchanged 10 km primary ray reach;
it is not another approved region. The outer edge and underside are closed.
This is fixed distance-based source density, not runtime LOD or GPU streaming.

34,050 triangles and 17,588 exported vertices fit the previous runtime manifest
without raising its limits. Two materials use exactly the previous admitted
maps. Native support is built from these same imported triangles. The regional
river/lake **beds** exist; distant water surfaces, structures and dense foliage
are still integration work. No claim of a finished woodland or mobile acceptance.

See `docs/evidence/2026-10-11-regional-terrain/README.md` for source correspondence,
tests, actual RT frames and pending gates. Source assets stay out of runtime
packages; the exact GLB is copied to `runtime/chapter-terrain-v1`.
