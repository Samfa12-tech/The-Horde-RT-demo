# Connected-region terrain milestone

The owner clarified that the [pinned regional atlas](../../references/horde-1.7/d37f1638/docs/design/world/dungeons/01-connected-region.webp)
governs surrounding land in every direction, alongside the detailed local topo.
The current runtime now consumes original Blender derivative r21 rather than a
clamped sample of the smaller sheet. Regional source bytes, editable master,
recipe, import validation and support correspondence are preserved.

The macro relief retains Bellwether west of the river, the southern lake basin,
eastern terrace and northeast ridge. The exact transform is unchanged. Outside
the reviewed East -150..410 / North -510..440 metre rectangle, provisional coarse
land continues beyond the unchanged 10 km primary-ray limit; this is explicitly
not extra approved geography. Distant water **surfaces**, landmark structures and
forest population are not completed by generating their underlying ground.

## Geometry and unchanged contracts

- 34,050 triangles / 17,588 exported vertices, versus 39,618 / 21,134 previously.
  The old runtime manifest ceilings remain unchanged. No shader, texture-layer,
  primitive/material capacity, quality or ray-distance limit was raised.
- All 15,472 triangles in the fine chapter core, all route stations and three
  openings match r19 exactly. Nine occupied tomb volumes and the enclosed entry
  roof remain clear. The 2.4 m trail and accepted landing stay in place.
- Stitched 4/8/16 m rings provide regional land without extending thin shaft
  grid lines across the world. Every surface seam has opposing shared edges;
  only the intentional apertures and closed outer boundary have single edges.
  This is fixed source density, **not dynamic LOD or GPU streaming**.
- Existing two opaque materials and all platform texture bytes are unchanged.
  The native imported triangles supply authoritative support. There is no hidden
  replacement floor and no terrain curtain extruded through the tomb.

## Inspected native RT gallery

These are actual Windows RT storage-image captures, Debug, Diagnostic High,
960×540 at 100%, twelve settling frames. No foreground input was sent. All five
new images and the earlier north comparison were opened and inspected. The
matched west Pipeline and QueryCompute captures both pass the full capture gate.
They are visually consistent but **not byte-identical**: 4,784 changed channels,
maximum channel difference 18. That is not phone parity or final visual acceptance.

| View | Earlier local terrain | New regional terrain |
|---|---|---|
| North / behind the rope | ![Earlier north](before-north.png) | ![Regional north](north-pipeline.png) |
| West | ![Earlier west](before-west.png) | ![Regional west](west-pipeline.png) |
| East | — | ![Regional east](east-pipeline.png) |
| South | — | ![Regional south](south-pipeline.png) |
| West, QueryCompute | — | ![Regional west Query](west-query.png) |

The north image fails the strict `armPixels=0` check. East/south instead fail
the claimed-reward player/lantern visibility check (lantern-body pixels zero);
their exact messages and state are retained. They show rendered terrain but are
**not completed capture acceptance**; no exemption or assertion change was added.
These capture/equipment gaps remain open. The historical images use the earlier
candidate identified in [its receipt](../chapter-live-terrain/receipt.json), not
a recreated claim of identical executable bytes. New identities, backend shaders,
successful-frame evidence where available, settings and errors are in
[receipt.json](receipt.json).

Observed: land now continues across the views that previously exposed the short
sheet edge. These views also plainly show insufficient forest density and the
old overhead rope structure. No finished-woodland claim is made. The full route,
lookout sightlines and final foliage/water/landmark visibility remain to inspect.

## Validation and cost

- Failing native regression against r19: 38 missing-coverage assertions. With
  r21: 15,617 chapter journey cases, zero failures, including repeated traversal,
  readiness and the new regional probes.
- Debug affected CTests: 6/6 pass, 21.56 s. Release: initial five pass plus one
  missing executable; that fixture was built and passed 1/1 in 2.00 s. Failed
  and not-run logs are retained under ignored reports; no test was waived.
- Export validator: 34,050 triangles correspond exactly to support; 2,781 capsule
  samples; maximum route-height error 0.081706 m; 57 actual regional triangle
  coverage probes; all nine room exclusions and thin-diagonal negatives pass.
- Authoring unit tests: 5/5 pass. The initial test used an unjustified one-metre
  delta between arbitrary far samples; it was replaced with an exact analytic
  relief check plus a non-clamping check. Initial failure is retained.
- Khronos glTF validator: zero errors, zero warnings. Native importer admits
  the candidate under the **unchanged previous manifest limits**.
- Android Debug build, Java unit lane and lint pass (59 tasks, 1m17s); actual
  APK/world/strict-ASTC package checks pass. This APK has not been installed.
- West Pipeline capture reports production-prop BLAS 7,547,008 bytes, static
  vertex bytes 12,976,128 and index bytes 2,231,148. Twelve-frame average GPU
  command-buffer time is 4.526 ms; this is a short diagnostic sample, **not a
  sustained performance result** or a mobile budget. Full records are retained.

The failed r20 sandbox export and original sources/backups are preserved. No
editable source is packaged. Audio/haptic manual revalidation required: **NO for
this geometry-only milestone**; the broader chapter's changed speech/mix review
remains pending. Device/owner acceptance, denser colliding forest, visible Kit,
zone resource retirement, moon alignment and complete chapter presentation remain
open. Mist issue28 remains deferred.

Next physical check, once a candidate is authorized: turn through 360° at the
rope landing, walk both sides of the clearing and the bends to the lookout, look
down into the regional valley, then backtrack and descend. Check ground continuity,
slopes and equipment. Phone testing must repeat this with actual RT/lifecycle
evidence; host results do not substitute for it.
