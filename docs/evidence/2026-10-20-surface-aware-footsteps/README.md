# Surface-aware dry footsteps - 20 October 2026

This bounded checkpoint connects the existing supporting-surface tags to one
authoritative dry-contact cue. It uses only already admitted FilmCow WAVs; no
new asset, audio engine, terrain class, save field or movement tuning is added.
The existing WP2 worktree/branch and draft PR 27 direction are retained.

## Result and authority

`GameSimulation::UpdateMovement` still drives the first contact after 0.24 m
and later contacts every 0.72 m of actual collision-resolved travel. At contact,
the event captures the current support's Stone/Earth/Wood identity plus one of
two variants. Alternation belongs to the simulation and is independent of mute,
Java polling and platform frame rate. Stone/Wood share their audible fallback
family, while Earth alternates independently. Crossing a material boundary does
not add an event or reset distance cadence.

Groundless travel and authored teleport/replay input cannot feed ground cadence.
Rope-owned movement resets partial cadence; pause, lifecycle synchronization,
checkpoint/retry and new-session restore also reset cadence and variants. There
is no new jump or landing mechanic. Existing wet membership has priority and
emits one wet event instead of a dry event; stream entry remains separate. The
existing wet variant counter is retained, and dry contacts no longer advance it.

Windows resolves the selected clip through the existing listener-local player
path. Android's existing type-scoped cue byte carries the same shared selection,
with type/source/target/sequence and stereo/vertical words preserved. Equipment
cues and the bounded 128-entry Android queue remain intact. Each contact plays
one existing sample at the accepted **0.29 player gain** through existing SFX
volume/mute handling. Music, Dialogue, saved preferences, spatial gains, haptic
patterns, voice caps and asset loading are unchanged. No layering or pitch/gain
randomisation is added.

## Local cue audit and honest limits

The importer-referenced FilmCow bank exists at the local Possum Cafe archive.
Its dedicated step families are dirt, grass, leaves and branch contacts. No
filename identifies stone/concrete/tile or wood-floor steps. Rock/wood impact
foley is not promoted to verified footsteps on the strength of its name.

| Support | Selected admitted clips | Source identity / disposition |
| --- | --- | --- |
| Stone/default | `player_step_1.wav`, `player_step_2.wav` | FilmCow dirt 9/14; preserves the accepted tomb pair. These are generic fallback, not a new verified stone recording. |
| Earth | `skeleton_step_1.wav`, `skeleton_step_2.wav` | FilmCow dirt 17/18 reused at player gain; the existing filenames do not imply a skeleton event. |
| Wood | Same generic pair as Stone | Explicit fallback; suitable admitted wood-floor cues remain missing. No bridge/content added. |

[Audio data audit](audio-audit.json) records source/runtime SHA-256, PCM headers,
durations, peak/RMS, source bank and the existing FilmCow licence-PDF hash. All
four runtime WAVs match HEAD and reproduce the importer's PCM24-to-PCM16/0.78-peak
recipe byte for byte in memory. Sources stay outside repository/packages. The
existing [licence entry](../../../ASSET_LICENSES.md) already covers all four;
no new licence admission or legal refresh is claimed.

All are mono 48 kHz PCM16 runtime, 0.334-0.544 s long, with peak 0.779999 and no
digital full-scale samples. At 0.29 player gain the peak is 0.226200 before other
mix gains. Earth full-file RMS is about 6 dB lower than the retained pair despite
equal peaks. Full-file RMS includes tails and is not perceived loudness. No
automatic compensation or listening approval is inferred from these numbers.

## Checks, builds and retained failures

- **459 new host checks pass**: shared surface/cue/payload selection and Android
  byte transport; fallback and wrong-source/event guards; per-family alternation;
  independent distance thresholds for straight/diagonal/run travel; wall,
  stationary, pause/resume and teleport/re-entry suppression; real wet priority;
  actual runtime terrain Stone-to-Earth traversal without boundary retrigger;
  actual rope ascent/descent and logical safe-side restore silence.
- The airborne gate is a cadence unit check, not a physical jump-motor pass.
  Restore fixtures are logical CPU tests, not naturally earned saves. No owner
  records are read or written by this suite.
- **13 affected CTest registrations have passing coverage** across the retained
  eleven passes and two follow-up passes: footstep, simulation gameplay, spatial
  audio, SFX volume, water, rescue/actual chapter terrain, campaign codec/
  checkpoint/play-session, combat foundation and dodge presentation contracts.
- **Five selected Java tests pass**: two native cue-byte mappings/invalid-cue
  cases and three existing equipment-cue guards. This is not the full Java suite.
- Windows Debug/Release application builds and Android Debug's arm64-v8a,
  armeabi-v7a, x86 and x86_64 packages pass. Android uses Shipping instrumentation
  and Mobile dielectric quality. The receipt pins binaries/APK and verifies the
  four reused cue bytes inside the APK. No install, launch or phone query occurs.

The first standalone suite passed 456 checks before the teleport guard and its
three regressions were added. Both affected platforms were rebuilt afterward.
The first selected run retains **11 passes and two failures**. Campaign play
session timed out in the restricted process while creating its isolated temporary
disk fixtures. The unchanged binary passes in 3.09 s under normal host permissions;
the precise restriction/timing cause was not instrumented. No storage or save
implementation changed to produce that pass.

The other failure was a stale source contract looking for the old zero-argument
`StopSurfaceInternal()` signature. HEAD/current bridge has the same Git blob and
normalized bytes, with `bool preserveGameplay=false`. The test now locates that
existing signature and retains the queue-clear/active-surface assertions. Only
that test was rebuilt; the two failed registrations then both pass. Original
failure output, exact initial/selected/final source freezes and follow-up output
remain here. No assertion or timeout was weakened, and no failed result is erased.

## Remaining acceptance and next substantial gaps

**Audio/haptic manual revalidation required: YES.** Surface cue selection, dry
event payload/transport and teleport suppression changed. Actual listening was
not performed. Current cue character, Earth/default balance, tails at run cadence,
speech/combat intelligibility, SFX-zero, pause/resume and mono-phone acceptance
remain open. Haptic routing/patterns are unchanged. No new RT presentation,
sustained performance or physical controller acceptance is claimed by builds.

The physical critical path remains natural tomb-to-lookout/return with earned
saves/death Load, first-time teaching/contact, live controls/listening and warm
Keeper/exterior performance with mobile **Mist Off**. No denied contact action
was retried. Canonical owner licence/memory changes are preserved. No phone,
public push/release, purchase, new worktree, reset/stash or Reef operation occurs.

The next substantial accepted source gap to reconcile is master section 11's
rescue/forest music continuation. The existing `MusicDirector` resolves A-H from
tomb combat/torch/finale state; it has no explicit rescue/exterior/forest
selection. Legacy F/G/H finale logic still determines chapter music: defeated
inactive states resolve F, while natural pickup can run G then H. This is source
inspection, not listening evidence. The admitted owner pack exists, so first map
its existing cue roles against natural and restored chapter transitions before
changing shared state selection/tests. This is not authority to create a second
engine, recompose the owner-accepted pack or buy a score. Exact listening remains
necessary for a later music slice. That work is not implemented by this checkpoint.

Other material gaps stay explicit: the first usable recovery method still needs
the bounded capacity/refill/use/death/save policy in master 10.5; nearby debris
kicking and a player-facing debris option remain unimplemented; whole-opening
art and distinct wood/stone cue suitability remain unaccepted. Do not trade
these for heavy outdoor content or another open-ended optimisation loop.
