# Android campaign Save/Load checkpoint

The bounded isolated phone milestone is now accepted in the separately sealed
[upper-return and fresh-process follow-up](../2026-10-18-android-campaign-readiness-followup/README.md)
on source `6d4033d3` / installed APK `5eb3282a…9128`. Combined with the earlier
actual durable manual replacement, it covers both supported checkpoint sides,
record isolation, real RT presentation and reload of that saved record after
process restart. Earlier defects below retain their own failed outcomes. The
later [torch diagnostic repair](../2026-10-18-live-torch-crouch-probe/README.md)
passes its affected host checks; its APK was not installed. Windows/death-screen
Load, chapter-earned saves, full route/audio and power-loss/performance acceptance
remain open.

Continues the existing `codex/horde-1.7-wp2-vertical-proof` worktree from
`766078a416b51052521f4e0db8b1e8beaa2e0c89`. No new checkout, branch switch,
public push, release or owner-data migration is part of this checkpoint.

The Android entry menu now selects one of three New Game slots and offers Load.
Pause offers Save checkpoint and Load. Each slot retains separate manual and
latest-autosave records; saving a manual record in another slot does not retarget
the active game's autosave. Occupied choices require explicit confirmation.
The UI distinguishes empty, checking, valid, invalid and unreadable records,
pending work, failures and uncertain publication. It does not report success
before the worker's typed receipt. See the [Java host receipt](../2026-10-18-android-save-menu/README.md).

The portable `PlaySession` facade uses the existing bounded single worker and
coordinator. Shared simulation latches the two supported authored boundaries
at fixed-tick granularity, so ordinary movement past the chest or a multi-tick
rope exit cannot miss a save merely because the strict stationary capture
predicate was false between publications. The durable pose is the validated
authored respawn anchor, never an arbitrary live position. All gear, combat,
reward, terrain/readiness, standing-clearance and traversal gates remain.
An ordinary-input host journey reaches both boundaries without imported poses
or checkpoint restore seeds; damage is disabled for this deterministic fixture.
This is CPU gameplay evidence, not campaign completion on a device.

Load keeps the previous simulation and RT scene until validated preparation and
commit. Real destination resources and terrain must be ready. GPU ownership is
retired before swapping a prepared scene. After commit, gameplay remains paused
until Java consumes old controls/feedback, rebases the camera and publishes fresh
neutral input, then a current RT-produced frame from the loaded world actually
presents. Surface replacement, resource recreation and Home/focus suspension
require a fresh handshake. Cancel can abandon preparation; after commit it keeps
the menu visible while the required handshake completes. Windows can reuse the
portable facade; its UI/resource/input adapter remains unimplemented.

Host validation before the startup correction passed all 15 selected native
tests in Debug (273.95 s) and Release (31.12 s), and both Windows applications
built. All 81 focused Java/Robolectric tests passed. Android Debug assembled for
arm64-v8a, armeabi-v7a, x86 and x86_64. The final startup-corrected applications
and APK were rebuilt; source identities, exact artifacts and supplemental tests
are in [validation.json](validation.json). The pre-readiness four-ABI Debug APK was
`ca08919a8c63aac269b4fc68a02bde706131a25a92d8765acd550297841918a9`
(183,957,866 bytes; 38 s build). The later Java notice correction passes all
24 selected decoder/Activity tests; the earlier complete 81-test run retains
its own source identities. Final admission-policy tests pass in both native
configurations. These are affected checks, not a complete release gate.

## Retained failures and limits

- The first phone candidate (`ddb9869e…14187`) accepted empty Slot 3 New Game,
  presented RT output with three hearts, correctly disabled Save before a safe
  boundary and quit cleanly. Its opened view faced rubble. The new neutral-input
  rebase exposed a legacy rescue fixture helper that overwrote the live opening
  yaw with pi. Live startup now preserves the caller's configured opening yaw;
  default/custom/reset checks pass while frozen fixture facing stays unchanged.
- Before any seeded load, review found that the existing RT Lab unlock preference
  could be written by test progress. The test was paused and the unlock hook now
  excludes the isolated test namespace. No seeded progress ran on that candidate;
  all 274 existing file paths and the settings hash were preserved.
- Replacement `bb4175ff…676f9` presented the corrected doorway-facing opening;
  its opened screenshot and terminal custody checks retain that positive result.
  Before seeding, final review found two failure-edge defects: a failed New Game
  after a completed load could inherit the old pause gate, and camera acknowledgement
  accepted a tolerance while the simulation input floor required exact equality.
  Failure handling now retains only a pending committed-load obligation, and
  native camera admission uses the exact finite float pose. Debug/Release barrier
  tests cover completed-load failure recovery and one-ULP/NaN rejection. A stale
  initial surface-unavailable notice also now clears on a valid current ready
  publication, while unrelated operation errors remain visible.
- Supplemental motion evidence tests fail in both configurations: the ordinary
  live torch probe sends standing movement beneath the crouch slice's low beam
  and stalls at approximately `(0, -2.995)`. The save-route fixture traverses it
  with actual crouch input. This diagnostic input gap is retained for the next
  bounded fix, now separately recorded in the successful torch diagnostic repair
  linked above; it is not an RT/performance pass or a reason to weaken collision.
- On exact final candidate `ca08919a…18a9`, the seeded lower manual load reached
  RT presentation and its opened live view showed one heart at the rescue anchor.
  Keep records cancellation preserved all six hashes. Explicit replacement of
  Slot 2 manual succeeded with `FileAndDirectoryFlushed`; its 162 bytes exactly
  matched the lower fixture, the other five records stayed unchanged, and Slot 1
  remained active for autosaves. The upper autosave restored three hearts and
  presented the night exterior, including same-generation Home/return, but the
  contextual descent control stayed disabled as Preparing rescue route. Testing
  stopped and Quit cleanly before restart. All 274 prior file paths and the
  settings hash were preserved; only seven test-namespace files were added.
  The renderer adapter skipped the resident dungeon zone, leaving its reconstructed
  readiness unprepared. That is a real acceptance defect, not a full Save/Load
  pass. The bounded adapter repair and upper/restart follow-up are pending.
  Seeded records prove persistence behavior, not earning either checkpoint through
  phone gameplay; normal owner save records remain untouched.
- The [bounded readiness repair](readiness-fix.json) now queries resident Dungeon
  zone 0 through the existing real scene readiness API. Exterior publications
  still require the scene's terrain; the GPU proof is unchanged. All four selected
  native suites pass in Debug and Release, including 25 new adapter checks and
  actual CPU motor descent. Both Windows applications and all four Android ABIs
  build. New APK `5eb3282a…9128` is sealed for only upper descent admission and
  fresh-process reload. That then-pending physical follow-up is now accepted in
  the separate receipt linked at the top; the earlier failed result stays intact.
- Death-menu Load, the Windows adapter, later campaign checkpoints, power-loss
  durability, full audio/route acceptance and sustained performance remain open.
  Existing exterior/two-torch performance blockers are not closed by this work.

Raw local build/test logs live under ignored `reports/`; compact identities and
observed outcomes are committed here instead of APKs or generated captures.
