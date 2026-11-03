# Android application candidate: Entry pass, New Game blocked

## Corrected-source retry completed

Source `41abc1150fc8a156a9b77a9730aa7398680eb05e` passed the rebuilt focused
native lifecycle/preflight test (3.76 seconds). Arm64 native rebuild/link passed
with explicit Ninja `-j2`; Gradle APK packaging then passed with two workers and
the already-completed native task excluded to avoid duplicate compilation.
CPU was released to the parallel laptop task before phone QA resumed.

The new arm64-only Debug APK is 219,083,026 bytes, SHA256
`ca91d875be6a2404e35d81d0303205f77ea76591e84854d99e757e7f21b7df04`, at
`reports/flask-application-20261102-idempotent-arm64.apk`. All three staged
runtime pins were rechecked. This remains an explicit private application
candidate with 1.6.2/code10 identity, not ordinary shipping or final catalog
admission. The older Windows interactive candidate was not rebuilt by this
focused host-test/Android window and must not be assumed to contain the fix.

Fresh backup again captured all 593 original files and all 27 preference/career
files, with every device hash matched before isolation. Standard `-r -t` update
and the normal resolved launcher were used. Actual results:

- **Passed:** normal presented Entry, empty-slot chooser, Slot1 New Game to a
  presented full Showcase (reported 475x980 internal), real touch-axis movement,
  pause menu and one Home/resume while the save chooser was open. The previous
  Entry-to-Showcase failure did not recur.
- **Passed in its limited scope:** Save chooser disabled all three save actions
  and explained `Reach a safe checkpoint before saving.` No earned checkpoint
  was reached, so no persisted save/load success is inferred.
- Bounded route gestures reached the `LIVE FIGHT: protection ended` HUD without
  earned lesson actions. This was not a natural practice-kill/cry/revival pass.
  Further automated wayfinding was stopped instead of inventing an outcome.
- **Unverified:** second New Game, encounter Retry, persisted save/load, earned
  sequential lessons, both kills, audible cry, visible moving revival, eligible
  damaged-flask use/return/cancellation, F03 pickup/Rest, physical simultaneous
  touch/controller feel and full recreation. These remain targeted checks; earning lessons/F03 progress needs a controlled
  gameplay route. A menu before an earned checkpoint cannot certify
  those behaviors. Compact benchmark UI and benchmark flicker verification also
  remain pending; no benchmark or Send was run in this bounded retry.

Cleanup restored **all 593 original hashes**, including **all 27 mutable state
hashes**. The ca91 Debug app is stopped, phone Home confirmed and custody
released; production/actor apps were untouched. No further device action or
build remains active. Private screenshots, XML, app-only logs, capability
publications, backup manifests and cleanup receipt are under
`reports/android-gameplay-qa-20261102-r2/`. Artifact/build receipt is
`reports/flask-application-20261102-idempotent-receipt.json`. These raw private
records are retained locally and are not committed or sent externally.

The first-attempt evidence below is retained as the failure that motivated the
fix, not the current status of the corrected Android New Game boundary.

2 November 2026. Physical **SM-S948B / Adreno 840**, RayTracingPipeline. The verified arm64-only Debug APK is `f55e105d017cc3f4a0d1e81c6a561a3f40bfc8a27c11272934cc6166af1670d4`, 219,047,290 bytes: the working-patch validation candidate documented in ORDINARY_ADMISSION.md, not an ordinary shipping identity.

## Actual result

