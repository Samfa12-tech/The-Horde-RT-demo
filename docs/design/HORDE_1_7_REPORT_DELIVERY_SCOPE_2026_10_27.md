# Approved1.7 report delivery and public statistics

Sam explicitly added this implementation scope on27October, after the current
bounded flask checkpoint. This is unfinished work, not acceptance inferred from
local review/export UI, proposed wire contracts or an earlier bug-report email.
No public website/backend mutation or test submission occurred for this record.

## Required outcomes

- Consented **private developer benchmark report** from Windows and Android,
  with actual submission, receipt and verified private destination evidence.
- Separately consented **public board submission**, publishing only approved
  sanitized fields. Private sharing must never silently publish to the board.
- A page consistent with existing routing, proposed
  `samfa12.com/games/the-horde/benchmark-stats`, linked from the Horde product
  page as **How does it perform?** Device model/hardware categories, filters,
  sorting, build/profile/settings comparability, counts and clear provisional
  status. Do not rank unlike/custom profiles together or imply verified hardware
  presentation from an unverified report.
- **Report compatibility to developer** on unsupported-device and startup/failure
  diagnostic screens, available even when RT renderer initialization fails.
  Bounded model/OS/GPU/driver, app/build, capability checks, failed stage and
  honest scene-presented status. Distinguish unsupported capabilities, startup
  failure, successful RT presentation and performance evidence.
- Preview plus explicit consent before sending; local export when transport is
  unavailable. Developer compatibility intake is private by default. Public
  compatibility summaries require appropriate consent, sanitization/aggregation
  and verified evidence classification.

Device tracking means category/model reporting, **not people/device tracking**.
No IMEI, serial, advertising ID, persistent fingerprint, unrelated raw logs,
personal data or client secrets. Deduplicate reports/retries without a persistent
device identifier. Keep private/public consent, approved-byte payload ownership,
explicit retry/cancellation and bounded abuse controls throughout the flow.

## Bounded implementation sequence

1. Inspect the existing website, compatible Cloudflare/reporting architecture,
   local benchmark review/export and proposed contract. Confirm exact permitted
   repositories, endpoints/configuration and current wire fields; do not invent
   a second service or assume old playtest delivery is benchmark delivery.
2. Implement shared sanitized report/receipt and consent boundaries first, with
   isolated transport/storage tests. Use report-scoped retry/dedup identities,
   not a device identity or hardware fingerprint. Separate private developer
   intake from explicit public submission and moderation/aggregation decisions.
3. Connect Windows/Android preview, send/receipt/failure/local export and the
   renderer-independent diagnostic entrypoints. Exercise unavailable/unsupported
   startup, successful presentation and failed/cancelled/retried submission.
4. Build the existing-site statistics page and Horde product-page link in a
   reviewable local/staging change. Compare only truthful compatible benchmark
   dimensions/settings/profiles/builds; label provisional/custom data honestly.
5. Demonstrate approved Windows/Android payload through receipt to the correct
   private/public destination, including deduplication and abuse/failure bounds.
   A successful build, mocked endpoint or bug-relay email is not end-to-end proof.
6. Return the exact reviewed deployment action and target for explicit approval
   before publishing. No merge, public deployment, new credentials/security
   settings, spending or owner release/signing action is authorized by this plan.

This scope does not certify any untested device (including the Pixel discussion
that prompted it). Preserve exact-build evidence classes in the compatibility
record and treat sustained phone performance as ongoing optimization, not a
release-FPS veto or reason to reduce the approved world/content scope.
