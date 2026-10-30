# Independent region pages: optimized Release route

The subsequently approved [host-phase follow-up](../2026-10-16-region-page-host-followup/README.md)
has now run. The pending-run statements below describe this earlier checkpoint.

The private page candidate completed the unchanged out-and-back route on both
real RT backends. Each run automatically unloaded and restored the selected
84,219-triangle forest detail owner, saving **12,453,440 bytes (11.88 MiB)** with
zero readiness-wait frames. The transition windows were smooth in these runs;
overall Pipeline smoothness and startup reliability remain open.

## Artifact and workload

Measured EXE SHA-256:
`e4ebf332f4e6e01157a6bfb3ef14f5c65a4ee0d6191fbb8529403d4195883d39`.
Source is `3a20b1c3b4199823b54039f143a7094a849f8fc8` plus four bootstrap-only
stderr markers. The archive retains every measured source file with hashes
verified against the original candidate receipt. It contains no executable.

Windows optimized Release CPU, Shipping/High private r10 shaders,
NVIDIA GeForce RTX 5050 Laptop GPU, driver610.47, Vulkan1.4.341.
Both runs used an uncovered foreground960x540 client at100% scale, Current
shadows, High water/fire and Dust Off. Each took96.46s and consumed5787 shared
60Hz simulation ticks. The scripted route starts after staged Keeper defeat,
raises the lantern, traverses the forest, returns/descends, backtracks through
the coloured passage, then returns and ascends again. It is a normal automatic
residency path exercised by test inputs, not a complete combat playtest.

Original launcher receipts incorrectly name the old default configure directory.
They remain unedited. The executable path was
`build/region-page-prototype/Release/HordeLanternRT.exe`; exact EXE/source hashes
and runtime shader manifests identify the private build. The updated launcher
records the selected executable's actual CMake cache/profile instead.

## Memory and timing

Device-local scene allocation is identical on both backends:

| State | Bytes |
| --- | ---: |
| Warm full | 372,042,608 |
| Detail retired | 359,589,168 |
| Restored | 372,042,608 |
| Transition peak | 376,298,736 |
| Peak above warm full | 4,256,128 (4.06 MiB) |

The managed page peak16,709,568B includes the normal detail owner; it must not be
added again to the scene peak. Shared textures/materials, coarse terrain and town,
collision, actors/held items and uncertain secondary-ray contributors stay
resident. CPU source remains cached. This proves independent GPU lifetime, not
package I/O or CPU-cache eviction. Production remains on its earlier combined
owner; this private candidate is not adopted by default.

Warm samples exclude only the predefined first5s. No slow sample is dropped.
Intervals are between successful present API returns, including host work and
waits. They are **not exact scanout FPS or GPU-only playable FPS**.

| Backend | Samples (all/warm) | Median ms | p95 ms | p99 ms | Warm max ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pipeline | 15184 / 14432 | 6.063 | 7.629 | 9.449 | 601.841 |
| QueryCompute | 15262 / 14523 | 6.064 | 7.710 | 9.492 | 13.559 |

Pipeline unload/restore occur at78.319/85.752s; the +/-1s windows have maxima
6.321/6.763ms. Query unload/restore occur at78.320/85.753s; maxima6.722/6.736ms.
Both return to the full allocation, finish with no simulation debt and pass the
unchanged route, rope/equipment and completed-frame identity checks.

Pipeline nevertheless has two early intervals of601.841ms at9.704s and484.069ms
at16.879s, before unloading. Maximum queued simulation debt reached32 ticks and
was paid by the end. Existing render/simulation timers explain only a small part
of these intervals. They do not establish the cause. Aggregate CPU load is
context, not attribution: Pipeline mean25.15%, maximum56.9%; Query mean21.0%,
maximum60.78%. Do not blame another process, shader compilation or presentation
from these data alone.

## Retained failure and bounded follow-up

The first approved Pipeline launch (EXE7541d1d10d0c7e708811dfc3acc7dd1f5288d55a2559825e324fc7bd07fa5bd7)
became unresponsive before route sampling. The unchanged180s process guard
terminated it. It produced no route evidence and is a **failed run**, not a
successful comparison. Its last marker is `directory-result=0`; the root cause
is unresolved. Both later runs reached checkpoint staging and first render and
completed. Success on retry does not prove that startup defect fixed.

Bootstrap markers and live raw stdout/stderr capture preserve progress before
another failure. Schema4 adds six monotonic host spans whose sum must exactly
equal each available present interval, plus the previous evidence-write subset.
It covers previous-frame tail, CPU-load polling, message dispatch, visibility,
clock/simulation and render-to-present work. Span ownership does not prove an OS
function caused a pause. Rewound clocks, wrong sums, stale/unknown fields and
invalid evidence subsets fail focused regression tests without deadline changes.

The host-phase follow-up EXE is
`15171b52831bbd7fe10a74009c7b4ca14cdd570d7b2c8ba303989595437b14b5`.
Debug and Release affected CTests each pass3/3. Its additional foreground run is pending owner
approval; these schema3 hardware timings do not validate those new executable
bytes. No renderer, shader, gameplay, quality or frozen budget changes here.

## Evidence and next gate

- [Receipt and source hashes](receipt.json).
- [Raw journals, exact measured sources and failed launch](release-r02-journals.zip).
- [Pipeline analysis](pipeline-analysis.json) and [Query analysis](query-analysis.json).
- [Affected host checks](host-phase-tests.log).
- [Debug checks](host-debug-tests.log). The retained
  [build-command failure](host-debug-build-attempt.log) requested the terrain
  CTest name as a nonexistent build target. It uses the successfully built rescue
  executable with `--authored-terrain`; the corrected three-test selection passed.
- Final restored ascent: [Pipeline](pipeline-final-ascent.png) and
  [Query](query-final-ascent.png). Both actual RT images were opened and inspected;
  restored trees, equipment and surface shadows are present. These moving frames
  are not a pixel-identical frozen comparison or complete chapter visual approval.

Next: one bounded host-phase Pipeline run, if authorised, to locate or retain the
unexplained gaps. If it does not reproduce them, keep their cause open. Then
validate private Android BDA/backend/lifecycle/route/memory behavior before
production adoption. Broader useful region coverage and package-provider/cache
lifetime remain engine gaps. No new physical-phone acceptance is claimed.

Audio/haptic manual revalidation required: NO. This checkpoint changes only the
measurement harness, logs and analysis; semantic audio/haptic inputs are unchanged.
