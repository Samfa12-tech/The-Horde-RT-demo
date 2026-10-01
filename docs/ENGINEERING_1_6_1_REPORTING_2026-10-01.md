# 1.6.1 local player-report integration

Single continuation record. Reporting remains part of the agreed programme;
this slice does not reopen player, glass or music work. Original dirty engineering
checkout and scratch files remain untouched. No main merge, release or publication.
Shared contract3e0f9be; reviewed native integration69afe4e.

## Implemented boundary

The shared schema1 builder is now used by native Android and Windows forms,
reached from paused/menu UI. Explicit export consent and separate basic-context
opt-in start **off on every new form**. Foreground local file export is not remote
delivery: neither client uploads a report, chooses a server, attaches logs/saves/
screenshots, persists report consent, or retries in the background. A user-selected
document provider may sync the exported file independently; Android discloses this.
No Briarhold endpoint/account is borrowed.

`PlaytestReport` owns validation, strict UTF-16-to-UTF-8 conversion, privacy-policy
checks, note4096-byte/context512-byte limits and final16KiB cap. Authored CR/LF/TAB
are now allowed and JSON-escaped; they remain forbidden in context. Malformed
surrogates, NUL and other controls still fail, and credentials split by whitespace
still reject. No silent replacement or shortening. Redaction-pattern rejection
is a best-effort preparation policy, not comprehensive secret detection.

Android publishes an owned typed context snapshot on the existing render owner
and copies it under the existing report mutex. JNI never reads the live scene or
extracts private diagnostic JSON. It returns raw strict UTF-8 bytes, not JNI's
modified UTF-8. Windows captures an owned context on its application thread and
uses the existing authoritative capability presentation flag, not a second flag.
Compiled Mobile/High quality is independent of water settings. Unavailable
renderer context can never be guessed into a note-only report.

Prepared bytes/opaque ID are frozen across explicit cancel/failure retries.
Android writes at most16KiB on a dedicated worker, invalidates late UI completion
on Back/destroy, and states that a confirmed destination's already-started write
cannot be undone. Windows uses a synchronous native foreground file picker and
checks write, flush and close. Both disclose possible partial files on failure.
These are local retry contracts, not server delivery/deduplication proof.

The cross-platform design skill informed native controls/pickers with shared
consent and error states, not a UI framework migration. Its nine linked reference
files are missing from this installed skill package; repository UI/privacy
contracts supplied the bounded fallback. Phone inspection found and corrected
invisible selected Spinner text, then low-contrast CheckBox marks.

## Finite validation record

See [retained evidence](evidence/2026-10-01-report-export/README.md) for exact APKs,
synthetic exported bytes, logs, native UI fixtures and limits.

| Check | Result and boundary |
| --- | --- |
| Shared native Unicode/privacy/retry tests | MSVC Debug/Release pass; initial literal-fixture encoding failure retained, fixed with explicit Unicode escapes rather than weakening assertions |
| Real Windows form with injected destination | Debug2/2 and Release2/2 (shared+UI) pass; unchecked consent, required choices, privacy rejection, exact Unicode/multiline, fixed retry bytes, no duplicate success and honest false RT context |
| Full Windows application | Current Debug/Release executable builds pass; not a new RTX presentation or real Windows picker/write acceptance |
| Android Java | 30/30 unit tests pass, six focused export tests; exact UTF-8, consent/envelope limits, clones, picker cancel/retry, late callbacks, stream open/close errors |
| Android Debug/lint | Build and lint pass; four ABI native builds, no shader/gameplay/music edit |
| S26 EF2ECFE4 candidate | Actual unchecked consent rejection, note-only picker cancellation and explicit retry/save, new-form consent reset, and independently opted-in context/save pass |
| S26 UI follow-up | Final8CB97689 installed/pulled match; visible off/on marks verified at unchanged large font scale. Disabled retry label corrected in source. Only affected visual checks repeated, not finished exports/performance/music |
| CI | ca58f09 branch36853938787/PR36853945981 SUCCESS; actual eight logs inspected: GCC55/Clang55/MSVC57/focused Vulkan CPU-host15 in each. Exact merge parents/tree verified |
| Windows actual picker smoke | Real entry/default consent/context/picker opening observed; two control-ID attempts, one initial UIA read and one bounded settled-visible UIA read cannot identify filename element. Four incomplete attempts stop before path/write; actual save remains inconclusive, not a product failure |

The S26 context JSON actually reports SM-S948B/Adreno840/RayTracingPipeline,
Mobile/75%/1080x2235 and `rtPresented:true`. Its embedded build string is `1.6.1`,
not a source hash; exact artifact provenance is supplied separately. Neither
phone report is a new frame-rate, transport-correctness or release acceptance.

## Next unfinished step

Current checkpoint982ff9a push36869518263 passes GCC55/55, Clang55/55,
MSVC57/57 and focused Vulkan-host15/15. PR36869523187 has the same source
tree in its synthetic merge; its MSVC lane fails56/57 at the native report
fixture (`native control message timed out`), other lanes pass. This is not a
green PR. The fixture previously admitted a form during WM_CREATE before
ShowWindow/UpdateWindow/initial focus completed. It now waits for the visible
form and category focus, with the same2s message bound and all consent/privacy/
Unicode/retry assertions retained. Timeout diagnostics preserve the Windows
error and identify phase/control/message. That startup race is source-demonstrated,
but the hosted timeout was not reproduced locally and its unique cause remains
unproven. Affected UI CTest repetitions pass10/10 each Debug and Release; fresh
current-head CI remains required after integration. Runtime reporting is unchanged.
Audio/haptic manual revalidation required: **NO** (test-only behavior).

1. Local integration/push and fresh source/integration CI are complete atca58f09.
   [CI and Windows continuation](evidence/2026-10-01-report-export/RUN_STATUS.md)
   owns exact handles and completed failures; no completed test/build replay.
2. Settled-visible Windows picker inspection is now complete once; automated
   filename discovery remains unreliable. Next is one manual export to a fresh
   local file and inspection of its exact JSON/saved-state UI. Keep this desktop
   interaction explicitly open; do not repeat the four helpers unchanged.
   Opening the picker or passing mocked destinations does not close this gate.
3. Complete remote reporting only with a confirmed appropriate backend destination
   and authority; deployment/account configuration and real delivery are still open.
   Do not send unsolicited test reports or introduce client secrets.
   Destination was requested once on October2; await that answer rather than
   repeating the question after resumption.

The historical S24 handback is superseded by renewed owner availability. Current
bounded TLAS refresh restores normal hands/second-enemy intersections in captures
and a live route, without player tuning; see the October2 programme handoff.
The already-requested S24 owner check and complete-visibility Shipping evidence
remain open and must not be interrupted/resent. S25 stays unverified.
All remaining glass/High/backend, music focus/balance, performance and final-candidate
gates remain in the [programme handoff](ENGINEERING_1_6_1_HANDOFF.md).

Audio/haptic manual revalidation required: **NO**. Existing paused/menu UI selection
feedback is reused; no listener/event identity, SFX/music assets, playback backend,
gain, haptic pattern, gameplay feedback or animation authority changed.
