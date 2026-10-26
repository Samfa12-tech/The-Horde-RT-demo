# Benchmark-to-developer status: local only, remote not admitted

Read-only answer against source `51d6f0102d85b83216f3cad2558adbc4406588f8`.
No live submission, inbox query, service/configuration change or phone action
was made for this check. Current service health/deployment was not reverified.

| Layer | Benchmark statistics | Existing playtest/bug reports |
| --- | --- | --- |
| UI | Windows and Android completed-run review, unchecked preparation/hardware choices, exact JSON, Copy and Save exist. Windows explicitly says nothing is sent; Android discloses statistics sending unavailable. | Native foreground report form and verification/send route exist. |
| Serialization / consent | Typed frozen summary/local report and separate explicit remote-consent wrapper exist, with bounded bytes and token-free local data. This is offline preparation, not transport admission. | Frozen report/context/image choices, consent and bounded transport exist. |
| Client sending | **Unavailable.** Android `BENCHMARK_SUMMARY_LIVE_SEND_ENABLED=false` rejects before queuing/opening transport. Windows summary review has Copy/Save/Close, no remote Send action. | Fixed Horde endpoint `https://briarhold-signal.samfa12.com/api/horde-reports` and verification page are configured in client source. |
| Backend acceptance | Compatible deployed benchmark-summary schema/intake is explicitly unproved; no current acceptance receipt exists here. | Prior reviewed private relay deployment and accepted native reports are documented. |
| Delivered email | **No benchmark email end-to-end evidence.** | One approved Windows fixture and one Android fixture were independently confirmed in the owner inbox in October2 evidence. These were playtest reports. |

## Source pointers and build distinction

- `src/platform/windows/WindowsBenchmarkSummaryReview.inl`:
  `CreateWindowsSummaryReviewControls` presents local preparation and Copy/Save;
  it contains no benchmark network action. `WindowsBenchmarkSummaryInteractive`
  excludes unattended/capture modes, not a Debug-only sending switch.
- `src/reporting/BenchmarkSummarySubmission.h` and `.cpp`: reviewed immutable
  offline wrapper/request construction; comments explicitly require compatible
  deployment before dispatch. Construction alone does not confer a network path.
- `android/app/src/main/java/com/samfa12/hordelanternrt/PlaytestReportSubmission.java`:
  the benchmark gate is an unconditional private constant `false`, with an early
  return in `submit`; no BuildConfig/debug/release override enables it.
- `MainActivity::prepareBenchmarkSummary` and summary strings offer review,
  Copy/Save/edit, not a live benchmark send flow. The source is common to Debug
  and Release. This does not claim a fresh Shipping artifact or physical UI run.
- `BenchmarkSummaryReviewTest::benchmarkNetworkGateRejectsEveryAttemptAndCancelInvalidatesOfflineOwner`
  verifies zero queued work, transport opens or callbacks even for typed
  preparation/fresh-looking tokens and production transport. This was inspected,
  not rerun or replaced with a live test.

The separate playtest route is implemented using Windows WinHTTP and Android
HTTPS, with fixed verification origins. Its [relay integration](../2026-10-02-report-relay/README.md)
records private Worker deployment `1ca58068-e876-4016-be6f-b1ac4d2035ed` and
bounded backend tests. The [Android receipt](../2026-10-02-s26-report-send/delivery-receipt.json)
identifies source `00af842e`, a1.6.1 Debug APK, one actual verify/send,202/QUEUED,
and one independently confirmed matching inbox message/PNG. The
[Windows UI/delivery record](../2026-10-02-windows-report-ui/README.md) retains its
separate evidence boundaries. Neither constitutes current benchmark backend or
email acceptance, nor proves today's service configuration unchanged.

## Exact remaining benchmark work

Review/admit the compatible private backend schema and configuration; wire an
explicit foreground verification/send/cancel/retry action on both clients;
preserve local JSON, separate consent, bounded data and same-report identity;
test backend rejection/acceptance plus lifecycle; then seek specific approval
for one labelled end-to-end benchmark email and independently verify delivery.
202 acceptance means queued, not delivered. Earlier single-email permissions
were consumed and are not authority for another send. No public leaderboard or
automatic hardware reporting is implemented or authorized by this status answer.
