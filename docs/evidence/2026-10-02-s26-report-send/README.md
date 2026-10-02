# Single Android acceptance report and rapid-resume failure

October2 finite continuation of the [completed preparation rows](../2026-10-02-s26-report-ui/README.md).
Owner explicitly approved one additional email ("you can send an email, i dont
mind"). This approval is consumed; no extra send/retry or Windows fixture email.

## Exact artifact and completed submission

SM-S948B / Android16 / Adreno840 / driver512.842.19. Normal candidate00af842,
Debug APK SHA256
`e9fd31e7c9aee83a13d0a91f25e2b61497eb986940032dbb534e13d9787f4774`.
The immediately preceding affected route installs/pulls the exact retained APK;
no rebuild, data clear, stable-package change, volume/font reset or S24 action.
Actual report context is Mobile75%1080x2235/RayTracingPipeline/RT-presented.

One fresh report uses synthetic marker `ANDROID_ACCEPTANCE_20261002_01`.
Default remote opt-ins are unchecked; the three explicit note/context/image
choices are selected. Preparation freezes them and produces a real game-only
RT preview containing the corridor, two skeletons and modelled hands/held props.
Lead inspected that preview: no menu/report/system UI inside the thumbnail.
OS screenshots retained here document the form; they are NOT the attachment.
The native form stays paused; this is not music or performance acceptance.

**Verify and send pressed exactly once.** Actual hosted Cloudflare verification
completes, and the UI displays QUEUED with no send button. The compiled controller
maps this state to validated202/accepted with matching report identity, not200/
sent; no raw HTTP/token logging was added. Gmail exact-marker search independently
finds ONE INBOX message. The exact message read confirms both text/HTML carry the
unchanged synthetic note and bounded typed S26/renderer context, with the
`horde-lantern-rt-playtest.png` attachment, MIME image/png,396183 bytes (<512KiB).
The existing fixed relay/mailbox/provider is unchanged. [Bounded receipt](delivery-receipt.json)
records no addresses, headers, credentials or verification tokens.

Attachment metadata and inbox receipt do not prove a remote pixel hash or decoded
attachment equivalence. The connector supplies no inline pixels for this PNG;
no remote download workaround was used. Pre-send image inspection and inbox MIME/
size evidence are distinct. No unrelated email was read and no second send made.

## Separate rapid Home/resume: FAIL, not a helper-only gap

After delivery, Home followed immediately by Activity resume returned a null
UIAutomator root. One settled fresh-filename retry also returned null; neither
hierarchy is accepted as evidence. The first owned PID13046 subsequently changes
to16246. Android exit-info identifies the actual cause of process termination:
ANR, input focus dispatch waited10000ms, October2 17:10:58.936. Not a crash-free
report lifecycle pass, despite the earlier showcase runner's settled Home/resume
passing. Phone is left at Home; no report is automatically resent.

The exact owned DropBox entry (timestamp17:10:58, tag data_app_anr, PID13046)
contains the UI **main** thread in Adreno LLVM/Vulkan pipeline creation:

`onResume -> startSurfaceIfReady -> ProbeBridge.startDiagnosticSurface ->`
`PresentableTinyRtScene::CreateBundleStrategyPipeline -> Adreno driver/LLVM`.

The retained [main-thread excerpt](owned-anr-main-stack.txt) is lines74–175 of
the exact raw trace, excluding other processes' statistics/threads. The raw
bounded entry remains local under `C:/Dev/tmp/horde-s26-report-send-20261002/`.
Direct run-as reading of the system trace file was denied; the timestamp/tag
filtered DropBox query retrieves only the matching owned entry. No bugreport,
private app/files, memory dump or unrelated log collection.

**Proven defect:** synchronous renderer/pipeline startup blocks Android's UI
resume. The trace does not establish music, reporting HTTP or WebView as the
cause, or blame the world-normal labels. This can affect normal lifecycle too.

## Next unfinished step

Close a bounded off-UI renderer-start ownership change with explicit pending-start
cancellation, native-window lifetime, stale completion and superseding-surface
tests. Preserve the render/simulation owner and RT presentation contract. Do not
move a long startup wait onto onPause or admit concurrent Vulkan owners. Lead owns
the architecture; no renderer/player/shader rewrite or quality cut is required.
Validate affected rapid Home/resume and interrupted startup with this new artifact;
no additional email is needed or authorised to reproduce the native startup path.
Full-game Windows prepare/cancel/readback remains [OPEN separately](../2026-10-02-windows-report-ui/README.md).
S24 report/WebView and S25 remain unverified. Audio/haptic manual revalidation:NO
for this evidence-only task. Goal remains ACTIVE/incomplete; no release/publication.
