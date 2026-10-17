# Horde 1.7 logical safe-checkpoint recovery

Parent: `1014a97ce5b3e467608fba8d151eaeb9c046ed23`. Work remains on
`codex/horde-1.7-wp2-vertical-proof` in the existing
`C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.7-wp2-vertical-proof`.
This implements the [bounded save slice](../../design/HORDE_1_7_SAVE_SLICE.md).
It is a local, unpushed development checkpoint. No device, public PR update,
release, renderer/shader/asset change or performance claim is part of it.

## Implemented boundary

`campaign::Checkpoint` is a closed schema/content-revision 1 logical value,
with authored `LowerClaimed` and `SafeExit` identities. It is not a byte codec
and its C++ memory layout is not a file format. A later decoder must validate
typed fields explicitly rather than copying raw bytes into it. Unknown versions
and IDs, nonfinite/out-of-range values, contradictory victory/zone/story facts
and missing required proof responses fail validation.

Capture admits only the completed tomb: two defeated guards, fully retired
Keeper and cleared mist, claimed reward, open lid, settled ready rope, alive
grounded player at the authored landing, no live combat/movement/equipment
transition, resource lease or pending restore, and current terrain/readiness.
The lower boundary excludes reunion progress. A returned player with reunion
history uses the previously committed exterior record; this slice supplies no
automatic last-safe selection. Arbitrary grounded positions are not checkpoints.

Owner-thread `BeginCampaignRestore` validates/stages without changing the live
snapshot. `CommitCampaignRestore` requires the matching request and fresh
destination-generation/zone token plus actual support/capsule clearance. It
constructs a fresh simulation offside before replacing the live session.
Invalid/replaced/cancelled requests, old readiness, failed preparation and reset
cannot partially load a record. Only the supplied destination is declared ready;
other zones still require their normal owner preparation. No presentation flag
or successful RT receipt is invented. The platform must obtain that readiness
from real resource preparation; the host fixture injects the owner boundary.

Victory, original torch-drop anchor, sword socket, settled lantern pose,
one/two/three hearts, first-ascent fact, open lid and night are retained.
Temporary damage/death protection, attack/dodge/run debt, animation clocks,
rope nodes, callbacks, input and GPU identities are reconstructed rather than
saved. Corpses use the existing terminal nonlooping death pose; the rope uses
deterministic safe-endpoint construction. Kit is grounded at the existing
reunion location on the imported terrain, rather than resuming an arbitrary
following waypoint or animation clock. Both reconstructed rope directions can
finish an ordinary traversal without a second claim/deployment.

Logical dialogue retains once flags and ordered unfinished continuations.
Active speech restarts at its beginning with current source/listener tuples and
fresh callback identity. Completed speech does not replay. Pending proof needs
a fresh manual raise plus matching presented-frame acknowledgement; merely
loading a saved raised lantern does not prove it. Active Keeper speech and the
unused ForestWait identity are unsupported. Existing forest once flags can be
retained, but there is no forest-end checkpoint ID in this version.

Commit consumes old command floors and returns a paused, neutral snapshot.
The owner must rebase its absolute camera to the saved yaw/pitch, release axes,
run and replay-only authoritative pose, and publish a newer coherent input
identity before simulation resumes. Repeated stale publications remain paused
without moving, turning or teleporting the player. Direct deterministic host
stepping uses sequence zero and a neutral rebased snapshot. A saved drawn sword
changes the loaded session only; a later New Game keeps its configured stowed
opening policy.

## Validation and retained development failures

Exact source, binary and local log hashes, commands and final results are in
[validation.json](validation.json). Native CTest selections cover the new
campaign fixture, dialogue, shared gameplay, combat foundation, rescue motor,
ordinary/authored-terrain journey, rig attachment and adversarial traversal.
The new fixture passes **722 checks** including 24 combinations of safe side,
health, sword and lantern pose through fresh simulations on the real imported
terrain. Initial logical records are constructed values; these tests do not
claim a new full physical campaign playthrough.

Final validation: Debug **9/9**, Release **9/9**; both Windows applications
compile. Android `assembleDebug --offline --console=plain` compiles arm64-v8a,
armeabi-v7a, x86 and x86_64. No APK was installed or launched. The selected CI
and sanitizer rosters include the new fixture, but no current-source cloud CI
or sanitizer run is claimed before push.

The first campaign test run failed five assertions: its return-descent fixture
used the exterior-terrain API for dungeon readiness, and its raise fixture
submitted the second toggle before lowering completed. The next run still used
only 0.5 seconds for a real 0.65-second pose transition and failed three checks.
Fixtures now use the correct dungeon publication and the actual transition
constant, with completed-pose assertions. No production readiness/pose rule was
weakened. Review separately found/fixed the saved sword leaking into New Game,
missing required dialogue suffix validation and old continuous input replay.
Regression checks cover those cases. A Release build failed LNK1104 when an
overlapping test held its executable open; linking and all nine checks were
then rerun in order against completed final binaries. Rejected runs remain
listed in the receipt rather than counted as acceptance.

Audio/haptic manual revalidation required: **YES, at platform load integration**.
Restore can restart active dialogue with fresh event-time/source/listener and
callback identities. Automated continuation/callback checks do not establish
perceived playback, subtitles, cancellation or vibration correctness. There is
no player-facing load entry point to exercise in this slice, and no owner check
is requested now.

## Remaining acceptance and next step

This closes logical reconstruction only. Disk codec, corrupt-file handling,
atomic durable writes, process death, three slot isolation, manual/autosave
retention, automatic checkpoint/death coordination, menus and the real platform
camera/resource/load adapter remain open. It does not close RT image/physical
phone acceptance, touch/audio feel, sustained performance or the active-light
regression. The retained optimisation and milestone map remain in the
[continuation receipt](../2026-10-17-checkpoint-admission/README.md).

Next bounded slice: a separate typed codec and atomic file adapter tested in
temporary directories with injected open/write/flush/replace failures, corrupt
and unsupported versions, slot isolation and stale success receipts. Preserve
the previous valid record on every failure. Do not connect menu/autosave
overwrite behavior until manual snapshot retention is settled; three slots
remain the direction. No destructive overwrite policy is chosen here.
