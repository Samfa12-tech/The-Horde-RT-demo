# Reporting completion: exact next interfaces and access boundary

**Later source checkpoint:** the [native Send receipt](../evidence/2026-10-28-statistics-send/README.md)
supersedes the implementation-status statements below. Private backend source
`1904c89f33941af8106d523b39bda502ad87de98` implements staged private intake and
public catalog/moderation/read contracts. Windows/Android now connect typed review
to local/mock Send and strict private/public receipts, with live transport still
unavailable. The website progresses in a separate authorized task; this worker
has not reopened its denied checkout. The original proposal below remains
historical context, not a claim that those source paths are still absent or that
any service has been deployed. See the linked remaining acceptance table.

This is a source/staging handoff, not deployed service configuration or proof of
delivery. Base02ece650 follows the all-six-green e2addc77 runtime checkpoint.
Current source adds Windows local public review alongside Android's existing
local public review. No phone, native game/form, backend or website operation was
used in this slice. [Current wire contract](HORDE_1_7_REPORTING_API_2026_10_27.md)
and [approved scope](HORDE_1_7_REPORT_DELIVERY_SCOPE_2026_10_27.md) still apply.

## Current, unfinished and required

| Boundary | Current source | Still required |
| --- | --- | --- |
| Private benchmark/compatibility preparation | Typed bounded reports; separate local review/export and private-send approval; mock destination-bound receipts; renderer-independent startup/failure preparation | Actual compatible service, durable idempotent intake and verified Windows/Android delivery |
| Public benchmark preparation | Typed sanitized aggregate projection; Android review/Copy/Save; Windows local audience switch/review/Copy/Save | Actual platform UI/export acceptance; explicit publication owner, public transport/receipts, moderation and website |
| Evidence classification | Unsupported/unavailable, failed startup and RT-produced presentation remain separate; aggregate scopes/unknowns are explicit | Real exact-artifact E2E tests; missing course/profile revision, artifact hash, OS/driver/output policy must not be invented |
| Website | Route/data/filter/sort proposal only | Permitted source staging, real public API, responsive page, product link, deployment approval |

Private and public consent must stay separate at every boundary. Local public
preview consent is NOT publication consent. Copy/Save only exports the exact
reviewed bytes. A report-scoped random UUID is not a device identifier. Current
public data are community/client-reported, unverified and custom/unranked. Neither
JSON acceptance nor successful client RT claims certify a device or ranked score.
Public counts mean reports, not unique devices or people.

## Private service change to stage in the existing relay

Last inspected source was Briarhold `hosting/signaling/src/horde-report.js` and
`src/index.js` at6ac0219fe4af37511404261f04ed609f01804a31, accepting schema1
bug/playtest reports. This historical inspection is not a fresh live-service
claim. Recheck the then-current authorized repository and concurrent work before
staging; preserve schema1 behavior. Do not create another service by default.

Proposed compatible route remains `POST /api/horde-reports`, discriminating the
product/schema/kind before strict kind allowlists. Current client kinds are
schema2 `benchmark-summary` and `compatibility`; local reports16KiB/8KiB, final
private request<=20KiB. Separate Send approval adds only
`consentToSubmit:true` and
`delivery:{destination:"developer-private",publishToBoard:false}`. Fresh one-use
verification is excluded from stored approved content/hash and local export;
its current printable ASCII bound is1-2048 characters inside the final cap.

Require durable atomic identity+content-hash admission: same ID/same approved
content returns the same disposition without another email; same ID/different
content conflicts. Retry keeps exact reviewed bytes/ID with fresh verification.
Edits/new reports need fresh review, consent and ID. Reject duplicate/unknown
keys, wrong kind/destination/consent, over-limit/invalid data and token replay.
Preserve existing abuse/rate limits; no persistent hardware fingerprint.

Client success receipt is exactly five flat fields:
`ok`, matching `id`, `status`, `destination:"developer-private"`, matching
`reportKind`. HTTP202/accepted is queued. HTTP200/sent is provider handoff, not
independent mailbox-delivery proof. Lost/malformed/stale/wrong-destination
receipts remain uncertain; cancellation cannot promise server deletion after an
uncertain send. Error/429/timeout paths require explicit retry, not background loops.

## Public submission and read API: proposed, not implemented

Stage `POST /api/horde-benchmark-results` with the existing typed public projection,
then a separately reviewed publication envelope. Proposed explicit field is
`consentToPublish:true`; this is not present in current clients and must be
co-reviewed with the receiver. Require destination `public-board`, kind
`public-benchmark-result`, a fresh result UUID and one-use verification. Do not
convert a private JSON report into public data or borrow private send consent.
Keep the public result<=16KiB and the final verified request bounded<=20KiB.

Proposed public receipt uses the same five identity/destination/kind fields,
HTTP202/accepted for pending admission and HTTP200/published only for a public
row. Public receipt parsing is a new typed path; do not reuse private sent semantics.
Receiver classification/moderation and public GET visibility must agree before
claiming publication. These are proposed interfaces, not callable current endpoints.

`GET /api/horde-benchmark-stats` returns only approved sanitized public rows and
schema/bucket definitions, provisional/eligible report counts, missing-data flags
and opaque next cursor. Bound response size/query work, limit<=100, filters and
sort to an allowlist: platform, reviewed model/GPU category, build, course/profile
revision, dimensions/quality/backend/timing-scope bucket and evidence class.
Stable sorting uses actual aggregate fields, with null/unavailable distinguished.
Unknown/null revisions stay provisional; unlike settings/scopes are not ranked
together. GPU duration is not scanout FPS. Public caches contain no private rows.
No fabricated/demo production results, hidden private fields or browser-side
filtering as a privacy boundary. Private compatibility intake does not grant
permission to publish compatibility/model aggregates.

The page is `/games/the-horde/benchmark-stats/`, linked as **How does it perform?**.
Use existing routing/content generators, accessible filters/sort, report counts,
provenance, loading/empty/error states and responsive table/cards. Empty means no
results. No site-side secret. Demo fixtures remain local and labelled.

## Exact actions held for authorization/access

1. Website access was denied at `C:\Users\sam_s\Documents\Samfa12 website`
   with `fatal: cannot come back to cwd: Permission denied`. It was not retried.
   Restore normal authorized workspace access or use an explicitly authorized
   existing-site task; do not clone/access another path as a workaround. Then
   inspect its AGENTS/concurrent state and stage on a non-deploy branch.
2. Stage receiver/tests in the existing authorized Briarhold `hosting/signaling`
   checkout after current-state verification. This Horde slice makes no mutation
   there. Identify the actual Worker/account/routes/bindings from approved config;
   no guessed target, new credentials, protection change or paid provider.
3. Return concrete reviewed backend and website diffs plus exact deployment
   target/command for a separate owner deployment decision. No auto-deploy main
   push, merge or production configuration change is authorized by this handoff.
4. Request a bounded live test scope naming destination, actual Windows/Android
   report kinds/counts and expected receipt/provider/mailbox evidence. No synthetic
   public scores or unsolicited email. Staging/mock acceptance must stay labelled;
   it cannot close production or physical RT evidence. Phone access and held
   Windows foreground/native acceptance require their own explicit allocation.

The implementation scope itself is already approved; do not ask the owner to
reapprove each field or routine local test. The actual access denial, external
mutations/deployment and live transmissions are the remaining boundaries.
