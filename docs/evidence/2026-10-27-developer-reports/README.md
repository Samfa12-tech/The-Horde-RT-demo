# Private-report and compatibility source checkpoint

This implements a bounded local/mock client slice of the
[approved1.7 scope](../../design/HORDE_1_7_REPORT_DELIVERY_SCOPE_2026_10_27.md).
It is **not** live benchmark/email delivery, public board completion, a new
device-compatibility record or release approval. Local checks are recorded below;
remote CI must be verified for the backed-up exact head. No phone, game launch, CUA, production report or
website/backend mutation is part of this checkpoint.

## Implemented

- Typed, owned `CompatibilityReportSnapshot` and immutable prepared local report,
  <=8KiB, independent of Vulkan headers/renderer objects. Explicit probe status,
  unknown/absent/present capability checks, typed attempted stage/outcome and real
  RT-presentation evidence; no invented performance values.
- Existing strict UTF-8/privacy/calendar validation reused. UUIDv4 syntax shared
  with benchmark identity without changing that policy. No IDs/fingerprint,
  raw logs/paths, saves/screenshots or arbitrary driverInfo.
- Sealed `PreparedDeveloperReportSubmission` for either typed local benchmark
  or compatibility report. Separate private-send approval; exact local bytes
  preserved, explicit private destination and public-board=false. Token is
  attempt-only and final request bounded20KiB.
- Windows existing asynchronous report owner reused for an **injected local
  exchange only**. No fallthrough to production. Android existing foreground
  owner reused through explicit transport admission, which production HTTPS does
  not grant. No new timer/worker pool or background retry framework.
- New-kind receipts require ID, kind and private destination. Queued is not
  delivered; wrong/missing/legacy/malformed receipts stay uncertain. Explicit
  retry preserves approved ID/body; old callbacks/cancel remain bounded.
- Android local compatibility review/export owner, and startup-failure plus
  diagnostics entrypoints. Separate local consent, exact selectable preview,
  Copy and explicit SAF Save, with responsive Close/unavailable state. Closing
  cancels owned picker/write callbacks; a pending picker must drain before a new
  report replaces it. No button offers an unimplemented live send.

Android native capture/preparation is outside renderer lifetime under the existing
report mutex, with revision/surface ownership. Preparation does not probe the GPU
or parse the diagnostic dump. Optional model/OS come only from Android model and
OS fields after local consent. Explicit probe/selected-device evidence is retained;
unchecked default false never becomes unsupported. Startup hooks cover observed
instance/surface, device-match/queue/backend selection, device/swapchain/RT-scene
and report-directory failures. Stable repeated publications retain their revision;
preparation checks revision and current surface before and after building bytes.
RT presentation is copied only from the existing observed `rtScene.presented` flag.
One additional bounded extension enumeration occurs at startup, not per frame;
negative feature-query completion is unavailable and stays unknown.

## Focused evidence

Final Debug selection passed5/5 in10.43s and Release5/5 in7.97s: compatibility
builder, benchmark summary/private wrapper, legacy playtest report/submission and
Windows local submission owner. Existing deadlines/coverage remain unchanged.

Android JNI compiled for `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`. Final focused
Java/widget selection passed48/48 and Debug lint passed in15s:8 benchmark review,
5 benchmark UI,3 compatibility dialog,5 developer flow,5 entry-menu,6 export and
16 existing submission tests. There was one retained test-only failure before
correction: AlertDialog posts its platform button callback; the new unavailable
Close test asserted dismissal before draining that main-looper callback. The test
now dispatches it, with the same Close assertion and unchanged runtime code. The
original native-build+failed-test receipt remains in
`reports/developer-reports-final-android.log`; corrected widget/lint receipt is
`reports/developer-reports-final-android-corrected.log`. Native compilation was
not repeated for that test-only correction. A final report-entry stale-menu guard
was then checked with the same48-test selection/lint in
`reports/developer-reports-final-android-admission.log`. SDK XML-version/deprecation warnings
remain visible; no tools/SDK/security update was made to suppress them.

`git diff --check` passed. SHA-256/readback verification found all151 original
parked owner files unchanged and untracked. No experimental RT candidate was enabled.

Tests exercise missing probe/unsupported/startup failure/both real-backend evidence
shapes, invalid claims/privacy/UTF-8/size/consent/owned bytes; uncertain acceptance
followed by explicit same-ID fresh-token retry; local mock dedup/content conflict;
wrong destination/kind; cancellation before dispatch; exact local export and
unavailable production transport. Android dialog tests cover portrait/landscape
widget ownership, consent/Copy and closed-picker suppression. Component/CPU tests
do not prove native screen appearance, physical SAF/touch, a working device, live
relay durability or received email.

Raw local logs stay in ignored `reports/developer-reports-*`; retain concise
receipts here rather than duplicating archives or unrelated assets.

## Backend and website handoff

The [staged API/data model](../../design/HORDE_1_7_REPORTING_API_2026_10_27.md)
records the actual inspected legacy backend, kind/destination-bound receipt,
token-free dedup key, strict private envelope, sanitized separately-consented
public projection, comparable benchmark buckets, filters/sorting/counts and
existing-site routing/generation requirements.

Website staging is **held**: read-only `git --work-tree=... status` at the existing
`Samfa12 website` checkout failed with `fatal: cannot come back to cwd: Permission denied`.
No complete concurrent-state check, site edit, replacement checkout/workaround or
auto-deploy branch push followed. The inspection itself completed with that error;
there is no pending website write or uncertain external mutation.

The canonical private relay was read only, not edited/copied to this public repo.
Its existing schema1 Horde endpoint does not admit these new kinds. Server
validators/storage/receipts/provider proof, public consent/projection/API, website
page/link and reviewed deployment remain genuine unfinished implementation. No
new credentials, account setting, provider or live test send was attempted.

## Follow-up and pause

The [bounded follow-up checkpoint](pause-followup.md) adds Windows owned local
review/failure wiring, Android asset/startup ownership, observed runtime-failure
classification and native source/input provenance. It records the affected
build/component checks, retained initial configure failure and current limits.
It supersedes the earlier platform/build-identity gaps below; backend/live delivery,
public board/site and native/device acceptance remain unfinished.

## Remaining acceptance and implementation

- Remaining platform coverage includes fatal termination, hangs and final teardown;
  unknown feature-query results stay unknown. Real native/device lifecycle and
  Windows appearance/input/export checks remain pending. Native-library startup
  failure offers honest unavailable/Close, not fabricated export data.
- Source/native-input provenance now complements package-version authority. It is
  not an exact packaged-artifact hash or full comparison key: Java/assets/package,
  shader/settings/profile and actual output evidence still need their respective
  trustworthy fields and artifact receipts.
- Backend staging and production admission are unimplemented. Do not enable
  live private send from mocks. Public board consent/storage/query and site are
  not implemented or published. Return exact deployment/config/rollback actions
  for approval when the staged implementation exists.
- After explicit deployment/test-send authorization, actual Windows/Android
  send→receipt→correct private destination, separate public submission→sanitized
  stats page, durable retry/dedup/abuse controls and mailbox evidence remain open.
- Native game/CUA and physical-phone restrictions remain unchanged. No previous
  held/rejected visual/asset gate is closed by these reporting tests. Normal
  flask/ring/yellow/shared64 remain OFF and all151 parked owner paths preserved.
