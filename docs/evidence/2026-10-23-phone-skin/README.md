# Phone CPU attribution, rejected comparison and listening checkpoint

**No Android normal-cache gain is admitted.** Three fresh landscape control
profiles supply finer CPU attribution, but candidate starts fail temperature
matching, then framework thermal status reaches 1. All rejections remain. The
phone is released at **05:24:29 UTC on 23 October**, with its exact starting
development/production APKs, all 18 prior save/lock files, preferences and original
rotation verified. No new save paths, debug PID or Horde surface remain.

Work stays in the existing `.worktrees/horde-1.7-wp2-vertical-proof`, branch
`codex/horde-1.7-wp2-vertical-proof`, draft PR 27. Runtime source base is
`bceed0c16936cb40400536e1cad58b0040f131f6`. No smoke, shader, gameplay,
quality, save-schema or recovery-policy change is admitted here.

## Exact builds and workload

Owner-offered **SM-S948B**, Android 17 / API 37, Adreno 840, real Vulkan
RayTracingPipeline. Both arm64 development APKs use Shipping/Mobile shaders,
private Debug outdoor-scale admission and route timing. Version remains
1.6.2-debug/code 10; this is 1.7 development, not a release.

| Variant | APK SHA-256 | Bytes | Native library SHA-256 |
| --- | --- | ---: | --- |
| Pure control | `778b1692563473a19d717b24bef6a94c90a5a4a4bd4fea5dbd9aa22343458a06` | 178,942,282 | `1cb3a64b76edf408df111ab62728972ba79cfa871110cbe76805f13dac8967a6` |
| Current candidate | `c1d657491ef072c99a405f0f8ca5264860834057742fc214c820f53fef3fbf09` | 184,174,718 | `f73400d0ae4cc544b2def20c01db379632e6fadc5852e90fb0ecbf69ed9375d0` |

The control changes only `SkinnedMeshAsset.cpp` to `c2bbc714^`, retaining the
new child counters. Candidate is the current c2bbc714 implementation. All other
198 ZIP entries, including assets, dex, manifest and resources, match byte for
byte. The native library differs by 304 bytes. Candidate's larger APK contains
an unreferenced 5,222,678-byte ZIP placeholder region; this is container layout,
not additional runtime asset content or an installer-size estimate for release.
Both native entries are stored and 16 KiB aligned.

Each finite hold stages the existing post-Keeper lookout, then runs ordinary
shared 60 Hz simulation for 40 seconds: 10 seconds warmup and 30 seconds sampled
by completed-frame owning state. Camera/route remain stationary. Damage is
disabled by the fixture. This is not a natural Keeper victory or route/save test.
Speech/SFX and music are suppressed during timed motion; listening is separate.
No mid-run image readback is requested.

Tuple is water Mobile/1, fire Low/0, cap 30, glass Off, Current shadows/1,
**Mist Off**, dust 1, LOD1. Render scale is transient; saved 33% is unchanged.
Landscape native view is 2981x1440; rotated swapchain is 1440x2981. Dispatches
are 288x596, 475x984 and 576x1192 at 20/33/40%. UI remains at output resolution.
The initial portrait control has a different output extent and is kept separate.
Twenty percent remains visually unadmitted.

## Measurements and limits

| Landscape control | Post-warm frames | GPU median | Skin median | Kit vertices median / p95 | Frame-cycle median |
| --- | ---: | ---: | ---: | ---: | ---: |
| lc20, 20% | 944 | 17.8562 ms | 11.4735 ms | 4.4940 / 5.7122 ms | 33.0509 ms |
| la33, 33% | 539 | 39.6475 ms | 11.7278 ms | 4.6984 / 5.6390 ms | 56.0719 ms |
| lc40, 40% | 390 | 61.7734 ms | 10.5930 ms | 4.4852 / 5.0669 ms | 78.0303 ms |

Starting temperatures differ beyond the declared 1.5 C bound, so this is **not an
admitted common-temperature scaling cohort**. Later candidate starts fail even
the same-scale thermal gate. The final attempted warm reference records
AP 46.1 C, battery 37.8 C, skin 40.3 C and framework/skin status 1, and stops
before sampling. No candidate timing, percentage gain, phone admission or
overall FPS claim follows. Earlier phone 12.06 ms skin evidence remains its own
snapshot, not the current component result or an interchangeable baseline.

The 33% control has active work on all 539 sampled frames:

| CPU child stage | Median / p95 | Work per sampled frame |
| --- | ---: | --- |
| Player pose | 0.03109 / 0.04245 ms | One shared solve |
| Player world vertices | 4.54885 / 5.05161 ms | 34,304 vertices |
| Viewmodel vertices | 1.58182 / 1.84641 ms | 15,855 vertices |
| Kit clip/palette | 0.02031 / 0.02734 ms | 24 joints |
| Kit vertices | 4.69844 / 5.63896 ms | 26,945 vertices |
| Kit copies/grounding | 0.66885 / 0.71589 ms | Two calls, 3,017,840 representation bytes |
| BLAS command recording | 0.03036 / 0.04057 ms | Four calls |
| TLAS command recording | 0.00766 / 0.01063 ms | One call |

