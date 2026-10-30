# Independent review and Android CI follow-up

Reviewed source: `b17a8fb4db4d39463ea16e85f243b5751c077d70`, against staged receiver
`1904c89f33941af8106d523b39bda502ad87de98`. This follow-up does not enable live
Send, change report schemas, deploy services or admit gameplay candidates.

## Findings and bounded corrections

1. Hosted Android job122821303407 in run40689773590 failed
   `BenchmarkSummaryUiTest.nativeFormStartsWithoutConsentAndRemainsAccessibleAtLargeFonts`
   at line160. Local reproduction matched:7 tests,1 failure. The failing assertion
   searched recursively for any Button text containing `send`; a new unchecked
   consent checkbox also inherits Button. The old no-Send-controls assumption
   predates the approved unavailable Send review. It was not a failed48dp target.
   The replacement directly checks unchecked remote consent, disabled production
   Send/review, unavailable transport, no prepared remote bytes, and no state
   change from a direct stale click. All original large-font/target checks remain;
   the three new controls also require48dp-equivalent measured height.
2. Independent read-only review found one product defect: Android pause replaced
   already verified pending/terminal receipts with unknown acceptance. It also
   dropped a receipt's UI notification if the owner finished just before pause.
   Two regression tests demonstrated both failures before the correction.
   Refresh/resume now render the authoritative submission state, retaining known
   pending/published/private-provider receipts and genuine uncertainty. Only
   revoked active verification/request work gets the interrupted message. Stale
   callbacks remain invalidated; no automatic retry is introduced.
3. MSVC job122821303650 completed its native test roster;166/167 passed. The
   failure was the Windows summary-review source contract, not a timeout.
   A local reproduction matched its UTF-8/size-bound assertion. Local preview
   conversion now retains its16KiB default, while only an explicit Send preview
   opts into20KiB. Native boundary tests cover both limits and invalid UTF-8.
   The old broad ban on any `WindowsPlaytestSubmission` text is replaced by
   explicit typed/admission/reset checks and bans on direct raw transport calls;
   the enum receipt type in the new handler is legitimate. Other assertions and
   the hosted timeouts/coverage remain unchanged.

The independent reviewer checked this correction and found no additional
actionable defect in the bounded receipt/consent/cancellation/retry review.
This is source review, not a substitute for tests or real service/device evidence.
Original failed CI/local receipts stay under ignored
`reports/statistics-send-review-20261028/`; concise outcomes and hashes are in
`review-receipt.json`.

## Windows and actual acceptance boundaries

The five Windows reporting contracts atb17a8fb4 remain baseline evidence. This
follow-up rebuilds the changed summary form and its actual native fixture, adding
the source-contract and preview-limit checks. The normal Release application is
compiled; it is not launched.
The existing isolated actual-form fixture covers synthetic Copy/Save destinations,
receipt handling, cancellation, scrolling and audience/consent reset without
acquiring the owner's game/controller/clipboard.

Real clipboard/picker and modal/physical input/DPI transitions cannot be closed
by repeating this fixture. The held game-launch/Computer Use route is not retried
or replaced here. No phone or live service was used. Full Android host/package
checks and replacement CI are recorded separately; host success is not physical
RT or natural campaign acceptance. The complete local Android run registered529
tests:523 passed,0 failed,6 host-link capability skips in the unchanged
EnvironmentSurveyExportTest. Those skipped Windows symlink cases are not passes.
Lint passed, and Debug APK assembly includes all four supported ABIs. The APK is
a working-tree build, not an installed or frozen playtest candidate.

See the [frozen PR27 gate assessment](../../design/HORDE_1_7_PR27_MERGE_GATES_2026_10_28.md)
for source merge versus1.7 publication requirements. No conflict was resolved and
no base merge, PR-body write, release or deployment was performed.

The replacement e8ebe380 CI closed Android but hit the MSVC15-minute job cap.
The [subsequent test-only build correction](transport-build.md) records the exact
timeout and one bounded removal of duplicate transport compilation. Test coverage
and timeouts remain unchanged; hosted acceptance is not inferred from local passes.
