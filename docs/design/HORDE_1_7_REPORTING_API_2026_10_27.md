# Private reports and public benchmark statistics — staged contract

This is an implementation/handoff contract, not a deployment or delivery receipt.
The client source checkpoint is recorded in the linked [evidence](../evidence/2026-10-27-developer-reports/README.md).
No report was sent and no service, binding, credential or website was changed.

## Existing system and safe staging boundary

Read-only inspection on27October found the existing private relay in Briarhold's
`hosting/signaling`: `src/index.js`, `src/horde-report.js`, its verification page,
tests and Wrangler configuration. The inspected root was `6ac0219fe4af37511404261f04ed609f01804a31`
on the concurrent Reef-report branch. The current Horde handler accepts only
schema1 bug/playtest reports, has strict field allowlists, bounded parsing,
one-time verification, token-free payload hashing, fixed developer email routing
and existing durable storage/rate-limit machinery. These are reusable seams;
their existence is not benchmark/compatibility admission or live-service proof.
No private backend source or configuration is copied into this public repository.

The existing website is GitHub Pages with managed content sources and generated
segments. Its checkout is concurrent Reef work. Read-only Git status failed with
`fatal: cannot come back to cwd: Permission denied`; complete concurrent-state
verification and safe staging are therefore held. No website write, new worktree,
main-branch push or permission workaround was attempted. Once access is resolved,
follow that repo's AGENTS.md, inspect the then-current branch/state, and stage an
isolated change from the appropriate base. Do not hand-edit generated content.

## Private envelope implemented in Horde

`PreparedDeveloperReportSubmission` admits only a prepared benchmark summary or
typed compatibility report plus **separate explicit private-send consent**.
Android mirrors this admission through its native-approved review owners.

- Local report: schema2/product `horde-lantern-rt`/kind `benchmark-summary` or
  `compatibility`/`consentToPrepare:true`/typed report schema1.
- Exact local bytes remain the Copy/Save authority. Private submission adds only
  `consentToSubmit:true` and `delivery:{destination:"developer-private",publishToBoard:false}`.
- Maximum final request20KiB; compatibility local8KiB, benchmark local16KiB.
  Fresh verification is printable ASCII,1–2048 characters, escaped and admitted
  inside the final request cap. It never enters local export/frozen approval.
- UUIDv4 is random per report, not per device. Explicit retry retains ID and
  exact reviewed bytes; a new/edited report requires fresh review/consent/ID.
- No screenshot, note, raw log, path, save, arbitrary diagnostic JSON or secret
  enters either new kind. Private consent cannot grant public-board consent.

The existing `/api/horde-reports` may admit these two schema2 kinds after a
reviewed backend change; do not send them to its current schema1-only handler.
Route by a bounded product/schema/kind discriminator **before** invoking the
kind-specific strict allowlist. Keep existing schema1 behaviour unchanged.

### Kind/destination-bound receipt

New-kind success is exactly five unique flat fields, with no unknown/duplicate
keys, bounded to8KiB:

```json
{"ok":true,"id":"<matching-report-uuid>","status":"accepted","destination":"developer-private","reportKind":"compatibility"}
```

HTTP202 +`accepted` means queued, not delivered. HTTP200 +`sent` means the relay
completed its provider handoff, not independently verified mailbox delivery.
The client requires the matching ID, destination and kind. Legacy three-field,
wrong-destination/kind, malformed or missing receipts leave delivery uncertain.
403 requires fresh verification;429 explicit later retry;409 same-ID/content
conflict;5xx/timeouts/cancellation are potentially accepted and need an explicit
same-report retry. No automatic client retry or redirect. Cancellation suppresses
stale callbacks; it cannot retract an already accepted report.

Backend storage key: product + private destination + report UUID. Compare the
validated token-free canonical payload digest, including report kind and consent
destination; fresh tokens never change deduplication. Same key/content returns
current receipt without another email; conflicting content returns409 without
replacement. A crash between provider send and durable completion still requires
the existing delivery/idempotency strategy to be proved, not an exactly-once
claim from an in-memory mock. Retention/deletion behaviour must be documented
against the actual backend before deployment, without retaining device IDs or
raw IPs as report identifiers. Reuse existing fixed recipient/provider/bounded
abuse controls; do not create a second relay or client-side secret.

## Renderer-independent compatibility evidence

The typed snapshot owns app/version/build/platform, optional model/OS/GPU,
lossless raw numeric driver/API versions, explicit probe availability, five
extension/four feature checks, last typed stage/outcome, selected backend and
actual `rtPresented`. Checks are `unknown`, `absent` or `present`; default false
is not evidence that a query ran. Missing hardware remains null/unknown.

The builder rejects inconsistent presentation and unsupported claims. Successful
backend selection is not presentation. Unsupported capability, probe unavailable,
startup failure, capability-only and actual RT presentation stay separate. There
is no FPS in a compatibility report. No serial, IMEI, Android/advertising ID,
hardware/pipeline UUID, hostname, numeric vendor/device identity or fingerprint.
No arbitrary driverInfo/exception/log collection. Platform revisions/surface
generations stay internal and reject stale preparation; exported bytes own no
renderer pointer. Native-library failure must show an honest unavailable/Close
state rather than a fabricated GPU report.

