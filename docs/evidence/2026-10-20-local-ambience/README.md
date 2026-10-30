# Finite local tomb and rope audio - 20 October 2026

This checkpoint closes two concrete accepted sound behaviors using the existing
licensed FilmCow archive: sparse local water activity in the cleared tomb room,
and contact/strain during actual rope grip travel. It continues the existing WP2
worktree, branch and draft PR 27. It adds no continuous bed or independent mixer.

## Shared behavior and platform ownership

The shared 60 Hz simulation owns both schedules, independent of platform frame
rate, mute and polling. Drips require the playable chapter's lower side,
grounded route-floor support, the waterfall room within four metres of the fixed
source, no remaining active guards, no speech, and a living unpaused player.
The source `(-3.4,-0.928,-15.25)` is inside the existing runoff face. The first
cue follows six quiet seconds, then intervals cycle through 18, 24 and 21 seconds.
Leaving eligibility stops the finite voice and discards cadence debt.

Rope contact requires claimed/ready rope, stowed equipment and active hands in
ascent/pull-up/descent/landing. It follows **0.35 m of actual grip-index travel**,
with at least **72 active ticks (1.2 s)** between cues. Solved rope sway and static
hands do not generate contact. A new ownership/generation, pause, retry, reset,
restore or discontinuous grip import rebases travel; there is no queued catch-up.
Source position is the current hand-target midpoint, using normal fixed-tick
listener position/yaw/height. Existing spatialization is planar; preserved height
metadata does not imply elevation mixing. No movement, grip or save rule changes.

Android and Windows reuse the existing **one finite chapter incidental voice**
with forest owl/cricket/pond cues. Latest-signal coalescing drops stale earlier
starts when a later stop or habitat change arrives in the same batch. Windows
stops/destroys that voice before starting another and during campaign replacement;
Android retains SoundPool loop-count zero, stop-before-play and existing surface,
focus, menu, speech and mute cleanup. Missing/unloaded sounds are dropped without
retry. The three appended IDs 32-34 preserve all existing event IDs and the
128-entry Android queue. Event intensity is applied once, followed by existing
SFX gain; Music, Voice, stored settings and haptics are unchanged.

## Licensed cut audit

The [manifest](../../../assets/audio/chapter-local-ambience/asset.manifest.json)
pins source/runtime/PDF hashes and exact processing. Private originals are read
only from the importer-referenced Possum Cafe FilmCow archive and stay outside
Git/packages. The existing owner-supplied FilmCow custom project-use licence
applies, not CC0; these derivatives are recorded in [ASSET_LICENSES](../../../ASSET_LICENSES.md).

| Cue | Exact private-source selection | Runtime candidate |
| --- | --- | --- |
| Tomb drip | `ambience - dripping water 1.wav`, source frames [5,083,200,5,131,200), 105.9-106.9 s | 48,000 frames, 96,044 WAV bytes, peak -21.47 dBFS, RMS -32.41 dBFS, emitter gain 0.25. |
| Rope contact | Complete `rope tighten 1.wav` | 47,600 frames, 95,244 WAV bytes, peak -13.00 dBFS, RMS -33.65 dBFS, emitter gain 0.18. |

Both are mono 48 kHz PCM16 with 10/80 ms edge fades and no normalization, filter,
pitch, time-stretch or synthesis. Total **191,288 WAV bytes / 191,200 PCM bytes**;
each is at most one second, shorter than the rope minimum interval. The recipe
reproduces both WAVs and the manifest byte for byte with `--verify`, writing
nothing. Purpose labels and isolated PCM activity support candidate selection;
they do not establish audible character or sufficient mix level.

The archive also contains rock/rock-handling cuts, but no filename certifies
structural stone creaking. None is relabelled as such. Grit/stone-creak suitability
remains missing, and the owner-supplied metal menu creak is not reused as rope.
Existing music, menu, combat, footsteps, species and waterfall PCM is unchanged.

## Verification and evidence

