# Horde 1.7 session/request receipt coordinator

Parent: `c721791180ebe73d009f96300c74c1c3aa4c4d9e`. Same branch
`codex/horde-1.7-wp2-vertical-proof`, same existing worktree
`C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.7-wp2-vertical-proof`.
This is the requested stop-for-review checkpoint. No public push, merge, release,
device interaction, actual save path, slot/menu overwrite policy or real worker
is connected. Owner files, art and the unrelated preview server remain untouched.

## Implemented arbitration

`CampaignSaveCoordinator` is owner-thread state plus an immutable work description
and weak, single-use worker ticket. The owner chooses a logical TargetId; it is
not a slot index or filesystem-location policy. Worker completion methods are
thread-safe. They never capture or mutate the Coordinator, GameSimulation,
renderer or menu. The owner consumes the one bounded completion slot with Poll.
There is no unbounded receipt history, automatic retry or background thread.

Every request binds owner-instance, session, request, target, operation and
logical checkpoint revision. OpenSession, another read/load, Cancel, Close and
destruction revoke obsolete tickets and prepared decisions. A newer logical
checkpoint also revokes an outstanding/prepared read so delayed load data cannot
replace newer gameplay. Closed/destroyed/duplicate tickets cannot deliver another
result or enter publication. New sessions always require a fresh explicit read
baseline; no old target/version is silently carried into them.

Read success is a **prepared candidate**, not adoption. ReadCandidate returns a
token-checked value copy for future offside simulation/resource preparation.
AdoptRead needs the same current token again. UseReadAsWriteBaseline preserves
the current logical checkpoint instead of silently loading the old file. A
missing target may establish an explicit create-only baseline, but cannot be
adopted as invented gameplay. Paths, encoded data, decoded logical values and
typed result/durability shapes must agree. Failure paths/operations are validated
too. Malformed, unsupported or failed receipts never acknowledge a checkpoint.

Writes require an accepted baseline and logical candidate; only one write may
be pending. Its candidate/expected version are frozen. The revocable PublishWrite
gate executes at most once while that exact request/session is current. Lifecycle
revocation serializes on the same mutex: a callback already inside may complete
one previously authorized publication, then Close/session change discards its
receipt; a callback arriving after revocation performs **zero** gated mutation.
Cancellation cannot undo publication that already won this boundary.

Saving A while gameplay advances to B updates the confirmed file version to A
and acknowledges revision A only. B remains unsaved. Its later request expects
exactly A's confirmed bytes. The fake backend enforces content comparison, so an
externally changed record produces Conflict without overwriting it. Any failed,
malformed or uncertain write blocks retry until a new read baseline is explicitly
accepted; previous confirmed/logical values remain intact. Failed requests and
duplicate late successes cannot replace a newer receipt or checkpoint.

Successful write receipts require publication through the gate and canonical
candidate bytes at the bound target. AlreadyCurrent additionally requires a valid
existing expectation equal to that candidate; it cannot acknowledge create-only
work or a different expected version. It reports no new flush. Durability enums
are retained exactly: currentCheckpointStored means acknowledged current bytes,
**not a new power-loss durability certificate**. Publication exceptions are
uncertain failures, never success or blind retry.

## Deliberately unwired boundary

There is no call to actual storage Read/Write here. Synthetic fake paths are
identities only. A real backend must obey the immutable description, exact-content
update contract and gate; a backend mutating outside the gate is outside this
guarantee. Logical IDs still need a stable trusted platform resolver later.

The existing synchronous storage adapter must be split into offside preparation
and a gated final name cutover before it is connected. **Do not put the entire
blocking storage::Write inside PublishWrite**: lifecycle revocation waits for a
callback already inside, so the callback must cover only the bounded publication
boundary and must not reenter Coordinator/Request methods. Worker queue limits,
shutdown/join/drain and real OS publication cancellation remain future integration
work, not acceptance established by these fake callbacks.

Similarly, real GameSimulation/resource preparation and camera/input rebasing
must succeed before an owner commits the matching read decision in the same
owner-thread transaction. The coordinator does not perform that transaction or
publish renderer readiness. A prepared value is not a loaded playable scene.

## Checks and review corrections