## Separately consented public results — API/data model handoff

28October adds `PreparePublicBenchmarkReport`: a separately consented LOCAL
sanitized public preview from typed frozen benchmark evidence. It cannot take a
private report/JSON body and cannot send or authorize publication. Private reports
must never be copied wholesale to public storage. A separate public-consent
control initially unchecked and a fresh public result UUID are still required in
the platform review owner; private-send consent does not grant public consent.
The builder rejects reuse of the measured run UUID. Suggested
route in the same existing Worker: `POST /api/horde-benchmark-results`;
`GET /api/horde-benchmark-stats` is public read-only. Private compatibility intake
does not authorize publication of models or aggregation of its private records.

Public allowlist should contain only:

| Group | Data |
| --- | --- |
| Classification | Community/client-reported, validation state, provisional/custom-unranked, successful RT presentation claim separately from any independently verified device record |
| Hardware category | Explicitly reviewed model/GPU category, platform/OS major, optional reviewed driver version; null where omitted |
| Comparability | App/build, course/scene version, profile ID/revision, exact shader pair, actual submitted internal/output dimensions, aspect/output policy, DRS state and all quality settings |
| Measurement | Completed eligible sample count, CPU/frame and GPU aggregate distributions with their actual scopes/availability; optional declared cooling distinguished from measured thermal data |
| Public result metadata | Opaque result key scoped to this report, moderation/classification, coarse date and aggregate counts; no private report body, full timestamp, client IP, name or persistent device ID |

An unverified client report cannot certify a device merely because the server
accepted JSON. A compatibility summary must distinguish unsupported reports,
startup failures and demonstrated RT presentation, with consent and evidence
classification preserved. Do not rank unlike builds/profiles/settings together.
Current completed summaries remain **custom-unranked**; no standard profile is
promoted by submission. Provisional standard profiles need their actual platform
acceptance. Legacy requested dimensions must not replace unavailable actual ones.

GET should accept bounded allowlisted filters (platform, model/GPU category,
build, profile/revision, exact comparability bucket and evidence class), an
allowlisted sort field/order, limit<=100 and an opaque page cursor. Return schema
version, bucket definitions, eligible/provisional counts, sanitized rows, missing
data flags and next cursor. Do not send private records and hide them with CSS.
Cap query work and response size; public static caches contain only approved
projection data. Empty state says no results, not synthetic scores. Demo fixtures
stay local and visibly labelled, never production rows.

Website route: `/games/the-horde/benchmark-stats/`, same brand/navigation/metadata
and responsive table/card behaviour; Horde page link **How does it perform?**.
Use the existing managed content/routing sources and their generators. Display
counts, provenance/limitations, clear custom/provisional labels, filters/sort,
loading/empty/error states and keyboard/touch access. GitHub Pages cannot host a
private intake secret; read only the reviewed public API without credentials.
Any new public route/CORS/storage/deployment configuration is a separate reviewed
deployment action, not authorized by this document.

### Current local public projection

The local preview has schema1/product/reportKind `public-benchmark-result`,
`destination:public-board`, `consentToPreparePublicPreview:true` and one `result`.
This consent is local review only, not `consentToSubmit` or publication consent.
No platform button, public transport, server acceptance or website is enabled.

Result allowlist: random resultId, coarse UTC date, community-client-reported
classification, explicit false verification/ranking, workload/simulation policy,
shared measured configuration, warmup/measured laps, true owning-frame RT
presentation and false scanout-FPS claim, timing scopes, unknown measured thermal
state, separate declared cooling, aggregate overall/zones and optional reviewed
model/GPU/Vulkan labels. OS-major/driver, course/profile revision, output policy
and package-artifact hash remain null because this immutable source does not yet
carry them. Exact comparison eligibility remains false; do not infer standards
from matching dimensions. Missing optional actual quality/dimension evidence is
not synthesized. No raw rows, run/private report ID, full capture timestamp,
private note/log/path, token, personal/device ID or source-owner pointers enter.

The existing configuration writer is shared by private and public builders;
private fixture bytes remain identical. Current public data are custom/unranked,
not accepted or ranked board rows. Client syntax/privacy/completion checks are
not server validation, authentication, verified hardware or moderation evidence.
See the [bounded projection receipt](../evidence/2026-10-28-public-benchmark-preview/README.md).

## Remaining implementation and admission

1. Complete platform UI/collector wiring and isolated lifecycle/export tests,
   preserving startup/renderer-independent ownership. Inspect native/physical UI
   with permitted tooling; do not bypass held game/CUA or device restrictions.
2. Implement strict server validators, private receipts/dedup/provider tests in
   the canonical private relay worktree after safe staging; test with mocks only.
3. Integrate the local sanitized projection with explicit platform public
   review/consent, then implement server storage/query; stage the existing-site page and managed product link safely.
4. Review exact code/config, secrets already available versus missing, retention,
   abuse controls and rollback. Return the precise deployment target/diff and
   action for approval. No production send follows from a successful build.
5. After deployment and explicit test-send authorization, prove Windows and
   Android submission→matching receipt→correct private destination; separately
   prove explicitly public submission→sanitized stats query/page and dedup. A
   bug-report email, local mock, provider receipt or CI result cannot substitute
   for this evidence. Leave all unproved outcomes open on the release checklist.
