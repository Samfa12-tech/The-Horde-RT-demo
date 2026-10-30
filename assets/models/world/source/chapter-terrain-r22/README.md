# Water-conforming terrain r22

Original Horde source repair of r21's coarse outer ring triangles, which bridged
the already-carved regional river beds. Only outer triangles intersecting the
exact lake/ribbon constraints, plus neighbours needing shared edge subdivisions,
are split. Added vertices sample the unchanged authoritative regional heightfield.
The two-metre inner lake contour improves shore sampling; it does not move the
water outline. This is source density, not runtime LOD or streaming.

The admitted runtime GLB is **18,408,928 bytes**, SHA-256
`12c0c577a67c5fc8e88fe0528a9eaa5b2ec950ba409bbffe969560b7c54ee060`.
It has 36,746 triangles and 18,936 exported vertices, within the unchanged
39,618 / 21,134 runtime limits. Growth from r21 is 2,696 triangles and 80,880 bytes.
The runtime manifest remains byte-identical, including Forest Ground's 0.5
material sidecar. Two material/image payloads, all 15,472 near-core triangles,
all 15,610 protected core/envelope triangles, bounds, route stations, entry
envelopes, holes and original grid heights compare exactly with r21.

`authoring-receipt.json` pins the Blender 5.2 recipes, inputs and local candidate
hashes. `water-conformance-receipt.json` records exact identity and admission
checks; `validation-receipt.json` and `gltf-validation.json` retain closed checks.
The local Blender master, duplicate source GLB and surface JSON are preserved
but ignored to avoid another large Git duplicate. The tracked runtime GLB is the
distribution copy. Existing r21 master/export remain untouched.

Reproduce from the repository root using Blender 5.2 background/factory-startup,
the versioned scripts and these pinned existing inputs. The destination must be
new; its basename controls the exported version name. For example, after
creating an empty parent `reports/r22-reproduction`, run:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --factory-startup --python tools/author-chapter-terrain.py -- --root "$PWD" --source-npz "$PWD/assets/source/beyond_the_tomb/terrain/topo-r2/proposed_terrain_heightfield.npz" --entry-audit "$PWD/assets/models/world/source/chapter-entry-return-r03/authoring-audit.json" --regional-layout "$PWD/docs/references/horde-1.7/d37f1638/docs/design/world/dungeons/baseline/shared-coordinate-layout.json" --output-directory "$PWD/reports/r22-reproduction/chapter-terrain-r22"
```

Checks completed locally, without a device or RT presentation:

- Source conformance 9/9 and existing regional foundation 5/5.
- Existing terrain validator: native GLB/support triangle correspondence,
  three open holes, nine occupied tomb volumes, 2,781 route capsule samples,
  263 entry roof intersections and 57 regional coverage probes pass.
- Existing Release native importer accepts the unchanged runtime manifest.
- Existing Release native support admission: 2,781 queries, 616 sweeps,
  zero failures, 2,703,656 retained bytes. Timings are host preparation/check
  diagnostics, not game performance measurements.
- Khronos glTF validator 2.0.0-dev.3.10: zero errors and warnings.

The source test samples the actual six-metre mitered river triangle domain
(1,288 probes; minimum sampled depth 0.828 m), orthogonal full-width river
stations (minimum 0.752 m), concave lake interior and its inner shore contour
(minimum 0.423 m). These finite samples do not certify every continuous point.
The underlying heightfield is only 0.181 m below datum at the shallowest probed
mouth/shore coordinate: a uniform 0.5 m shoreline depth would require changing
the accepted source. No such height change was made. Initial seam, budget and
overstrong shoreline trials failed and remain named in the receipt and local
reports; the final tests check actual non-burial and source identity.

Actual RT water appearance, shoreline art, phone acceptance and performance are
pending. This receipt makes no swimming, full-body collision or visual claim.
