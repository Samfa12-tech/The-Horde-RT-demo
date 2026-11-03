# Skeleton and lich audio sources for 1.6.2

Organised on 2026-10-01. Status: **source assets staged; 1.6.2 integration has not started**.

The user selected these four clips for skeleton and lich sound effects in 1.6.2.
This intake records that request separately from source-page descriptions.
It does not change the release version, gameplay, playback, haptics, or packaging.

## Source inventory

| Stable ID | Source / credit | Original duration | Format |
|---|---|---|---|
| `skeleton_falling_bones` | [Falling Bones](https://pixabay.com/sound-effects/film-special-effects-falling-bones-87568/) — spookymodem (Freesound) via freesound_community | 2.928000 s | MP3, 24 kHz, stereo |
| `skeleton_rattling_bones` | [Rattling Bones](https://pixabay.com/sound-effects/film-special-effects-rattling-bones-105394/) — spookymodem (Freesound) via freesound_community | 5.520000 s | MP3, 24 kHz, stereo |
| `lich_i_sense_you` | [Lich Demonic Voice - I Sense You!](https://pixabay.com/sound-effects/people-lich-demonic-voice-i-sense-you-490452/) — PhatPhrogStudio | 2.716719 s | MP3, 44.1 kHz, mono |
| `lich_come_closer` | [Lich Demonic Voice - Come Closer!](https://pixabay.com/sound-effects/horror-lich-demonic-voice-come-closer-502312/) — PhatPhrogStudio | 2.873469 s | MP3, 44.1 kHz, mono |

[manifest.json](manifest.json) records each original filename, SHA-256, byte size,
source item, publication date, credit, and media details. Durations are probed MP3
container durations, not authored cue windows.

The original files are copied byte-for-byte into `originals/`; the Downloads
copies remain untouched. The original MP3s are local-only and ignored by Git.
The manifest and this document can be tracked. A fresh checkout therefore needs
the four licensed downloads restored under their exact filenames and checked
against the recorded hashes.

## Licence and distribution

All four supplied source pages were checked on 2026-10-01 and identify the
[Pixabay Content License](https://pixabay.com/service/license-summary/).
The [full terms](https://pixabay.com/service/terms/) were checked too
(page last updated 2024-11-18). They permit commercial/non-commercial use and
adaptation subject to their conditions, with optional attribution, and prohibit
standalone distribution. Keep the original sound downloads out of public Git
history and release packages. Any future runtime derivatives should be used as
part of the finished game rather than offered as a standalone sound library.

The bone pages identify spookymodem (Freesound) as the artist and
freesound_community as the uploader. Their original Freesound item IDs/licences
have not been independently established; these records use the acquired Pixabay
licence and do not infer CC0 from the existing chest-opening asset.
The lich pages identify PhatPhrogStudio. Voluntary credits are retained in the
manifest and [asset licence register](../../../../ASSET_LICENSES.md).
No download certificates were supplied.

## Later integration

Exact triggers, repetitions, cooldowns, trims, gain, and any replacement of
existing cues remain undecided. Possible roles to review when 1.6.2 begins:

- Falling bones: skeleton collapse/death candidate.
- Rattling bones: skeleton movement or idle bone-rattle candidate.
- I Sense You: lich awareness/encounter candidate.
- Come Closer: lich invitation/taunt candidate.

Those are proposals, not approved trigger mappings. Prepare mono 48 kHz PCM16
runtime derivatives through the existing audio conventions once cue windows
and mixing are chosen. Preserve ordered shared gameplay events, source/listener
identity at event time, the 140 ms skeleton impact/fall separation, and
reset/retry/lifecycle cancellation. Validate Android and Windows routing and
audition the exact integrated candidates before acceptance.

## Intake validation

All four MP3s were probed and decoded fully to null with FFmpeg without reported
errors; copied originals must match the manifest hashes. No listening or in-game
review has been performed. Android uses explicit runtime asset includes and
Windows copies only the runtime FilmCow/Pixabay WAV directories; this source
directory is outside those selections.

**Audio/haptic manual revalidation required: NO** for this source-only intake:
no active cue, asset, event, backend, or haptic routing changed.
**Audio/haptic manual revalidation required: YES** when these clips are integrated
into playback, because the audible cue assets and potentially timing will change.
