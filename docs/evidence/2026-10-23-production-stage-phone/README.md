# Production-shader outdoor GPU attribution

23 October 2026. One bounded physical Galaxy S26 Ultra **SM-S948B**, Android 17
API 37, Adreno 840 / driver 2150932499 / Vulkan 1.4.295 run. No renderer candidate,
worker comparison, old private shader bank or new tracing source was used.

## Artifact and checks

Source `8c3246d07cfd341779d719d9a75dd3df4600a981`; arm64 Debug, explicit
Shipping/Mobile, route timing + outdoor hold + GPU stage timestamps. Dispatch
clock, staged primary, private root, benchmark validation and actor worker are
OFF. The unchanged package buildId remains `1.6.2`; this is a 1.7 development
probe identified by its source and artifact hashes, not a release.

- Local offline build passes in 59.03 s (Gradle reports 58 s).
- APK: **184,180,590 bytes**, SHA-256
  `91f2ff1d7fe9dcb979d91978c9befe72b577d971a998572487b2c862a0e114bc`.
- Native library: 5,226,136 bytes, SHA-256
  `6f7e1c6a616b528ff52167104e8e21d0a9055b97b1cd55f37c48274dcc2d4b8a`.
- All four production Shipping/Mobile Pipeline/RayQuery SPIR-V payloads were
  found byte-for-byte in that native library. The physical completed-frame
  receipt selects Pipeline / opaque-fast with the matching production hash.
- All 17 packaged runtime music WAVs match their repository assets exactly.
  No source FLAC/ZIP/PCS1/Blend masters, alternate outdoor bed or parked chimney
  v5 payload is packaged. Accepted town shell v4 remains in the APK.

The [APK receipt](apk-receipt.json) records flags, shader hashes and assets.
The unchanged existing-cache configuration script also passed after explicit
owner approval; its earlier denials remain in
[the configuration record](../2026-10-23-production-stage-admission/README.md).

Build-only command from `android/`, at the recorded source:

```powershell
.\gradlew.bat assembleDebug --offline --console=plain --max-workers=2 `
  '-Pandroid.injected.build.abi=arm64-v8a' '-PhordeRtRouteTiming=true' `
  '-PhordeRtOutdoorScaleProbe=true' '-PhordeFrameStageProbe=true' `
  '-PhordeRtInstrumentationOverride=Shipping' '-PhordeRtDielectricQualityOverride=Mobile'
