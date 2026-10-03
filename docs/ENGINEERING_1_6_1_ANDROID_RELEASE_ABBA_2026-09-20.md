# Android Release route comparison — September 20

Status: **four complete warmed route measurements; no demonstrated speedup,
and no glass optimization or final-release pass**.

## Exact artifacts and unchanged workload

- A: published 1.6.0, runtime source `57c81b635a6c10e2772283639026936adac80f8b`.
  The already-installed stable package was pulled back without replacement or
  app-data access and matched APK SHA-256
  `52a64255ad5dec82cc866fb2ea3545be498ca06c73a789019be851c77e5d6c48`.
- B: clean detached source `167ce8b09374bd522d7e8b2493693bc50c00931c`,
  `C:\Dev\tmp\horde-c4-clean`. Fresh four-ABI `assembleBenchmark` passed.
  APK SHA-256 `02112a48aea45431f52270ab9ee82d68091497aed0816dfcc359ba4068ee24bc`
  (85,952,031 bytes) was installed only into the isolated `.benchmark` package,
  then pulled back byte-identical. This is development-signed, non-debuggable,
  Release-derived `RelWithDebInfo` / Shipping / Mobile, checkpoints OFF; not a
  public release artifact. Stable app data/settings were not changed.
- The clean build's full `AndroidBenchmarkValidationBuildTests.ps1` passed its
  opt-in/negative-policy, manifest/development-certificate, four exact native
  build-model/cache and packaged ARM64 containment checks. The ARM64 library hash
  is `5d9dcd982a4d3668a675bd406071f4c6895992f1c0ca6dbe1b5e87158597944c`.
  All four packaged modules passed external `spirv-val`/`spirv-dis`, retained
  hardware ray queries, contained zero atomics and no diagnostic binding 22.

The connected device reports exact model **SM-S948B**, Android 16, Adreno 840,
Vulkan 1.4.295, driver 2150932499 / 512.842.19 (fresh system Vulkan JSON).
The stable settings UI showed **76% scale and Mobile water**. The isolated
candidate was deliberately set to the same 76% through its UI, leaving the
stable setting unchanged. This is not a 75% or 100% result and is not comparable
to the earlier 100% lantern timings. No other rendering quality was reduced.
Both must report internal 1094x2265, presentation 1440x2980, MAILBOX, strict ASTC
and the preferred RayTracingPipeline backend.

The comparison retains the original two-lap, 13-waypoint route: 1,838 warm-up
frames and 1,838 measured frames, 26 total waypoints. The gameplay simulation and
asset files are unchanged between these sources except benchmark/checkpoint
harness files. This route still does **not** acquire the heavy held lantern.

## Timing and thermal procedure

The common Android native metric is `RenderFrame` entry through
`vkQueuePresentKHR` return, including its fence/acquire/record work. It excludes
later report publication and is not display/input latency. Candidate completion-
owned CPU/GPU distributions are additional evidence, not baseline GPU samples.
Baseline text reports have three decimal places; candidate JSON has four.

Run order is A1–B1–B2–A2 without cooling, on USB power and without overlapping
builds or another foreground renderer. Each run retains its full warm-up lap.
The device was already warm before A1. Five-second context samples retain UTC,
battery temperature, thermal status and exposed GPU thermal power level. No
privileged GPU-frequency access is attempted. Baseline reports are read from the
visible benchmark UI, not private app storage or the clipboard.

Thermal status rose from 2 to 3 during this sequence; this must not be described
as four thermally identical runs. Report every result and the thermal ranges,
including earlier/later differences. Overlapping ranges alone do not establish
equivalence or causal attribution. A brief A1 measured-frame screenshot and an
unsuccessful A1 warm-up UI dump also occurred; later screen observations landed
after completion/during Activity recreation. No measured simulation was restarted.

Local evidence is under `reports/phone-release-abba-20260920/`. The clean package
scanner report is under `C:\Dev\tmp\horde-c4-clean\reports\`.

## Results and limits

All four reports pass the common workload, extent, scale, backend, ASTC, GPU/API,
two-lap/26-waypoint and ten-zone denominator checks. Every run has 1,838 measured
RT-presented frames. Each candidate additionally has exactly 1,838 completed,
CPU-accepted and GPU-valid rows, zero rejects/cancellations/outstanding samples,
exact submitted/completed identity joins and monotonically increasing owning
completion serials (final serials 4502 and 8186). Shipping diagnostics are
compiled out, not zero-valued physical-glass counters.

| Order | Build | Median ms | P95 ms | Thermal status | Battery C | GPU thermal power level |
| --- | --- | ---: | ---: | --- | --- | --- |
| A1 | Published 1.6.0 | 52.329 | 69.449 | 2 | 39.5–41.9 | 0–2 |
| B1 | Clean candidate | 54.1482 | 72.6266 | 2–3 | 42.4–43.5 | 0–6 |
| B2 | Clean candidate | 56.5649 | 74.3675 | 3 | 43.5–44.1 | 1–5 |
| A2 | Published 1.6.0 | 58.795 | 78.986 | 3 | 44.0–44.1 | 1–2 |

Thermal ranges cover whole two-lap run windows, not just the measured lap;
candidate windows also include startup and completion-observation delay. The
baseline process remained alive across A1/A2; the candidate process across
B1/B2, but surfaces/Activities were recreated between apps/runs. This was a
roughly 15-minute warm sequence, not uninterrupted same-surface gameplay.

Mean of the two run medians: baseline **55.562 ms**, candidate **55.35655 ms**,
a descriptive **−0.37%**. These are not pooled frame medians. A2 is 12.36% slower
than A1 without a baseline code change; the thermal drift is larger than the
aggregate build difference. B2 is 3.79% faster than adjacent A2, but their exact
temperature/power histories differ. Neither calculation demonstrates a causal
optimization. Every overall median is above the 33.333 ms / 30 FPS reference
line (also above the 20.000 and 16.667 ms lines), around 17–19 median-derived FPS.
These are honest workload observations, not an automatic threshold failure.

The hardened analysis script and byte-preserved reports are in the
[GitHub recovery bundle](evidence/2026-09-20-android-route/README.md). An initial
analyzer metadata-projection omission was fixed before the final successful
run; no report values were edited or substituted. Missing reports/identities,
wrong settings, invalid completion rows and unavailable thermal values cannot
silently become a valid comparison. This proves report arithmetic/integrity,
not thermal equivalence or final release correctness.

After measurement, the isolated candidate's scale was restored to its original
100% through the settings UI and confirmed; the stable app remains at its
untouched 76%. The phone was returned Home and the context sampler stopped.
Latest GitHub CI found for this engineering branch is still the historical
`191d799` host run, not a fresh CI pass for `167ce8b`.

## Separate lantern baseline

The published APK cannot select the new lantern presets. A source-only narrow
backport is being prepared in `C:\Dev\tmp\horde-160-lantern-baseline`, branch
`codex/horde-160-lantern-baseline`, based on exact source `57c81b6`. Shared core
commit `513b5d3` is reviewed, pushed and remote-verified. Its fresh MSVC Release
build and two focused tests (lantern core and existing route smoke) passed.
It must retain
the old renderer, shaders, assets, physics and timing interval, with identical
production-state workload staging. It is not the published binary. Platform
wiring, isolated packaging and actual lantern measurement remain pending;
Phase 1 is not declared closed by this route comparison alone.

Audio/haptic manual revalidation required: **NO**. This is benchmark/build
evidence; normal playback assets and semantic feedback inputs are unchanged.
