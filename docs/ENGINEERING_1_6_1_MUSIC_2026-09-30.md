# What the Dark Keeps: shared resolver checkpoint

Status: Core-backed Windows/Android playback and independent persisted volume
implemented; exact-S26 rate/crackle/timbre listening **accepted**, longer-loop,
lifecycle/focus and full route/SFX balance **not complete**. Historical checkpoints
below retain their original scope.
Source: owner-supplied `What_the_Dark_Keeps_Horde_RT_Music_Pack.zip`, SHA-256
`e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa`.
Owner explicitly confirmed: **Owner-supplied; authorised for Horde use only**.
This is not a general permissive redistribution grant or a change to Chordsmith,
Hotstrike or existing project licence statements. Publication remains unauthorised.

## Instrumentation - October 1; whistle-lead selected

Owner finds the bank too sci-fi. Bounded A/E whistle/reed studies and the owner's
felt_piano /soft_pluck1+2 /cowboy_whistle3 allocation were rendered through the
**retained v68 actual app voices/live FX**. The later explicit owner preference
is whistle-lead, not that screenshot allocation. Accepted A/E PCM is reused;
the other six cues are rendered once. Revised canonical JSON/PCS1, sixteen
runtime derivatives and matching manifest/pins are admitted locally with the
same notes/timings/frame lengths and20,160,000-byte decoded PCM bank. Original
reference remains in Git/evidence. Objective schedule/format/headroom, Core
sample-clock and Android asset-staging checks pass; remaining device gates are
listed below. Read the [finite record](ENGINEERING_1_6_1_MUSIC_INSTRUMENTATION_2026-10-01.md)
and [bank evidence](evidence/2026-10-01-music-whistle-bank/README.md) before resumption.
Owner exact-S26 listening accepts instrumentation/cue sequencing but rejects
6f1be881's slow/crackly playback; native1440-frame cap gives400 underruns/12s
content played in16s wall. Android-only35ec7e46 removes the post-creation cap,
uses bounded actual5766-frame queue, first two periods12.002s/no underruns.
Java24/24, build/lint/package PASS; owner: **"Music sounds perfect now"**. Six
underrun-free periods; controlled run interrupted by owner closure, announced run
by death-overlay suspension after one period. Idle opening is not a safe long-test
state; do not repeat it. Twenty-period/lifecycle/audio-focus gates remain open.
[Single phone record](evidence/2026-10-01-music-whistle-phone/README.md)
owns remaining evidence. Do not rerender or change tempo. S26/DAW available;
no further DAW use needed. Non-equivalent Core/WAV renderer stays prohibited.
Manual music listening:YES; no haptic/SFX-event implementation changes.

## Native playback checkpoint - October 1

Shared single-owner worker session and copied-event inbox bridge gameplay to Core,
without changing the SFX queue or simulation. Native XAudio2 and AudioTrack sinks
submit actual Core PCM with bounded queues and separate generated/accepted/device-
consumed counters; ordinary pause retains queued content, explicit epoch reset
discards stale output. Settings music volume is persisted independently (default70).
Normal benchmarks now include music: obtain new matched measurements, not an
unlabelled comparison to older silent/no-music builds.

Windows fresh Debug/Release native-clock pause/resume/retry/join smoke and ASan
PASS; Release consumed twenty actual12s loops at gainzero, not an audible-seam/mix
pass. Android Debug builds all four ABIs and lint has0errors/38warnings; exact
final APK509b7981…ccd1bae4 package/module admission passes, not installed. The
initial283b receipt remains historical; Android audio-focus acquisition/loss/return
needs a focused follow-up. Score/renders/
Core pin/player/RT remain unchanged. [Windows evidence](evidence/2026-10-01-windows-music/README.md)
and [Android evidence/phone matrix](evidence/2026-10-01-android-music/README.md).
Audio/haptic manual revalidation required:YES for actual music playback/gain.

## Current Core ownership checkpoint - October 1