- **107 new native checks pass**: location/wet-face membership, life/guard/speech/
  pause/legacy/height gates, cadence at multiple render groupings, duplicate tick
  and identity suppression, actual grip travel versus sway, cooldown/import,
  transport and platform source-wiring guards. Actual imported terrain and the
  real shared motor cover both rope directions, pause, restore and end silence.
  An ordinary CPU walk from a logical defeated lower restore reaches the quiet
  water room and emits correctly positioned current-tick drips.
- **16/16 selected Release CTests pass**, 38.21 s: new scheduler/asset contracts,
  forest, dry contacts, shared gameplay, spatial/SFX, music, campaign and rescue/
  chapter-terrain checks. This is an affected lane, not a complete repository gate.
- **28 selected Java tests pass**, zero skipped/failures/errors: chapter mapping
  (2), menu ambience (7), music focus (9), output buffer (5) and output gain (5).
  These use host/mocked boundaries, not real output or listening.
- Windows Debug/Release and four-ABI Android Debug application builds are pinned
  in [the receipt](receipt.json). Android uses Shipping/Mobile. The APK audit
  checks exact new cue/manifest bytes and excludes private original filenames.
  The existing Windows package helper stages all eight admitted night/local
  audio/metadata files byte-exact twice; no release package is produced.

The [source freeze](source-freeze.json) records nineteen build/test/recipe/runtime
files before checks/builds, with final raw hashes and normalized Git/LFS blobs.
No source fix was required after freezing. The independent final source review
found no actionable defect. Original build/test output is retained here. A minor
combined documentation patch failed its context preflight before any file write;
its diagnosis and corrected separate application are retained in
`patch-preflight.log`. No denied action or failed runtime gate is hidden.
The first staged whitespace check also reported four compiler-log blank lines
containing spaces. `staging-check-first.log` retains that result. A scoped
`.gitattributes` raw-log rule follows existing evidence conventions and preserves
the original log bytes; no compiler output was trimmed or game check changed.
The first raw-byte staging assertion then caught Git retaining its previously
normalized LF index entry after the new attribute was added. Source/runtime
blobs had already matched. `staging-log-normalization-first.log` retains the
diagnosis; scoped reapplication of the clean attributes to these log entries
restored byte-exact CRLF evidence before committing.

## Remaining acceptance and next priority

**Audio/haptic manual revalidation required: YES.** New audible assets, semantic
events and shared voice routing need actual listening on both platforms: cue
character/audibility, localization/pan, balance beside water/steps/combat/Kit,
pause/focus/mute, repeated ascent/descent, area exit and save/load cleanup.
Haptic routing/patterns are unchanged. No output audio device was opened, no
platform playback/leak measurement was made, and no listening pass is claimed.
Logical restore fixtures do not establish a naturally won fight, physically
earned save, controller/touch acceptance, RT presentation or warm performance.

The next independent accepted source gap is **gentle dialogue ducking with smooth
recovery** for music/ambient beds (master 11). Current finite incidentals stop for
Kit, but music only has the Keeper reveal envelope and waterfall gain remains
spatial gain times SFX. Keep combat SFX, Voice, settings, cue cursors and lifecycle
ownership intact; test completion/skip, pause, load/reset and stale identities.
This is a concrete story-intelligibility requirement, not extra score polish.

First recovery remains the major gameplay-policy decision. The [morning record](../../design/HORDE_1_7_RECOVERY_DECISIONS_FOR_MORNING_2026_10_20.md)
retains exact method/availability, capacity/amount, refill, use/interruption, death
and migration choices, including established health resets. No value is chosen.
Nearby debris kicking and a detail option are conditional lower-priority work;
no other mandatory story/progression mechanism was found missing in this audit.

Natural full-route/return, earned saves/death Load, first-time teaching/contact,
live controls/listening and sustained warm Keeper/exterior acceptance remain open
with mobile **Mist Off**. Canonical owner files/assets are preserved. No phone,
public push/release, purchase, new checkout, reset/stash or Reef operation occurs.
