# Ordinary static-detail unload and return checkpoint

The normal renderer now automatically prepares and adopts a smaller static owner
while the post-claim player walks into the enclosed coloured passage. It restores
the full owner before the return to the rope. This first slice removes **84,219
forest triangles** and frees **6,215,936 device-local bytes (5.93 MiB)** on Windows.
The complete chapter and broader two-way streaming remain incomplete.

## What changes ownership

The selected triangles are admitted ForestBark/Pine/Moss/Alder detail whose every
vertex has Z > 24 m, with ExteriorDressing provenance. Selection validates actual
packed indices, ownership, finite bounds and a continuous contributor envelope.
It retires the replaced static index/BLAS owner after completed GPU work and fresh
current-frame TLAS recording. Static vertex streams are immutable shared storage;
they are neither recopied nor retired during this transaction. Full source CPU
geometry remains cached for bounded restoration.

Coarse terrain, distant Bellwether, collision/support, tomb/niches, shared atlas,
all remaining forest detail and uncertain contributors remain resident. This is
not whole-zone unloading or a texture-memory saving. The existing Keeper lifetime
savings are separate and are not counted as this static-detail benefit.

## Safety and continuous coverage

The unchanged passage's actual north wall, roof and floor have a ten-triangle
certificate in `src/scene/CorridorStaticDetailFaces.h`. At runtime the builder
checks exact binary32 positions, indices, material codes and immutable boundaries;
the live frame also checks opaque BLAS ownership, world mask and identity transform.
Any mismatch retains the full owner. Open portal ends and the uncertain native
niche seams are not filled or assumed closed.

`tests/test_corridor_detail_certificate.py` proves entire rectangular face coverage
with rational edge arrangements. Six independent synthetic cases include a thin
slit, overlap and disconnected patches; deleting any of the ten required triangles
fails. The C++ tests reject displaced faces, changed materials, nonopaque geometry,
unsafe target/source bounds and nonfinite inputs. The historical copied-builder
inventory guided selection; it is not substituted for the runtime byte checks.

Camera bob, native claimed-lantern panes and light endpoints must stay inside a
verified source prism. The target envelope includes the 18 m moon shadow sweep
and 12.02 m bounded secondary receiver reach. All source-target box corner-plane
intersections enclose the continuous coordinate domain. Local-light visibility is
covered by the reverse endpoint-to-target segment, including endpoint trim margins.
The live transmission-owner roster excludes unknown long-continuation origins;
water droplets, altered fixtures, staff light and unknown source states keep the
full owner or restore it before tracing. Existing shaders, shadow samples, masks,
quality, transport ranges and resource limits are unchanged.

A generation-bound simulation lease prevents translation outside the safe region
until real restoration. It defers reset/retry/checkpoint commands without consuming
them. Look and timely input publication continue. Each preparation advances by the
existing 256 KiB bound or one owned AS submit/nonblocking poll. The current graphics
fence and obsolete command references must be retired before descriptors/owners
change. Cancellation and a single bounded restoration retry preserve the live owner;
unproved GPU work is never freed. Even a concurrent explicit diagnostic transaction
must wait instead of tracing a reduced owner after its source state becomes unsafe.

## Final optimized Windows measurement

Both r04 runs use executable SHA-256
`cbb478772a0c8e664b4f105a7f74f4029d3038c0525fbdc6edc8726c452340cf`,
optimized Release CPU code and actual Shipping High shaders on RTX 5050 Laptop GPU.
They are foreground, uncovered 960 x 540 at 100% scale, Current shadows, High
water/fire/optical quality and dust Off, matching the earlier controlled profile.
The route starts after staged Keeper defeat, traverses reunion/lookout/return,
descends, backtracks into the passage, returns and ascends again using ordinary
simulation inputs. It does not certify the earlier combat encounter or owner feel.

| Measurement | RT Pipeline | RayQueryCompute |
|---|---:|---:|
| Duration | 96.4633 s | 96.4602 s |
| Completed presented frames | 15,138 | 15,093 |
| Warm present-return median / p95 | 6.062 / 7.741 ms | 6.063 / 7.777 ms |
| Warm maximum | 11.203 ms | 15.086 ms |
| Worst within +/-1 s of unload | 7.509 ms | 7.656 ms |
| Worst within +/-1 s of restore | 7.418 ms | 7.530 ms |
| Unload / restore count | 1 / 1 | 1 / 1 |
| Readiness-wait frames / pending simulation ticks | 0 / 0 | 0 / 0 |