The owner's architecture clarification is implemented without discarding the
validated cue or sample behaviour. `MusicDirector` is unchanged. Horde's
`MusicPcmStream` now only validates/maps its nine cue slots and C→D/G→H policy;
`MusicPcmWave` delegates decoding. All reusable PCM looping/tails/crossfades and
strict RIFF parsing are in canonical Pocket Audio Core's native C++20 target at
`534a6e6811ce653efd5422138c5772b967263ed0`. Horde consumes a byte-exact native-only
subset with configure-time size/hash/inventory admission on Windows and Android.
No build-time network fetch, editor/synth app copy, cue/score change or package
relicensing. The canonical PCS JSON/PCS1 and sixteen rendered WAVs are unchanged.

Fresh canonical Core CI passes GCC/Clang/MSVC Debug+Release (six jobs); local
Debug/Release and MSVC ASan each pass1/1. Horde integration passes8/8 focused
MSVC Debug and Release (resolver, stream, decoder, pin, Core, simulation gameplay/
timing and existing spatial feedback); real Windows RT executables link in both.
Android Debug builds all four ABIs. These do not establish playback/device/
listening acceptance. [Exact integration evidence](evidence/2026-10-01-pocket-audio-core/README.md)
records actual source/build/package scope and retained failed checks.

Source5dbb7df fresh push36784556913 and PR36784563898 both pass GCC/Clang/MSVC
50/50 common CPU-host CTests and13/13 Vulkan CPU-host fixtures; actual current-head
logs inspected. New decoder/Core/pin contracts pass on all three compilers.
This does not establish physical RT or platform playback acceptance.

The owner-observed MSVC modal was a standalone Core test fixture supplying a
12-float output span over an8-float test array, not a game-loading diagnosis.
Its storage/span construction is fixed and fresh sanitizer/contracts pass.
The generic natural-tail port also retains the body-complete precondition;
early interruptions remain bounded fades, not prematurely replayed tails.

## Runtime asset admission - October 1

The canonical owner ZIP is rehashed; its editable schema16 JSON and equivalent
PCS1 entries are imported verbatim under `assets/audio/music/what-the-dark-keeps/source/`.
Both source hashes and all16 runtime derivatives agree with the original/archive
receipts. No generation, synthesis/editor vendoring, score/effect/gain/sample
changes or new licence inference. Historical `-loop` becomes `-body` because
C/G are one-shots. The reviewed manifest hash is
`1126f9f537efb607b11bd492e1c79d6e8b94814567ce06b654e03b0d915c9ff3`.

Windows/foundation packaging and Android Gradle stage only that manifest plus
the16 WAVs; closed-roster/hash checks reject source or unrecognised content,
missing/corrupted files and altered manifest/cue metadata. The shared compiled
`MusicPcmAssets.h` is the assets-relative A–H path/frame/loop contract used by the
Core adapter and actual-WAV test. Immutable PCM is20,160,000 bytes, WAV payload
20,160,704 bytes plus manifest. This is admitted pre-rendered music, not a new
runtime synthesizer. Prototype evidence remains unchanged and non-runtime.

Native platform playback, consumed-sample clock, independent persisted volume,
native seam/transition and owner warning/SFX listening still remain open.
No phone install, listening or performance acceptance follows from package tests.
Audio/haptic manual revalidation:NO for unwired assets; YES when audible music
or mixing is integrated.

## Immutable native bank - October 1

`MusicPcmAssetBank` now owns the admitted20,160,000-byte decoded bank. Sixteen
bounded file/asset reads occur before audio starts; all decoding delegates to
Pocket Audio Core. A late read/size/format/allocation failure releases every
partial clip and exposes no playback spans. Ready storage cannot reload, move
or copy; the playback owner must stop/join before its destruction. No platform
reader, PCM output callback, game event drain or shared-source change is wired yet.

Fresh MSVC Debug/Release each6/6 focused tests pass, including the real asset
bank, thirteen-successful-clip failure/retry cases and actual Core output. The
focused bank test also passes MSVC AddressSanitizer; initial incremental-link
warning and corrected warning-free run are retained separately. Real Windows RT
executables link both configurations. Android Debug builds four ABIs; actual
APK450d2cf6…dfb3f2d has exact17 music entries, current attribution and52 unchanged
render/SFX assets. Four actual ARM64 Diagnostic/Mobile modules freshly val/dis
PASS, identities unchanged. Not installed, and no native playback/listening or
device claim. [Bank evidence](evidence/2026-10-01-music-bank/README.md).

