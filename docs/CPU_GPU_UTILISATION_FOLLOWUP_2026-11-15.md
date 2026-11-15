# CPU/GPU utilisation follow-up - 15 November 2026

## Owner observation (not a captured performance result)

Sam reported rerunning the laptop benchmark: GPU usage appeared capped around 70%, while total CPU usage peaked around 15%. His laptop is an i5-13420H / RTX 5050. Exact build, benchmark preset, resolution, FPS, monitoring tool, power mode and thermal state were not supplied. This is a profiling lead, not proof of a CPU bottleneck or unused achievable GPU throughput.

## Future capture checklist

- Record exact binary/build, scene/preset, resolution, graphics settings, display refresh, VSync/present mode, FPS cap, AC/battery and power/thermal conditions.
- Capture per-thread/per-core CPU activity, CPU critical-path frame times, GPU timestamp frame/pass times, and acquire/present/fence wait times together. Overall CPU percentage can conceal a saturated critical thread.
- Distinguish the selected GPU engine's reported utilisation from total GPU work; record clocks/power/thermal throttling and the monitoring tool.
- Compare an uncapped run where supported without silently changing visual quality; lower resolution only as a labelled diagnostic experiment.
- On Android, capture a scheduler/CPU-frequency trace and GPU timing on the exact handset/build. Multiple threads do not establish simultaneous use of all cores; scheduling and heterogeneous core placement are runtime observations.
- Optimise measured critical-path work. Do not target 100% CPU/GPU utilisation for its own sake or introduce worker overhead without an A/B frame-time and thermal result.

## Source inspection, development checkpoint cf048d47

This is native Vulkan, not browser execution. Android owns gameplay and rendering on its existing application/render thread (`android_probe_bridge.cpp`, `gGameSimulation` and `SwapchainRenderLoop`), with separate lifecycle, audio and service/background work. Music and ambience have dedicated PCM threads; startup asset preparation and reports use serial executors. This is multithreaded, but not a general all-core gameplay/render job system.

An exclusive companion-actor pose worker exists in `SkinnedPbrActorWorker.h`, guarded by `HORDE_RT_ACTOR_WORKER_EXPERIMENT`. Root and Android CMake default it OFF; its option explicitly states it is not admitted for shipping. Its existence must not be described as enabled in the public APK. No device trace was captured for this note, and no assertion is made about all-core utilisation in Sam's installed build.

## USB-connected mobile capture, 15 November 2026

Owner-authorised non-destructive run on SM-S948B (Galaxy S26 Ultra), installed native Android 1.7.0/code 11, source identity `d1c6d152d18c7002c41638d51dc9d9bcceea1bd4`. No installation, data clearing, setting change or public score submission. Automated release-safe `showcase-route-v1` completed at 09:33:10 UTC and returned to the launcher. Two laps, first warm-up, last measured; 26/26 waypoints and 1,838 accepted CPU/GPU evidence rows, zero rejected rows. Existing resolution was 25% (360x743 internally, 1440x2971 presentation), MAILBOX, Adreno 840, Vulkan 1.4.295, RayTracingPipeline. These results are not representative of a higher resolution.

Measured report means: frame cycle 17.046 ms (median 16.538; p95 23.514), GPU RT duration 9.699 ms (median 8.924), skinning CPU 5.159 ms (player 5.053; character 0.106), simulation 0.060 ms, dynamic upload 0.321 ms, fence wait 9.989 ms. Frame cycle is a CPU entry-through-present duration, not achieved display FPS. Fence wait is waiting, not busy CPU execution; stages are not independent costs to sum into an additional budget. Player vertex work (3.685 ms mean) and viewmodel vertex work (1.343 ms) dominate reported skinning.

Fifteen approximately two-second `top -H` snapshots during the run showed one native application thread dominating: mean 42.05%, range 30.0-49.5%, where 100% means one full CPU core, not the whole eight-core device. Music averaged 1.01% and other individual app threads below 1%. Thread identities were matched against app-only frame logs. These samples support a mostly serial CPU critical path, not all-core saturation; they do not establish a fully saturated core.

A separate approximately 24-second `/proc/stat` interval near the end reported whole-device (all processes) CPU busy 15.08%. Per-core busy: 36.13%, 19.02%, 3.47%, 3.06%, 27.97%, 29.51%, 1.57%, 1.57%. This interval spans the end of the run; it is neither isolated app CPU utilisation nor perfectly aligned with the measured lap. Nonzero KGSL `gpubusy` interval samples ranged 45.9-70.2% (mean 58.5%); these device-level intervals are likewise not a benchmark score. GPU clock access was denied. USB was connected; end battery temperature was 31.5 C. Battery temperature does not establish GPU junction temperature or absence of throttling.

A 90-second scheduler/frequency Perfetto trace and raw benchmark exports are retained locally outside the public repository for follow-up analysis; scheduler trace has not yet been parsed. Raw system samples may contain unrelated process details and must not be uploaded without sanitisation. No exact display-presentation timestamps or sustained thermal soak were established.

Interpretation: there is meaningful serial player/viewmodel skinning work and meaningful GPU/fence cost. Spreading all gameplay over all cores is not demonstrated as the answer. A measured experiment reducing or overlapping skinning and examining frame dependencies is warranted before selecting worker or GPU skinning architecture. This is an investigation lead, not permission to change the renderer or claim a performance gain.
