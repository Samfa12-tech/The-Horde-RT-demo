# Kit authored step-turn proof — review only

Later [bounded sole cleanup and native export](../2026-10-23-kit-turn-native-export/README.md)
retains this first proof unchanged, adds a separate editable take and reduces
support drift to 6.856 mm. This page/receipt/videos describe the original
`0cf8829c` snapshot; the authoring recipe now additionally offers an opt-in cleanup.

Base `f6eab6c20aa97ee0daff2789267393564bfb287f`, existing PR27 worktree.
**Production motion/bindings are unchanged.** This is a separate editable motion
study, not an admitted runtime asset or a completed planted-turn fix.

## Deliverables and inspection

- [Editable Blender source and reproduction instructions](../../../assets/models/characters/source/kit-step-turn-proof-v1/README.md).
- [Left motion preview](left-authoring-preview.mp4) and [right motion preview](right-authoring-preview.mp4).
- Foot-level witnesses: [left](foot-left-0013.png), [right](foot-right-0013.png).
- [Compact source/build/contact receipt](receipt.json).

The two independent 45-degree takes last 0.75 seconds; video adds still holds
before/after, without synthesizing intervening motion. Actual generated full-body
sequence frames, both endpoints and close foot views were inspected. Workbench
uses plain studio shading to expose pose/contact: **these are offline authoring
previews, not native RT game captures**. Artistic owner/video acceptance is open.

The study uses the existing 24-joint rig. One foot steps while the other supports,
then the feet exchange roles. A small pelvis weight shift and foot-aligned knee
plane are baked offline; no engine IK or general animation system is added.
The explicit preview root yaw curve is paired with the bone keys. Exporting only
one of those, or applying yaw twice, would produce an incorrect take.

## Measured result and limits

The reopened saved file is sampled at **120 Hz, 91 samples per direction**.
Contact is measured on the actual deformed semantic sole vertices (39 left /
38 right within 6 mm of the starting sole minimum), including original mixed
leg/foot weights. This is stricter evidence than checking an ankle target alone.

| Measurement | Left | Right |
| --- | ---: | ---: |
| Maximum support-sole horizontal drift | 12.125 mm | 12.125 mm |
| Maximum all-vertex Idle-zero endpoint error, root yaw removed | 0.000403 mm | 0.000389 mm |
| Required geometric leg stretch | 0 | 0 |
| Approximate swing clearance | 56 mm | 56 mm |
| Imported bind matrices / geometry / weights | unchanged | unchanged |

The take visibly steps rather than using forward Walk at rest. It is useful as
an editable proof, but **12.1 mm of support-sole deformation remains**. Do not
claim world-space planted feet from the fixed ankle goal or call the residual a
pass. Idle-compatible endpoints were established against **Idle phase zero**;
arbitrary idle phase, speech/recognition overlays, repeated/aborted turns, slopes,
native GLB import and actual RT joins are not established by this study.

Retained failed work: r0 mistakenly composed the root yaw twice for the feet;
its approximately 154.6 mm endpoint mismatch and 113.1 mm support drift rejected
it. Correcting that authoring coordinate error produced r1 (14.08 mm saved-take
support drift). One bounded knee-plane refinement produced the current 12.125 mm
result. All raw recipes, failed `.blend` files, JSON and logs remain under ignored
`reports/kit-step-turn-proof-20261023/`. No further motion tuning loop was run.

The portable source recipe was rerun in a fresh directory; its geometry/bind
identities and sampled authoring metrics match the retained take. Native
`horde_rt_kit_source_intake_tests` passes once in 7.17 s for the **unchanged original
source/runtime pair**. It does not validate these new Blender keys. No full suite
was rerun for this offline study. No native runtime GLB was exported or admitted.

## Minimal integration boundary, after review

1. Review the clips and remaining sole drift before choosing further contact
   cleanup or accepting a stated tolerance. No new tolerance is silently granted.
2. Bake the approved take into versioned native clip data while retaining original
   geometry/weights/inverse binds and original clips. Validate coordinate/pose
   correspondence and apply the declared root yaw **once**. Preserve the existing
   pinned-master intake guard; this needs a distinct derivative recipe.
3. Add explicit turn bindings to the current four-slot clip interface rather
   than mislabeling turns as Attack/Dead. Publish bounded turn phase/direction
   through the shared simulation snapshot and both serial/optional-worker cache
   paths, retaining one skin pass. Use complete supported foot exchanges and
   hysteresis before changing direction; partial-angle coverage is unproven.
4. Evaluate turn-then-walk with a short acceleration, reusing the existing Walk
   and distance phase. This avoids requiring new backward/strafe starts for the
   first slice. Keep 1.8 m/s cruising as the current baseline; instant full-speed
   departure is implementation behavior, not a found locked owner requirement.
   Any deliberate departure delay must preserve dialogue/proof rules and be
   checked against spatial VO, tether/obstacles, pause/restore and endpoint saves.
5. Reuse the retained tether departure and turn RT witnesses plus focused native
   rig/lifecycle checks. Native/phone/continuous gameplay quality and cost remain
   separate acceptance work; this proof supplies none of those results.

## Custody and provenance

Original master SHA-256 remains
`47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e`, runtime remains
`1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713`.
Geometry, weights and imported bind data are checked before/after authoring.
All 151 parked owner files are preserved. The new source stays outside runtime
package paths; no textures, geometry/BLAS counts or renderer work are added to
the game. The existing game-dev CLI is unavailable; no canonical CLI package
verification is claimed. Available Blender 5.2.0 LTS and FFmpeg produced the proof
locally; no provider, paid generation or licence change occurred.

Audio/haptic manual revalidation required: **NO** for this offline-only source
study. No phone, foreground activation, production binding, merge or release.
