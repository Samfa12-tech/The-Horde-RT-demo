# Four-direction dodge presentation - 20 October 2026

The existing rig now adds a bounded directional step/dip during accepted modern
dodges. This is a procedural pose family over the existing Idle/Walking samples,
not new imported clips or combat rules. The [receipt](receipt.json) pins exact
source/binary identities, all native image/mesh hashes, the shared snapshot
timeline, resource inventory, timing and original capture locations.

## Implementation and preserved authority

Final shared snapshot publication derives the visual layer from the existing
accepted world direction and 0.20 s timer. Weight is `16*p*p*(1-p)*(1-p)`, zero at
both ends and one at the midpoint. Model direction follows the current yaw and
existing gait twist, so releasing/changing the stick or turning aim does not
rechoose the accepted dodge. No private timer, accepted command, camera motion,
movement, collision, protection window, damage rule, audio event or save field
was added or changed. Legacy checkpoint/constructor fixtures retain their old
presentation; current production rules select the new layer.

The rig translates the pelvis at most **8 cm down and 4 cm toward the dodge**.
Continuous leading-foot bias moves the model-space ankles 0-8 cm horizontally,
retaining their height/orientation. Existing two-bone leg IK preserves lengths,
then the same arm solve reaches the unchanged held-item targets. Torso queries
and sword stow mounts use the identical translation. World body and primary
viewmodel consume the same immutable solved palette. Zero weight preserves the
ordinary palette; crouch/planted and dodge requests cannot overlap. These are
visual ankle targets, **not world-foot locking or root motion**.

Publication happens after traversal/death overrides. Existing acceptance-state
cancellation prevents a dodge timer suspended by rope traversal from reviving a
pose after completion. Pause already cancels the dodge, and retains that policy.
Reset, retry, restore, authoritative capture, crouch and reveal holds clear the
visual when their existing state cancels or suppresses it. Cadence/cache checks
include the new immutable animation fields and reject malformed requests before
reusing a palette.

## Checked result

- **16 affected CTest registrations pass** on frozen source: the three new dodge
  suites plus player animation/directional/crouch, frame adapter/survey launch,
  combat foundation, held sockets, lantern reward/pendulum, rescue traversal and
  campaign codec/checkpoint/play-session contracts.
- The publication suite passes **750 checks**, including all four 0.90 m paths,
  existing half-open protection timing, released input/changing aim, event parity,
  zero-delta publication, cancellation and all 64 native fixture snapshots.
- The actual runtime-asset rig passes **562 pose samples / 9,230 checks** covering
  phase and full-circle direction sweeps, rigid leg lengths, ankle height and
  orientation, at most 1 mm floor penetration, bounded knee continuity, unchanged
  grips, primary/world shared palette, zero-layer vertex/tangent bytes, mounts,
  same-tick cache changes and invalid-input recovery.
- **28 lifecycle checks** use real shared rope ascent/descent and logical restore
  fixtures with actual terrain. They retain the existing suspended timer/accepted
  ID while preventing immediate, completion and first-post-rope pose resurrection.
  Prepare/cancel/failed/committed restore, retry and zero-tick pause are covered.
  Hostile same-tick death was not newly exercised; no denied contact test was
  repeated. The final death-state mask was inspected and existing combat checks
  pass, which is not a new physical death acceptance claim.
- Windows Debug/Release application builds and Android Debug's four packaged ABIs
  pass. Android uses Shipping instrumentation / Mobile dielectric quality and is
  **build-only**. APK SHA-256:
  `8fe8abf46e170a1a6dfdf8ae5cf9d8cc269c36b2747cbb8a341ee95d40e275d0`.
  No phone was queried, installed, launched or changed; Java source is unchanged.

The initial five suites passed. A later fixture check failed once because its
label assertion expected an imported lantern after Reset, although normal Reset
correctly restores the torch. The assertion was corrected, current held-light
identity was added to capture metadata, and the full affected set then passed.
[That failed result](rig-lifecycle-first-tests.log) is retained. No gameplay fix
or asset substitution was used to conceal it. One orchestrator build batch was
interrupted after writing the freeze file; checks found no launched builds/logs.
The subsequent individually started builds completed. No uncertain process or
approval rejection remains.

## Native RT and exact upload evidence

Four unshown finite captures each pass **64/64 matching presented and completed
hardware RT frames**: visual off/on for trace-rays and BLAS/TLAS-backed hardware
ray-query compute. They inspect separately stepped shared production snapshots,
without advancing the application's simulation, earning progress, opening menus,
acquiring input or writing saves/preferences. The imported lantern is explicitly
a noncombat equipment fixture. Each direction/equipment pair has before, ticks
1/3/6/9/12/18, plus real pause cancellation/resume/reset for each equipment family.
The `lantern-...-reset` label names its fixture family; its recorded current item
is correctly the restored torch.

