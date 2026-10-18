# Android fresh checkpoint publication repair

**Seeded diagnostic: fresh Slot 2 manual creation now succeeds.** The earlier
diagnostic failed at create-only publication with native EACCES (13). The repaired
Android candidate reports success with file-and-directory durability and produces
the expected record bytes. The completed fresh-process reload and terminal
custody checks are recorded below; the separate natural earned/death gate remains
open. This receipt certifies bounded seeded creation and reload only.

## Failure and repair

This is separate from the [failed normal earned run](../2026-10-18-android-earned-recovery/README.md).
It uses `files/campaign-tests/create-diagnostic-20261018-r1`, seeded with one prior
isolated Lower chamber / 1-heart record in Slot 1 manual. The seed hash is
`72e136f8b3d673aa80fc390659a95e6fc3e960365a07dd460766ea685dbbddf8`.
No earned-route success, death Load, ascent or full recovery follows from this
seeded diagnostic.

On the unchanged-storage diagnostic APK
`00f196f24eb07cc4eacf4e096f05496912b069f79505f98350812ada657a42b1`,
PID 24510, first creation of Slot 2 manual failed. At line 987 of
`reports/android-earned-recovery-2026-10-18/pid-24510-diagnostic.log`:
`operation=1 slot=1 kind=0 status=15 system_error=13 codec_error=0 publication=0 durability=0`.
Here slot 1 is the zero-based native index for UI Slot 2. Status 15 is
PublishFailed, native error 13 is EACCES, and no publication/durability was reported.
The source's create-publication branch contains one syscall, `linkat`; combined
with this result it identifies denied hard-link publication. A particular SELinux
policy is not proven, and the result must not be described as a traced AVC cause.

The repair changes only Android create-only publication to raw
`syscall(SYS_renameat2, ..., RENAME_NOREPLACE)`. This retains atomic no-replace
cutover and minSdk 24 without depending on the API-30 libc wrapper. Unsupported
kernels/filesystems fail with their real errno; there is no weaker replacement
fallback. Existing-record replacement still uses `renameat`; non-Android POSIX
still uses `linkat`, and Windows behavior is unchanged. The companion native
bridge addition reports the detailed storage failure at the owner completion
boundary. Exact hashes for both changed native files are in
[validation.json](validation.json), against source base
`bc0bef02358c9d647586def20b0c34677c1fb25a`.

## Exact device observation

Repaired installed APK SHA-256:
`4c84eb456f97717df055245410196101ea681a3bc55d0c9776eb42a59d591073`.
All four Debug native ABIs assembled successfully in 8 seconds. Existing Debug
package and signer match; the compatible update was installed in place. Device
is SM-S948B, Android 17 / API 37, user 0. No Release acceptance is implied.

After loading seeded Slot 1 manual, PID 28974 submitted manual save to empty
Slot 2. At 15:18:57.125 native request 4, generation 1 reached phase 7/error 0,
durability 2 (FileAndDirectoryFlushed); active slot remained 0 (UI Slot 1).
The actual new Slot 2 record and unchanged Slot 1 seed both hash to the value
above. Opened screenshot `97-fix-created.png` shows “Manual checkpoint saved.”
and both manual summaries as Lower chamber / 1/3 hearts. Opened earlier
`88-diagnostic-create-result.png` preserves the failed storage message and empty
Slot 2. Raw images/logs remain ignored; the JSON pins their paths and hashes.

## Host checks and limits

Focused Windows storage tests pass: **1,202 checks, zero failures** in Debug
(3.90 s) and Release (1.67 s). These include host concurrency/race checks;
they are not Android kernel race certification. The Windows reparse runtime
fixture is explicitly **NOT_RUN** because symlink creation was unavailable;
nonregular rejection was tested. No WSL/non-Android POSIX runtime check ran.

After explicit Quit and completed activity teardown, only the known cached test
PID 28974 was terminated with `am kill`. An initial immediate teardown guard
stopped before termination; the later verified zero-activity boundary allowed it.
PID absence was verified before launching fresh PID 32269. That fresh process
loaded the newly created Slot 2 manual, completed the real camera/input/current-RT
handoff and explicitly Resumed alive with 1/3 hearts and enabled Climb rescue rope.
Opened `105-fresh-slot2-live.png` shows that view; it supplies no FPS claim.
Terminal explicit Quit leaves zero Horde activities. Settings and all six prior
test records remain unchanged. Original paths remain present; additions are
confined to the earned and separate diagnostic test namespaces.

A new earned journey on this repaired artifact, actual power-loss recovery,
full traversal/death Load and sustained performance remain unaccepted here.
The historical earned failure remains intact.
