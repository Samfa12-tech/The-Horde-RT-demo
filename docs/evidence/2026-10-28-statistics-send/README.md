# Native statistics Send: local contract checkpoint

This is the original `b17a8fb4` checkpoint receipt. The subsequent
[independent review and CI follow-up](review-followup.md) records its Android
form-test correction and receipt-status lifecycle fix; the original hashes and
results below remain evidence for their original source.

This checkpoint continues Horde source `9d84eaac9d1b123ec8a6f2d4d16fd63f9a11af53`.
Its source and tests are in the same commit as this receipt. The staged receiver
is private relay source `1904c89f33941af8106d523b39bda502ad87de98`.
No backend source, configuration, credentials or private reports are copied here.
These are synthetic/local results, not deployed delivery or public scores.

## Implemented

- C++ public submission accepts only a typed sanitized public preview plus a
  separate publication decision. Private developer sharing cannot grant it.
- Windows benchmark and renderer-independent compatibility forms and Android
  counterparts have a separate Send review/consent owner. Token-free submission
  previews are independent of local Copy/Save; remote consent is never added to
  those local exports. Tokens exist only in an attempt.
- Public attempts use a distinct injected route, verification purpose and strict
  five-field receipt. HTTP202/accepted means pending, **not published**.
  HTTP200/published must match result identity, destination and kind. Private
  accepted/sent semantics and schema1 reporting stay separate.
- Explicit retries preserve approved ID/content and require fresh verification.
  Pending public receipt checks are explicit; there is no polling send loop.
  Cancellation/lifecycle interruption invalidates callbacks and cannot promise
  remote non-acceptance. Bad/missing/conflicting receipts never claim success.
- Default Windows services are empty and Android HTTPS admits neither new report
  route. All injected UI receipts say **Local test**. Live sending remains off.
- A demonstrated Android reader defect is corrected: Java's `Character.digit`
  admitted non-ASCII hex characters inside JSON Unicode escapes. A malformed
  receipt could otherwise match a successful identity. The reader now accepts
  ASCII hex only; the original failing regression receipt is retained privately.

## Checked locally

The focused Windows tests exercise actual form controls and procedures, with
synthetic data and injected destinations/verification. The fixture never opens
the RT game, acquires controller input, or uses the owner's clipboard/files.
It checks separate consent/preview, exact exports, same-ID retry, fresh tokens,
wrong ID/destination/kind/status, duplicate/unknown/truncated receipts, 503,
cancel/late success, pending versus published, audience reset, and Close cleanup.
The existing schema1 remote form/submission checks remain included.

Android checks use Robolectric API34, including portrait/landscape resource
configurations. These are host behavior checks, not physical touch, RT presentation
or full MainActivity startup acceptance. Java compilation, lint and a single
arm64 Debug native build are included; no new APK was packaged or installed.

Actual C++ fixture requests were also read by the staged receiver's strict local
validators. Both private and public benchmark requests passed within the20KiB
limit. No HTTP request was made. Sizes, hashes and check outcomes are in
[`receipt.json`](receipt.json). The representative own-window image is synthetic
and explicitly labels its receipt; it is not gameplay or a real public result.

## Remaining acceptance

| Boundary | Required next evidence |
| --- | --- |
| Receiver/operator at1904c89 | Reviewed moderation access/workflow; retention/full-shard/tombstone policy; uncertain-provider-send recovery; production CPU/memory/traffic measurement |
| Native source in this checkpoint | Approved real verifier/transport wiring after compatible deployment; preserve the purpose/receipt/consent gates tested here |
| Website | Independent authorized website task must verify its exact page/API integration and build. This worker did not reopen the denied website checkout |
| Windows interaction | Real clipboard/picker/modal owner, physical keyboard/controller, DPI transitions and failure/retry feedback; game remains unlaunched in this slice |
| Android interaction | Allocated physical device; exact installed build, both orientations, background/resume/cancel, no-RT diagnostics, local export and consent controls |
| End to end | Separately authorized Windows/Android private benchmark and compatibility submissions, exact receipts/provider/inbox evidence; explicitly approved public submission/moderation/GET visibility and deduplication |
| Publication | Separate reviewed deployment, merge and release authorization; no such action is included here |

Private relay hosted run40685884210 at1904c89 did not execute either job because
of an account-level GitHub Actions policy restriction. The exact annotation is
retained in the private owner handoff. Local105/105 receiver
tests, dependency audit and dry-run evidence remain source/local evidence. No
workflow retry, billing/limit change or alternate public CI route was attempted.

All151 parked owner files are preserved. Phone use, gameplay feature admission,
renderer capacities, quality, worker defaults and performance claims are unchanged.
Sustained phone optimisation remains separate from reporting correctness and is
not an indefinite release veto.
