# Android application candidate: Entry pass, New Game blocked

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
