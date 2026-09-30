# What the Dark Keeps: shared resolver checkpoint

Status: original shared logic implemented; playback/music integration **not complete**.
Source: owner-supplied `What_the_Dark_Keeps_Horde_RT_Music_Pack.zip`, SHA-256
`e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa`.
Owner explicitly confirmed: **Owner-supplied; authorised for Horde use only**.
This is not a general permissive redistribution grant or a change to Chordsmith,
Hotstrike or existing project licence statements. Publication remains unauthorised.

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

Current local Chordsmith inspected at2b87d7b / v68 (schema17 accepts16).
App HTML SHA-256 `b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302602457fa9b6e2cf`
differs from the supplied preview's historical tested app. The UI WAV exporter
tries Core0.2.0 first; its documented PCM renderer lacks app sound/FX parity.
The fallback offline exporter also has a distinct compressor graph and event-plus-
tail duration, not certified musical loop windows. Do not accept either as the
supplied score's faithful game audio without actual rendering/trace/listening proof.
No Chordsmith/Core code copied, shared source modified or supplied music regenerated.
Their source is UNLICENSED; the owner's score grant does not relicense that code.

Next use a bounded local-tool rendering route retaining actual app synthesis/FX,
deterministic noise, exact musical windows and separately accounted tails; preserve
the source JSON/hold/repeated-pitch masks and relative cue dynamics. Then admit
hash/provenance/loop metadata, wire bounded playback/crossfades and pause/focus/
retry/late-ending reconstruction on both platforms, add independent persisted
Music Volume0–100%, validate actual20-loop seams/drift and final owner listening.
Generic asset CLI was unavailable; no package admission or paid generation occurred.

Audio/haptic manual revalidation required: **NO** for this unwired shared-logic
slice (feedback semantics/playback unchanged). **YES** when music playback/mix is
wired, to protect warning/SFX clarity; no unrelated haptic retuning requested.
