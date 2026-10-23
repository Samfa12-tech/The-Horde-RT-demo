# Physical touch HUD check and retained continuity gap

23 October 2026. Galaxy S26 Ultra **SM-S948B**, Android 17 API 37, effective
density 560 dpi, owner font scale 1.7 / brightness 115. Source `1bd99485`, ordinary
arm64 Debug APK SHA-256
`8be1c9272b2ce840083904f25f13b61e85cc13c5cf890920b3a1883f28b77228`.
Compatible installed hash and both original restore identities were verified.
No code, settings policy or graphics quality was changed during this batch.

## What passed

Independent portrait and landscape checks use the existing **staged**
`lantern-held-high` fixture, normal live UI and actual Pipeline RT presentation.
They do not prove naturally earned equipment or a journey. Portrait screenshot
1440 x 3120; landscape 3120 x 1440. Native RT internal extents are respectively
475 x 980 / 980 x 475 at the preserved 33%; UI stays at display resolution.

- Five available targets (menu, swing, parry, dodge, lantern) stay inside the
  app's measured window and do not overlap. Menu is 72 x 48 dp; combat targets
  104 x 72 dp; lantern **56 x 56 dp**. Bounds are identical before, after the held
  gesture and after the return tap, in both independent orientation cases.
- One 1500 ms injected lantern hold per orientation shows the pressed rim while
  retaining the initial arrow. After release, the visible/accessibility state
  changes from `LOWER. Lantern raised.` to `RAISE. Lantern lowered.`; one tap
  returns it. This is an observed state sequence, not a native event-count trace.
  No combat button was pressed and no prior denied combat sequence was repeated.
- Actual screenshots were inspected before/during/after the hold. The circular
  lantern, rounded text backings, full labels and sharp display-resolution HUD
  are visible over the RT tomb scene. World detail remains visible through the
  backings at the unchanged source default 70% (no saved UI overrides).
  Physical alpha was not numerically reconstructed; exact 55/70/95% alpha has
  native host pixel evidence in the preceding checkpoint.
- The contextual Interact button is unavailable in this held-lantern state and
  was not physically exercised. No dialogue line is present, so concurrent
  caption overlap is not closed. Controller/keyboard hardware and Sam's comfort,
  reachability and touch feel remain unaccepted by this automation.

The [bounds/frame evidence](bounds-and-frame-evidence.json),
[portrait action result](portrait-action-result.json),
[landscape action result](landscape-action-result.json) and
[receipt](receipt.json) preserve exact hashes and scope. Eight unedited full-size
screenshots, raw hierarchies, logs and private custody originals remain **local**
under ignored `reports/touch-hud-20261023/phone/`; large PNGs and campaign/preferences
are not added to Git. No FPS, sustained warm-play or listening claim is made.

## What did not pass

**The first portrait-to-landscape continuation lost the staged lantern control.**
The combat HUD remained visible; the target guard stopped without guessing a
coordinate or injecting a lantern action. The
[original hierarchy observation](rotation-continuity-gap.json) is retained.
It is not yet attributed to debug-fixture rehydration, production equipment state
or contextual visibility. One explicit, independent landscape restaging then
allowed the second HUD/action check above. That does **not** turn the continuity
case into a pass.

Setup also encountered the legacy ending panel, then Home/resume's normal menu.
Missing-target/page guards stopped before input. Source inspection confirmed
their ordinary Enter/Resume and Continue handlers only hide the menu/dismiss the
ending. Freshly observed controls were used; no Begin Again, restart, save, load
or new completion flags were used as natural-play evidence. A local duplicate
receipt-name guard occurred after a resume command completed; subsequent UI state
was inspected rather than repeating that uncertain command.

## Release and custody

**Phone released at 09:35:17 UTC; safe to disconnect.** Both original APK hashes
are restored. All **18** prior save/lock files and preference bytes remain exact;
no new save file was added. Original rotation/autorotation, font and brightness
are restored/unchanged. Launcher is foreground, Debug PID absent, no Horde content
surface, owned capture, install or transfer remains.

The original cleanup script's stronger assertion that *every* compositor name
containing Horde must disappear failed after the restore completed. It was not
silently marked passed and the installation was not repeated. Fresh readback
finds only an `ActivityRecord` and `ActivityRecordInputSink`, with no app process
or SurfaceView/BLAST/BufferQueue layer. The sink is a system-owned input handle
without an input channel in [AOSP's implementation](https://android.googlesource.com/platform/frameworks/base/+/refs/heads/main/services/core/java/com/android/server/wm/ActivityRecordInputSink.java).
No system task metadata was forcibly removed. The
[precise closure receipt](closure.json) preserves both the failed stronger guard
and the verified release facts; it does not claim a zero-name compositor list.

## Remaining 1.7 work and next slice

**Next executable slice:** diagnose the staged-lantern loss across Android
orientation/surface rebind using current source and host lifecycle/fixture tests.
Establish whether it is fixture-only before changing gameplay or claiming an
earned-equipment regression. A new phone batch is not required to begin that
bounded diagnosis.

Other approved remaining work:

- Natural Keeper victory through the full earned journey, save/load and death
  recovery acceptance. Refillable/upgradeable flask is approved, but its six
  recovery-policy choices remain owner decisions; no values are invented.
- Kit stationary-turn foot sliding and authored backward-running/animation gaps;
  preserve the withdrawn turn-step trial. Final movement/touch/caption acceptance.
- Kit voice balance, dialogue/music transitions and loop listening, and retained
  foliage/night cue listening. Owner's positive forest-main listening remains
  recorded; do not ask to repeat it as if it never happened.
- Chimney story smoke remains parked pending the bounded reusable High/Low
  volumetric decision. F03 road masonry exists; no new playable town is added.
- Sustained warm performance, the default-OFF worker's matched phone comparison,
  acceptable low-resolution image quality and the approved benchmark/report
  profile work. Production dispatch attribution is now available; no optimization
  win follows merely from this UI batch.
- Debug route deadline, incomplete MSVC time-cap lane and PR conflicts remain
  validation/integration gates, not new features or permission to merge.

**Forest decision, separately:** the nine-view sheet demonstrates missing middle/
far woodland. The proposed smallest sector is F01 north/west background, with
shared mesh/BLAS ownership, per-placement root fitting and actual memory/RT budget
proof before expansion. Existing Easy Tree/replacement-canopy deferral remains;
64 baked near-trail placements are not a completed vast forest. Terrain-led
visibility constraints must respect the topo/landmarks and prove any real RT or
residency saving. Full playable town remains 1.8.

Sam now grants persistent routine, normal checkpoint backups to the existing
development/PR27 branch. No per-checkpoint reapproval is needed. This does not
authorize force pushes, merges, releases, deployment or security changes.
