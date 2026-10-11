# Planted pose renderer and mount continuation — 12 October 2026

Source parent: `91a68a58288b9d0654ed61a1628bf180b693372c`.
This connects the previously proven stationary native constraint to the player
render slot and its Hips mount. **Positive use is still inactive in gameplay.**
The overhead apparatus remains; this is a prerequisite for its physical
replacement, not a completed graveyard or a visually accepted folded traversal.

## Behavioural reproduction and implementation

The [corrected-fixture baseline](planted-renderer-current-before-test.log) runs the
new native test against the actual parent render-slot implementation. That
implementation ignores the requested stance, publishes no planted leg sockets
and reports 115 mm left / 113 mm right palm errors, above the unchanged 15 mm
tolerance, at the low targets.
Earlier fixture attempts are preserved and labelled below; they are not evidence
of production equipment failures.

`PlayerAnimationSnapshot` now carries the optional stationary pelvis drop.
`PlayerRenderSlot` forwards it into the single native palette used by body and
viewmodel. Shoulder and Hips preparation apply the same native torso translation,
so equipment composition can obtain its mount before the arm solve without
another skin/palette evaluation. Ordinary animation explicitly clears the field;
no new movement control, camera policy or rope route is enabled.

Reversible native bounds remain Idle only, `[0, 0.72]` m pelvis drop, setback
along model -Z at 0.25 times drop. These are engineering pose bounds, not new
owner-approved balance. Invalid stance requests reject before cadence reuse and
invalidate the old solved palette. Moving mount requests leave caller output
unchanged. Zero retains normal-play output.

The [final native test](planted-renderer-current-after-test.log) covers fifteen
stance/heading combinations, actual imported low palm targets, planted ankles,
the minimum height of the actual skinned mesh, same-tick 30 Hz refresh,
pre/post-solve mount identity, agreement with the solved Hips and the production
stowed-sword transform. It also covers invalid cached-pose rejection and ordinary
animation step/reset clearing. Model +X retains anatomical Left. The native
test checks a flat grounded support; it does not certify contact with the shaft
ledge, armour/equipment clearance against scenery or a moving pull-up.

## Checks and exact identities

- Windows Debug and Release applications/affected targets rebuilt. Seven
  affected CTests pass in [Debug](planted-renderer-debug-current-tests.log) and
  [Release](planted-renderer-release-final-tests.log): animation, directional rig,
  arm reference, native character smoke, viewmodel admission, rescue rig and
  adversarial rescue regression. No assertion/deadline changed.
- [Android](planted-renderer-android.log): four native ABIs rebuilt;
  assembleDebug, Java unit-test and lint tasks succeeded. Unchanged Java/lint
  analysis was UP-TO-DATE. No phone installation or execution.
- [Closed package](planted-renderer-package.log) and
  [held-item/source separation](planted-renderer-held-item-package.log) checks pass.
  Existing asset bytes, source/backup files, three actors, 18 texture layers,
  shader words and frozen budgets are preserved. The prior full torch sweep and
  its unchanged 180-second policy are retained, not claimed as newly executed.
- [Parent exact-commit CI receipt](ci-91a68a58-verified.json): all six jobs pass.
  [Selected log evidence](ci-91a68a58-required-tests.txt) proves Vulkan-host 37/37,
  all seven requested traversal CTests and viewmodel admission ran; MSVC 132/132
  includes the report UI test in 2.50 seconds. This parent result does not certify
  the subsequent source. Its exact pushed CI result is supplied through PR27 and
  the task handoff after execution.
- [Exact source-file, asset, executable, APK and budget hashes](identities.json).
  The legacy build label remains 1.6.2; these are unreleased PR27 development
  artifacts, not altered released 1.6.2 packages.

## Inspected normal-play RT frames

Both current frames were opened and inspected: RTX 5050 Laptop, 960×540,
100% scale, Diagnostic High, twelve frozen settling frames, checkpoint
`chapter-landing-north`. Arms, raised lantern, floor shadows and the still-present
overhead frame match the parent. [Pixel comparison](image-comparison.json)
records zero changed pixels for each backend. Full camera/state, shader,
resource, submission/fence and available short timing facts are in the manifests.
These are compatibility captures, not sustained performance or positive-stance
visual evidence.

| Backend | Inspected current frame | Exact manifest |
| --- | --- | --- |
| RayTracingPipeline | [Normal play](pipeline-normal.png) | [Manifest](pipeline-normal.json) |
| RayQueryCompute | [Normal play](query-normal.png) | [Manifest](query-normal.json) |

## Required next integration

The apron begins at z=-14.95, top Y=2.05. A ground-level stake cannot be installed
by changing the anchor coordinates alone: the old anchor-relative root clamp
would make the rim staging point unreachable. Rope collision also currently
omits the apron. The replacement must establish a real supported stake, a
rope-radius-aware rim contact, the hanging span and its longer loose tail,
separate reachable ascent/pull-up/descent, and actual foot/equipment clearance.
Publish the same stance through immutable traversal, camera/capsule and lantern
hinge/light authority before enabling it. Preserve readiness, pause, reset,
stale callback and repeat traversal tests; inspect the resulting live pose on
both hardware RT backends. The exact final stake layout is not accepted yet.

Graveyard composition, genuine two-way GPU zone preparation/retirement, complete
route gallery, sustained measurements and owner/phone acceptance remain open.
Mist issue28 and replacement tree generators remain deferred. Phone Current /
Higher and saved Lower → Current migration remain physically pending.

Audio/haptic manual revalidation required: **NO for this inactive renderer/mount
change** (event-time source/listener and playback remain unchanged); **YES remains
pending for the wider chapter's changed cues, dialogue and contact timing**.

## Preserved investigation failures

The first fixture left the inactive OriginalTorch attached and constructed
non-rigid imported Grip axes; the second omitted the fully-stowed grip state and
passed Hips directly where the sword's composed stow mount was required. Their
logs are retained as `fixture-*`, not attributed to production gameplay. A
restore of the temporary baseline source preserved its old timestamp, so the
first after-build reused the baseline object; the explicit current-source rebuild
and final logs above supersede it. No file-history/source backup was overwritten.
