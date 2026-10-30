# Three campaign slots with local save dates — 2026-10-30

Source checkpoint: `7e0b166110bf0be40e75989779894be7e69cd67f`. Prior checkpoint CI [41017548876](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/41017548876) completed successfully, all six jobs. The keyboard follow-up is `bdb660c299b60ddaf1e1fb1b2e6c9b5809241518`. Intermediate head3815ce2c CI [41021067796](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/41021067796) passed all six jobs. This receipt includes bounded actual Windows interaction; **phone acceptance, natural chapter completion and release approval remain open**.

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

## Windows interaction and recovery chronology

The first isolated R7 launch stayed black, AX was unavailable and the report remained `rtScene.presented=false`, dispatch0x0. A responsive process (PID5440) was observed; the first Computer Use activation returned **failed to activate captured window**. No activation loop or alternate route followed. That exact owned process was stopped, with all four copied fixtures unchanged. This failure is preserved in the receipt/history; it is not evidence of a game crash.

After the owner returned and explicitly said the test window could come forward, the **same frozen R7 binary** launched and rendered. Mouse checks passed: three New Game/Load/Save slots, empty/undated labels, occupied New Game and Save confirmation/cancel, direct Slot1 Load into actual outdoor RT gameplay with three hearts, Save to empty Slot3, then explicit replacement of that new disposable save. The new date was30/10/2026 6:43:15PM local (+11); its schema4 bytes store07:43:15UTC. Closing and cold-reopening R7 retained that exact date, and Slot3 loaded directly into gameplay without an extra Resume. The original four synthetic records stayed byte-identical.

Actual keyboard checking identified a pre-existing integration omission: campaign child buttons were created after the startup keyboard/controller-focus hook installation. The narrow follow-up applies that existing hook to those buttons; no new navigation framework. In separately frozen R8, Tab moved Back->Slot1, Down twice selected Slot3, Enter loaded it into real RT gameplay, Escape opened pause, and keyboard-selected overwrite confirmation could be cancelled with Escape without writing. Menu/controller host checks also passed. Physical controller feel was not exercised.

R8 contains a disposable copy of the actual GUI-saved Slot3 record, with its filesystem mtime deliberately set to2030-01-01. The Load/Save card still displayed the embedded2026 time and loading left its177 bytes/hash unchanged. Both successful packages report real RT swapchain presentation at1232x803. These are functional observations, not FPS results, natural chapter completion or current-phone acceptance.

All test sessions closed through Computer Use Alt+F4; no Horde process remains. Existing packages, original fixtures and151 parked owner files are preserved. Phone was not acquired/accessed or installed. Sleep is only the owner's suspected correlation: no locked session, sleep event or causal explanation was established, and no PIN or security/sleep setting was requested/used/changed.

Remaining short acceptance: arrange installation of the frozen current Android APK with save/settings preservation, then test three slots, actual local date, touch selection/confirmation/cancel, orientation, direct Load and lifecycle. Current installed older builds cannot validate this change. An awake unlocked laptop with a foreground window is the demonstrated supported Computer Use arrangement; its documented API does not provide a Windows-unlock/PIN operation.

## Frozen artifacts (local, not published)

- Windows: `reports/horde-windows-acceptance-20261030-r7-three-slots/HordeLanternRT.exe`, 4,905,472 bytes, SHA256 `412dd0367164995fed53249e2bb3dd92166e80957077c2fbdbd3abec8e0fcbdc`. 184 staged assets verified against current source. Live Send unavailable; ring/flask/yellow/shared64/worker normal gates remain OFF.
- Android: `reports/three-slot-ui-20261030/horde-three-slots-clarity-debug.apk`, 201,720,323 bytes, SHA256 `aeace11f5e0d07f2bf531a893fc54d543ce6676f0ba0a7d5e0ec3b276264d8fb`; `com.samfa12.hordelanternrt.debug`, versionCode10, `1.6.2-debug`, arm64-v8a/armeabi-v7a/x86/x86_64. **Uninstalled**; a save-safe, compatible installation must be arranged before current-device acceptance.

R8 Windows artifact: `reports/horde-windows-acceptance-20261030-r8-keyboard/HordeLanternRT.exe`, SHA256 `38144dd9d9176b2d4af16e9bf0e12034452438332231ff55eab803669c5c4e85`,4,905,472bytes,184verified assets. It contains3815ce2c plus the now-committed bdb660c2 keyboard follow-up.

The original R7 and Android binaries were built from the bounded dirty input based at9d7df2cc that is now committed as7e0b1661; they are not falsely labelled clean builds of the later commit. Embedded per-platform/per-ABI provenance labels and hashes are in [receipt.json](receipt.json). Raw local reports and APK remain ignored; only this compact receipt and two representative widget images are tracked. No merge, release, live report or deployment.


## Source-only clarity follow-up

`d618199f` changes Android save success to simply "Saved.", retaining distinct manual/autosave operation identities internally and all uncertain-write checks. The old distinct-message assertion was updated to verify that distinction independently; final full Java suite remains535 registered /529passed /6unchanged skips, lint0errors /92warnings, and all4ABIs build. The latest frozen Android APK above includes that wording and the small benchmark-pace explanation; earlier88988a12APK is preserved privately. No phone install occurred.

Windows Release compile and the existing shared simulation cadence/hitch/pause test passed for the source-only benchmark-copy addition. The [sanitized report/source-boundary comparison](../2026-10-30-benchmark-comparisons/README.md) separates successful RTX5050/Radeon840M benchmarks from unresolved field crashes. No simulation timing, benchmark course, renderer budget or quality change was made.

Exact827e09f7 CI [41022958685](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/41022958685) passed all six jobs. Fresh CI is required for this later copy/documentation checkpoint. Computer Use was physically stopped before the subsequent mouse-capture check; no further Computer Use or alternate input route followed. That specific check and the fresh benchmark-copy visual review remain open, while the earlier R7/R8 save/menu/date/keyboard evidence is retained.

Later exact `6078e191` CI41025879954 passed all six jobs. A separately authorized
Windows mouse/menu retry and current Android installation/save/date/lifecycle
check are now recorded in the [device acceptance receipt](../2026-10-30-three-slot-device/README.md).
The dated earlier "uninstalled" and "no phone install" statements describe
their original checkpoints; the verified `aeace11f…` debug APK is now retained
on the SM-S948B by owner instruction. Full natural chapter/input acceptance and
the original field crashes are not closed by these isolated tests.
