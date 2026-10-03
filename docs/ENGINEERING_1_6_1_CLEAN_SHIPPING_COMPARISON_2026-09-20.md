# Clean Shipping comparison preparation

Status: **clean build/static proof and short Windows route A/B recorded;
lantern-specific and sustained matched performance gates open**.
This is a targeted development check, not the final 1.6.1 validation matrix.

## Frozen Windows candidate

- Source: `62ed1f13e815699ccb7551e1b83fc4220135dcfb`, clean detached checkout
  `C:\Dev\tmp\horde-c4-clean`; separate build `C:\Dev\tmp\horde-c4-build-clean`.
- MSVC 19.44, Visual Studio 2022 x64, Windows SDK 10.0.26100.0,
  Vulkan SDK 1.4.350.0; native Windows target **Release / Shipping / High**.
- Fresh configure and `horde_rt_diagnostic_window` build both exited 0.
  This targeted build used `BUILD_TESTING=OFF`; it is not a full CTest pass.
- EXE SHA-256: `d0911c2917e848363eab8833619928869d67c397439f53378061ce433d07a0df`;
  2,888,704 bytes. The staged runtime contains 52 individually hashed files.
- Local evidence: `reports/c4-20260920/artifact-provenance.json`,
  `candidate-shader-containment.json`, `prepare-windows.ps1`, and `candidate/`.
  The preparation script copies the public packager's existing 29-entry runtime
  inventory, audio and unchanged licences without running its release workflow.
  No publication or production signing was performed.

The first clean checkout at `e451f97` failed configuration: Windows
`core.autocrlf=true` converted the byte-checked pipeline catalog header to CRLF.
The file was identical after CRLF-to-LF normalization. Commit `62ed1f1` adds the
missing `text eol=lf` attribute; the strict freshness check is unchanged. The
existing pipeline-policy/ownership tests passed, then a **new checkout** configured
and built successfully with the same Git setting. No shader regeneration was needed.

## Exact packaged shader proof

The existing containment scanner extracted and validated/disassembled all four
expected modules from this EXE. Every module has zero atomics, no diagnostic
binding 22 and retained hardware ray-query capability/instructions.

| Backend / strategy | Words | Ray-query initialization sites | SPIR-V SHA-256 |
| --- | ---: | ---: | --- |
| RayTracingPipeline / OpaqueFast | 122292 | 23 | `7ebdb794794a854b6cb5c44c75dd9a9decd42f4999c44a9cfa78f13b12a13f21` |
| RayTracingPipeline / GenericDielectric | 54232 | 3 | `1be2b430ba190ae82250693ae079adf3f0712b361914b1355f386fb1f57fc5aa` |
| RayQueryCompute / OpaqueFast | 122357 | 23 | `400005de2b4c24385c6a945bda9c1893c13b792883b86a05b79d104d3d99edb6` |
| RayQueryCompute / GenericDielectric | 54295 | 3 | `29cb2f3a4e4f36b27774ed375a15cbe93971357958fec73017fead9dac29098f` |

This proves the exact packaged module properties, not image equivalence,
runtime performance, CPU readback behavior, or acceptance of either backend.

## Published baseline and comparison contract

Fresh local archive hashes match the published 1.6.0 record:

- Windows ZIP: `7b0dcf24b4a47771a9c3a27cbc52e3899c87781109afcef20f7a9a8472411d77`.
- Android signed APK: `52a64255ad5dec82cc866fb2ea3545be498ca06c73a789019be851c77e5d6c48`.
- Published runtime source: `57c81b635a6c10e2772283639026936adac80f8b`.

The Windows ZIP was extracted to `C:\Dev\tmp\horde-c4-baseline-160`. Its binary
is unmodified. It has no unattended benchmark command; use its existing menu.
The candidate supports `HordeLanternRT.exe --benchmark-showcase <output-directory>`.
Baseline reports are saved beneath its executable-relative `reports/` directory.

Source comparison confirms both builds retain the same 13-waypoint route,
0.032-world-unit frame step, two laps (first warm-up, second measured), 4,000-frame
lap cap and ten reported zones. The existing route expectation is 1,838 measured
frames. Verify actual totals and per-zone counts from every completed run.

The common Windows metric starts immediately before `RenderFrame` and ends after
`UpdateRtLabTelemetry`. Candidate metadata names it
`windows-render-plus-rtlab-telemetry`; the baseline predates that label but source
confirms the interval. Compare the common overall/per-zone mean, median, P95 and
slowest-one-percent-derived FPS. The candidate's new stage/GPU distributions are
additional evidence, **not comparable baseline stage measurements**.