Asset checkpoint199697b fresh push36789021436/PR36789031208 each passed51/51
GCC/Clang/MSVC plus13/13 Vulkan CPU-host fixtures. Bank adds the52nd common test;
its current-source CI must be obtained separately after push. Next remains
platform output/consumed-sample clock, persisted independent Music Volume and
native seam/transition/lifecycle/SFX listening. Audio/haptic manual revalidation:
NO while unwired; YES when playback is audible.

## Implemented contract

`src/audio/MusicDirector.*` owns no playback resources or allocation. It reads
const simulation snapshots and ordered events without draining the SFX queue.
Callers supply a monotonic audio clock, an event-queue lifetime token and optional
external suspension. Output is cue, loop/one-shot, position, suspension and
revision/discontinuity; no render-clock timing or file decoding enters fixed ticks.

| Cue | State and musical boundary |
| --- | --- |
| A | Living, disengaged exploration before actual torch failure; 12s loop |
| B | Actually engaged incomplete skeleton encounter; persists after first death |
| C | Fresh torch event held pending until actual extinction/release; once per session, 3s |
| D | Actual torch failure, absent higher-priority combat/reward; 12s loop |
| E | Active lich, including charge/recovery; 12s continuous loop |
| F | Defeated lich and reward wait/raise/reveal; 12s loop until roof opening |
| G | Actual SkylightOpening; immediate 6s one-shot, Dawn offset4.50s |
| H | G completes or late completed-finale attachment; 12s loop, no stale-state override |

Actual source reconciliation: `GameSimulation` emits its existing
`TorchExtinguished` event when Guttering **starts**, while the flame is still lit.
Physical extinction/release begins0.70s later. The resolver preserves that event
for C at actual failure; Guttering alone does not prematurely select D. Existing
SFX emission/timing, simulation and gameplay animation are unchanged.

Route/retry resets clear cue/engagement/one-shot latches but retain event dedup
while the queue lifetime is unchanged: queue `Clear()` does not restart sequence
numbers. A new queue token resets the high-water mark. Out-of-band checkpoint
imports call Reset. Late torch attachment reconstructs D without inventing C.
Pause/background freeze the musical clock; G resumes from durable finale timing.
G-to-H completion is session-latched, preventing repeated G on a lagging Dawn
snapshot and preserving H phase into Complete. Death silences/cancels the sting.

## Fresh validation and boundaries

MSVC Debug and Release each pass focused2/2 (music resolver + gameplay simulation).
Lead additionally verifies a real240-fixed-tick torch sequence with unchanged
SFX queue, plus the existing fixture matrix: priority, two-skeleton completion,
monotonic retry/queue identity,60s reward wait,4.50/6s G boundaries, late attachment,
suspension, bad clocks, irregular updates and20 exact12s logical loop periods.
Logical periods are **not** rendered seam/drift/listening acceptance.

Android ARM64 Debug compiles/links the new shared original source and packages.
Exact intermediate APK SHA-256
`dc9215a23b9ae94c5419f32e310309d38b352564fbc51cc6155661db1ea08dbc`,
code9 /1.6.1-debug, `.debug`; not installed. Conventional output APK was stale,
so its hash is not reused. Build-only, not playback or device acceptance.
Earlier argument-splitting and queue-reset failures are retained alongside green
reruns under `C:/Dev/tmp/horde-music-director-20260930`; evidence index below binds
the reviewed source and selected logs. [Retained evidence](evidence/2026-09-30-music-resolver/README.md).
New-head CI is required after push.

## Remaining integration

