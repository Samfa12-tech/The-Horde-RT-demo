# The Horde — adaptive score review suite

## What this is

15 editable soundtrack families: the preserved **What the Dark Keeps** source theme and 14 newly authored companion banks. There are 120 section arrangements, plus 2 optional event banks containing 16 short signals/variants. Every score is a valid schema 17 Pocket Chordsmith JSON project with an equivalent `PCS1:` share code. This is original note-level composition derived from the supplied theme, rendered with the actual Pocket Audio Core. No paid AI music service, purchased sample, generic MIDI synth or replacement oscillator implementation was used.

**Status: composition review, not production-mastered or integrated game audio.** The current native eight-cue bank and its accepted original recordings are unchanged. New bank section letters are local to each project; do not substitute this manifest for the game's existing asset manifest.

## Start here

- Play `previews/The-Horde-First-Listening-Tour-Core-Reference.mp3` for a 2:07 tour.
- See `previews/Listening-Tour-Guide.txt` for timestamps.
- The full listening assemblies contain a possible musical journey through each bank. They are not fixed gameplay timelines. Players can linger in loop states, and boss changes follow real encounter events.
- Canonical JSON and `PCS1:` files are valid schema 17 and preserve the performed score in Pocket Audio Core. Read `evidence/editor-compatibility.json` BEFORE editing in the current app: its compact grid clamps lower melody octaves and cannot yet preserve every bank’s intended register.
- `event-scores/` contains optional short discovery, item, light-solved, access, seal, lantern, failure, retry and boss/tool-variant cues.
- `manifests/cue-manifest.json` describes every section, proposed state sequence, story guardrail and future-engine requirement.

## Bass feedback and alternate preview

The owner liked the first listening tour’s musical direction and reported that the bass rendered poorly. Source and signal checks confirm correctly pitched fundamentals, but the Core classic bass omits the app’s 420 Hz main and 220 Hz sub low-pass filters and uses a different envelope. Core also ignores the intended bass beat-volume adjustment. No filtered-app audio parity is claimed.

`previews/The-Horde-Listening-Tour-Soft-Upright-Alternative.mp3` is the same musical tour with an existing softer engine bass voice; `previews/bass-voice-alternative/Bass-Voice-Comparison-Classic-vs-Soft-Upright.mp3` gives a short A/B. Only cloned bass-event voice IDs change, with original per-bank gain retained. The Standard normalizer currently discards a raw soft_upright bassTone setting, so this is explicitly a render-only voice alternative, not an implemented score/runtime fix. Notes, timing and canonical scores are unchanged by this alternative.

## Musical through-line

The source signature is **D–E♭–A–E–D**, with its original A-section rhythm intact in the preserved theme. Chromatic pitches have not been quantized into D minor. The original G/H F♯ release is intentional and preserved.

The new banks use omissions, delayed answers, different registers and rhythmic compression rather than 15 transpositions of the same loop. The forest introduces a COMPANION answer, the village gives it a modest communal form, and the return gives the signature a settled answer. These names are thematic descriptions, not extra Chordsmith section letters. Minor/modal tension stays in the tomb and encounters; G-major forest/village and D-major homecoming create contrast. The mint's displaced plucked pulse is distinct from the Abbey's spaced bell phrases and the Court's high-register imitation.

Every new melody track leaves room to breathe. Supporting third parts are short responses, not sustained full-section drones. The previously accepted A/D drone removal is retained in the source theme, including its separate short opening A4 notes. The new tomb bank is a companion proposal; it does not overwrite the accepted tomb section.

## What the sound does and does not represent

These files use the current app's shared Pocket Audio Core reference renderer at commit `0c6975cadcd4aba0047ab94170a2adabc722517f`. The current app's preferred WAV-worker path uses this same engine. This is genuine Chordsmith output.

However, the Core reference engine omits several filters, voice envelopes and master FX present in the historical accepted app/live render. In particular, its fiddle approximation can be buzzier because the intended band-pass voice filtering is absent. Instrument names such as harp, whistle and fiddle describe synthesized approximations, not recorded medieval instruments. Do not treat a capability report or a clean waveform as proof of timbral parity. Perceptual/owner listening acceptance and phone/headphone/device testing remain outstanding.

No external EQ, limiter, added reverb or replacement synthesis is concealed in the audio. One documented scalar gain per bank makes previews listenable while retaining relative state levels. It is not a cross-bank loudness master. The short medley alone has editorial 10 ms edge fades and 400 ms gaps between unrelated excerpts; those edits are not in the canonical individual clips.

## Audio formats and loops

Individual sections are stereo 44.1 kHz 16-bit PCM, delivered as lossless FLAC. Decoding each FLAC has been checked for byte-identical PCM to its WAV. The archive stores lossless FLAC section masters; WAVs can be recovered by lossless decoding and were retained separately during rendering. Full-review WAVs and raw diagnostic WAVs are not included to avoid redundant audio storage. MP3 full tracks and the medley are audition conveniences, not runtime loop masters.