These are wall-clock intervals between successful present API returns, including
CPU/render/acquisition work; display scanout was not measured. GPU timings and
aggregate machine CPU load are retained separately. The machine was not CPU-pinned
in these runs (mean 14.7% / 16.7%); this does not attribute older stalls.

Scene device-local allocation is **371,998,256 -> 365,782,320 -> 371,998,256 bytes**
at warm full / reduced / restored states. During preparation, the scene peaks at
**420,071,600 bytes**, 48,073,344 bytes (45.85 MiB) above warm full. The measured
static owner overlap is 117,432,832 bytes. Host-visible peak is 53,410,544 /
53,410,032 bytes; memory flag categories can overlap and must not be added.
The reduced owner lasts approximately 6.83 seconds on this route. Its saving is
modest compared with temporary overlap: this is a working bounded first slice,
not completion of the requested useful chapter-wide lifetime coverage.

[Pipeline analysis](release-pipeline-analysis.json) and
[Query analysis](release-query-analysis.json) retain tails, per-phase values,
exact shader identities and both transition windows. [Raw evidence](release-r04-raw.zip)
contains all completed frame rows, joined identities, source/build/settings receipts,
load samples and captures. [Member hashes](release-raw-files.json) bind its bytes.
After extracting it, run `tools/analyze-chapter-route-measurement.py <backend-folder>
--analysis-name analysis-streaming-r02.json` to verify the stored arithmetic.

## Evidence boundaries and next work

Optimized Release4/4 focused checks pass, including the final explicit-candidate
fallback regression and exact coverage certificate. Final native Debug build and
both complete hardware routes pass. The full194-test Debug sweep completed106
tests before interruption, then resumed at its first unfinished test107; its final
aggregate disposition is pending. Android four-ABI Debug/build/lint, strict ASTC
and closed-roster package checks and286 freshly rerun Java tests pass. Logs and
exact EXE/APK/source identities are in [the receipt](artifact-receipt.json).
APK`850acb668dce442597e6ec6b603640620d0273219066a18e8b9f41f37a6a70f1`
is prepared and uninstalled. Exact-commit CI remains pending the checkpoint push.

Both final Debug r03 routes completed1,488 fixed-step render frames and33 captures,
including reset to the initial tomb. [Joined route summaries](debug-route-summary.json)
and [raw journals](debug-r03-journals.zip) identify each camera and successful
submission. These selected frames were opened and inspected:

| State | RT Pipeline | RayQueryCompute |
|---|---|---|
| Return passage, full owner | [View](gallery/pipeline-return-passage.png) | [View](gallery/query-return-passage.png) |
| Passage,84,219 triangles absent | [View](gallery/pipeline-detail-unloaded.png) | [View](gallery/query-detail-unloaded.png) |
| Second ascent, complete owner restored | [View](gallery/pipeline-forest-restored.png) | [View](gallery/query-forest-restored.png) |

Frame1,271 is the partial-owner view; frames1,181/1,226 are still full. The inspected
dynamic views show no obvious wall, light, floor-reflection or restored-tree loss.
These different poses are not a same-frame full-resident pixel-equivalence test.

The initial r01 focus failure and accidental r02 Pipeline repeat saved under a
query-named folder remain retained as failed or mislabelled evidence. The first
synthetic cutover test also failed: its helper wrongly required no pending TLAS
definition after recording a fresh frame. It now requires that exact pending
current-frame definition and address; production assertions were not relaxed.
Only the explicit real backend identities establish Query
coverage. The r04 pair above includes the final transmission and diagnostic-fallback
guards. Debug captures are image/route evidence, not performance measurements.

Physical-phone streaming, Current/Higher and saved Lower-to-Current migration,
complete owner route/feel and sustained mobile performance remain pending. This
checkpoint introduces no new audio or haptic semantics; manual revalidation is not
required by this change alone. Existing chapter audio/feel acceptance gaps persist.
No release, merge, quality reduction or shader-budget increase is authorized here.

The owner's subsequent direction is a reusable engine loading system for connected
regions, towns and dungeons, validated with the existing chapter rather than new
world art. Prioritize independent region allocation/BLAS ownership, shared-resource
dependencies, measured memory/overlap budgets, location/prefetch/retirement rules
and reliable reverse travel. Enlarging this particular tree selection alone is
not the requested architecture milestone. Coarse world/distant landmarks remain
visible and uncertain RT contributors remain retained.

The owner route for this slice is: claim the lantern, ascend, visit the lookout,
return and descend, walk back into the coloured passage and turn around, then
return to the rope and ascend again. Check continuous movement, floor reflections,
shadows, restored trees and equipment; stop/report any wait, hole or hitch.