Parent skin timers include their children; do not add them again. Upload and
player output repacking are separate. The 33% fence-wait median is 39.9757 ms,
not CPU busy work. GPU durations overlap CPU/wait intervals; frame-cycle
reciprocals are not scanout FPS. Presentation intervals use joined
VK_GOOGLE_display_timing records, not video gaps or measured photons.
Representation bytes do not measure memory-bus traffic. BLAS/TLAS figures cover
CPU command creation, not GPU AS execution. Ranges, tails, call/operation counts,
thermal context, pipeline hashes and raw evidence hashes are retained in the
[receipt](receipt.json) and [summary](analysis-summary.md).

No scheduler/thread sampling CPU profile or frequency/power trace was captured.
A read-only `run-as` read of `/proc/uptime` was denied by Android and was not
retried or bypassed. A public `dumpsys cpuinfo` snapshot covered an older broad
interval rather than the requested owning frames and does not close CPU-use
attribution. The useful new evidence is elapsed per-phase work and separate wait
timers. Finer busy/wait and GPU-AS attribution remain open.

The [read-only thread assessment](THREADING_ASSESSMENT.md) maps actual owners,
serial skin/upload/AS work and the one-frame fence dependency. It proposes a
single persistent generic pose-worker experiment and the missing scheduling
trace; no worker, affinity, frame-count or broad threading change is implemented.
The parent is independently reviewing performance before coordinating that plan.

## Failures and observer corrections retained

- Initial APK selection used a stale legacy output path. That b47df517
  development APK installed compatibly, but the error was caught before any
  timing and excluded. Both measured variants were then resolved through
  Gradle's authoritative redirect/listing, with exact installed hashes verified.
- The portrait a33 sample remains standalone. a20 and landscape la20 fail
  starting temperature admission before sampling. Same-scale lb33 and final
  warm wb33 also fail before sampling; no limit is relaxed.
- Rotation pauses/recreates the native surface and restarts its benchmark
  generation counter. The old observer incorrectly expected the counter to be
  monotonic across this lifecycle and reaches its unchanged 150-second deadline.
  Fresh post-request completion timestamps and a changed scene epoch separately
  prove the new landscape staging completed. The local observer now uses fresh
  completion timestamps; all hold, frame, presentation and thermal gates remain.
- A presentation-only JSON was initially supplied to the offline analyzer and
  rejected. Its proper input is the full benchmark wrapper. No report or schema
  assertion was weakened to accept the wrong file.

Raw reports, exact APKs, custody records, observer failures, local source freeze
and scripts remain under ignored `reports/phone-skin-20261023/`. No owner saves,
raw XML, battery histories, APKs or media are added to Git.

## Owner listening and remaining physical acceptance

Sam reports: “music sounds good in current phone test - its the new forest
track”, followed by “horde”. This was relayed during the connected control APK
778b1692 session after landscape staging, when the approved outdoor main could
play outside timed motion. Saved Speech 100 / SFX 20 / Music 55 are preserved.
This accepts **subjective forest-track listening in this session only**. It does
not accept Kit balance, dialogue ducking, every loop seam, tomb/outdoor/return
transitions, all endpoints or all devices. No repeat listening request is needed
for that same forest track/session.

Landscape subtitle/touch and auto-crouch/analogue feel remain pending. No extra UI,
combat, natural recovery or audio-acceptance operation ran after thermal
rejection. Restoration verifies development fe3a51be and production cc4de768,
all 18 prior save/lock hashes, zero new paths, preferences byte equality,
rotation 0/auto 1 and unchanged font scale 1.7. Launcher is foreground with no
Horde process/surface. All owned device helpers/installations/transfers completed.

## Reproducible analysis and exact-head CI

Preserved `analyze_hold.py` keeps the original strict settings, residency,
completed-owner and presentation joins. `analyze_skin_hold.py` adds unsigned
64-bit validation for eight appended stage groups on **every** completed row,
then uses the identical 10–40-second owning-state window. Only imports and
private output location are adapted for this evidence directory.

```powershell
python -B docs/evidence/2026-10-23-phone-skin/test_skin_hold.py
python -B docs/evidence/2026-10-23-phone-skin/analyze_skin_hold.py reports/phone-skin-20261023/la33/HordeLanternRT-benchmark-latest.json reports/phone-skin-20261023/la33/android-motion-skin23-la33-resources.csv --expect-scale 33 --output reports/phone-skin-20261023/la33/reanalysis.json
```

Nineteen preserved analyzer checks and six new corruption/selection test methods
pass. These are synthetic tool checks, not runtime acceptance. Both exact
arm64 probe builds pass. No further full-suite rerun is needed for prose and
offline-analysis preservation.

Source head bceed0c1 [run 39903122389](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39903122389)
finishes **cancelled**: Vulkan-host, Android, GCC, Clang and sanitizer jobs pass.
MSVC's annotation says “The job has exceeded the maximum execution time of
15m0s”. Its main selection completed 159/169 cases and started
`horde_rt_region_page_build_policy_tests` when cancellation arrived; no final
selection total or subsequent offline checks are available. This is not a green
required programme or a demonstrated gameplay hang. No deadline/test is changed.

The next source slice is the bounded [shared-volumetrics proposal](../2026-10-23-chimney-assessment/VOLUMETRIC_FOLLOWUP.md).
The isolated v5 outlet derivative stays unadmitted; incomplete owned package
paths were archived and restored to accepted v4. No smoke confounded the phone
work. The next phone optimization comparison should begin with a cooled,
status-0 device and one same-resolution pair before multi-scale conditioning;
avoid leaving the renderer active while preparing unrelated work. PR conflicts,
the Debug 30-second route gate, natural earned recovery and unspecified flask
policy remain open. No merge, release or deployment occurs.
