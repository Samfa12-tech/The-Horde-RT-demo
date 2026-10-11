# Live topo terrain and forest-ground correction — 11 October 2026

This is an intermediate chapter milestone, not chapter completion. It replaces the old ribbon support/render surface with the same imported Blender terrain used by the shared simulation. The roughly 76 m trail, descent/dip/rise, nine occupied-room exclusions, three distinct openings and lower/upper rope interfaces remain. Whole-body/headroom collision and finished dressing are not established by support probes.

## Actual changes

- Versioned editable r19 preserves r18 geometry/support exactly (excluding its source-GLB digest). Forest Ground 01 replaces the unsuitable masonry bank map at its published 2 m width. Damp-sand trail remains. Original source PNGs, provider hashes, CC0 provenance and all prior masters/candidates are preserved.
- Two existing static-owner primitive slots carry 39,618 terrain triangles. High uses 32 primitive records and 30 static material records plus the existing two world rows. Mobile uses 31 primitive records. No capacity, shader, scale or quality limit changes.
- Exact original prop mip bytes are selected and two terrain layers appended into the same GPU arrays: 17 base / 14 normal / 14 ORM, below the unchanged 18 ceiling. No extra descriptors, shaders or full-atlas duplicate. Appended data is validated for format, DFD, mip bounds, overlap and capacity before publication. Additional format upload bytes are 33,554,424 Windows / 5,301,408 Android; these are not driver allocations or performance claims.
- Renderer-ready publication binds the actual immutable triangle surface to the current simulation generation. Missing/failed/stale preparation cannot admit the destination. Scene move/destroy ownership is tested; platform rebuild/destruction invalidates readiness before replacement. GPU residency is still shared: this does not implement streaming/retirement.
- Existing forest trees query the same terrain for grounding. This is only ten-tree early dressing, not the complete wooded chapter.

## Inspected native gallery

All frames are hidden, no-input native RT captures at 960x540, 100% scale, High optical diagnostic profile. Exact backend, shader, executable and package identities are in receipt.json. The frozen landing is staged through actual fixed-step rope deployment/ascent after renderer readiness, not a camera teleport.

| View | Evidence | Result |
|---|---|---|
| Same north view before correction | [Masonry bank](before-masonry.png) | Incorrect broad stone appearance reproduced |
| Same north view after correction | [Pipeline](north-pipeline.png), [QueryCompute](north-query.png) | Forest ground replaces masonry; shaft stone retained |
| Landing sweep | [East](east-pipeline.png), [South](south-pipeline.png), [West](west-pipeline.png) | Ground surrounds landing; sparse dressing/unfinished anchor remain visible |
| Shaft | [Shaft](shaft-pipeline.png) | Opening retained; complete downward/return inspection still pending |

The west capture completed. North/east/south/shaft frames were written but failed the unchanged dedicated-arm pixel assertion (`dedicated=1, visible=1, armPixels=0`). They are inspected diagnostic frames, not accepted capture passes. The older entry arm failure and initial material-sharing process crash remain in historical evidence. No cropping exemption or assertion weakening is introduced here. The five landing views are not a full route/Kit/return gallery.

The broad terrain no longer resembles masonry. The captures also show unfinished sparse vegetation, the obsolete overhead anchor frame, the coarse shaft apron/lid and a high-frequency ground appearance under the current night lighting. Moon/light alignment, final composition, Kit and true zone lifetimes remain required. Static twelve-frame GPU timings in the receipt are observations only; no sustained/mobile performance claim.

## Validation

- Windows Debug affected tests: 6/6 PASS, 60.15 seconds.
- Windows Release affected tests: 6/6 PASS, 8.61 seconds.
- Android Debug build/lint/unit tests: PASS, 59 Gradle tasks, 283 Java tests, zero failures/errors/skips. No installation or physical run.
- Closed package/staging negative suite: 176 cases PASS. Actual APK held-item and world/strict-ASTC package checks PASS.
- Independent review caught missing terrain move ownership and stale readiness during renderer replacement; both corrected and reviewed. Native ownership and actual-route tests passed afterward.
- Earlier development-registry negative boundary failed because 193 became a valid capture; it now rejects the genuinely unassigned 198, while all earlier negative/lookup coverage remains.
- Additional route/rig/torch/shader regressions: 8/8 PASS, 276.10 seconds. Original torch gate passed in 171.59 seconds within its unchanged 180-second policy. Shader gate passed in 38.84 seconds. Exact-commit CI remains pending push.

## Remaining gates and next owner route

Whole chapter remains incomplete: full collision/headroom and dressing, graveyard/lid/stake anchor, visible Kit, safe third-slot ownership, real two-way GPU resource handoff, moon/shadow direction, final route gallery, lifecycle and fresh audio/haptic acceptance. Mist issue28 is deferred. No merge/release or production renderer adoption is implied.

When a complete candidate is ready: start in the tomb, check portal/niches/guards and captions; defeat Keeper, claim lantern, climb; turn fully around on the landing and look down the shaft; raise the lantern for Kit, follow all bends to the lookout; return, descend, and climb again. The connected phone still has the historical WP2 Debug APK; an explicitly approved update is required before candidate Android evidence.
