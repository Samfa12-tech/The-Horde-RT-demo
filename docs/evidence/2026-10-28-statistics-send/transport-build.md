# Bounded Windows fixture build correction

Source baseline `e8ebe3807c47781f8a8a316c14ad19bdde6dbc17`, CI40692998073:
five jobs passed. Android reported529 tests with0 failures/errors/skips and passed
package checks. MSVC was cancelled; its annotation explicitly says the job
exceeded15m0s. It reached120/167 selected tests without a reported test failure;
the native summary interaction fixture passed. This is not a complete Windows
suite pass or a phone-performance result.

One avoidable cost was verified in that log and the generated projects:
`WindowsPlaytestSubmission.cpp` compiled separately in four reporting test
executables. They use matching C++20, Release runtime, compiler definitions and
source settings. The remote UI fixture additionally has a WebView2 external
include path, which the transport source does not use. No per-source override or
target-specific transport behavior was found.

This checkpoint builds that translation unit once in a Windows-only static
**test** target, linked by the same four executables. Each executable still owns
its separate process-local worker. Production and the explicit acceptance tool
retain their original source lists. No workflow, runner, permission, paid service,
test registration, assertion, command or timeout is changed.

Local verification:

- Four consumers have zero direct transport compile entries; the shared target
  has one. The Release compile contract was checked against the prior projects.
- All250 local CTest names, commands and properties, including timeouts, match
  the prior roster exactly.
- All six affected reporting/source tests pass in7.28s. This is local test
  execution time, not hosted CI build savings.
- Independent read-only review found no dropped coverage or production impact.

See [`transport-build-receipt.json`](transport-build-receipt.json). Raw project,
roster, build and test receipts remain in ignored reports. Only this bounded
candidate is attempted; if fresh CI still lacks headroom, report that result
instead of raising the cap, dropping checks or repeatedly rerunning the same job.
The [frozen merge gates](../../design/HORDE_1_7_PR27_MERGE_GATES_2026_10_28.md) and
physical/live-service gaps remain open. No game launch, phone, merge or deployment
is part of this correction.
