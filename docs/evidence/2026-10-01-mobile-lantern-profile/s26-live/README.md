# S26 unfrozen candidate containment and music clock

Exact SM-S948B/R5GL219SZGK, normal Debug APK
`e10b02034e0a0a78828c6ff885d03b79dd2058f77c6f20346a7c977edd794af4`,
runtime source1da483d, evidence head0ff5860; Diagnostic/Mobile pipeline75%.
Installed SHA was checked; no rebuild/reinstall, data clear, asset/shader/player/
audio change, system-volume change, invulnerability or enemy masking. This is
functional/clock evidence, **not** another Shipping comparison or owner acceptance.
S24 was disconnected/released and was not accessed.

## Failed setup retained, then one legitimate authored state

`lantern-chest-unlock` finishes its frozen480-frame setup, then unfreezes with
the underlying lich still alive: its reward/finale import does not kill the
actual encounter. Player dies; ordinary Retry restores the live finale and dies
again during inspection. Retain `start-screen.png`, `retry-later-ui.xml` and
scoped log. Neither is an audio freeze, valid live lantern test or performance pass.

Existing `finale-roof`11 builds the authored defeated-lich/claimed-lantern state
through its established encounter/death/reward sequence. Normal non-capture
intent begins19:43:00, finishes its480 frozen setup frames19:43:46; ending card
appears after Resume. Ordinary Back invokes Continue; normal HUD/vitality3,
high/low, parry/swing and movement/look inputs are then active. No checkpoint
source, gameplay rules or accepted geometry was changed to obtain this state.
This is a post-claim live test, **not** pickup-to-reveal interaction acceptance.

## Continuous inputs and review

`live-motion.mp4`:60.0084s,29,924,404 bytes, SHA256
`efdcb13007673a0074dc72a29d202601db5397f70b21c7e7c10c1e5e483d47f8`.
Recording ends approximately19:47:31 local; it is not performance instrumentation.
Normal controls, from actual UI bounds, were sent as follows:

| Local time | Input |
| --- | --- |
| 19:47:08.989 | Lower, tap642/2609 |
| 19:47:10.064 | Low parry, tap768/2882 |
| 19:47:12.128 | Low swing, tap1174/2882 |
| 19:47:14.193 | Look down, swipe1100/1400 to1150/1580 over1200ms |
| 19:47:15.463 | Forward, swipe300/2100 to300/1800 over2500ms |
| 19:47:18.031 | Strafe, swipe300/2100 to500/2100 over2000ms |
| 19:47:20.090 | Raise, tap642/2609 |
| 19:47:21.152 | High swing, tap1174/2882 |
| 19:47:23.217 | Look up, swipe1150/1580 to1075/1400 over1200ms |
| 19:47:24.478 | High parry, tap768/2882 |

Lead inspected three4fps contact sheets from video offsets38/50/54 seconds
(`ffmpeg -ss OFFSET -i live-motion.mp4 -vf fps=4,scale=240:-1,tile=4x4 -frames:v 1`).
Hands, grips/open cage and changing sword/camera poses remain visible; normal
HUD stays alive. Sampling does not certify every attack-arc frame, perception,
all shadow configurations or glass physics. Full movie retained in Git LFS.
No player/viewmodel tuning follows from this profile containment check.

## Twenty consumed music periods and lifecycle

Epoch4 periods1-20 advance the **device head** to11,520,000 frames (240s content),
with zero underruns and bounded generated/accepted/head ordering. Nineteen wall
intervals are11.996-12.004s; first-to20 elapsed227.997s. Reuse the existing music
clock's unchanged50ms logging bound and queue checks in the offline helper.
The safe state selects looping cueH; it does not certify every cue/transition or
combat masking. Before pause reaches period28, still zero underruns.

Menu/settings pause: no further period logs while paused. Normal saved Music70%,
SFX70%, sensitivity100%, Mobile profile and restored75% are in settings XML.
Two Back inputs briefly resume then reopen the menu; this is **not** claimed as
a completed resume check. Home19:53:26, Activity return19:53:41; native RT reaches
successful presentation19:54:00. First UI dump during startup has a null root;
the pulled `home-resume-ui.xml` is **stale settings XML**, excluded from validation.
Fresh `home-ready-ui.xml` is the actual menu. Resume tap19:55:11 yields alive HUD,
retained camera/lantern and epoch4 periods29-37, zero underruns. No epoch reset or
audio worker error. Home cleanup19:56:58 stops the surface/pauses music normally.
Audio-focus interruption and full route/SFX balance remain open; no new listening
request is triggered by unchanged playback/geometry.

## Resize, short display observation and memory boundaries

Settings75→100→75 makes two existing output-only transitions:
19:52:03.906:1440x2980,107.186ms;19:52:08.231:1080x2235,165.946ms.
These are `idle_and_resize_ms`, **not** button-to-first-frame/input latency,
ownership inspection on the phone, or matched old/full-scene-versus-new speedup.
Post-resume image/state retains lantern, camera and alive HUD. Defaults restored.

Actual live SurfaceView BLAST layer#83341 is queried only after recording ends.
Latency stats cleared19:48:44; second-column valid actual-present timestamps
give81 timestamps/80 intervals over8.941667s: median112.500ms,p95116.667ms,
average8.947FPS. This is short stationary **Debug post-finale** display evidence,
not the timed Shipping opening/heavy windows or an A/B gain. Column/invalid-value
interpretation follows [AOSP FrameTracker](https://android.googlesource.com/platform/frameworks/native/+/refs/heads/main/services/surfaceflinger/FrameTracker.cpp)
`dumpStats`/`isFrameValidLocked`; the Samsung build's finite usable history is
retained, not extrapolated to a sustained sample. An earlier shell quoting error
was corrected before collection; refresh period alone was not treated as FPS.

One active sample after recording: PSS524168KiB,RSS645100KiB,
native heap PSS180977KiB,Graphics283268KiB. System memory PSI avg10/60/300=0.00;
separate thermal snapshot status3,reported skin44.4/45.0C (HAL/cached entries,
not one aligned frame). Console-only context excerpts are retained below;
not frame-aligned or a sustained RAM
plateau/A/B; recording allocation and previous replay context differ. GPU memory
bandwidth/cache/stall/register/spill/occupancy remain unavailable counter gaps.

`analyse-retained-live-check.ps1` is a finite **offline** projection of this log,
installed receipt, actual live UI/settings and latency history; no device action.
`analysis.json` and SHA256SUMS bind the retained evidence. Do not repeat valid
timings, captures, long loop gate or unchanged builds on resumption.
Offline admission also rejects four synthetic mutations: missing period20,
one underrun, a one-second clock error and wrong APK. The first underrun-mutation
producer used an ambiguous regex replacement and removed its matched row;
`offline-negative-checks-invalid-underrun-mutation.json` retains that invalid
counter-test result. Only that synthetic case was corrected/rechecked, with its
row preserved and actual underrun gate failing. No physical run was repeated.

Next independent implementation: foreground explicit-consent local player-report
export on both platforms. Owner live feel/pickup, matched Shipping display pacing,
S24 hands/enemies, S25, High glass/backend and final-candidate gates stay open.

Console context (not a complete raw thermal dump):

```text
some avg10=0.00 avg60=0.00 avg300=0.00 total=614215963
full avg10=0.00 avg60=0.00 avg300=0.00 total=537277795
Thermal Status: 3
Temperature{mValue=45.0, mType=3, mName=SKIN, mStatus=3}
Temperature{mValue=44.4, mType=3, mName=SKIN, mStatus=3}
```
