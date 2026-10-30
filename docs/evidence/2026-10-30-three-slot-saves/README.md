# Three campaign slots with local save dates — 2026-10-30

Source checkpoint: `7e0b166110bf0be40e75989779894be7e69cd67f`. Prior checkpoint CI [41017548876](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/41017548876) completed successfully, all six jobs. This receipt is source/host evidence, **not current Windows/phone interaction acceptance or release approval**.

## Result

Windows and Android now present three primary slots instead of separate manual/autosave choices. A slot shows recognizable checkpoint location, health and the date/time of exactly the record Continue will load. Empty slots say Empty; undated local fixtures say Date unavailable. Error-only Try again replaces permanent developer recheck controls. Populated Save/New Game choices require explicit confirmation; manual and autosave retention and atomic storage remain intact. No fixture or owner save is erased, and no public legacy-migration system is added.

Normal Load automatically enters gameplay only after the existing matching-request, neutral-input and real RT-presentation gates. Interrupted/cancelled post-commit recovery retains an explicit Continue. Windows app deactivation revokes direct-continue intent. Accepted guard/attack transitions, resource ceilings and normal experimental gates are unchanged.

The shared selector waits for both catalogue reads and uses the newest embedded UTC timestamp among valid records; dated ties prefer manual. With both local records undated, autosave is the deterministic fallback, explicitly shown without an invented date. Invalid/unreadable files are preserved. Schema4 adds an atomic LE64 save-request timestamp and resource-presence bool (177-byte complete record); original schema1/2/3 encodings remain unchanged. The resource-presence bit preserves flask admission rather than enabling it merely to display dates. Reads never use mtime, copy time or the current time. Windows uses current locale and historical timezone conversion; Android uses locale DateFormat and the player's timezone.

## Checked

- 13 relevant native CTests passed: campaign codec/storage/worker/coordinator/session/checkpoint, earned-save producer and cross-process return/cleanup, Android load barrier, Windows menu/isolation and present completion. Final menu continuation/cancel delta rerun passed, 53 hidden HWND assertions. Tests retain original lifecycle, failed-write and independent-slot coverage.
- Android full unit suite: 535 registered, 529 passed, six unchanged Windows host-link capability skips, zero failures/errors. `lintDebug`: zero errors / 92 existing warnings. `assembleDebug`: four ABIs passed. No device performance claim.
- Actual Android widgets were rendered at coherent 411×801 and 804×381 host display metrics and inspected. Landscape keeps all three slot actions visible with >=48dp targets and no overlap; narrow/large-text layouts retain vertical scrolling. These are **Robolectric host images with no RT world or physical touch evidence**.

![Portrait Android host widget fixture](android-host-load-411x801.png)

![Landscape Android host widget fixture](android-host-load-804x381.png)

## Windows interaction blocked, not passed

The isolated R7 package launched through Computer Use, but its window stayed black and AX state was unavailable. Report remained `rtScene.presented=false`, dispatch 0×0. Read-only process inspection showed a responsive owned process (PID5440). The first coordinate activation returned **failed to activate captured window**. No further activation or alternate launch was attempted. This is not evidence of a menu/game crash or successful gameplay.

The exact owned R7 process was stopped after UI cleanup was unavailable. Its four copied synthetic save records are byte-identical to their baseline; Slot3 was intentionally left empty in a new profile. Existing R6 package and every original fixture remain. All 151 parked owner files hashed unchanged. No phone session/settings/install was started.

Next Windows check requires a coordinated foreground window that Computer Use can actually activate: launch the frozen R7 package, inspect all three slots, use keyboard/mouse selection, cancel and confirm overwrite, verify newly saved local date and direct Continue, then cold reload the same record. Do not relabel earlier R6 gameplay/save acceptance as this UI's acceptance. Physical controller and current Android touch/orientation/lifecycle acceptance remain separate.

## Frozen artifacts (local, not published)

- Windows: `reports/horde-windows-acceptance-20261030-r7-three-slots/HordeLanternRT.exe`, 4,905,472 bytes, SHA256 `412dd0367164995fed53249e2bb3dd92166e80957077c2fbdbd3abec8e0fcbdc`. 184 staged assets verified against current source. Live Send unavailable; ring/flask/yellow/shared64/worker normal gates remain OFF.
- Android: `reports/three-slot-ui-20261030/horde-three-slots-debug.apk`, 201,720,323 bytes, SHA256 `88988a12f4f2642e5fb3e379d816f497624a0a955f0d3cbed03ea269a3e683b5`; `com.samfa12.hordelanternrt.debug`, versionCode10, `1.6.2-debug`, arm64-v8a/armeabi-v7a/x86/x86_64. **Uninstalled**; a save-safe, compatible installation must be arranged before current-device acceptance.

Both binaries were built from the bounded dirty input based at9d7df2cc that is now committed as7e0b1661; they are not falsely labelled clean builds of the later commit. Embedded per-platform/per-ABI provenance labels and hashes are in [receipt.json](receipt.json). Raw local reports and APK remain ignored; only this compact receipt and two representative widget images are tracked. No merge, release, live report or deployment.
