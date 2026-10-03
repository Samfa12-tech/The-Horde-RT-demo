# Original music resolver evidence

Lead-reviewed shared implementation plus real production torch fixture. Final
MSVC Debug/Release each2/2PASS (music resolver and gameplay simulation); Android
ARM64 Debug compile/link/packagePASS, not installed. No playback/assets or
feedback semantics changed. [Contract and remaining gates](../../ENGINEERING_1_6_1_MUSIC_2026-09-30.md).

Retained red/green: the pre-final retry fixture incorrectly expected restarted
event sequences from the same queue; the resolver/fixture now preserve queue
identity with a monotonically newer event and reject stale events. The initial
Android unquoted dotted Gradle property split into a task name; quoted retry
builds. Neither red result is silently labelled green. Older worker2/2 logs do
not certify the lead's added real-simulation fixture; final lead logs do.

[Receipt](source-and-build-receipt.json) binds current source checkout hashes,
precise intermediate APK identity and selected logs. These raw checkout hashes
are evidence of this validation, not an assumption about CRLF on future hosts.
The conventional outputs APK was stale and is not assigned to this build.
APK/native binaries/build caches remain local-only; source/tests/rebuild recipe
and selected logs are retained in Git. [Manifest](manifest.json) binds each
retained file's original bytes/length. No source music/Chordsmith code imported.

This is logical-loop/state and build evidence, **not** musical seam/drift,
faithful rendered cues, platform playback, phone, owner listening or final1.6.1
acceptance. Audio/haptic manual revalidation:NO until actual playback/mix wiring.
