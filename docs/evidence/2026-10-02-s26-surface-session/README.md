# Bounded Android surface-start ANR repair

Base `383a596`. Control APK `e9fd31e7c9aee83a13d0a91f25e2b61497eb986940032dbb534e13d9787f4774`.
The [owned ANR stack](../2026-10-02-s26-report-send/owned-anr-main-stack.txt)
proves UI-thread resume was inside Adreno pipeline compilation. The already
delivered acceptance email is not repeated. This is a lifecycle repair, not a
player, music, shader, material or glass investigation.

## Ownership change

One persistent native lifecycle owner probes/initializes off UI and joins the
old render owner before starting another. The existing render thread still owns
simulation and rendering. JNI acquires an ANativeWindow reference before enqueue;
the move-owned latest request either releases it or transfers it to the context.
Pending and ready are distinct. Activity-owned generation tokens scope stop and
readiness, including old-Activity teardown. One atomic generation/status word
rejects cancelled/stale readiness. Cancellation is immediate; checks between
initialization stages and around frame work discard stale work safely.

A driver call already in progress cannot be interrupted. Its eventual completion
and GPU idle/cleanup run off UI, not in pause/destroy. No concurrent Vulkan owners,
new frames in flight, quality change or fake presentation. Reports carry an
internal owning generation; getters and new report context reject a stale
surface snapshot. Private report files remain historical snapshots, not a live
readiness signal. Probe refresh is retained off UI.

## Finite matrix / next unfinished step

| Row | Result |
| --- | --- |
| Portable CPU ownership/cancellation | Debug3/3 PASS: surface mailbox, music-worker session, input mailbox stress. Move-only release, pending/worker cancellation, stale completion/Activity stop, blocked startup and Close wake tested. |
| Android build/unit/lint | Revised integration: all4 Debug ABIs/assembleDebug PASS1m40s; unchanged Java inputs reuse the fresh76/76 PASS/lint42 warnings,0 errors from the preceding1m8s check. Review's stale-report rejection is included in the new native artifact. |
| Interrupted pending startup | PASS: S26 generation1 requested18:15:06.097, cancelled18:15:06.389 before presentation. Replacement3 presents18:15:30.164; no old-generation presentation. Cold Activity426ms, immediate resume wait11ms. |
| Rapid resume after ready | PASS: two cycles, generations5/7 honestly present; Activity wait8/9ms, fresh own-app UI roots during startup, same PID10077, no new owned ANR. |
| Affected route / normal presentation | PASS: run182844 on runtime242e573/exact retained APK,13/13 replay waypoints,1838 skin updates, four inspected opening/combat/held-lantern views, and honest RT after Home/resume. All four PNGs byte-match the preceding normal-correction control. Not Shipping timing or owner touch acceptance. |

The first broader CTest invocation had two NotRun results because those selected
executables were not built; building the explicit three targets yields the
recorded3/3 pass. Initial build observer handles disappeared during an interrupted
turn; logs lack terminal success and no owning process remains. They are not
passes. A fresh final build is justified by that interruption and subsequent
source review changes, not by repeating unchanged completed artifacts.

Local raw/build artifacts: `C:/Dev/tmp/horde-surface-session-20261002/` and
`C:/Dev/tmp/horde-surface-session-*.log`. Retain one candidate receipt and mark
completed rows here; do not restart this experiment after compaction.

Affected runner receipt: [phone/run-20261002-182844](phone/run-20261002-182844/summary.json).
No rebuild/install was repeated. Tracked runtime inputs were clean at launch;
the runner's `sourceDirty:true` records preserved unrelated S24 scratch (and
later documentation), not an unrecorded runtime modification. Four exact image
hash comparisons are retained alongside the manifest; identical blobs are not
new visual tuning. Same process13226 owns the route/captures/resume; generation13
presents after settled Home/resume. The timing CSV is empty in Replay mode, so
incidental Diagnostic frame logs are not promoted to performance evidence.
Full own logs remain local; bounded lifecycle markers are retained. After the
completed run the app was left at Home and the phone returned to owner/Briarhold
use; do not operate it again in this run. Next unfinished step is the current-source
[integration matrix](../2026-10-02-final-integration/README.md), not another route.

## Exact candidate and limits

Debug APK SHA256 `d5cea1487b58f29a146a1195cd807cce5441f845ac705a16aeabdd5065add4be`;
installed APK pulled back byte-identically. Native bridge build-input SHA256
`9aa8867e01e45424a4085f510a14bc25a295f03abc2d1160e1d55cefc5d30549`;
MainActivity `d2b4434d71675924407bf6adb4b1b7721204cdec1a977413402554bd5b8d07bc`,
ProbeBridge `6e0d70d9386b5ed7de2ce58efddce7705b45d6b675c5ba29354556c63d8542d6`,
mailbox `e55be37b4aa2b1b69fd8d4d908a437b630be178a31316d8740b3b10e2759b572`.
SM-S948B/Android16/Adreno840 driver512.842.19; actual RayTracingPipeline RT
presentation1080x2235. No data clear, stable-app change, volume/font reset or S24
action. Full owned logs/exit history stay local; selected lifecycle markers,
fresh own-app hierarchies, launch receipts and capability JSON are retained here.
Exit history contains only the pre-candidate17:10 ANR and expected package-update
termination of the old process, not a new candidate ANR.

This fixes UI blocking, not pipeline compile time. Ready resumes still take
13.119/13.795s from request to RT frame; the interrupted-start replacement takes
23.666s while serially cleaning up the cancelled compiler work. No gameplay FPS
gain, total startup latency reduction, Shipping/device-family or subjective
audio/touch claim is made. A driver call already running is not forcibly aborted.

Audio/haptic manual revalidation required:NO for unchanged assets, gains, event
identity/timing and intended ready-surface gate. Automated audio-focus/session
contracts remain separate from the already-open exact external-focus device row.
Goal ACTIVE/incomplete; no merge, release, signing recovery or publication.