The application snapshot bytes and settings INI remain identical throughout.
All processes exit 0. Foreground APIs return null across **689 samples** and the
native window stays unshown/unfocused. This does not certify continuity of a
particular external foreground HWND. The stationary environment rebase still
rejects active dodge; its guard was strengthened rather than relaxed. The new
profile supplies a complete immutable frame through SimulationFrameAdapter.

RTX 5050 Laptop / driver 610.47, Diagnostic/High, **960x540 at 100%**, High water/
fire, Current shadows, Low dust and **PC Mist On** are matched. No shader or FOV
change is used. All 64 resource inventories remain constant in every run, with
**zero feature delta in live bytes, buffers, allocations, BLAS/TLAS or instances**.
Both retain 20 BLAS, one TLAS and 28 instances. Maximum completed-frame grip error
is **10 micrometres** in all four runs.

| Active-pose rows, 34 matched snapshots | Pose off median | Pose on median | Delta |
| --- | --- | --- | --- |
| Trace-rays whole RT GPU | 8.323184 ms | 8.275824 ms | -0.047360 ms |
| Compute whole RT GPU | 7.958288 ms | 7.942736 ms | -0.015552 ms |
| Trace-rays Debug player skin CPU | 20.768200 ms | 21.101900 ms | +0.333700 ms |
| Compute Debug player skin CPU | 20.829100 ms | 20.768950 ms | -0.060150 ms |

Median AS command-record deltas stay within 0.003 ms. Full tails and counters are
in the receipt. These short Debug readback runs show no matched material
regression, not an optimisation gain, sustained FPS, Release CPU cost, warm phone
headroom or physical controller acceptance. Fixture construction is outside frame
timing. Diagnostic skin time must not be relabelled as Shipping performance.

Each run retains 64 original RT PNGs and sixteen **exact CPU-upload** world-body
meshes with their TLAS transforms; mesh export is not GPU geometry readback. The
orthographic overlay in ignored reports compares actual off/on midpoint uploads
without root travel or exaggerated displacement. It shows the bounded dip and
directional ankle/knee changes while arms continue to the same targets. Native
first-person images verify the held-item view; they cannot establish complete
boot readability or all shadow/reflection composition. Host tests verify palette
sharing, while full artistic/live sequence and physical acceptance remain open.

Complete originals remain under ignored `reports/player-dodge-20261020-r1/`.
Compact completed-frame extracts and hashes are committed here; no PNG, OBJ, APK
or executable is added to Git. The preserved [runner](hidden-rt-runner.py) is an
exact-run recipe whose existing r1 directory must not be reused. Use fresh named
reports paths for an explicitly authorized reproduction. The
[analysis script](analyze-surveys.py) checks the full manifests/artifacts and
constructs the separately labelled scientific geometry overlay.

## Remaining gates and next planned implementation

Mobile **Mist Off** startup/reset and prior explicit owner direction remain
unchanged. Canonical owner-dirty licence/memory files retain their hashes. No
phone, public push/merge/release, paid acquisition, new worktree, owner reset/stash,
security/budget or Reef operation occurred. Debris remains static; kicking and a
player-facing debris toggle are still not implemented.

Audio/haptic manual revalidation required: **NO**. Gameplay trajectories, event
timing/identity, source/listener data, playback and haptic inputs are unchanged.
Natural full-route/return and earned saves/death Load, live contact/first-time
teaching, controls/listening and sustained warm Keeper/exterior acceptance remain
open. This checkpoint is not a release candidate approval.

Next meaningful planned item is **surface-aware dry footstep dispatch** (accepted
master 5.5). Support triangles already distinguish Stone/Earth/Wood and authored
terrain marks earth/stone, but dry step events still emit payload zero and select
the same two FilmCow clips. Reuse those actual support tags and existing cadence/
event/audio owners; first audit licensed local cue availability, then carry a
small typed surface identity through the event and shared cue selection for
surfaces actually present. Preserve the existing shallow-water path, default for
untagged ground and no-step rules at walls/pause/rope/restore. No invented wood
bridge, new audio engine or paid acquisition is implied. Actual material-specific
audio needs suitable provenance and later listening acceptance; transport changes
would require audio manual revalidation in that subsequent slice. Healing still
needs its accepted bounded policy decision.
