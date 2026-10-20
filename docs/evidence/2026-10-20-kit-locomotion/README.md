# Kit locomotion joins - 20 October 2026

Kit now keeps a distance-driven gait phase across movement edges and blends the
admitted idle/walk joint poses over 0.18 seconds. This removes the hard reset at
starts, stops and the one bookkeeping tick between route segments. The 1.8 m/s
route motor, its coordinates/waypoint timing, stationary yaw policy, gameplay,
dialogue, checkpoints and serialized saves are unchanged. Both restores/reset
clear this transient visual state; pause/zero-delta finalization freezes it.
[Exact source/artifact/custody receipt](receipt.json).

The indexed PBR path blends local joint TRS before its existing single vertex,
normal and tangent skinning pass. Weight 0/1 retains exact legacy endpoint bytes,
including existing speech acting. It does not interpolate skinned vertices, skin
another mesh or allocate new GPU actors. Renderer cache invalidation includes
phase/weight and commits after successful upload.

## Measurement and actual result

The admitted walk lasts 0.833333313 seconds. Offline skin-weighted boot-centroid
stance fits across 120/240/480 samples and 10/20/30 mm contact proxies imply a
1.56026-1.56104 m full cycle and approximately 1.873 m/s at 1x. The existing clip was
already close to the route. A rounded 1.56 m visual cycle gives approximately 0.962x
playback at unchanged route speed. Independent native foot/toe-node fits yield
1.996/1.807 m/s and a 1.585 m mean cycle. Different contact proxies and foot roll
explain that modest difference; this is not precise biomechanical foot locking.
See the [offline method and limitations](offline-measurement.md), retained
[240-sample sensitivity result](walk-240.json) and native rig log.

| Actual native same-root transition | Previous hard-reset peak boot displacement per 60 Hz frame | Blended peak |
| --- | --- | --- |
| Stop, retained 12-frame window | 0.578439 m | 0.0527043 m |
| Restart, retained 12-frame window | 0.202567 m | 0.116409 m |

These maxima include identical route translation/yaw on both comparison sides
and use the same real floor-normalized Kit mesh. They show reduced joins in this
fixture, not a universal contact bound or owner feel acceptance.

Straight walk as a stationary stepping pivot was rejected before implementation:
its stance foot sweeps backwards around 1.8-2 m/s without compensating root travel.
Adding yaw cannot lock both feet. Current turn captures honestly retain the idle
swivel. A turn recipe/clip or bounded foot constraints needs separate work and
contact evidence; this slice does not claim that problem is solved.

## Verification

- **17 unique Release host suites pass**: the actual rig suite plus 16 integration
  suites covering movement/acting, dialogue, campaign records/sessions/codecs,
  actual-terrain journey, generic skin/node poses, player rigs and frame adapters.
  There are 2278 locomotion checks; the flat-support route takes 2254 ticks. Generic
  blend endpoint, intermediate normalized frame/topology, invalid-input/last-good
  asset preservation and all seven existing clips / 35 source-runtime samples pass. This is a selected
  lane, not a full repository or physical-device gate.
- Windows Debug/Release and build-only Android Debug for all four ABIs pass.
  APK SHA-256 `eca28ac146bd094af3938b9835da131bd619f58949a54173efbe778cc2bbaf89`.
  Java source is unchanged and Java tests were not rerun. No install or launch.
- The hidden noncombat Windows runner passes **51/51 checks**, 31 successful RT
  frames in 35.235 seconds. Twenty original PNGs sample 100 walking ticks, 11 stop
  ticks and 40 turn ticks after 60 quiet fixed steps. It uses the production route
  component and actual chapter support, rendering separate immutable snapshots
  with a tracking diagnostic camera. Loaded campaign state and fixture/INI bytes
  remain unchanged, with zero attempted campaign writes. This is sequential RT
  motion evidence, not live input, naturally earned progression or full-route play.
- RTX 5050 Laptop / driver 610.47, normal 100% 1232x803, Diagnostic/High trace-rays and
  PC Mist On remain unchanged. All 20 motion frames retain 51 buffers / 60 allocations,
  20 BLAS / 1 TLAS / 28 instances and identical observed memory totals. These short readbacks
  establish no sustained FPS or phone performance gain.
- Ten original movement images were inspected: full body/boots remain visible,
  authored lifts change between strides and the stop returns progressively to
  idle. No missing/collapsed body was seen. Dark/noisy appearance, stationary
  swivel, artistic quality and physical comfort remain unaccepted.

The owned test window remained hidden and was never observed as foreground across 189
samples; the API returned null throughout, so this does not establish continuity
of a particular active external HWND. Its process exited 0. Release/unbound/old
mode guards reject with 2 as expected. First fixture export also exited 2 because
the required empty directory was missing; it created no records and launched no game.
After verifying absence and creating only that named reports directory, export
succeeded and one RT run passed. The [preparation result](preparation-first.log)
is retained, with no approval denial or uncertain process left behind.

Original captures remain under ignored
`reports/horde-campaign-fixtures-kit-locomotion-20261020-r1/`; all 24 PNG hashes
(including saved view and three acting poses) are pinned in the receipt. No APK,
executable or PNG is added to Git. To reproduce the offline analysis, copy the
preserved `measure_walk.py` into a fresh ignored reports directory, then follow
its existing bounded output rule and the command in the method note. The copied
[RT wrapper](hidden-rt-runner.py) preserves explicit fresh-directory/focus/time
and create-only fixture guards; its r1 paths are an exact run record, not a
permission to overwrite/reuse that nonempty fixture.

## Scope and next milestone

Sam's latest instruction is preserved verbatim in the receipt: mobile mist off
because the two Keeper lights plus mist were chuggy. Existing mobile startup/reset
policy and the earlier saved Off receipt agree. No phone was queried or changed;
Windows quality was not lowered. No public push/merge/release, paid acquisition,
new worktree, owner reset/stash or Reef action occurred. Canonical owner-dirty
`ASSET_LICENSES.md` and `PROJECT_MEMORY.md` retain their exact hashes.

Audio/haptic manual revalidation required: **NO**. Visual posing changes no
semantic event timing, source/listener data, playback or haptic input.

The [current 1.7 milestone map](../../design/HORDE_1_7_MILESTONE_MAP_2026_10_20.md)
reconciles already implemented regional water/pond ambience/opening dressing and
records the next bounded source slice: sparse fallen masonry chips beneath
opening damage, using existing primitive/material ownership and measured RT
cost. Natural save/death/full traversal, controls/listening and sustained warm
Keeper/exterior acceptance remain open. Healing needs the accepted bounded
policy decision; no recovery economy is invented here. Move to the story-facing
slice rather than extending this into indefinite animation polish.
