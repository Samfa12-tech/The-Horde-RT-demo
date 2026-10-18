# Isolated Android persistence review: upper return blocked

The finite phone review established lower-checkpoint loading, cancelled/manual
save isolation and a reported durable write. It also found an upper-return
readiness failure. This candidate is **not accepted as a complete Save/Load
milestone**; the planned fresh-process reload was not run after that failure.

## Exact candidate and scope

Samsung **SM-S948B**, Android **17 / API 37**, user 0, build
`CP2A.260605.016.S948BXXS4BZIG`, serial `R5GL219SZGK`. Final tested source commit
`7d89445030727a9099524c3728c190dfd636e51b`, separate Debug package
`com.samfa12.hordelanternrt.debug`, code 10 / `1.6.2-debug`. APK SHA-256
`ca08919a8c63aac269b4fc68a02bde706131a25a92d8765acd550297841918a9`,
183,957,866 bytes. Public V2 certificate SHA-256 remains
`2bc406088b23ac80f2a00a40e2d5d8daa012844dc62f31ad6145d129f5566736`.
The compatible update used `adb install -r`; installed bytes matched the seal.
The compiled-source manifest is
`reports/2026-10-18-campaign-final-compiled-sources.json` (24 files).

Evidence class: **seeded persistence fixtures**, actual Android UI, opened
app-only screenshots, native campaign transitions and genuine RT presentation.
The fixtures do not establish normal chapter completion or earning checkpoints.
Only `files/campaign-tests/sl-20261018-r1-8b14` was used, selected by the explicit
Debug `campaign_test_namespace` intent extra and visible TEST SAVES ONLY label.
No record was placed in or loaded from the owner `campaign-v1` directory.

## Finite observations

| Case | Observed result |
| --- | --- |
| Seed | Six supplied 162-byte `.hrd` fixtures, 972 bytes total, written directly into the verified empty app-created test namespace; all six SHA-256 hashes matched |
| Slot 1 manual | Native request 2 / generation 1: reading -> rebase token 1 -> awaiting presentation -> phase 7, error 0, presented 1; explicit Resume shows 1/3 hearts and the lower rescue-rope/lantern view |
| Keep records | Manual Slot 2 replacement dialog cancelled; all six record hashes unchanged |
| Manual Slot 2 replacement | Explicit Replace manual confirmation; native request 4 reaches phase 7 / error 0 / durability 2 (`FileAndDirectoryFlushed`); UI reports Manual checkpoint saved |
| Record isolation | Slot 2 manual becomes byte-identical to Slot 1 lower fixture; other five target hashes unchanged, including Slot 2 autosave; active slot remains Slot 1 |
| Slot 1 autosave | Request 6 / generation 1 / rebase token 2 reaches phase 7 / error 0 / presented 1; actual Loaded menu and resumed exterior/night view show 3/3 hearts |
| Upper traversal blocker | Disabled Preparing rescue route label persists before Home, after Home/resume and in one bounded final readiness check; descent is not accepted |
| Home/resume | Same generation 1: native suspension reports `idle_result=0`, wait ends with `current=1`; actual exterior view and 3/3 hearts return. No surface-generation replacement or interrupted-load recovery is claimed |
| Terminal | Observed QUIT DEMO used; later activity check finds zero Horde activity rows |
| Fresh process reload | **NOT RUN**, stopped at the first material upper-readiness failure |

Root source review identified that the restored upper checkpoint admitted the
exterior destination while resident Dungeon zone 0 remained Unprepared. The
renderer readiness adapter iterated from zone 1 and never republished real zone
0 readiness. This diagnosis does not turn the physical failed case into a pass;
a source fix and a separately sealed, bounded follow-up are required.

The saved Slot 2 manual record is 162 bytes, SHA-256
`72e136f8b3d673aa80fc390659a95e6fc3e960365a07dd460766ea685dbbddf8`.
It replaced fixture hash
`4f8ce6b60c205821e475f21419228dc0461206ea0e13ac4b0e090d9cc4ade70d`.
[The compact receipt](validation.json) retains all before/after record hashes,
opened-image hashes and exact supporting paths. Reported file/directory flush
success is not an actual power-cut, filesystem-fault or hardware durability test.

## Preserved earlier attempts

Before fixture loading, candidate
`ddb9869ea339b044f0d8c5f19d2ba304717d142e5440cd954a520c745ff14187`
(183,957,882 bytes) exposed a New Game camera facing close rubble. It reached
native success and RT presentation but its opening camera was rejected. Save
was correctly unavailable before a safe boundary; explicit Quit preserved
settings and all 274 prior paths. No fixtures were seeded or loaded on it.

Replacement
`bb4175ffbc40f220d016b6b9ec1d7e44fd0aca1fb6692bf5b1edfdc564f676f9`
(183,957,930 bytes) showed the corrected forward doorway view, 3/3 hearts,
enabled ordinary HUD and native New Game success / RT presentation. Empty Slot
3 selection created no records; Save remained unavailable before a safe boundary.
This opened-image observation is bound to that APK, not silently reassigned to
the final APK. The final build did not repeat the full empty-opening case.

Both opening-only passes stopped before seeded loads while the parent corrected
preference-custody and failure-edge concerns. The replacement's initial
surface-pending menu sample was retained; one bounded readiness check enabled
its Empty slots. Early unavailable text remained after readiness and was reported.
One test-helper request used the wrong case for Keep records and failed before
sending input; the observed exact-case target then cancelled successfully. This
was a harness selection failure, not an application failure.

## Custody and remaining acceptance

Fresh preflight found the launcher foreground, no Horde activity, and PID 30596
matching the prior test's cached process. No owner-running activity was displaced.
After final explicit Quit, all 274 original file paths remain. Seven new files
are confined to the test namespace: six records and the manual-save lock.
The owner `campaign-v1` directory remains absent. Horde settings SHA-256 is
unchanged before updates, after installation and after final Quit:
`e3966b09cd9c354cb92833496b7f812139178dd7b2a906a4f809dbf9538c3dee`.
The final app-PID log has no fatal exception or signal. No uninstall, data clear,
save deletion, settings restoration, security change, process termination,
Release targeting, desktop input or Reef interaction occurred. Ordinary reports
may change; whole-app-data byte equality is not claimed.

The local sandbox initially could not connect/start the ADB daemon. The minimal
read-only inventory succeeded outside that restriction, and no failed phone
mutation was retried. No APK was copied or pulled; verification stayed hash-only,
respecting the prior [redundant-copy denial](../2026-10-18-android-back-device/README.md).

RT presentation reached the unchanged saved 40% scale, 576x1188 dispatch into
1440x2971 swapchain. These are presentation facts, not performance measurements.
All large captures, UI XML, scripts and full logs remain ignored under
`reports/android-campaign-device-2026-10-18/`.

Still unaccepted on this candidate: upper descent/playability, fresh-process
save recovery, interruption during an in-flight save/load, power-loss/fault
recovery, normal chapter-earned autosaves, full chapter traversal, movement/
combat/root acceptance, sustained performance, audio/haptics and comfort.
