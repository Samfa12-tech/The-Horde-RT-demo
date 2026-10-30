# Bounded host-phase Pipeline follow-up

The approved additional optimized Release Pipeline route completed96.4627s,
15069 presented samples and5787 shared60Hz ticks. It automatically unloaded and
restored the detail page once, with zero readiness waits and zero simulation
backlog. The earlier602/484ms gaps **did not reproduce**. Their cause, and the
original startup hang, remain unresolved; this run is not a fix for them.

The first attempt here (r03) lost foreground after2.7s and exited with a failed
capture. It is rejected for pacing. The owner confirmed readiness before r04;
the entire accepted run remained foreground and uncovered. The game closed
afterward. No additional foreground run is requested by this checkpoint.

## Exact identity and unchanged workload

Measured source: `c80df058d20635ebc4c88dcdbd972bbf2877d246`.
EXE SHA-256:
`15171b52831bbd7fe10a74009c7b4ca14cdd570d7b2c8ba303989595437b14b5`.
The analysis-only correction below does not change these executable bytes.
The archived launcher receipt records the actual CMake profile and each measured
source hash, verified before archiving.

Optimized Windows Release CPU (`/O2 /Ob2 /DNDEBUG`), private r10 Shipping/High
shaders; NVIDIA GeForce RTX5050 Laptop GPU.960x540 at100%, Current shadows,
High water/fire, Dust Off. Same out/back, descent, coloured-passage backtrack and
second-ascent route as the preceding both-backend comparison. No render scale,
sample count, shader, budget, gameplay or production adoption change.

## Result

Warm samples exclude the predefined first5s only (14337 samples). These are wall
intervals between actual successful present API returns, **not exact scanout FPS**.

| Metric | Milliseconds |
| --- | ---: |
| Warm median | 6.0623 |
| Warm p95 / p99 | 7.6883 / 8.4220 |
| Warm maximum | 13.9391 |
| Whole-run maximum, including warmup | 16.2483 |
| Unload +/-1s window maximum | 6.4697 |
| Reload/adoption +/-1s window maximum | 7.2603 |

The largest warm interval is at29.3023s, with13.5607ms in render-to-present work;
all six host spans sum exactly to13.9391ms. Warm maxima in the other spans are
previous-tail2.0676ms (previous evidence-write subset2.0643ms), system-load
poll0.8823ms, messages0.5989ms, visibility0.2766ms and clock/simulation5.0809ms.
These maxima occur on different frames and must not be added as one frame.
All15069 rows pass exact partition/subset/availability checks. The counters can
locate a host span if a large gap recurs, but cannot attribute the earlier gaps
without a reproduction. Aggregate CPU mean15.21%, maximum50.72% is context, not
proof that other applications caused or resolved either earlier failure.

Full/reduced/restored device-local scene allocation is again
372,042,608 /359,589,168 /372,042,608B: **12,453,440B (11.88MiB) saved**.
Peak376,298,736B is4,256,128B (4.06MiB) above full. Host-visible peak48,123,248B.
The same certified84,219-triangle detailed forest region unloads. Coarse world,
landmarks, shared materials/textures, collision, actors/held equipment and
uncertain secondary-ray contributors remain resident; CPU sources stay cached.
This does not establish package I/O, cache eviction or broad world coverage.

## Analysis regression and retained evidence

The first analysis of the successful run rejected the existing `host_visible_bytes`
allocation column as an unknown host timing field. Synthetic tests had omitted
that column. The fix explicitly distinguishes this one established allocation
field, validates its nonnegative value, and retains exact timing sums, known
timing roster, unknown-field rejection and all existing assertions. A fixture
preserves the exact captured CSV header plus first two rows (unavailable/available
intervals); focused CTest passes1/1. No game rebuild or repeated hardware run was
needed for this analyzer-only correction.

- [Verified analysis](analysis.json), [receipt/source hashes](receipt.json),
  [focused CTest](ctest.log).
- [Raw r03/r04 journals, exact measured sources and original analyzer failure](release-host-r04-journals.zip).
  The rejected attempt and original failure are preserved. A `unittest` discovery
  command ran zero tests because the fixture is a direct CTest assertion script;
  it is not counted as a test pass. Correct CTest execution is recorded separately.
- [Actual final repeated-ascent image](final-repeat-ascent.png), opened and
  inspected: restored trees, surface shadows and held lantern remain visible.
  This is a moving frame, not frozen pixel parity or complete chapter art approval.

The preceding c80df058 has all six exact-commit CI jobs passing. The current
analyzer regression follows through its own exact-commit checks in PR27; do not
transfer predecessor CI to a newer source SHA. Production remains on the earlier
combined owner. Physical Android BDA/backend/lifecycle/route/memory/thermal and
owner acceptance, package-provider/cache lifetime and wider useful region
coverage remain open. No merge or release.

Audio/haptic manual revalidation required: NO; only offline analysis/tests and
evidence change, with no semantic playback or feedback change.
