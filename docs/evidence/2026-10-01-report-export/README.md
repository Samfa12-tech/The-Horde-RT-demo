# Consent-based local report export evidence

Continuation of the [historical unwired contract](../2026-10-01-report-contract/README.md),
not a remote reporting/release completion. Shared contract commit3e0f9be; platform
integration69afe4e built from that base plus the reviewed UI/JNI/Windows/CMake
changes (same source tree; no rebuild solely for commit identity).
Only synthetic owner-authorised test notes are retained. No private inbox or real
report backend was accessed.

## Host producers

- MSVC configured Debug/Release Windows-x64 presets in the isolated
  `codex/horde-mobile-lantern-profile` worktree.
- `horde_rt_playtest_report_tests|horde_rt_windows_playtest_report_ui_tests`:
  Debug2/2 PASS2.49s; Release2/2 PASS1.24s. UI fixture creates its own native window
  and drives only its own thread/controls, with bounded waits. File destination is
  injected: actual Windows picker/write and RTX presentation are **not** certified.
- Full `horde_rt_diagnostic_window` Debug/Release executable builds pass. Initial
  `LoadCursorW` resource-type and undeclared `CB_SETMINVISIBLE` compile errors are
  retained separately from the corrected builds.
- Android `:app:testDebugUnitTest :app:lintDebug :app:assembleDebug`: PASS;
  all30 Java tests pass, six focused report-export tests. Later CheckBox-only UI
  correction rebuilt/linted and reran this affected unit lane once, justified by
  actual screenshot contrast failure. No unchanged audio/performance reruns.

## Exact S26 artifacts and interactions

Only authorised SM-S948B/serialR5GL219SZGK was touched. S24 was released and no
further S24 operation was performed. Android16/API36, debug package
`com.samfa12.hordelanternrt.debug`, version1.6.1-debug/code9, Mobile75%.
All installations use `-r`, with no app data, volume or system font-scale reset.

1. Initial report UI APK AA312769B3603890C08F490E41325E5456EB17F3EC93821C2927E2F6257B6B31
   (114,217,807B): actual phone inspection showed invisible selected Spinner text.
   `form-start.png` is negative evidence, not the final UI.
2. Corrected Spinner APK EF2ECFE41C3A157D4B228E7EDB6012E0FD0707663E3FADBE8373B156C34F2E19
   (114,217,807B): installed base.apk pulled back with the identical SHA-256.
   Consent starts off; unchecked export rejects before picker. Context stays off
   for the first report. Picker Back saves nothing and exposes explicit same-report
   retry; retry chooses a unique test filename, and completed UI/file bytes agree.
   Fresh new form proves both choices reset off. A second independently consented
   report enables only the typed basic-context allowlist.
3. Context-saved screenshot exposed low-contrast checkbox marks on the dark panel.
   Distinct final UI APK8CB976891E5719ECEB7FD809EC91AAC939FF3EC58EB2D6DF44F15B5526F4FF87
   corrects their tint and restores the normal disabled save label after success.
   Source logic/native payload route are unchanged; affected UI verification is
   recorded separately, not inferred from the earlier installed candidate.
   Installed base.apk pulled byte-exactly; final screenshots/XML prove default
   off and explicitly checked on have visible amber outlines/marks at unchanged
   large font scale. No second save was needed for this contrast-only check;
   the minor disabled-label correction is source/build evidence only.

Actual local reports, no BOM:

| File | Bytes | SHA-256 |
| --- | --- | --- |
| note-only-export.json | 216 | 7EFC8D9BE306AFF6AA4540CF91C04587AC2E9C4836DF112E502AB6FC436E73DC |
| context-export.json | 452 | 6851AD16FA7A737EA888B4DECF80850EF101D9D2C37F6EF9F89902A5A508AFA0 |

Synthetic note includes the accidentally typed trailing ` V`; it is preserved
exactly rather than rewriting the output. Context note's trailing space is also
preserved. Actual optional context is product/version/build1.6.1, Android,
SM-S948B, Adreno (TM)840, RayTracingPipeline, Mobile,0.75,1080x2235 and honest
`rtPresented:true`. Neither contains serial/account ID, logs, captures, saves or URI.
Existing build string1.6.1 is not falsely presented as a commit hash.

The first pull immediately after approving Save found a provider-created zero-byte
file before asynchronous writing completed. It was not counted as success; a
later same-attempt pull yields the216-byte report and the saved UI. Transient
`uiautomator` null roots during activity/RT restoration are retained as missing
snapshots, not passes or a justification for replaying the export.

Initial accidental launch inspected the unchanged stable1.6.0 package rather
than the debug suffix. That stale menu XML/old stable APK is excluded from this
validation. Correct package was then launched; no stable-package update occurred.

## Evidence minimisation and remaining boundaries

Native report-form XML/screenshots contain only synthetic test content. Full
Android document-picker dumps can contain owner Downloads filenames and are
**not committed**. Only the exact chosen test filenames/buttons were used.
APK binaries stay local; identities/logs/selected exported bytes are archived.
Temporary test files are removed only after their exact bytes are retained.

Final Back returns the paused menu and Home exits foreground; no test runner,
benchmark or phone recording remains active. Full private picker XML is excluded.

The forms explicitly state that JSON was **not sent to the developer**. No
endpoint, background upload/retry, actual remote delivery/deduplication or backend
account/deployment claim. Windows real picker/write and final exact-release matrix
remain open. No new physical glass, RTX/S24/S25, sustained-performance or all-cue
music acceptance follows from these UI checks.

Audio/haptic manual revalidation required:NO; unchanged existing menu feedback.
