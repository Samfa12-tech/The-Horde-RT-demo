# Android three-slot save menu: Java host evidence

PLAY opens a three-slot New Game chooser. Load is available from Entry > More and
the pause menu; Save checkpoint is available from pause. Each slot displays its
manual checkpoint and latest autosave separately, with native checkpoint/hearts
or explicit empty, checking, invalid/unsupported and unreadable states. No dates
are invented. A native safe-checkpoint publication controls manual availability.

Occupied manual replacement requires explicit confirmation. Occupied New Game
also confirms that future autosaves can replace the latest autosave while the
manual record is kept; starting a game does not delete either record. A changed
catalogue publication revokes the confirmation. Invalid/unreadable manual files
cannot be overwritten through this menu. An uncertain write is never called Saved
and disables another write until refresh succeeds.

The versioned immutable Java decoder accepts the agreed 33-word native snapshot.
It rejects unsupported versions, malformed enums/flags/records and nonfinite
camera poses. Older revisions and other surface generations cannot apply. Missing
or malformed native state disables the previously displayed record actions.

During load, the menu remains paused through native preparation and logical
commit. On the current rebase token, Java cancels touch/controller/attack/dodge,
run/crouch intent and delayed audio/dialogue feedback, publishes the restored
absolute camera plus neutral input, then acknowledges native admission. Resume
appears after native success and the restored RT scene's presentation. A queued
second Load cannot reuse an earlier success to enable Resume. Back before
commit requests cancellation; after commit it still completes the required rebase
and leaves the game paused. Matching rebase token and surface are required when
cancellation changes the request identity. Pause revokes outstanding requests and
confirmation; no paused callback may acknowledge a load.

One background worker prepares only the directory. A UI-handler completion checks
both Activity lifetime and a process-local Activity-instance epoch before fast,
metadata-only native configuration of `files/campaign-v1`. A newer Activity
revokes an older worker even before the older Activity is destroyed. Debug-only
`campaign_test_namespace` values matching `[a-zA-Z0-9_-]{1,48}` instead use
`files/campaign-tests/<name>` and show TEST SAVES ONLY. A malformed debug namespace
does not fall back to the normal namespace. There is no directory deletion or
migration. The namespace is fixed for the Activity lifetime.

New Game success uses the same neutral-input cleanup and native-authored absolute
camera rebase before the existing Entry Play handoff. Its Activity test supplies
opening yaw pi, a nonzero pitch and held pointers/movement, and verifies rebased
neutral input before Play, with no premature unpause. Background autosave success
is labelled Latest autosave saved; manual success remains distinct. Uncertain
autosave outcomes are never labelled saved.

## Checks and limits

The initial full affected Gradle run passed **81/81** host tests in 45 seconds: decoder 7,
Activity fake-native integration 16, controller 35, lifecycle 7, touch 6, entry 5,
layout 5. The integration tests call real Activity menu and input methods with
immutable fake JNI publications; they do not perform filesystem/native save IO.
The exact command, source hashes and XML hashes are in [receipt.json](receipt.json).
Local raw XML is under `reports/2026-10-18-android-save-menu/`.

Two intermediate failures were resolved: the first exercised posted Builder
button delivery; the guarded positive handler now follows the existing explicit
button-click pattern. The second was the old controller expectation that PLAY
immediately starts; it now verifies chooser/Back behavior. The exact failure XML
was overwritten by subsequent Gradle runs, so only observed summaries are kept.

This is Java host evidence. Native session/resource/presentation execution is
parent-owned and must be joined with these Java sources before a playable claim.
Current-build device touch, controller focus, Back, background/return, real RT
presentation, save/relaunch/load and physical camera/input acceptance remain open.
No device, native Android build, owner save access, push or release occurred in
this subtask.


## Phone-observed stale surface notice follow-up

The initial current-surface-unavailable notice could remain above an Empty
catalogue with enabled slot actions after a new valid generation arrived.
The reducer now clears only that exact notice on a valid current READY/SUCCESS
publication with native error0. Operation failures, uncertain-write notices and
confirmation behavior are preserved. A real Activity fake-JNI regression checks
stale generation/disabled slot, current Empty catalogue/enabled slot with the
notice removed, then preservation of an unrelated operation notice.

Only the affected decoder and Activity integration suites were rerun for this
follow-up: **24/24 passed in 22 seconds** (7 + 17). The top-level receipt now
identifies this run and current source hashes; `earlier_full_affected_run`
preserves the original 81-test run, its older exact source hashes and raw XMLs.
New raw XML is in `reports/2026-10-18-android-save-menu/stale-surface-notice/`.
No physical seeded load, native build or device action was performed by this
Java follow-up; final candidate resealing remains coordinator-owned.
