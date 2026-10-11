# Tomb stone finish — 11 October

Original, deterministic 30 mm single-segment stone chamfers now soften twelve
door/overhead/return solids and sixteen closed wall joins in the development
chapter. The existing niches and loose offerings remain unchanged. Closed entry
returns replace front/back paper faces; both sides now use real outward normals.
Legacy demo geometry is preserved. No shader, light, shadow-mask, new texture,
quality, capacity or collision-envelope change is involved. The replacement
surfaces use the existing MedievalWall02 static masonry family, including its
real UVs and normal mapping, rather than the old hidden-shell material.

The new 656 triangles share the existing static masonry primitive/material and
BLAS owner. They replace 126 ordinary-world triangles (net +530). Native admission
still measures Mobile 31 / High 32 primitives, 30 imported materials and two
authored world records, 17/14/14 texture layers and nine static descriptors.
All original limits remain enforced. Cut bevels stay inside their original solid
bounds; corner fills reach only 30 mm into the existing capsule-wall separation.
No new small collision snag is introduced.

## Inspected native RT views

All final images are from the actual Debug executable recorded in
[receipt.json](receipt.json), Diagnostic/High, 960x540, 100% scale, twelve settling
frames, frozen time zero. No foreground input or owner game takeover was used.

| View | Image | Scope |
| --- | --- | --- |
| Front | [front](front.png) | Geometry inspected; complete capture **FAIL**, strict arm pixels zero |
| Wall join | [corner](corner.png) | Geometry inspected; complete capture **FAIL**, strict arm pixels zero |
| Reverse, before closed returns | [earlier local preview](rear-before.png) | Same camera/shaders; old EXE hash was not retained, so not exact-build acceptance |
| Reverse, closed returns | [Pipeline](rear-pipeline.png) | Complete capture **PASS**, geometry inspected |
| Reverse, closed returns | [QueryCompute](rear-query.png) | Complete capture **PASS**, geometry inspected |

The new front/corner capture contracts have no cropping exemption. Their failures
remain failures; both the first upward-looking framing and revised downward view
are retained privately. The two reverse captures satisfy the existing complete
capture contract. This does not certify every viewmodel pose or full traversal.

The owner approved the bevel preview ("that bevel looks NICE") and the final
reverse view ("perfect!"). The owner also correctly cautioned that a dark edge
may be a legitimate shadow, since it changed between views. The initial claim
that all dark strips were a rendering defect was too strong. The closed stone
uses normal PBR lighting and real RT visibility; no artificial brightening or
shadow suppression was added. Attribution of every historical dark edge is not
established. Screenshot approval is separate from live-route and phone acceptance.

## Validation and identities

- Debug focused geometry/resource/rig/portal: **4/4 PASS**, 22.84 s.
- Release same checks: **4/4 PASS**, 4.15 s; both Windows executables built.
- Debug development registry, original niches, nine occupied tomb volumes and
  QueryCompute shader freshness: **4/4 PASS**, 45.80 s.
- Android Debug native build, unit-test task and lint: **PASS**, 1m02 s.
  Java sources are unchanged; 283 existing Java test results have zero failures.
- Actual APK package/world/ASTC contract: **PASS**.
- Phone install/appearance/lifecycle on these bytes: **NOT RUN**.
- Live Windows bevel movement/clearance owner test: **NOT RUN**.
- Sustained performance: **NOT RUN**; twelve frozen frames are not that evidence.

Negative tests reject invalid radius before append; ray tests prove a cut jamb,
closed return and open walking lane. Every bevel box has matching geometric and
shading normals, finite orthogonal PBR bases, outward winding, and two opposite
uses of every welded edge. Original 17,680-triangle decoration tests retain their
prior exact bounds/grounding checks separately from architecture; integrated
renderer intake tests include all 656 new triangles and unchanged capacities.

Failed runs remain in ignored reports: initial missing-bevel red test; stale
decoration-only assumptions; stale exact scene count; stale registry boundary;
compiler temp access failure; failed arm captures. Counts were updated for actual
authorized geometry/registry additions, not budgets. The original torch test,
180-second policy, shader source and shader ceilings are unchanged.

Exact SHA-256:

- Windows Debug: `d2bd36bb267624292b995ca76de4a1069ee11fd3ffcc96c4baf9c42576e82759`
- Windows Release: `dc580f0bad9eaa78b050727833c941a8f921f7a0ad08f80b2e92bc4f3a5c6227`
- Android Debug: `0fa8de3eb34243dff8fd5406d74dd34dedb3dee83f93813358edea9473d292ed`

Phone remains closed on its previously approved package. No install, launch,
merge or release is implied. Audio/haptic manual revalidation: **NO for the bevel
change**; the preceding Android live-start/speech/combat correction still needs
its own fresh owner review. The wider chapter is incomplete, including visible
Kit, real zone retirement, denser woodland/ground extent, moon alignment, touch
layout and combat-performance investigation.

Next owner check: walk through the entry portal, turn around, move the torch view
along its jambs and the first corridor corners, then backtrack. Look for open
edges, flicker or movement snags; allow physically moving shadows. Repeat on
Android only after approving installation of the new exact candidate.
