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
