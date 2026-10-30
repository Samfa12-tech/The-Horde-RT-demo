# Full host campaign save/load path

This slice exercises an actual save/load path through the
[real worker/storage bridge](../2026-10-18-campaign-worker/README.md), the fixed
162-byte codec, imported terrain, current GameSimulation and coherent input
publication. The caller supplies an exclusively owned disposable directory.
No application owner-save root is selected or created.

## Implemented and exercised

Session owns two coordinators for the selected one of three slots and one
bounded worker. Each slot has a separate manual file and latest autosave file;
repeated autosaving cannot change manual bytes or create history. Save captures
the actual simulation at a supported safe checkpoint, reads the current expected
version, and only acknowledges the matching real disk publication. Invalid
existing files remain unchanged and report failure. New Save/Load after failure
reads again rather than blindly retrying.

Loading reads a prepared candidate without changing gameplay. PrepareLoad issues
an exact simulation restore/destination token for resource preparation while
retaining the old playable snapshot. CommitLoad copies the CURRENT simulation
at final apply, not a copy held across resource/worker delay. It validates the
matching read, terrain/readiness and simulation restoration offside, prepares
neutral camera/input values, strongly adopts the same read decision, then uses
no-throw simulation move/POD control updates. Failed or stale preparation leaves
the simulation, camera and input unchanged. The caller must discard borrowed
Snapshot/Events references across the successful swap.

Input command counters stay monotonic; movement/run, replay pose, stale audio
completion and edge-history debt are cleared. Old held publications remain
paused; a fresh neutral rebased publication resumes movement with the caller's
original configuration. Cancellation uses the exact owned restore token, so
cancelling an old load cannot cancel a newer independently prepared restore.
Slot changes and shutdown revoke receipts before joining the worker.

The host fixture loads real chapter terrain and constructs validated safe states
in the real production-configured simulation. It captures manual/lower and
auto/exterior records across all three slots, performs repeated autosaves,
loads via real storage and terrain, rejects bad/missing readiness, advances live
ticks during preparation, checks held controls/configuration/identity floors,
rejects duplicate and stale slot loads, preserves a newer restore during cancel,
preserves corrupt files, and reconstructs a persisted exterior record through a
fresh controller. Exactly six record files remain. Fresh controller testing is
not an OS process-death test or a player completing the chapter.

The first 55-check Release fixture passed. Final ownership/current-state
regressions extend it to 58 checks. Exact final build/test/log/artifact identities
and results are in [validation.json](validation.json). Final Debug and Release each pass 7/7 selected suites (15.12 s / 4.86 s); both Windows apps build. Android Debug assembles all four ABIs in 2m38s, without installing this candidate. No tests or deadlines
were weakened. Existing codec/storage/coordinator/logical/dialogue suites remain
in the final selection.

## Remaining playable/platform boundary

This closes the **full host-side path**, not game-facing Save/Load acceptance.
No platform creates a live Session or points it at owner files yet. Still needed:

- Three-slot summaries and Save/Load/Continue/New Game/death menus, explicit
  occupied-slot handling, private platform root and automatic safe-boundary hooks.
- Real renderer resource preparation/ownership and no-fail publication together
  with the simulation transaction. Host terrain readiness is not RT readiness.
- Windows physical/controller latch clearing and timing/camera/audio reset;
  Android Java absolute-camera/touch/controller rebase acknowledgement under
  existing publisher ownership before unpause. InputMailbox must not be copied
  or mutated from the simulation thread. This host seam returns rebased owner
  controls; it does not implement those cross-thread/native adapters.
- Forest-end logical checkpoint and actual full-route recovery coverage.
- Authorized isolated Android save/load/Home/resume/process-death tests, POSIX
  filesystem/directory-fsync runtime evidence, Windows reparse runtime and
  physical power-loss acceptance. Windows reports FileFlushed only.
- Integrated physical camera/audio/haptic/RT/touch and IO-hitch acceptance.

Audio/haptic manual revalidation required: **NO additional live playback change**
in this host-only seam; actual platform load integration still requires its
physical acceptance checks. The separate phone cold-start receipt belongs to
0d6c22d6 and establishes no Save/Load or sustained performance pass.

Save infrastructure is no longer the only workstream: the
[reconciled feature map](../../design/HORDE_1_7_FEATURE_STATUS_2026_10_18.md)
records all confirmed 1.7 work and distinguishes missing implementation from
owner/physical gates. The reproduced Android shadow-menu crash is fixed in its
separate [38-test checkpoint](../2026-10-18-mobile-shadow-cycle/README.md).
