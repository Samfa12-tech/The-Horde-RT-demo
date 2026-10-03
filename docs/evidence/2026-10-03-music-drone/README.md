# A/D Melody3 held-drone removal

Owner-authorised October3 asset-only edit. No new instrumentation, FX, synth,
PCM playback, music director, gameplay, renderer, SFX or haptic change.

## Exact source and delta

Canonical editable JSON/PCS1 under `assets/audio/music/what-the-dark-keeps/source/`.
Reference JSONf5d4bbc8 /PCS1747e5a9a /manifest ea7adbc1 are retained in Git and this
record. Melody3 is zero-based track2, `mellow_sax`: A has MIDI69/A4 and D MIDI62/D4
at step15, followed by48 held continuation cells16..63. Actual v68 schedule starts
at2.8125 musical seconds and holds8.4525s. Remove those two phrases and their
continuation metadata only. Preserve both short step0 MIDI69 notes, every other
note/hold/slide/tuplet, repeated attacks, instruments, FX, pan, tempo80, section
bars, songSequence, adaptive cue decisions and gameplay timing.

Only four readable JSON rows change; PCS1 is its equivalent regenerated encoding.
New JSON3ab832e0 /PCS1fc7dab9a /manifest672f2321. Exact differential assertion
compares the complete score with the reference; four negative fixtures reject
tempo, short-note, other-part and stray-hold changes. Actual app JSON/PCS import
and scheduler traces agree. All remaining lead/chord/bass/drum events are exact.

## Rendering and scoped validation

Retained Pocket Chordsmith v68 actual app voices/scheduler/live FX, isolated source
2b87d7b, HTMLb266814f…b6e2cf; same retained adapter, seeded noise and virtual pruning
clock. No DAW update/use, Core/WAV synthesis, gain normalisation or shared preset
edit. Render A/D once; only their bodies/tails change. Twelve other WAVs remain
byte-exact. Bodies576000/tails144000 frames,48kHz stereo PCM16; bank decoded PCM
20,160,000 bytes unchanged. A peak0.396661/D peak0.155831FS, zero nonfinite/clipped
float samples. `SHA256SUMS.json` binds sources, derivatives and retained evidence;
the exact render producer is recovered and checked against its original hash.

- Native Release production Core/Horde adapter: twenty periods each A/D, pause/
  resume/default70% gain,15 affected early/near-boundary crossfades and3 natural
  C→D tail handoffs PASS. Maximum sample error3.57628e−8, unchanged tolerance
 4×float epsilon. Includes H→A wrap and A↔D song edges.
- Initial supplemental check wrongly asserted an already-completed C one-shot
  would fade D in from silence. Core correctly uses natural tail handoff. Original
  failure retained; corrected fixture tests the early fade and full/partly-used/
  exhausted natural tail, without changing samples, tolerance or Core.
- Existing current-source Debug real-WAV/transactional bank CTests2/2 PASS against
  the new assets. Asset admission/staging and closed17-entry ZIP/APK positive/
  corruption/duplicate/source-leak negative fixtures PASS.
- Android Debug four-ABI assemble SUCCESS47s. Exact APK315b1c9f4ba99f397f1aa638fd0b19f251a7dd1877c3d9fff6680587feda3f04
  passes runtime music entry hashes/roster; installed/pulled byte-identically on
  SM-S948B. Package `com.samfa12.hordelanternrt.debug`,1.6.1-debug/code9, development
  signature only. Immutable local artifact is under C:/Dev/tmp/horde-music-drone-20261003/.

The first launch accidentally opened the separate unchanged1.6.0 production
package; owner noticed and it was closed. It is **not** this candidate's evidence.
Correct package is subsequently verified by APK metadata/pullback and1.6.1 UI.
Existing Debug checkpoint setup freezes simulation briefly; it is not live-motion
or audible playback evidence until setup completes and ordinary gameplay resumes.

## Packaged game check / owner acceptance

On the exact S26 package: quiet `worst-bend` live state selects A via the unchanged
gameplay director. Native RT UI/vitality3 and five consumed12s periods are observed,
zero underruns, consecutive intervals11.997–12.002s. This is actual device PCM
clock/loop evidence, not proof of audible seams. Opening idle setup was discarded
after death; no invulnerability or gameplay changes were added to prolong it.

Existing `lantern-drop` setup completes and returns to live gameplay. Eight further
consumed periods are observed at11.997–12.003s, zero underruns, RT-active/vitality3
UI. Its normal torch-failure state selects C then D via the existing director;
the clock log does not expose the individual cue enum or prove audible transition
quality. Retained UI/screenshot is not a new player/renderer visual-acceptance gate.
Owner hears the correct package in post-torch-failure D and reports **"It sounds
great"**: D tone/removed drone/repeated loop accepted. A is returned to the same
safe quiet state for a short listening-only check; no automated rerender/long
clock/performance repetition. Owner then confirms **"i listened to both, theyre
both perfect"**: changed A/D packaged listening accepted. This does not claim
independently observed audible acceptance of every gameplay transition.
The game's soundtrack is adaptive:
it does not play the canonical16-entry songSequence linearly. Source songSequence
and its H→A boundary are preserved/checked; do not claim a full linear-song OS
playback from an adaptive cue-period log. A/D loops are accepted on the phone;
affected transition and song-wrap correctness have the scoped native sample
oracle evidence above, not a separately observed full linear-song phone run.
Do not repeat completed renders/loops or request full unchanged-bank/SFX/haptic
approval. No shader/performance matrix is rerun for this musical delta.

Audio/haptic manual revalidation required: **YES**, changed A/D audio only;
owner listening now **PASS** for both sections. Haptics/SFX semantics remain unchanged.
No main merge, signing recovery, release or publication.
