# What the Dark Keeps - bounded instrumentation audition

Status: owner selected **whistle-lead** after both comparisons. Full-bank
instrumentation admitted locally; objective source/PCM/Core/package checks pass.
Exact-S26 rate/crackle/timbre listening accepted; twenty-period cueH device clock
and basic pause/Home/resume now pass. Android focus/lifecycle gating is implemented
and host/device-smoke verified; external interruption, full cue-route/transitions
and SFX masking remain open. [Current focus record](evidence/2026-10-02-music-focus/README.md).
Owner steering: preserve the composition and gameplay/cue timing; replace the
sci-fi instrumental character with an intimate, ancient, mournful palette.
No paid generation, shared-preset edits, renderer/gameplay changes or publication.

## Frozen reference and route

Frozen original reference is A/E PCM, manifest1126f9f5…5c9ff3, owner score
bc092a0f…595d4a6 /PCS1b8f86ad9…5c270. Owner-supplied; authorised for Horde use only.
Original source/runtime bytes remain in Git and the initial render evidence.
Candidate JSON/PCS1 pairs remain separate editable audition sources. The selected
canonical derivative supersedes the runtime bank, not its retained reference.

Use retained `music-render/method/render-sections.cjs`: actual Pocket Chordsmith
v68 app voices, scheduler and live FX graph, seeded noise/virtual pruning clock,
fresh48k stereo OfflineAudioContext,40ms excluded leader, exact12s A/E bodies
and separate3s tails. No Core/WAV exporter or copied synthesizer. Renderer
2b87d7b1 is isolated outside Horde; HTMLb266814f…b6e2cf matches the old bank.
Current Chordsmithe803ae08 /HTML41d6d115…e5c3c3 is inspected but **not substituted**.
The wrapper only selects source/output/cues and adds import/parity evidence.

## Finite variants and checks

Render A/E only: one reference rerender to verify the pinned route in the current
browser, then two candidates. No full-bank rerender before owner listening.

- `whistle`: cowboy_whistle lead; soft_pluck answering part; mellow_sax held
  backing; felt_piano chords at0.20 rather than0.28.
- `reed`: same supporting palette with mellow_sax lead instead.
- Both: chorus0 rather than0.09; delay0.04 rather than0.12; FX mix0.22 rather
  than0.26; reverb stays0.46. No western_fiddle, extra reverb, new voices or
  invented per-track gain fields. Original sparse bell-answer pitches/onsets
  become plucks in A/E; bell placement in the other sections remains unchanged.
- Owner subsequently supplied a preferred allocation: felt_piano chords,
  melodies1/2 soft_pluck and melody3 cowboy_whistle. `owner-combo` supplements
  the completed auditions, with original chord0.28 and the same reduced FX.
  Read-only Pocket DAW0.6.49 MCP confirms those names/gains; its unsaved project
  and transport are untouched. DAW and v68 audio parity is not assumed.

Preserve every other source value, including all notes/onsets/holds/slides,
repeated attacks, octaves, panning, tempo80, section bars, sequence and cue mapping.
Validate JSON/PCS1 equivalence, supported instruments, actual import/export and
scheduler calls, float headroom and independent PCM headers/frames/hashes.
Preview loudness matching is **preview-only constant gain**, separately recorded;
do not normalize admitted runtime assets or flatten relative cue dynamics.
Check twenty body+tail loop periods and representative250ms crossfades without
claiming numerical checks establish audible seams or SFX intelligibility.
SFX masking/listening and phone/native acceptance remain owner gates.

## Completed auditions (historical)

Reference/whistle/reed and one owner-combo A/E pair rendered once each. Source,
app import/export and actual musical schedule agree. Sixteen exact-format WAVs
pass float/PCM/header/frame/hash checks; original repeat max1LSB, tails byte-exact.
Twenty-period arithmetic and corrected250ms loop-aware crossfade supplements
remain unclipped. Review finding and original transition receipts are retained;
no rerender. Current closed-roster/pin policy passes, PCM20,160,000 bytes unchanged.
Audition manifest72134d1f…293e3 binds45 archived files/39,632,706 bytes plus four
referenced tools. [Evidence and listening timeline](evidence/2026-10-01-music-instrumentation/README.md).