[validation.json](validation.json) records exact source/artifact/log identities
and the actual native/Android results. The targeted native selection covers
coordinator, codec, storage, logical checkpoint and chapter dialogue in Debug and
Release. New tests use fake storage only; the separately retained storage fixture
continues to use its exclusively created disposable directories.

Final-source results:

| Check | Debug | Release |
| --- | --- | --- |
| Focused CTest selection | 5/5 passed, 16.35 s | 5/5 passed, 4.03 s |
| Coordinator assertions | 306, zero failures | 306, zero failures |
| Codec / storage / logical checkpoint assertions | 1970 / 1178 / 722, zero failures | 1970 / 1178 / 722, zero failures |
| Chapter dialogue fixture | Passed | Passed |
| Windows diagnostic application | Built | Built |

Android `assembleDebug --offline --console=plain` passed in 1 minute, with
native builds for arm64-v8a, armeabi-v7a, x86 and x86_64. No APK was installed.
Storage revalidation observed 485 concurrent valid reads in Debug and 386 in
Release; the optional Windows symlink fixture was unavailable in both.
Coordinator cases cover explicit missing/found baseline/adoption; stale and
foreign tokens; read candidates before adoption; delayed/out-of-order callbacks;
A/B revision sequencing; failed read/write and explicit retry; malformed/wrong
operation/path/value/durability receipts; forged positive completion outside the
gate; impossible AlreadyCurrent receipts; external version conflict; callback
exceptions; destruction; four competing worker callbacks; and both orderings of
publication versus cancellation/Close. Only the test owner thread calls owner
methods; workers use the ticket channel and atomic test counters.

The first 195-check Release run passed. Review then identified an impossible
AlreadyCurrent receipt that could acknowledge unpublished candidate bytes; the
implementation and two focused negative cases now close it. Review also tightened
failure operation/path validation and added inspection of prepared load copies.
No failing assertion was removed or deadline weakened. No current-source cloud
CI/sanitizer run is claimed; their target rosters include the new fixture.

Android's four ABIs compile the coordinator, which is **not POSIX filesystem
runtime evidence**. The previous POSIX/directory-fsync and Windows reparse runtime
gaps remain open, along with physical power-loss testing. No device was used.
Audio/haptic manual revalidation required: **NO** for this isolated coordinator;
actual load/audio/camera integration will retain its own owner-feel gate.

## What remains for genuinely playable three-slot Save/Load

| Remaining work | Acceptance / owner decision |
| --- | --- |
| Real bounded worker and storage bridge | Prepare/flush off-thread; insert the revocable gate only at final publication; bind real target/session/request receipts; test cancellation, shutdown, uncertainty/readback and failure on actual adapters. Engineering, no new slot policy. |
| Real load transaction | Coordinate candidate, terrain/resources, GameSimulation restore, camera/input rebase and final matching adoption; preserve the prior playable session on failure; handle pause/background/death/close. Engineering using existing contracts. |
| Three persistent slot identities and menus | Platform-private path resolver, empty/valid/unsupported/corrupt summaries, Save/Load/New Game/Continue/death flows and explicit destructive-action confirmation. Do not silently replace unsupported/corrupt owner files. Standard interaction work after retention policy is settled. |
| Complete checkpoint coverage | Wire accepted lower-claim/safe-exit triggers and add the forest-end logical record/recovery. No arbitrary mid-combat/mid-rope saves are implied. Engineering plus route validation. |
| Platform acceptance | Disposable POSIX runtime/directory-fsync evidence; then authorized Android process-death/storage/lifecycle/touch and Windows load/reparse checks, plus audio/camera/RT route and IO hitch validation. Compilation is not acceptance. |
| **Sam's remaining policy choice** | Does manual Save store the active slot's latest safe checkpoint alongside autosaves, or retain a separate manual snapshot/history? This also determines occupied-slot/New Game retention/overwrite behavior. Three slots are already the direction; no need to ask that again. |

Sam's later physical playtest/feel feedback remains needed once the integrated
candidate exists. Save file encoding, identity/cancellation handling, ordinary
layout/error states and safe platform plumbing do not need another product
decision now. There is no question or device request while Sam is away.

The wider 1.7 update still has its independent performance/route/physical
acceptance gaps in the [continuation map](../2026-10-17-checkpoint-admission/README.md).
This checkpoint adds no town/outdoor content or performance claim. Stop here for
review; do not proceed into real saves, menus or further feature expansion.