- Exactly one authorised, awake/unlocked phone was verified at Home. No Windows foreground or desktop RT session was used.
- Original APK `6397363c...` and all 593 files/preferences were backed up; every archive file hash was compared to the stopped app's device bytes. This includes all 27 original preference/career files. Originals were retained in separate app-private directories during the fresh isolated run.
- The first install reported `INSTALL_FAILED_TEST_ONLY`; PackageInstaller explicitly requested `-t`. Standard `adb install -r -t` succeeded. No device security setting or permission policy changed.
- Normal Entry initialized and displayed the RT lantern with enabled PLAY/MORE/SETTINGS. The native coherent Entry state reported presentation; normal PLAY showed three empty slots. This is positive Entry/presentation/chooser evidence, not gameplay acceptance or a performance result.
- One normal Slot 1 New Game command was accepted (`action=5`, slot 0, error 0). Entry faded, then changed from loading to failed with no new scene-initialization receipt. The actual screen displayed RT RENDERER ERROR. The stopped diagnostic publication correctly reports `presented=false`; it is not reused as a gameplay frame.
- No second New Game, Retry/load, natural lesson/kill/cry/revival/live-damage, flask use/pickup, lifecycle/touch, benchmark or Send pass was established. No blind gameplay retry was performed.

## Cause and bounded source correction

`PresentableTinyRtScene::Destroy()` retains its device and immutable selected utility asset. Entry-to-Showcase calls scene initialization again; the new application integration reasserts the same utility asset. The old setter rejected every call while `device_` was non-null, including this unchanged selection after resource retirement. Rollback reasserted it and failed for the same reason. This matches the immediate loading-to-failure transition before a replacement initialization receipt.

The narrow correction makes the **exact already-selected path** idempotent. Different paths or clearing the selected path remain rejected once ready/device ownership exists. It does not normalize aliases, swap resources, weaken clip/readiness checks, move Vulkan work across threads or admit ordinary shipping.

The existing native preflight regression now covers unchanged selection while ready and with a retained device identity, plus changed/cleared-path refusal and preservation of the selected path. The retained-device sentinel exercises only the setter policy and is cleared before destruction; it invokes no fabricated Vulkan device.

**New regression/build/runtime status: pending.** Heavy compilation is held for a parent-coordinated Reef-safe CPU window. Previous candidate checks do not certify this fix. Estimate one serial window of approximately 4–8 minutes for the affected host preflight and arm64 native/APK build, subject to cache/link cost; no desktop foreground is required. Then resume one bounded normal-entry/New Game phone check before the remaining lesson/flask flows. The narrow compact benchmark summary/progress UI work and flicker verification remain pending and are not passed by this stopped session.

## Cleanup and retained evidence

Debug app stopped; all original **593 file hashes**, including the **27 mutable state hashes**, restored exactly. Phone returned Home and released. The latest f55 Debug APK remains installed under the standing owner policy; production/actor apps were untouched. The known failed candidate must not be presented as ready for an owner combined test.

Ignored private evidence lives under `reports/android-gameplay-qa-20261102-r1/`: backup APK/archive/hash manifests, Entry/slot/error XML and PNG, app-only logcat, stopped diagnostic JSON and `cleanup.json`. The 547,787,776-byte full backup archive SHA256 is `4276cf73df61e361869af7537a57e726f87b073c9e94c11064bf55e2ca81bc3e`. No private raw data, audio/model asset, report or device identifier is committed or transmitted. No app process or device operation remains owned.

## Remaining human route (authored-source guidance, not a passed playthrough)

From fresh New Game, pass the opening doorway/first room and follow the shadow corridor around its right-hand bend toward the waterfall/skylight chamber. Stop at the two guards instead of continuing through the torch bays. Follow the HUD in order: block, parry the yellow attack, evade the red attack during its threat, then guard-attack. After all four are earned, kill both practice skeletons, let deaths settle, and observe directional cry, visible revival and the live-damage HUD transition without a Finish menu action. An early protection-ended cue without those actions is not that proof.

Flask use needs an eligible injured/live state; the full-health opening UI did not offer a drink. The later capacity pickup is at the F03 waystone toward its existing dry approach (source placement 0.85m from the waystone). Look at it and use contextual Upgrade flask when eligible; it is not automatic. Capacity increases without changing health/current charges until a later eligible Rest. Actual visibility, pickup, drink/cancel/return joins and save/load remain unverified.