Two49.5s comparisons: owner-combo and whistle-lead,
each old A/new A/old E/new E,12s bodies plus0.5s gaps. Constant-gain loudness match
within0.02LU; owner gains+4.07/+0.73dB are preview-only. Reed sources/derivatives
and completed checks retained, not discarded or repeatedly auditioned. Offline
default70% combat probe is unclipped but not perceptual-masking acceptance.
Measured seam deltas are retained; no audible/native-clock pass is inferred.

## Selected full bank and next gate

The owner's later explicit “i prefer whistle-lead :)” supersedes the screenshot
allocation. Six looping cues use cowboy_whistle /soft_pluck /mellow_sax, with
felt_piano chords0.20, chorus0/delay0.04/mix0.22/reverb0.46. C retains its three
authored bell attacks; G retains four bell answers. No note, hold, repeated-note
structure, register, pan, tempo, cue mapping or gameplay timing is changed.

Accepted A/E PCM is reused after active-cue source-projection equality; only
B/C/D/F/G/H rendered once through the retained app. Canonical JSONf5d4bbc8…b92f4
and PCS1747e5a9a…64dab agree; manifest/pins nowea7adbc1…8c3e. All16 WAVs remain
48kHz stereo PCM16, exact bodies and144000-frame tails,20,160,000 decoded bytes.
No preview loudness gain is applied to runtime. C's actual FX tail falls below
PCM16 quantisation; the full silent tail is retained, not trimmed or fabricated.
The decoder fixture now explicitly preserves silent PCM tails while bodies
must remain non-silent; format/corruption/hash/budget contracts are unchanged.

Native production Core/Horde-adapter checks cover A/E twenty-loop playback and
crossfades, plus a finite supplement for the four newly rendered looping cues,
C/G one-shots, full/partly-consumed/exhausted natural tails and early crossfades.
MSVC Debug/Release real-WAV and transactional-bank suites each2/2PASS; Android
asset verification/staging SUCCESS. These are offline sample/packaging checks,
not an OS/device-played clock or perceptual acceptance. Failed schema-projection,
silent-tail and compile checks are preserved alongside corrected results.
[Exact bank evidence and limits](evidence/2026-10-01-music-whistle-bank/README.md).

Owner exact-S26 listening on6f1be881 accepts instruments and cue sequencing,
but rejects slow/crackly output. Native1440-frame cap underruns roughly400 times
per576000 consumed frames, taking16s wall for12s content. Keep this negative
result; do not rerender or change tempo. Android sink candidate35ec7e46 removes
post-creation shrink and uses bounded actual platform buffer (5766frames here);
first two periods12.002s/zero underruns, full Java24/24 and build/lint/package
PASS. [Finite phone record](evidence/2026-10-01-music-whistle-phone/README.md)
records owner's **"Music sounds perfect now"** and six underrun-free periods.
Separate controlled long run is interrupted after one period; twenty-period/
lifecycle, full route/SFX balance, audio focus and final acceptance remain open.
Owner confirms closing that check. Announced follow-up reaches the death overlay,
which suspends music after one underrun-free period; this is not an audio failure.
Do not repeat the unsafe idle-opening setup. Use a legitimate safe unpaused live
state for the unfinished long gate; raw incomplete receipts/UI remain retained.
Subsequent unchanged-playback Debug APKe10b0203 uses existing authored dead-lich
finale11 and ordinary Continue, not invulnerability or an idle live enemy. Twenty
consumed cueH periods pass unchanged clock/queue gates at11.996-12.004s with zero
underruns. Menu pause/Home/resume retains epoch and nine more periods. See the
[single S26 live record](evidence/2026-10-01-mobile-lantern-profile/s26-live/README.md);
do not repeat the long gate. Audio focus/full route/SFX balance remain open.
S26/DAW available; no further DAW use needed. Dirty renderer/scratch preserved.
Uncommitted diagnostic primary-hit work and unrelated scratch are preserved.

Audio/haptic manual revalidation required: **YES** for the changed music timbre/
balance once auditioned or installed; no haptic/SFX-event changes are requested.
