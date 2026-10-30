# Authorized scheduling retry, 23 October 2026

One owner-approved host-stdin retry used the independently existing **unchanged**
config: SHA256 f68b8e88,50seconds,64MiB, same scheduling/frequency/process sources.
`adb shell -T perfetto --txt -c - -o -` follows the documented
[Android interface](https://perfetto.dev/docs/learning-more/android) and
[CLI](https://perfetto.dev/docs/reference/perfetto-cli#normal-mode). No denied
config path was read, no permissions/security settings changed, no extra retry.

SM-S948B Android17/API37; fixed normal-cache APK c1d65749/native f73400d0,
runtime bceed0c1. Repository head249ec107's copy candidate was **not installed**.
Both completed holds have identical actual settings/pipelines: RT Pipeline,
Shipping/Mobile,33%,475×984 dispatch,2981×1440 native view, rotated1440×2981
swapchain, Water1/Fire0/cap30/GlassOff/Current shadows/MistOff/Dust1. Timed audio
is muted. This is seeded post-Keeper stationary normal simulation, not a journey.

## Actual scheduling evidence and its limits

The recovered derivative has1,735 fully consumed root protobuf packets. Pinned
Perfetto v58.2 loads it without reported nonzero loss/error counters. Two
MONOTONIC snapshots agree on BOOTTIME offset308025711123551ns; the post-warm
30seconds fit wholly inside the trace. Native motion-owner logging identifies
render TID13775, distinct from Android's UI thread named RenderThread. State
coverage is exactly30seconds; Running time equals the independent sched sum.

| Native render-owner state | Seconds | Share of this window |
| --- | ---: | ---: |
| On-core | 8.226177 | 27.4206% |
| Runnable, including preempted | 0.088909 | 0.2964% |
| Sleeping S | 21.650147 | 72.1672% |
| Uninterruptible D | 0.034767 | 0.1159% |

These are event-based observations, not stage fractions or whole-phone CPU use.
About78.5% of this owner's on-core time is on CPU indices4/5. Frequency/idle
events exist for all eight indices; raw frequency ranges and per-core observed
busy time are in [scheduling-summary.json](scheduling-summary.json). They do not
certify CPU architecture class, GPU clocks or a causal clock explanation.
Music/ambience threads use0.274/0.244seconds in this window despite suspended
timed playback; ordinary mix cost remains a separate question.

Runnable delay is small here. Combined with the source's serial owner path and
39.93ms median elapsed fence wait, this makes GPU dependency the next attribution
priority. That is an inference: no call stacks/stage markers identify each sleep
as Vulkan wait, or each CPU interval as skinning. GPU AS/trace/transfer splitting
and the production-bank probe prerequisite remain open. No worker, affinity or
pre-fence change was made.

The raw12,246,538-byte stream does not parse: all90,297 LF bytes have CR prefixes.
A separate12,156,241-byte CRLF-to-LF derivative parses completely; reapplying
LF-to-CRLF reproduces the raw bytes exactly. This supports apparent Windows
shell-stdout text conversion. Raw bytes are untouched and both hashes retained;
there is no on-device trace hash. Treat this as a **recovered qualitative
profile**, not an unqualified byte-verified original. CLI stdout accounting said
zero bytes; actual captured bytes and structural validation establish the size.

## Warm observations remain useful

| Completed hold | Post-warm frames | GPU median / p95, ms | Skin median, ms | Frame cycle median, ms |
| --- | ---: | ---: | ---: | ---: |
| Opening control | 620 | 35.859 / 36.765 | 7.939 | 47.780 |
| Traced hold | 543 | 39.598 / 40.654 | 11.110 | 55.737 |

Both are retained observed-device evidence. GPU durations exceed the
16.667/20.000/33.333ms bands. Frame cycle rises16.65%, a material unresolved
observation. The APK is identical: this is not a code-regression comparison.
Tracing differs, and context warms; neither cause is isolated.

| Current-HAL starting state | AP, °C | Battery, °C | Skin, °C |
| --- | ---: | ---: | ---: |
| Opening control | 39.6 | 29.8 | 34.2 |
| Traced hold | 39.3 | 30.5 | 34.4 |
| Rejected closing start | 41.4 | 32.1 | 35.9 |

The closing start exceeds the unchanged1.5°C comparison gate by deltas
1.8/2.3/1.7°C from opening. It was rejected **before sampling**. All framework
and sensor statuses were0; these readings alone do not prove throttling or
overheating. Traced end is39.1/31.8/35.4°C. Without matched closing control,
tracing overhead and temperature/clock/background effects cannot be separated.
There is no admitted causal gain or thermal-only explanation.

Warm playability is a concern and separate acceptance gap. Future planning should
record/hold power, charging, brightness, settings, scene and build; alternate
control/candidate order; and predeclare a separately bounded sustained warm-play
test. Keep warm observations when a causal-comparison gate fails. This checkpoint
starts no further test and changes no threshold.

## Custody, replay and checkpoint

Released **07:04:48 UTC**: both exact starting APKs, all18 prior save/lock files,
preferences, rotation and font verified unchanged. Zero new save paths, launcher
foreground, no Debug PID/Horde surface, active trace or owned install/transfer.
Safe to disconnect. [Receipt](receipt.json) binds the build, settings, child
timings, thermal rejection and custody. Private raw files remain in ignored
`reports/phone-trace-stdin-20261023/`; traces and owner inventories are not committed.

Offline replay: `python analyze_scheduling.py <raw-report-directory>
<pinned-trace-processor-exe>`. The script emits its SQL/results and checks clocks,
span, loss counters and state/scheduler coverage. [Unchanged config](scheduler.pbtxt).
Replay and packet/roundtrip checks pass. No runtime changes or game builds needed.

Exact source249ec107 CI run39913009629 ends cancelled: Android, Vulkan-host,
GCC,Clang and ASan/UBSan pass; MSVC cancels in the Windows selection at its
unchanged15-minute cap, later offline steps skipped. Full hosted acceptance,
PR conflicts, Debug route deadline, phone copy comparison and physical/audio/
sustained-play requirements remain open.

Audio/haptic manual revalidation required: **NO** for this evidence-only change.
