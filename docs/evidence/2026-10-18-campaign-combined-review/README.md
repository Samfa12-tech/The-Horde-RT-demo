# Five-commit campaign review and focused corrections

Reviewed `bd55878d..04891bbc` together in the existing
`codex/horde-1.7-wp2-vertical-proof` worktree. Review covered checkpoint admission
diagnostics, logical reconstruction/dialogue/input, serialization/storage,
session/request arbitration, lifetime/concurrency and build integration.
Two independent read-only reviews covered storage/codec and logical restoration;
the integrating reviewer checked the combined contracts and fixes.

## Findings corrected

- **P1: moving torch release rejected both save boundaries.** The live sequence
  follows ordinary player movement during its 0.70-second guttering delay. Its
  final anchor can leave the drench trigger rectangle. Checkpoint validation
  incorrectly required that rectangle, and restoration tried to trigger a new
  sequence at the final anchor. A production-configured simulation walking west
  reproduces the invalid record. Validation now also accepts finite walkable
  route anchors; an explicit completed-state helper reconstructs the settled,
  extinguished torch at that anchor without replaying a timed sequence.
- **P2: explicit diagnostic checkpoint replacement retained a load input
  barrier.** Authored staging could report success while its requested camera
  pose remained blocked/paused. Accepted ApplyCheckpoint now clears the old load
  barrier after resource-lease and checkpoint-ID admission. The ordinary load
  barrier remains required until neutral rebased input arrives. A load followed
  immediately by real authored Keeper staging verifies the replacement pose.
- **Minor: restored snapshot used the old input publication identity.** Current
  input commands were consumed with the prior session publication number.
  Synchronization now receives the current coherent publication number; the
  existing monotonic floor is retained.

The unchanged-implementation reproduction with the explicit pose check failed eight assertions
in 736 checks. It included both moving-release records, publication identity and
the actual replacement pose. Staging's return value alone did not expose the
second defect, so the regression explicitly checks paused state and camera pose.
The same assertions pass with the fixes; no failing assertion was removed.

The prior negative torch anchor `x=0` is a legal walkable/reversal position under
the corrected contract; that rejection case now uses off-route `x=100`. The new
positive moving-release cases prevent restoring the erroneous restriction.
All original schema-1 trigger anchors, including rectangle edges, remain valid.
The format stays 162 bytes, with unchanged field order, versions and golden
bytes/CRC. Added codec round trips cover both safe boundaries outside the trigger
and an original edge anchor. No schema migration is introduced.

## Validation and boundaries

[validation.json](validation.json) records final source, artifact and local log
hashes, actual selected test results and compile-only Android evidence.
Final-source Windows results: Debug **6/6** selected suites (24.06 s), Release
**11/11** (31.70 s), and both diagnostic applications built. Each configuration
passed 738 checkpoint, 1991 codec, 1178 storage and 306 coordinator checks, plus
dialogue and the original torch/gameplay smoke fixture. Release also passed
simulation timing/gameplay, 683 combat checks, 16226 rescue checks and 41977
actual-authored-terrain checks. These are host fixtures, including the logs whose
historic mode names contain `phone`; they are not device runs.

Android Debug assembled all four ABIs in 2m19s; no installation or launch occurred.
The Windows optional reparse fixture remained unavailable. Host storage runs used
owned disposable directories and observed 8 concurrent valid reads in Debug and
542 in Release; no platform durability claim is added.

The first broad-review build request mistakenly used the authored-terrain CTest
name as a build target and stopped with MSB1009 before tests. It was corrected to
its registered shared rescue-integration executable, retaining the same test.
The failed reproduction and build invocation are preserved as separate receipts.

No further actionable integration, storage/codec, lifetime/concurrency or
compatibility defect was found in this bounded review. The coordinator's explicit
read decision, revision tracking and publication gate serve separate observed
contracts; no abstraction rewrite or feature expansion was warranted.

Actual worker/storage bridging, save paths, menus, game load transactions and the
manual/autosave retention policy remain unwired. Sam's three-slot retention
choice stays pending. No owner save files, device, preview server, push, merge or
publication was used. POSIX filesystem runtime, Android lifecycle/process death,
physical power-loss durability and integrated RT/owner-feel gates remain open.
Host tests and Android compilation do not establish those passes.

Audio/haptic manual revalidation required: **NO additional live-playback change**
in this isolated fix. The new completed-torch reconstruction emits no events;
actual load/audio/camera integration retains its existing physical acceptance
gate. Stop after this review correction commit.