Historical original helper checkpoint at4870532 (now superseded by the Core
ownership seam above): `MusicPcmStream.*` added an audio-thread-owned PCM
cursor/mixer, not a synthesizer or platform playback backend. Caller-owned
immutable stereo PCM16 clips at48kHz have exact576000-frame looping bodies
(C144000, G288000) and144000-frame tails. Loops add the previous complete tail
over the first3s of the next12s body without moving the loop boundary. One-shots
append their tail; natural C→D/G→H handoffs preserve only the remaining tail
after the body actually ends. Early interruptions use a bounded250ms crossfade.
At most current+outgoing streams exist. Repeated selection updates retain the
cursor; revisions/discontinuity edges seek, suspension/bad clocks silence/freeze,
and reset cancels stale streams while retaining the separately configured gain.
No allocation, locking or I/O occurs in Render; all methods require one owner
thread and spans must outlive it. This is not concurrent platform command routing.

Lead-reviewed synthetic tests cover20 actual576000-frame sample periods with
irregular chunks, full/partial/exhausted one-shot tails, early/natural handoffs,
discontinuity edges, rapid replacement, invalid inputs and volume persistence.
MSVC Debug/Release each4/4PASS including unchanged resolver and simulation tests.
These are sample-cursor contracts, not audible20-loop/native-clock acceptance.
[Exact helper evidence](evidence/2026-10-01-music-pcm/README.md) keeps build/source
identity separate from native playback. Android compilation is recorded there.

An offline sum of the actual rendered bodies/tails preserves20 exact12s starts
with no new overrange samples or overall-peak increase; largest same-sample
increase0.002747FS. Start boundary deltas decrease to0.000031–0.000092FS, but
15s tail-off remains measured up to0.001495FS. That is not listening approval
or proof of musical phase continuity. Large transient float WAVs were verified
then removed; original PCM prototypes remain unchanged. No score/FX-rate change,
normalisation, paid generation or source-tool code copy is admitted.

Current local Chordsmith inspected at2b87d7b / v68 (schema17 accepts16).
App HTML SHA-256 `b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302602457fa9b6e2cf`
differs from the supplied preview's historical tested app. The UI WAV exporter
tries Core0.2.0 first; its documented PCM renderer lacks app sound/FX parity.
The fallback offline exporter also has a distinct compressor graph and event-plus-
tail duration, not certified musical loop windows. Do not accept either as the
supplied score's faithful game audio without actual rendering/trace/listening proof.
At that historical rendering checkpoint, no Chordsmith/Core code was copied or
shared source modified. The newer native Core utility seam above is deliberate;
the supplied music is not regenerated. Core remains private/UNLICENSED and the
owner's score grant does not relicense software.

[A-H rendering prototypes](evidence/2026-10-01-music-render/README.md) now retain
actual app-voice/live-FX outputs, exact musical windows (40ms leader excluded),
separate3s tails and pre/post source hashes. Source starts/holds/repeated pitches
and actual scheduler traces agree; G's F-sharp4 is4.50s. Independent16-WAV PCM audit
passes exact headers/frames/hashes and peak/RMS/seam quantisation checks. Original
bad61-hex source-hash receipt is rejected and preserved externally; the rerender
is a new run. At most1LSB repeat differences are documented, not bit-exactness.
These are non-runtime prototypes, not admitted music assets or listening proof.
Nonzero FX tails and0.421Hz chorus (5.052cycles/12s) mean exact-duration crops alone
cannot certify seamless looping. This is not proof of an audible problem and does
not approve a new synthesis framework, altered score/effect rate or licensing.

The bounded local-tool rendering route retains actual app synthesis/FX,
deterministic noise, exact musical windows and separately accounted tails; preserve
the source JSON/hold/repeated-pitch masks and relative cue dynamics. Next admit
hash/provenance/loop metadata, wire bounded playback/crossfades and pause/focus/
retry/late-ending reconstruction on both platforms, add independent persisted
Music Volume0–100%, validate actual20-loop seams/drift and final owner listening.
Generic asset CLI was unavailable; no package admission or paid generation occurred.

Audio/haptic manual revalidation required: **NO** for this unwired shared-logic
slice (feedback semantics/playback unchanged). **YES** when music playback/mix is
wired, to protect warning/SFX clarity; no unrelated haptic retuning requested.
