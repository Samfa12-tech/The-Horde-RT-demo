# S26 remote-report preparation and lifecycle admission

Single finite continuation record, October 2. This is actual Debug UI/readback
evidence, not Shipping performance, another music audition or backend delivery.
Normal engineering source checkpoint `5ea481d1b891d57a3d951485bbe781b454f0719b`;
unchanged retained Android child-document candidate `23ac884` supplies the APK.

## Exact artifact and device

SM-S948B / Android16 / Adreno840, driver512.842.19, Vulkan1.4.295.
Development package `com.samfa12.hordelanternrt.debug` only. Existing installed
APK `2c93d30322b5ca7b176d7e3543b770582828cf7945d57d3d35be9bf44d94e04e`
was replaced with the already-built Debug APK
`35776e1cb18d2089211a807e3c0f7a24bb15b31b248154684ed1a523125b9b10`.
Local source artifact and post-install pulled base.apk match that hash exactly.
No rebuild, stable-package change, app-data clear, system-volume/font change,
benchmark/performance repeat or S24 operation.

The initial Activity launch wait timed out after10s; a subsequent fresh hierarchy
showed the actual menu. This is not a fast-start acceptance claim. The retained
capability report proves actual RT swapchain presentation, Diagnostic/Mobile,
OpaqueFast,1080x2235. Its incidental timing is not a matched performance result.

## Completed finite rows

| Row | Actual observation | Receipt |
| --- | --- | --- |
| Default consent | Remote consent/context/image all unchecked; offline opt-ins also unchecked | `report-default-ui.xml`, `prepared-review-ui.xml` |
| No-consent preparation | Refused: "Check remote submission consent before preparing. Nothing was sent." | `prepare-without-consent-ui.xml` |
| Consented image preparation | Selecting all three opt-ins freezes fields, reaches review and produces a real game RT preview | `prepare-consented-ui.xml`, `prepared-review-ui.xml`, `prepared-review.png` |
| Image inspection | Preview contains the rendered corridor, both skeletons and held items/hands; no report/menu/system UI inside the image | `prepared-review.png` |
| Home/resume after READY | Same app process resumes; frozen preview remains, explicit verification button enabled; no automatic submission | `home-resume-fresh-ui.xml` |
| Edit after READY | Three remote opt-ins reset to unchecked, fields enabled and prior preview removed | `edit-reset-ui.xml` |
| Note-only preparation | With remote consent alone, review is reached without an ImageView; context/image remain unchecked | `note-only-ready-ui.xml` |
| Back before verification | Returns to paused main menu without verification or submission | `back-ui.xml` |

The note is synthetic test text only. No Verify-and-send action was invoked.
The earlier approved Windows fixture email is already accepted and is not repeated.
One additional Android acceptance email has been requested, not assumed approved.

One immediate post-resume UIAutomator attempt returned null root. A previously
named XML was still present remotely; that stale sample is **excluded**. The
accepted retry used a new remote filename and required a successful fresh dump
before pulling. All retained hierarchy packages are the Horde Debug package only.
OS screenshots here document the form; they are **not** report attachment data.

## Next unfinished step

If explicitly approved, prepare one fresh synthetic image/context report and
complete real Android WebView verification and one exchange. Record the actual
ACK without equating queued202 with delivered200/email. Otherwise retain this
preparation-only admission and keep verification/delivery open. Do not repeat the
completed preparation rows or rebuild this unchanged APK.

Capture-pending/verification/in-flight lifecycle races retain host-test coverage;
this run certifies only the READY Home/resume row. S24 report UI/verification and
S25 remain unverified. Owner external-audio interruption check remains separate.
Audio/haptic manual revalidation required:NO (reporting/readback only).

Current-source CI is independently green: push36969363479 and PR36969366368
for5ea481d both SUCCESS. It does not certify physical verification/delivery.
Goal ACTIVE/incomplete; no merge, release, publication or licensing changes.
