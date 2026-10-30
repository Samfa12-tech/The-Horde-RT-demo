# Moving-frame static handoff and chapter capture admission

Parent source `c8ee094b5f2084e35f616547be3ccc07d65aabcb` on the same PR27 branch.
The chapter is incomplete. This is an integrated diagnostic capability,
not normal-play streaming: **residency savings remain zero**.

A prepared TLAS records preparation-time poses. Ordinary camera, held-item,
actor and rope poses can change before adoption. The new combined operation
requires completed prior references, publishes descriptors transactionally,
then records the actual first frame's dynamic TLAS and trace before retiring the
old owner. Strict snapshot-only cutovers keep their original pose guard.
Failed/throwing, absent, wrong-world or stale recordings retain both referenced
owners and stop readiness; descriptor failure still rolls back before recording.
Successful command recording remains distinct from successful submission.

The Windows diagnostic drains completed prior work and resets prior commands
before starting the new evidence token/command, then records through the normal
RenderFrame seam. It changes yaw by 0.025 radians using zero-delta authoritative
publication after preparation, adopts, submits/presents, restores the frozen
view, and repeats. It does not advance live combat to make a capture pass.

## Inspected native evidence

RTX5050 Laptop, Diagnostic High, HIGHER shadows, 100%/960x540,
`chapter-landing-north`, both actual hardware RT backends. All four replacement/
restore comparisons have zero changed pixels. These images were opened and
inspected; the safe apron, open shaft, equipment, grave markers and shadow
placement remain intact. The sparse supplied canopy remains owner-deferred.

| Backend | Before | Adopted owner | Restored | Exact state/cost |
|---|---|---|---|---|
| Pipeline | [Before](pipeline/cutover-00-before.png) | [Adopted](pipeline/cutover-01-selected.png) | [Restored](pipeline/cutover-02-restored.png) | [Manifest](pipeline/capture-manifest.json), [handoffs](pipeline/static-zone-cutover.json) |
| QueryCompute | [Before](query/cutover-00-before.png) | [Adopted](query/cutover-01-selected.png) | [Restored](query/cutover-02-restored.png) | [Manifest](query/capture-manifest.json), [handoffs](query/static-zone-cutover.json) |

Each actual replacement owns 69,234,240 bytes, with 159,979,264-byte scratch-inclusive
owner overlap. Device-local scene bytes return from 474,659,280 to 405,427,664;
textures and unchanged BLAS stay shared. Pipeline preparations take about 2.17s
across 357 frames, cutovers 41.36–43.54ms. Per-hop Query costs are retained in the
receipt. These bounded Diagnostic captures do not establish sustained performance
or frame-hitch acceptance. No source, geometry, mask, quality or shader budget was
cut for this proof; neither completed recordings nor those timings prove eviction
of any still-relevant ray/light contributor.

## Android capture defect and validation

The failed physical chapter request never reached the native chapter checkpoint:
Java rejected the new names and retained the previous live-start report. The
new failing test reproduces that rejection. All 22 authored chapter IDs and
camera poses now resolve; the test reads the shared native roster so omissions
and view drift fail. Unknown names remain rejected. Native staging remains in
the shared simulation, not Java gameplay.

- Debug/Release applications and the affected two native CTests pass in each.
- Android four-ABI build, lint and 286 Java tests pass with zero failures/skips.
- Exact closed runtime/ASTC and held-item package contracts pass.
- RED and GREEN evidence and exact source/EXE/APK identities are preserved here.
- New identified Debug APK is not installed: the owner took the phone before
  retry. Current/Higher, saved Lower→Current migration and phone Query/capture
  validation remain pending. No deadline is extended.

Normal-play two-way preparation/readiness, safely omitted-zone resources,
complete route/sustained evidence and final owner/device acceptance remain
required. This operation alone is not streaming or complete chapter acceptance.
No merge, release, signing, main import, capacity expansion or optional mist/
canopy work occurs. Parent c8ee094b passed all six exact-commit CI jobs in
run38208151214; this milestone's exact commit is verified separately after push.