```

Resolve its APK with `tools/resolve-android-apk.ps1`, rather than an old outputs
path. Installation is a separate authorized action, not part of this command.

## Workload and admission

One staged **post-Keeper** lookout with normal fixed-step simulation, zero image
readbacks: `chapter-outdoor-scale-hold`, `lookout-sweep`, 10 seconds warmup plus
30 seconds sampling. This does not prove a natural victory or earned journey.
Portrait output/window **1440 x 2971**, internal **475 x 980**, explicit 33%.
Settings: Mist OFF, Mobile fire, Low dust, Current shadows, Mobile water, glass
off, requested cap 30. Owner preferences, orientation, brightness 115 and font
scale 1.7 were not edited. This is not a landscape control cohort.

The unchanged strict analyzer admits all **838 completed submissions** and
matches every GPU stage's tick/serial, including warmup and final rows. The
post-warm sample contains **626 frames** selected by each submission's bound
simulation-owner wall time. Query errors, missing rows and rejected presents
are zero; no assertion or timestamp gate was relaxed.

| GPU execution interval | Sample median | Sample p95 |
| --- | ---: | ---: |
| AS update/refit path | 0.547604 ms | 0.560052 ms |
| RT dispatch | 33.814895 ms | 34.635937 ms |
| Post-barrier transfer/copy | 0.124505 ms | 0.134687 ms |

RT dispatch is the dominant observed GPU interval in this view. Its median alone
exceeds the 33.333 ms 30-Hz budget. These intervals do not isolate material,
lighting, geometry or shader-instruction causes and are not additive counters.
AS work is a much smaller measured interval here; the result does not justify
prioritizing a broad AS rewrite. No shadow, mist, resolution or quality reduction
was used to claim a gain.

| CPU wall interval in the same sample | Median | p95 |
| --- | ---: | ---: |
| Simulation | 1.589115 ms | 2.673698 ms |
| Skin preparation total | 8.693516 ms | 11.883334 ms |
| Player skin preparation | 4.462631 ms | 6.858281 ms |
| Character skin preparation | 4.290079 ms | 5.302604 ms |
| Pose copy | 0.378516 ms | 0.579116 ms |
| Dynamic upload | 0.436356 ms | 0.555833 ms |
| Fence wait | 34.887136 ms | 35.749688 ms |
| Queue submission | 0.159479 ms | 0.300521 ms |
| Whole frame cycle | 47.748230 ms | 51.355105 ms |

CPU stages are nested wall intervals, **not per-core utilization**; fence wait
is not busy CPU. Do not add these medians. Finer pose/vertex/copy/AS-recording
breakdowns and actual counters are retained in [CPU analysis](cpu-analysis.json)
and [raw resource rows](resources.csv). The 8.69 ms skin value applies to this
exact run; it is not a gain over the historical 12.06 ms observation, whose
cohort/thermal context differs. The worker remains untested on Android and OFF.

Across the complete 40-second hold, actual presented-image spacing has median
50.000 ms, p95 54.167 ms and no interval above 250 ms. The strict collector's
observed image rate is 20.918/s; this is **not scanout/photon FPS or sustained
play acceptance**. [Stage and timing analysis](stage-analysis.json) retains those
distinctions and the original counters.

Device-local bytes stay 160,610,688; host-visible bytes stay 48,298,048. BLAS 20 /
TLAS instances 28 remain constant with no unload/restore event in this hold.
These are scene resource counters, not whole-process RSS or total device RAM.

## Thermal context and custody

Idle snapshot AP/BAT/SKIN: 27.3 / 26.4 / 28.5 C. After fixture preparation, hold
start: **38.6 / 29.7 / 33.9 C**; hold end: **38.5 / 30.5 / 34.3 C**. Framework and
typed sensor statuses remain zero at those observations. Power/brightness were
preserved; no clocks, power counters or scheduler sampling were collected.
This short post-preparation observation is useful field evidence, not thermal
equilibrium, a sustained warmed test or a matched control/candidate comparison.
There was one hold and no cooldown/retry loop.

**Phone released at 08:49:44 UTC. Safe to disconnect.** Both original installed
APK hashes are restored. All 18 pre-existing save/lock files and preferences are
byte-exact; no new save file was created outside the isolated test namespace.
Rotation, font and brightness are unchanged. Launcher is foreground; no owned
Debug PID, Horde surface, install, capture or transfer remains active.
[Custody receipt](closure.json) contains no campaign bytes or preference content.

Audio/haptic manual revalidation required: **NO**, this batch changes neither
audio nor feedback. It provides no new listening or touch/image acceptance.

## Reproduction and remaining checkpoint

[Receipt](receipt.json) binds original report hashes, production modules,
thermal readings and workload. The 343 KB CSV preserves every original field;
only line endings are normalized to LF, with source/canonical hashes recorded.
[sample-window.csv](sample-window.csv) preserves tick/serial/owner-wall selection.
Run `python docs/evidence/2026-10-23-production-stage-phone/verify-samples.py`
to reproduce the saved statistics. Full manifests/report/APKs and unchanged
strict-analyzer input remain locally under ignored
`reports/production-stage-admission-20261023/`; no binary is added to Git.

Backed-up head `9bd46a8f` CI
[run39924505547](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39924505547)
is terminal: Android, GCC, Clang, sanitizer and Vulkan-host pass; MSVC exceeds
its unchanged 15-minute job cap. Its selection reaches 159/169 passes and starts
test 160 before cancellation. The lane and subsequent tooling remain incomplete,
not an all-required-checks pass or a demonstrated gameplay failure.
[Exact summary](ci-result.json) and [retained excerpt](ci-msvc-excerpt.txt).

Next performance decision: use these production intervals to bound one
dispatch-cost investigation; the measurement does not authorize a shader or
quality change. Do not infer a phone win from the default-OFF desktop worker.
Next feature decision remains the documented F01 background sector and shared
tree/root-fit prerequisites, with the existing owner deferral preserved.
Natural journey/recovery, sustained warm performance, final input/audio/image
acceptance, Debug route deadline and PR conflicts remain open.
