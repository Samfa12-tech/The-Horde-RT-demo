# F01 / F03 / F04 actual-view acceptance sheet

23 October 2026. Existing WP2 checkout, source basis `9bd46a8f` plus two
diagnostic-only F04 camera entries. **Visual inspection, not finished woodland
acceptance.** No terrain, lighting, tree placement, collision, saves or package
asset changes were made.

The local clickable sheet is `reports/forest-view-sheet-20261023/index.html`.
It references the original capture pixels; no generated scenery, brightness
correction or replacement backdrop is used. Full raw manifests/process custody
remain beside it. The compact [receipt](receipt.json) records every image,
manifest, executable identity, camera, dimensions and shader identity.

Six fresh Windows hardware RT views use Diagnostic High / Pipeline, 960×540
internal AND output dimensions, 100% scale, twelve settling frames and unchanged
owner settings. F03 forward/return and F04 centre reuse explicitly identified
earlier actual captures; these are **not one exact-executable performance cohort**.
All nine pass their own unchanged strict capture gates. No phone, listening,
natural Keeper victory or whole-journey acceptance is inferred.

| View | Evidence | Finding |
| --- | --- | --- |
| F01 north | Fresh `f01-north/193-chapter-landing-north.png` | Shaft/graves and a few near trunks read clearly; broad bare land behind them lacks forest depth. |
| F01 east | Fresh `f01-east/194-chapter-landing-east.png` | Exit path/bank are readable. Trees concentrate along the route; surrounding land remains exposed. |
| F01 south | Fresh `f01-south/195-chapter-landing-south.png` | Near trunks frame one side, while the lateral middle distance remains bare. |
| F01 west | Fresh `f01-west/196-chapter-landing-west.png` | Grounded graves and near trunks; most background reads as open hillside/haze. |
| F03 outward | Reused `reports/f03-masonry-20261023/forward/226-chapter-forest-clue-forward.png` | Banks, path and waystone read well. Sparse upper canopy permits an early settlement peek. |
| F03 return | Reused `reports/f03-masonry-20261023/reverse/227-chapter-forest-clue-reverse.png` | Bend, roots and banks already provide enclosure. No demonstrated bank-height defect. |
| F04 left | Fresh `f04-left/228-chapter-forest-lookout-left.png` | Regional relief is visible left of the church, but large unwooded foreground/middle ground dominate. |
| F04 centre | Reused `reports/actor-worker-20261023/control-rt/203-chapter-forest-lookout.png` | Church/roofs remain readable, but the clearing fails to communicate surrounding woodland. |
| F04 right | Fresh `f04-right/229-chapter-forest-lookout-right.png` | Two near trunks frame one edge; the valley and middle ground remain largely bare. |

Fresh paths in the table are relative to `reports/forest-view-sheet-20261023/`.
The historical r21 north/side framing rejections remain rejected. These four
new current F01 views pass; this does not retroactively admit the old shots or
establish that their specific underlying framing issue was fixed.

## Concrete source/vision comparison

The [forest sheet](../../references/horde-1.7/d37f1638/docs/design/world/connected/01-forest-track-1-7.webp)
defines a 76.3 m trail, 2.4 m clear path, graded dip, 6.4 m reunion and 7 m lookout
pockets. It requires the fixed town reveal, backtracking and no enormous temporary
road wall. Tree circles illustrate massing, not a quota. The
[regional sheet](../../references/horde-1.7/d37f1638/docs/design/world/dungeons/01-connected-region.webp)
preserves Bellwether west of the river, a southern lake, eastern terrace and
northeast ridge. Its playable future town/dungeons do not become 1.7 scope.

Existing r23 terrain already includes regional relief and preserves all 15,472
near-route triangles. The F04 left view actually shows relief. The current
primary-ray maximum is 10,000 m (`rt_frame.glsl`), so a short primary clipping
distance does not explain the near-only forest. Lighting/distance-fog settings
were not altered to reveal or conceal scenery. Blue-grey distant surfaces alone
are not evidence that land is absent or misplaced.

The shared placement contract quantifies a stronger explanation:

- 32 pine and 32 alder roots span engine X **-41.355..18.170 m**, Z
  **-19.891..42.445 m**.
- Minimum/median/maximum horizontal distance to the pinned six-point trail is
  **2.916 / 7.429 / 9.919 m**. **No root lies beyond 10 m.**
- This measures root positions, not canopy extent, ray visibility or performance.
  It does not establish that a particular tree is visible from every shot.

Thus the route has a narrow near-tree band, without surrounding middle/far forest
mass. It currently bakes two source models into 323,264 triangles/four families;
that is not shared master-mesh/BLAS instancing. The existing
[bounded proposal](../2026-10-23-forest-scope/README.md) and owner Easy Tree/canopy
deferral remain in force.

## Terrain disposition and smallest next sector

**No terrain correction admitted.** These views do not demonstrate a misplaced
route, floating ground or a map-conformance defect that raising a ridge would
fix. F03 banks already enclose the walk. Raising the route to hide the settlement
peek would change protected grades; raising an arbitrary wall would conflict with
the lookout/atlas intent. This is a clean inspection result, not a reason to
manufacture an edit.

The smallest useful visual candidate is one **F01 north/west background sector**,
behind the existing graves/near trunks, outside the landing/rope/tomb keepouts.
Use existing topographic rolls as the boundary and preserve all landmark/path
coordinates. First define/prove shared geometry ownership with the supplied pair,
including current per-placement root fitting and fixed metadata/TLAS limits;
do not simply multiply baked copies or assume two shared BLAS preserve deformed
root contact automatically. Replacement canopy remains deferred.

Then judge whether a bounded real 3D middle-distance band makes that one sector
read as woodland. If measured budgets cannot support it, author one versioned
off-route bank/rock/elevation constraint from the topo and show it against this
sheet before expanding. Preserve F04's town window and the dry northwest-to-
southeast approach; no river crossing, playable-town expansion or map redesign.

Measure actual vertex/index/BLAS/TLAS/metadata and overlap memory plus RT timing,
not just triangles. Terrain concealment alone establishes no CPU/GPU/residency
saving: hidden objects may still shadow/reflect. Any retirement needs ray/light
relevance, readiness, backtracking and fence proof. No baked fallback is proposed.

## Checks and custody

Two diagnostic F04 views only rotate the existing centre gaze by ±45 degrees.
They preserve route station 7, the accepted-death diagnostic seed, camera position,
pitch, authored-scene requirement and strict equipment visibility. Ordinary player
cameras/gameplay and the 13 release checkpoints remain unchanged. Registry counts
are updated exactly to 109 development entries / 37 authored-scene selections,
with additive lookup/staging assertions.

- Debug registry CTest passes: 5.67 s.
- Release registry and authored-terrain route CTests pass: 1.78 / 31.52 s.
  This is functional evidence, not a route-speed gain or closure of the unchanged
  Debug 30-second gate.
- All six fresh hidden capture processes exited; settings SHA remains
  `ed609b868fd2951698c46bb17b98ffbad647bf30ca7cfacfa5d64f5342fd57c3`.
  Foreground unchanged; owner campaign directory stayed absent.
- Worker remains OFF. No phone operation, asset rewrite, paid generation, merge,
  release or deferred tree-system implementation occurred.

Compact test logs accompany the receipt. Raw images/binaries remain ignored;
the existing material/terrain/source assets are preserved.
