# Original PCM helper — not native playback acceptance

Owner supplied the score and explicitly authorised Horde use only. No score/FX
change, synthesis framework, source-tool code copy, paid generation, new general
redistribution licence or publication. Existing prototypes remain development-only.

`MusicPcmStream` is an original bounded PCM16→float stereo cursor/mixer at48kHz,
with caller-owned immutable spans and one audio-thread owner. It adds unchanged
3s tails at exact12s loop boundaries, appends complete one-shot tails and keeps
at most one outgoing stream. Ordinary transitions crossfade250ms; C→D/G→H retain
remaining tails only after the old body actually finishes. Suspension/bad clocks
silence/freeze; same-revision refreshes do not seek, discontinuity edges do;
reset cancels stale streams without resetting music gain. Render has no I/O,
allocation or locks. No platform thread-safe command delivery is claimed.

MSVC19.44 Debug/Release each4/4PASS: new PCM tests, existing MusicDirector and
simulation gameplay/timing. Lead final reruns also4/4each. Tests cover20 actual
576000-frame intervals under irregular chunks, stereo tail sums, G's exact4.50s
marker (synthetic fixture, not pitch analysis), natural/early/partial/exhausted
one-shot tails, repeat/discontinuity-edge semantics, rapid switches, volume and
invalid clip/selection/output/clock handling. These are CPU sample contracts,
not musical listening, native sample-clock drift or actual20-loop playback.

Canonical shared source list includes the unit once. Windows simulation library
owns it and probe-core excludes duplication; Android consumes the shared list.
ARM64 Debug build SUCCESS35tasks, compiled107000-byte MusicPcmStream.cpp.o linked
into the native target. Resolver-selected intermediate APK16154b5d…57000 is
1.6.1-debug, not the possibly stale conventional output. Stripped/native APK
entry hash06b35169…4cbb0 agrees. Lead actual module scan val/dis PASS4/4 with
exact Diagnostic/Mobile pipeline/compute keys. All53package assets match the
previous accepted normalDebugC11 exactly. No install, phone playback or visual
claim. Initial worker containment invocation omitted external val/dis; its
subsequent wrapper falsely failed on stale LASTEXITCODE. That raw result is
retained separately from the lead's explicit-SDK successful module validation.

Offline loop-tail composition of the actual source PCM prototypes places20
body windows without resampling/gain/fades/FX changes. Period drift is zero by
construction. Start deltas0.000031–0.000092FS versus naive0.000580–0.001404;
tail-off at15s remains measured up to0.001495FS. No overrange/global-peak increase;
maximum positive same-sample rise0.002747FS. Transient float WAV hashes/metrics
are retained; those large outputs were verified and removed, not shipped assets.
This is not proof of a perceptually seamless loop or owner listening acceptance.

[Receipt](source-and-build-receipt.json) binds source checkout bytes, exact APK,
module identities and selected logs. [Manifest](manifest.json) binds retained
evidence. Original source PCM and score remain in the existing rendering evidence;
no duplicate large audio, native object/library/APK/build cache is committed here.

Remaining: admit runtime audio/metadata, wire bounded playback/authority/lifecycle
on both platforms, separate persisted Music Volume0–100%, actual loop seams/drift
and final listening with warnings/SFX. **Audio/haptic manual revalidation required:
NO** for this unwired slice; **YES** once playback/mix changes. Haptics are untouched.
