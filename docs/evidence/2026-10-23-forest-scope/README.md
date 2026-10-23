# Forest visual scope and deferred shared-tree system

Read-only comparison at source `ba8aeacf`, 23 October 2026. No geometry, terrain,
lighting, tree count, package or streaming policy changes are made by this review.
The owner says the surroundings still look like a few trees rather than the
intended vast forest, and endorses terrain-led visibility constraints if needed.

## Actual RT evidence

Existing desktop captures were inspected, not generated stand-ins or phone proof.
Paths are relative to the existing WP2 checkout; raw captures stay in ignored
reports rather than being duplicated into Git.

| Capture | SHA-256 | Observed gap |
| --- | --- | --- |
| `reports/player-template-copy-20261023/candidate-rt/203-chapter-forest-lookout.png` | `a303359e63eeebedd9cf4b77c3a2ef26a8fd3ea32b34f4ef714fb15328ef79c3` | Broad treeless foreground and isolated town masses dominate. The distant horizon communicates little surrounding woodland or regional relief. This image is byte-identical to that experiment's control. |
| `reports/f03-masonry-20261023/forward/226-chapter-forest-clue-forward.png` | `6c9126ae6226e1c3d802e0dbc272bb39afb473851015d75cce6b7a90dcccc0d0` | Readable trail, banks, roots and masonry, but spaced trunks/open sky provide little layered canopy or mid/far forest depth. |
| `reports/f03-masonry-20261023/reverse/227-chapter-forest-clue-reverse.png` | `993997d3d82c7dcd9d71f099f596a1eea8e918c5fc260448ba4ebcb5145f3bac` | Grounded near trees and a clear bend; enclosure comes mostly from nearby banks/two large trunks. Sparse background silhouettes do not suggest extensive woodland. |

Regional relief is already implemented. The
[topographic integration handoff](../../superpowers/plans/2026-10-11-topo-terrain-integration-handoff.md)
requires connected land around visible directions; its implemented section and
[regional terrain receipt](../2026-10-11-regional-terrain/README.md) record joined
near/mid/far geometry. The current gap is what the views communicate, not a wholly
missing topo import. Older directional captures remain historical context; the
regional north shot's arm-pixel rejection remains rejected.

The pinned vision's section 8.1 and
[forest track sheet](../../references/horde-1.7/d37f1638/docs/design/world/connected/01-forest-track-1-7.webp),
[connected region sheet](../../references/horde-1.7/d37f1638/docs/design/world/dungeons/01-connected-region.webp)
describe a bounded route inside a wider region: clustered, real 3D near/mid/far
tree forms from reusable archetypes. Illustrative map circles are not placement
quotas. The [world layout](../../references/horde-1.7/d37f1638/docs/WORLD_LAYOUT.md)
keeps a distant settlement shell in 1.7; playable town streets/interiors/cast and
services belong to 1.8.

## Production versus draft

`ForestDressing.cpp` copies/rotates/translates two admitted LOD1 sources across
64 grounded placements, producing 323,264 triangles in four shared material groups
inside the existing static owner. It avoids hundreds of source models, but does
not implement shared-master mesh/BLAS instancing. The original
[PR 31 plan at a89ca287](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a89ca287/docs/superpowers/plans/2026-10-11-horde-forest-instancing-and-wind.md)
is a draft direction, not production admission or a measured saving.

The design index records the owner's later tree-generator/replacement-canopy
deferral and unadmitted private Easy Tree trial. Local
`reports/easy-tree-ff95ef5-intake/README.md` explicitly limits its authoring studies;
its prototype also fails the unchanged production index budget. No local-only
shared-BLAS implementation is admitted. Pine LOD2 rejection is a separate result;
keep LOD1. This review does not override the deferral.

## Smallest coherent proposed path

1. Use a short actual-view acceptance sheet: F01 outward directions, F03 outward
   and return, F04 centre/left/right. Keep landmark coordinates, route and lighting
   fixed. Judge forest depth, silhouette, enclosure and controlled settlement reveal.
2. Refine one exposed view sector with existing topo-led bank crests, bends,
   rock faces/elevation and a bounded lookout opening. Preserve the dry northwest
   approach, town sightline, safe backtracking and rope traversal; no map-wide redesign.
3. Layer near/mid/far real 3D forest forms around that sector using the supplied
   pair initially. Before multiplying current baked geometry, prove shared-master
   ownership, transform/material metadata capacity and BLAS/TLAS publication.
   Current packed-scene ceilings remain unchanged. Replacement canopy needs its
   deferred status explicitly resolved before adoption.
4. Measure actual vertex/index/BLAS allocation, TLAS/metadata cost, startup/overlap
   memory, RT frame time and transition hitches. Shared geometry may reduce
   duplication; denser overlapping foliage/instances can raise traversal and shadow
   cost. No savings are established by this proposal or by triangle counts alone.
5. Treat terrain occlusion as visual control first. Hidden geometry can still
   cast shadows or appear in secondary rays, so concealment alone proves no CPU,
   GPU or residency saving. Region retirement requires separate ray/light relevance,
   readiness, safe backtracking and submission/fence ownership proof before
   unloading. Preserve real hardware RT, with no baked fallback.

The forest is **implemented in part, visually incomplete**. This is an acceptance
gap and a bounded proposal, not permission for density inflation or full town work.
