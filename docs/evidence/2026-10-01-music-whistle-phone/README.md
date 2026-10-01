# Whistle-bank S26 listening and Android sink correction

Exact device: **SM-S948B**, ADB serial R5GL219SZGK. No S24/S25 claim.
Source base97f07e2; no dirty primary-hit renderer work included. Normal
Diagnostic/Mobile gameplay; initially saved render scale75/default Music Volume70.
No frozen checkpoints/benchmarks, app-data clearing, system-volume change,
score rerender, Core update, player/glass/shader change or publication.

## Finite checks and original failure

Original APK6f1be881156d76a9806b161ebbbb6d478d97d451473f9d3d8c79a5f6c92dc595,
110,078,719 bytes, installed/pulled back byte-exactly. Owner accepts the
instrument colours and reports correct cue sequencing, but slow, glitchy,
crackly playback. Native device-head increments576000 frames/12s content take
about16s wall; effective queue1440frames, underruns400/800/1200/1600/2000.
Original log and installed APK are retained, not overwritten. Ordinary game
performance is owner-observed acceptable until the lantern, which chugs;
this is subjective workload evidence, not new matched Shipping measurements.

Two fresh-checkout metadata mismatches initially fail strict52-asset parity.
The accepted clip manifest has mixed CRLF/LF; the viewmodel manifest has LF.
Normalizing **both** old/new text gives exact current Git-blob equality, not
just parsed-JSON agreement. Restore only accepted checkout bytes from C11,
retain canonical source, and repackage. Git status still reports checkout
line-ending changes; the receipt labels them, not a pristine checkout.
No semantic asset replacement or gate relaxation. Corrected package passes
all52 pre-existing asset hashes,17 admitted music entries, sources excluded,
four native ABI libraries/JNI exports and actual four ARM64 RT modules.

## Bounded candidate

APK35ec7e46b12a55474ad66f11509a1aa4566e2f25d86d6c34992a4a5ee18058d6,
110,139,455 bytes. Same source base plus Android music Java change only;
all4 native libraries are byte-identical to6f1be881. Installed and pulled
back byte-exactly, prior debug APK581ea466 retained for recovery. Stable and
benchmark packages/data are untouched.

Remove the post-construction `setBufferSizeInFrames(1440)` shrink. Use actual
`getBufferSizeInFrames()` for existing bounded backpressure/partial writes;
honour the creation minimum, with explicit250ms music-only maximum. Invalid
minimum/effective capacity fails music closed; existing worker cleanup releases
the sink/session. No larger bank or changed sample format/tempo/cue authority.
Initial S26 report: minimum46128bytes, allocation/effective5766frames (120.125ms),
48kHz. This is not a low-latency sink claim; queued cue responsiveness increases
relative to the defective30ms effective cap, while SFX remains independent.

The sizing regression initially encounters Robolectric's unimplemented native
queries (zero); retain that failure. A narrow documented shadow models only
capacity versus effective size, not hardware playback. Old shrink then fails
only the larger-minimum case. Corrected full Android Java suite24/24PASS,
including5 sizing/fail-closed cases. Candidate assembleDebug/lintDebug succeeds
in17s; lint0errors/33warnings. Native/asset/shader package gates remain strict.
Independent read-only review finds no blocking ownership/lifecycle defect.

## Accepted listening, interrupted long gate and next unfinished step

Owner now reports **"Music sounds perfect now"** on35ec7e46, accepting perceived
speed/crackles and selected timbre. Six consecutive device-consumed12s periods
have zero underruns, wall intervals11.999–12.002s (median12.000). These span owner
gameplay, not six isolated exploration loops or a twenty-period pass.

A subsequent controlled quiet-opening check ends after one period/Activity
teardown. The owner confirms closing the app because its test purpose was unclear;
retain the incomplete receipt, not an inferred audio failure. Starting that check
required a test-package-only relaunch after retained finale UI; settings/data and
other packages stayed intact. No gameplay reset/UI fix is claimed.

The explicitly announced follow-up (process21893) consumes one period without
underruns, then stops progressing while its Activity remains present. Fresh
app-scoped UI shows **YOU FELL**: an idle player is killed in the ordinary opening,
and the death overlay intentionally suspends music. Retain the scoped log, UI and
**INCOMPLETE-Death-overlay** receipt. This quiet idle-opening setup is unsuitable
for the long gate; do not repeat it or disable enemies/simulation to manufacture a
pass. A first startup UI dump had no root; no capture gate was relaxed.

Subsequent [S26 live record](../2026-10-01-mobile-lantern-profile/s26-live/README.md)
uses unchanged-playback APKe10b0203 on authored dead-lich finale11 after normal
Continue. Twenty consecutive consumed cueH periods pass the existing50ms clock/
queue bounds,11.996-12.004s and zero underruns; menu pause/Home/resume retains
epoch and nine more periods. This closes the safe long-clock/basic lifecycle
step, not every cue/transition or owner combat masking. Original interrupted
receipts and this original35ec7e46 identity remain unchanged.

**Next unfinished step:** affected audio-focus interruption and full cue-route/
SFX balance. Other devices, matched Shipping music-on cost
and final candidate gates remain separate/open. Do not repeat completed renders,
builds, host checks or accepted rate/timbre listening. Negative results stay retained.

Raw receipts/methods and five passing Java XML suites are bound by `SHA256SUMS`.
APKs stay in the immutable local evidence directory; this source archive contains
no APKs, score/audio duplicates or other apps' private data. The narrow test shadow
models capacity only, not timing; actual phone clocks remain decisive. See the
[AudioTrack buffer API](https://developer.android.com/reference/android/media/AudioTrack)
and [retained Robolectric4.14.1 shadow](https://raw.githubusercontent.com/robolectric/robolectric/robolectric-4.14.1/shadows/framework/src/main/java/org/robolectric/shadows/ShadowAudioTrack.java).

Audio/haptic manual revalidation required: **YES**, changed playback buffering;
the exact-candidate rate/crackle/timbre check **passed**. Remaining gates above are
not certified by that response. No SFX/haptic routing, timing or assets changed.