Loop/bridge clips have exact sample-clock musical duration, with genuine overhanging event tails folded into the start for periodic playback. One-shot clips retain their terminal event and at least 0.6 s tail allocation. There is no silence padding inside loops and no hidden loop crossfade. Rounded fractional sample durations are recorded. A periodic loop's opening may include preceding-cycle tail energy; a cold start should use the authored entrance rather than assume they are identical.

Waveform metrics, loop boundary checks, nonfinite/clipping checks, mono reduction and spectral review are evidence, not subjective listening approval. The game still needs adaptive join tests, attack/dialogue intelligibility, rapid state changes, pause/resume and save/reload testing.

## Authoring, export and migration

The canonical sources are schema 17 with A–H blocks, 1–4 bars per block, three melody parts and bounded sequences. Larger forms are intentional sequences of these blocks. Compact notes/holds remain authoritative in Core. IMPORTANT: the current app grid allows melody octave −1..1, while Core allows −2..2. Low melody/counterlines in flagged banks therefore need an editor range upgrade; importing/re-exporting them through today’s grid would silently transpose those tracks. The canonical files preserve the intended lower register rather than conceal this limitation with an octave lift. The original main theme and the forest, town, Glass Court and treasury families are within the current grid’s register range. The exact pinned app importProject/sync/exportProject code has also run in Node VM with DOM-only mocks: notes, holds, instruments, harmony and arrangement survive; only the documented register clamp occurs. Browser/audio parity was not validated because the cloud Chromium/browser routes were blocked.

For immediate grid work, `editor-compatible/` contains 11 optional octave-projected editions with a per-note shift manifest. All 11 pass the same actual-app-code import/export check without further musical changes. These are deliberately different register arrangements, not silent replacements for the canonical scores. The remaining 6 canonical banks fit the current editor register range.

One initial tomb-C sketch extended below even Core’s compact range. Its canonical answer is explicitly F4–E4–D4–B♭3–A3, the register the first Core render already played. The earlier lower sketch is retained only as noncanonical future intent. All canonical pitches, voice IDs and held durations are now individually checked against actual Core timeline events; the corrected tomb audio is byte-identical to the earlier render.

A real format issue was detected during validation: the generic16→17 migration creates sparse rich-note placeholders with duration 1 and velocity 100. Without marking them as compact mirrors, Core would incorrectly ignore the authored hold/mix data. These scores explicitly mark migrated tracks as compact mirrors and preserve the original normalized sound profile. Validation asserts exact performed event equality before and after migration, in addition to schema validation and exact `PCS1:` object round trips. No engine source was patched.

Bellwether and Homecoming presently use a supported 4/4 lilt. Genuine compound 6/8 remains an explicit artistic alternative for an evolved engine/editor. Six counted quarter-note beats would not by itself establish 6/8 phrasing.

## Verification summary

- 17 valid canonical banks: 15 families plus 2 event banks; 17 exact PCS1 round trips
- 120 family sections plus 16 event sections; every performed melody pitch, voice and hold duration checked against authored data
- 136 lossless FLAC/PCM matches; 90 exact-length loop/bridge clips and 46 tailed one-shots
- No clipped or nonfinite samples; loop seam diagnostics pass; mono reduction passes
- Four-times oversampled section peak remains below 0 dBFS; all 17 full MP3 assemblies decode and remain below 0 dBFS true peak
- 28 actual-app-code import/export cases inspected: 17 canonical plus 11 explicit editor editions; no browser/subjective-listening claim

## Future integration boundary

The manifest proposes state names and bounded section sequences only. It does not claim the following are implemented:

- True section-end transitions: current Core's `section` quantization actually behaves like bar quantization
- Playing a sound when a stinger event fires, then executing `thenReturnTo`
- Independent runtime stem/layer mixing and dialogue ducking
- Persistent state, one-time reveal flags, pause/resume and cancellation of stale rewards after death
- Native streaming/registration, production voice limits, memory budgets or platform loudness targets

The music does not assume Abbey precedes Foundry, a daytime ending, a specific entity form, a dead Lucid Keeper, a normal rope loading screen, or a mandatory rhythm puzzle. Audio warnings and actual light/attack tells must remain independent of music.

## Provenance and permission

`source-reference/` preserves the exact supplied original score and its admission notice: owner-supplied, authorized for Horde use only, with no inferred general redistribution or publication permission. Existing original authorship is not reassigned. New companion arrangements were authored by the OpenAI assistant for this user. No claim of exclusive legal copyright is made.

Pocket Chordsmith/Core/PCS Format code is separately licensed private/UNLICENSED; this score permission does not relicense those programs. Their synthesis implementation is not redistributed in the score package. The included wrappers import the pinned repository modules and contain no copied sound recipes. Repository commits, runtime replacement, merging and public release were not performed by this composition task.

## Repository archive

See [ARCHIVE.md](ARCHIVE.md) for the archive boundary, original credit, external renderer dependency and verification instructions. This source-review directory is excluded from the Android and Windows runtime asset allowlists.
