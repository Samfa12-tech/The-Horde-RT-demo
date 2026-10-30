# Bounded owl/cricket integration and prepared pond/cicada cuts

The same-night forest now maps its finite bird request to an attributed tawny
owl cut and adds an evening-cricket request twelve seconds later. Owl cues remain
at eligible ticks 720, 3120, 5520; cricket cues occur at 1440, 3840. Each recording
is eight seconds, followed by a quiet gap. They share one finite native voice;
the newest signal wins and missed cues are dropped. There is no continuous loop,
backlog, random source, save field, scene geometry, light or quality change.

The existing shared eligibility remains: ready ForestApproach/Lookout, same-night
rescue continuation, alive/grounded/not stowed, accepted lantern proof and no
dialogue. Pause, speech, death, route/restore and platform focus/lifecycle stop
the voice and require the normal quiet interval on re-entry. Old event IDs remain
stable (owl keeps 28, stop 29); cricket appends 30. Windows XAudio2 and Android
SoundPool map the same semantic cues. Owner SFX/voice settings are unchanged.

## Sources, processing and evidence limits

The [asset manifest](../../../assets/audio/night-ambience/manifest.json),
[packaged notices](../../../assets/audio/night-ambience/NOTICE.md) and
[licence ledger](../../../ASSET_LICENSES.md#free-nature-recordings-for-the-same-night-exterior-18-october-2026)
pin each original hash, author, source page, exact licence version and derivative.
All originals were retrieved anonymously from the original-file links on their
verified Commons pages; large originals remain in ignored local staging. No paid
service, account, generation or credential operation was used.

| Cut | Source metadata and disposition |
| --- | --- |
| Tawny owl | W.carter, Strix aluco at night, CC BY 4.0. Prior author cuts/noise reduction are disclosed. Source 0–8s, mono 24kHz PCM16, -12dB, 150ms fades. Runtime peak -22.98dBFS / RMS -42.29dBFS. |
| Evening cricket | Gunther Tschuch, Oecanthus pellucens at 21:04, CC BY-SA 4.0; adapted audio retains ShareAlike terms. Source 2–10s, mono 24kHz PCM16, -18dB, 150ms fades. Runtime peak -31.33dBFS / RMS -41.67dBFS. Evening time is metadata; darkness is not independently established. |
| Pond frogs/crickets | Glaneur de sons, mixed dusk pond, CC BY 3.0 as recorded by Commons. Source 2–10s, -12dB, same format/fades. Prepared candidate only, excluded from packages and playback. |
| Periodical cicada | Fontaine/Cooley/Simon, Magicicada septendecim, PLOS Audio S1, CC BY 2.5. First 5.8s, -15dB, same format/fades. No night metadata; prepared candidate only, excluded from packages/playback. |

These clips are **unheard here**. Levels, fades, waveform/spectral inspection and
source metadata do not establish native audibility, quality, species identification
by listening or VO/mix acceptance. The inspected provisional owl spectrum shows
a finite tonal passage; it is not an audition. No desktop foreground was taken,
no audio was played for acceptance, and no APK was installed. Owner listening
and exact-platform mix remain open; further paid generation is unnecessary.

The first crop/fade check failed on the two cuts starting at 2s: output seeking
did not reset filter time, so the start fades were missed. The corrected recipe
resamples, trims and resets timestamps before applying fades. Final checks retain
the endpoint assertion and pass. All six prepared files reproduce byte-for-byte
through `tools/prepare-night-ambience.py`; the failed log remains pinned.

Two active clips decode to 768,000 PCM bytes, 27,662 fewer than the retired
generic bird. Their WAV payload totals 768,156 bytes; each is below
[SoundPool's documented decoded-sample bound](https://developer.android.com/reference/android/media/SoundPool).
Android's exact package roster includes those two clips,
manifest and notices; excludes both candidates, originals and old generic WAV.
The Windows runtime-copy helper copies the same four files and refuses to
overwrite a different destination. It is tested in isolated ignored staging;
the release packager is updated to call it but is **not executed**.

## Frogs and exterior water

Current regional terrain contains opaque lake/river **beds**, not admitted water
surfaces. The r21 source receipt explicitly leaves distant water integration open.
The lake bed is far from the playable forest, engine X/Z bounds
[17.3,244.3] x [366.2,585.2]. The current `WaterSurfaceAt` API covers only tomb
catchment/runoff. It cannot supply honest exterior frog proximity.
The prepared mixed pond cut therefore remains silent. A later water slice must
admit actual surfaces and authoritative proximity before enabling it. The
requested near-water frog behavior and verified nocturnal cicadas remain gaps;
this does not claim a completed species soundscape or full regional water.

## Checks and remaining work

[validation.json](validation.json) pins source/build/test/package hashes and the
exact uninstalled APK. The focused native fixture includes ordinary host
guards/Keeper/claim/ascent/forest progression, both emitted ambience events,
fixed-step cadence at four render rates, suppression and fresh-restored clearing.
It uses CPU terrain readiness with damage/teaching disabled; it is not physical
traversal, combat, playback, performance or owner acceptance. Affected Android
host control/menu/lifecycle checks and platform compilation are separate lanes.

The [read-only contact follow-up](contact-diagnostic-followup.json) records the
remaining phone evidence gap. No gameplay was resumed and the denied combat
sequence was not retried. Natural earned autosave/manual separation and physical
death Load remain incomplete. The next combat-related slice is a bounded paused
owner-thread current player/target/contact diagnostic, then report/review before
any evidence-based test. Merely resolving diagnostics would not authorize the
previously denied blind input sequence.
