# Kit speech and lantern acting - 20 October 2026

Kit previously used only idle/walk during all conversation and recognition. The
accepted [animation plan](../../KIT_ANIMATION_PLAN_1_7.md) requires restrained
speaking and a response to a fresh visibly raised lantern. This slice adds small
shared-simulation head/chest acting over the existing admitted clips and corrects
a separately verified model-facing defect. No new asset, clip download or spend.
[Exact tested sources, artifacts and scope](receipt.json).

## Implemented behavior

`CompanionActing` runs on the existing 60 Hz simulation owner and publishes transient
angles in the immutable companion snapshot. Active visible conversational Kit
lines get small head/chest movement, with smaller head-only speech while walking.
Only the existing director's live false-to-true accepted-proof transaction starts
the 1.2 s recognition nod. The existing fresh manual edge and successful current
presentation gate still owns proof; animation cannot accept or advance it.
Grate/rescue calls stay unseen and Keeper lines never animate Kit.

Pause freezes pose and recognition time. Speech skip/interruption blends toward
idle; loss of a valid living grounded exterior body cancels recognition. Route
reset and both campaign restore destinations clear transient acting. Restored
accepted story or active proof speech can resume its talk but cannot replay the
recognition beat. There are no new save fields, dialogue durations/completions,
semantic audio/haptic events, event-time source changes, route roots or controls.

The native indexed PBR path accepts the existing generic additive-node rotation
span, using Head and upper-chest Spine in this rig. It retains the same one-pass
skin/tangent/repack and GPU mesh/BLAS lane. Renderer cache identity includes acting
and commits only after successful pose/upload. Neutral/empty-span output remains
byte-exact; invalid node/quaternion requests preserve the last admitted actor.
No per-vertex post-skin warp, new mesh/texture, extra actor or quality reduction.

## Facing and clipping diagnosis

The actual idle-zero Head-to-headfront vector confirms imported +Z forward.
Logical forward is (sin yaw,0,-cos yaw), but the old model rotation yaw+pi mapped
it to (-sin yaw,0,-cos yaw), reversing sideways headings. At the lookout Kit's
logical yaw -pi/2 should face the player to -X; the old model faced +X. Model yaw
now uses pi-yaw, preserving proper rotation, metre scale and root position.
Four cardinal native-rig cases prove heading agreement and determinant +1.

The earlier **left-edge crop is camera composition**, rather than established
mesh intersection: Kit sits 1 m along +X while the saved village-facing yaw is 2.51.
He is about 53.8 degrees left; the 1232x803 shader view has about 103 degrees horizontal FOV.
The approximate equal-ground head projection lies beyond the left edge. The
ordinary new saved-view capture still shows peripheral crop, as expected. No Kit
teleport, player/camera forcing, FOV widening or scene-quality edit hides it.

A separately labelled render-only inspection puts the camera 3 m in front of Kit
and coherently rebases player attachments using the existing pure adapter. It
does **not** use the environment survey's fog override. Neutral, speaking and
recognition recipes share camera, idle phase, lighting and quality; only the
render-input acting recipe changes. Actual simulation/story/acting stays neutral.
These are seeded pose inspections, **not live fresh-raise acceptance**.

## Validation and original captures