This is a net engine A/B: candidate CPU evidence bookkeeping is included in the
same outer interval. Do not attribute the entire difference to shader changes.
Both must use the RayTracingPipeline backend, matching GPU/driver/API, present
mode, material route, render scale, internal/presentation extents, water quality,
route/order and warm-up. Retain exact source/artifact/shader identities separately;
they deliberately differ. Settings are executable-relative
`HordeLanternRT.settings.ini`, so a separate directory does not inherit settings.
Water quality, driver and thermal/power context require accompanying evidence;
the legacy benchmark JSON alone does not contain everything needed for matching.

Use balanced repeated A/B order with no overlapping renderer/build workload,
retain temperature/clocks/power context, and report sustained behavior separately
from initial process warm-up. Investigate matched regressions over 15%; do not
lower quality or resolution to make a result pass. The same-build Shipping versus
Diagnostic image gate remains separate from this different-build comparison.

## Short Windows A/B observations

The owner unlocked the desktop after the initial locked-screen observation.
Baseline settings were inspected through its UI: High water, 100%, windowed.
The candidate has no settings file and uses the same source-verified defaults.
No settings, quality or resolution were changed. Builds/scanners were held during
the four runs; only one Horde process ran at a time. All four completed both laps,
26 waypoints and 1,838 measured frames, and each process exited normally with code 0.

| Order | Artifact | Overall median ms | P95 ms |
| --- | --- | ---: | ---: |
| A1 | Published 1.6.0 | 6.3450 | 8.4849 |
| B1 | Clean candidate | 6.9207 | 8.9366 |
| B2 | Clean candidate | 6.9574 | 8.7479 |
| A2 | Published 1.6.0 | 6.1193 | 8.0250 |

The mean of the two run medians is 6.23215 ms baseline / 6.93905 ms candidate:
**11.34% slower in this short test**, not an improvement. These are not pooled
per-frame medians. Every run's median lies below the descriptive 16.667 ms reference
line, but that does not certify other scenes or sustained operation. The ten
per-zone mean-of-run-median differences range from +4.97% to +13.24%.

Both candidate ledgers have 1,838 expected/completed/CPU-accepted/GPU-valid rows,
zero rejects/cancellations/outstanding samples, exact submitted/completed identity
joins, monotonically increasing completion, final completion serial 3676, all
presented/valid CPU rows and compiled-out diagnostics. Common report workload
fields and all per-zone denominators match across all four runs. GPU/API/packed
driver identities agree; NVIDIA's utility calls this driver 610.47 while the
application's packed-version formatter reports 610.188.0. Preserve both labels.

Reports, hashes and checks are retained under `reports/c4-20260920/` in
`baseline/`, `candidate-b1/`, `candidate-b2/`, `windows-abba-summary.json` and
`analyse-windows.ps1`. `gpu-context.csv` retains 1 Hz temperature, power, clock and
utilization samples. Its incomplete final buffered row is not usable. The retained
242 complete rows span runs **and idle gaps**: 54–64 C, variable clock/power states.
They must not be presented as per-run thermal equivalence. A1 followed long menu/
background residence, unlike the fresh B1/B2/A2 processes; each still had its own
warm-up lap. These limitations prevent a sustained, thermal-matched causal claim.

## Lantern coverage gap and next measurement

The owner correctly pointed out that this benchmark does **not** exercise the
heavy held reward lantern. The `finale` report zone is a spatial zone name, not
proof that reward interaction, lantern raise or skylight reveal ran. The route
sets player position/yaw and traverses waypoints without obtaining the lantern.
This A/B validates the general route and evidence machinery only; it cannot pass
the audit's exact-lantern performance gate or stand in for a worst-case workload.

Next implement an explicitly labelled Release-safe lantern benchmark preset,
using gameplay-owned reward/held-item state and real production geometry. Cover
held high/low, relevant motion/grazing views and the reveal separately; report
per-case results so route averaging cannot hide the expensive glass. Existing
authored `lantern-held-high`, `lantern-held-low`, `lantern-glass-transmission`,
motion/sweep and wall checkpoints are references, not permission to enable all
Debug mutation controls in Release. Preserve the old route for historical
comparison. Exact published 1.6.0 lacks the new preset, so a backported measurement
harness must be explicitly identified as a source-baseline test build, never
mislabelled as the unmodified public binary. Agree identical staging/workload
semantics before attributing any difference to glass optimization.

No new phone run, final cross-device pass, remote CI pass or image parity is
claimed here. The original player/viewmodel/glass, music and reporting work remains
required before final comprehensive validation. Phase 1 is not complete.

Audio/haptic manual revalidation required: **NO**. This slice changes checkout
formatting only and prepares existing benchmark artifacts; no feedback semantics
or assets changed.