| Gate | Actual result |
| --- | --- |
| Shared/native regressions | 12/12 Release suites pass in 31.95 s: acting, dialogue, actual-rig intake, rescue/imported terrain journey, codec/checkpoint/play-session/session, pose batching, geometry ownership and camera adapter. Includes 150 acting, 2283 codec and 1026 checkpoint checks. [Run](kit-acting-native-tests.log), [details](kit-acting-native-tests-detail.log). |
| Live host story gate | Existing actual-terrain ordinary-input round trip now checks no recognition before accepted proof and observes the reaction after the real shared acknowledgment transaction. 5788 moving-probe frames; CPU acknowledgments are not physical GPU/phone acceptance. |
| Actual native rig | The 2,071 tested boot/low-shin vertices across both sides remain byte-exact, along with topology/UV/material/node contracts and finite unit tangent frames. Peak speaking displacement 0.011135 m, recognition 0.0405345 m. Neutral exactness, missing-node/NaN/Inf rejection and all 7 clips/35 samples pass. These are model-space/host facts, not artistic feel. |
| Windows RT | 30/30 checks pass in 21.75 s, 10 real RT frames. Three pose images have owned completed Pipeline submissions 3/4/5 at simulation tick 2, then ordinary camera/pose restores and Load/panel/Return regression completes. [Raw native receipt](runtime.native.json), [log](hidden-rt.log). No performance claim. |
| Guards/custody | Release, unbound inspection and old campaign+inspection mode reject exit 2 before Vulkan/window creation. Isolated record/manifest/lock and owner INI hashes remain identical; zero write attempts. Window stays hidden, process retires/exits; 107 foreground samples (83 retained) stay null and never observe owner activation. [Custody](custody.json), [guards](guards.json). No active external foreground window is available to attribute. |
| Builds | Windows Debug/Release and all four Android Debug ABIs pass. APK SHA 4af1c097ae224adb42478e34f570e28416586443d168e5753ca64bc7758e1fa3, 186150965 bytes; build only, no install/launch. [Native](kit-acting-native-build.log), [Debug](kit-acting-windows-debug-build.log), [Android](kit-acting-android-build.log). No Java UI source change or new Java acceptance claim. |

All four original RT images were inspected. The diagnostic front view shows Kit's
whole body and boots in frame, facing the camera; recognition gives a restrained
head/chest change. The view remains dark/noisy. Ordinary village-facing capture
still crops Kit at left. Final lighting/composition, detailed mesh-intersection
and walking/turning/phone acceptance remain open. Original PNGs stay only in
`reports/horde-campaign-fixtures-kit-acting-20261020-r1`; their hashes are in
[raster-analysis.json](raster-analysis.json), with no large generated Git assets.

Exact RGB analysis finds 4558 speaking and 6494 recognition changed pixels, with
2,601/3,440 changes in the manually selected head-region box and zero changes in
the static sky control. Shadows also change outside the body. Resource
inventories match across the three poses. These observations support that native
deformation reaches the RT image; they do not certify acting quality or savings.
Pillow performs the analysis without resampling or modified image output.

Reproduction uses a new empty canonical direct reports child named
`horde-campaign-fixtures-*`: export via the Release play-session test executable's
`--export-lookout-fixture <absolute-directory>`, then run Debug `HordeLanternRT.exe
--lookout-validate-hidden --lookout-inspect-kit --campaign-test-root
<absolute-directory>`. The existing bounded, nonactivating, read-only validator
owns GPU retirement and the exact current RT proof; the older combat/death runner
is never invoked. No in-run retry or hidden state repair occurs.

## Next planned gap and limits

The next bounded host gap is to measure the admitted walk stride against the
existing 1.8 m/s route and inspect idle/walk start-stop joins and turn-foot pivots
before changing motion speed or adding clips. Dedicated listening/hand gestures,
live native speech/recognition sequencing and owner visual/VO feel remain partial.
Do not claim this finishes the entire animation matrix.

The phone remains released after Sam's unplug request: **zero phone operations**
this slice, installed c5a11bb2 and saved mobile Mist Off untouched. No denied
physical combat action, Keeper preparation/arm/timing, heavy world content,
public push/merge/release, process kill, budget/security change or Reef operation.
Existing unrelated inventory/source-pin failures remain separately recorded;
this is a selected lane, not a full repository pass. Historical failed native
panel/Resume harness observations remain in their earlier receipts.

Audio/haptic manual revalidation required: **NO for this pose-only change**.
Semantic events, listener/source data, transport, playback and gains are unchanged.
The previous endpoint restore listening gate remains open. Current builds and
seeded RT views do not close naturally earned saves/death/full-route, physical
touch/controller/comfort or sustained warm performance acceptance.
