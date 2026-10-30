# 1.6.2 final technical review — 9 October 2026

**Verified publication — 9 October:** the goal is complete; PR18 merged to main at `a3f120cf`, and signed Android/Windows 1.6.2 is live on itch. Fresh downloads match the signed APK and all 96 Windows files. [Exact post-release receipt, package hashes and Release smokes](evidence/2026-10-09-release-publication/README.md) supersede earlier no-merge/no-release statements. Historical validation snapshots and failures remain unchanged. Mobile default 50%, experimental 33/40, Dust Low and owner-deferred limitations remain authoritative; no sustained30FPS or completed Eric audit claim.

This record assembles the final-candidate technical evidence for independent review. It is not Eric’s completed audit, release certification, or publication approval. The final immutable Host run and exact unsigned artifacts pass; physical-device results remain separate and are scoped below. Owner-approved controller support, touch rotations, audio/haptics, menu, dust appearance, and waterfall/stand collision are not reopened. The owner retains current forgiving combat for 1.6.2 and defers precision-contact correction and the long sustained-phone programme. Shafts, the walking-arm wiggle, and music handover remain deferred. Mobile default remains 50%; 33%/40% remain experimental. No sustained-30-FPS claim is made.

## Preserved first Host failure and fixture correction

The first immutable Host run remains recorded as a failure: source 60ed8448c07990d3d2ccf60c5ad2e5b283630444, run 20261009-084719, clean before/after. Windows Debug compiled; 142/144 tests passed. The two failures were stale manifest/artifact shader pins, and the runner correctly stopped before later stages. The 12/12 CI result for push 37849259409 and PR 37849265911 is separate evidence.

The bounded test-only correction 612a8e66eccbbc6d811709e31ac7520c035d4e19 updated only pinned test hashes. Its focused manifest run passed; the first artifact rerun retained a further stale pin failure; the corrected artifact test passed. Those focused working-diff runs are preserved as historical evidence and are not presented as tests of the later immutable candidate.

## Final immutable Host candidate

The sealed candidate is source db62032d9ab54ebaa5bd12d17dc7e987fc8f54c9, tree bccbfe9469c69c0657b4f99c06a22c8c145dba28, Host run 20261009-092225. Git status was clean before and after. The foundation result is PASS: Windows Debug and Release each passed 144/144 tests in1482.69/963.58seconds respectively; 13 fixed Pipeline RT captures completed; Android Debug and Shipping builds and lint passed; all seven Host stages passed. The validation packages remain unsigned/unpublishable. The final-candidate seal records sealComplete=true, the matching source commit, and the exact hashes below.

| Sealed artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| Debug inspection APK | 25af2fc1895eca03c03feac2b955d410d0759a2834e27d9e1dafd3d24eda8ca3 | 123,889,887 |
| Shipping unsigned Android APK | 4f0fccec7cfe247f71c87852605b7c040f3213794758fdac5ff7d55236ea6ec4 | 121,049,362 |
| Windows Release unpublishable ZIP | 3fc8f065f900d28d529883f7b2681b6303940be9cdc1ed589425a92be69778bd | 145,572,438 |
| Windows Debug inspection executable | 4095f2b669657540637e81a19fe98b57dec5ae14c31adcce809aa34a73dc6e37 | 11,572,736 |

The matching current-source CI result is 12/12 jobs for push 37852998306 and PR 37853004515. CI, Host captures, package validation, and phone evidence remain distinct evidence classes.

## Exact Debug phone and UI evidence

The sealed Debug APK was installed and pulled back byte-for-byte on the SM-S948B running Android 16. The installed base SHA-256 equals the sealed APK SHA-256 above. The install receipt records main game settings and menu mix as byte-identical; no data was cleared.

The portrait Settings/Graphics review used that same APK and returned through Entry → Settings → Graphics → Graphics details/close → authored RT preview → Preview details/close → Graphics → Settings → Entry. It reviewed both grouped-option sections and actions, the live resolution choices and horizontal preview row, with dialogs dismissed. The system font scale was 1.7 and cutout safe-inset readback was available. Main settings and ambience preferences remained byte-identical; the owned app stopped. This route did not rotate the screen, and the app’s own reduced-motion preference was not explicitly active in that review.

A separate 20-case portrait Entry/Controls/Credits/Report navigation review returned to Entry. Main settings and ambience preferences remained unchanged; Report consent remained unchecked; no report was prepared, exported, or sent; the owned app stopped. This is navigation evidence, not a gameplay-comfort or FPS claim.

The later landscape/accessibility receipt is also bound to source db62032d9ab54ebaa5bd12d17dc7e987fc8f54c9 and the same Debug APK SHA-256. It covers landscape Entry → Settings → Graphics → Details/close → authored RT preview/scroll → Back → Entry → Home → normal return, with system animator scale 0 during the route. The original free-rotation setting and animator scale 1 were restored; preferences remained byte-identical and the owned app stopped. The observed Home return rotated back to portrait (1440×2980), recreated surface generation 3, and presented Entry. Do not describe the Home return as landscape. This is one current landscape orientation, not a current automated pass in both landscape directions; prior opposite-direction touch/rotation acceptance remains tied to its earlier runtime. The in-app reduced-motion preference itself was not explicitly enabled or tested. A separate harness preflight stopped before changing system settings and is excluded from the passing route; it did not expose a product-code defect.

## Exact-candidate moving phone evidence

Four sealed, harness-driven motion ledgers use the same exact Debug APK. Each completed its scenario, verified exact joins, preserved preferences, and stopped the owned app:

| Scenario | RT backend | Completed RT frames | Scripted duration |
| --- | --- | ---: | ---: |
| Waterfall equipment | RayTracingPipeline | 151 | 6.533 s |
| Waterfall equipment | RayQueryCompute | 162 | 6.550 s |
| Torch drench | RayTracingPipeline | 215 | 9.233 s |
| Torch drench | RayQueryCompute | 227 | 9.233 s |

Selected actual portrait Parry/attack and torch-lowering/settled images are inspected. These scenarios do not expose every moving world-body/shadow/mirror view; static mirror and host channel/ownership contracts remain separate evidence. Unsupported/recoverable error handling retains affected host checks rather than an invented unsupported-device phone result.

These runs establish bounded automated motion and completed RT frames. They do not establish owner review, touch latency, scanout, displayed or sustained FPS, thermal/power behavior, or full-game performance.
The public [motion summary](evidence/2026-10-09-final-review/motion-summary.json) retains the four sanitized run records and ledger/manifest hashes. The [selected image manifest](evidence/2026-10-09-final-review/selected-images.json) records the four public captures: [portrait Pipeline parry](evidence/2026-10-09-final-review/portrait-parry-pipeline.png), [portrait compute attack](evidence/2026-10-09-final-review/portrait-attack-compute.png), [landscape menu](evidence/2026-10-09-final-review/landscape-menu-reduced-motion.png), and [landscape preview](evidence/2026-10-09-final-review/landscape-preview.png). These images illustrate captured states; the run receipts and their stated limits remain the evidence.

The retained Windows Debug moving waterfall-equipment attempt did not arm: visible-window activation was rejected, foreground was not verified, no motion state or RT frame completed, and the run exited 1. Synchronization validation was enabled and recorded zero error markers. This is an arming/setup failure, not a renderer failure or a pass; it is retained without retry.

## Whole-PR source review and owner disposition

The independent whole-PR static review found no new actionable code defect. That conclusion is static review only and does not substitute for the Host, package, or physical-device evidence above.

The owner-approved controller/audio/haptic, touch-rotation, menu, dust appearance, waterfall appearance, and stand-collision decisions remain approvals, not new tests of every input/device combination. Current combat contact spacing remains a documented accepted limitation; precision correction is deferred. The long sustained-phone programme and the other deferred visual/audio items are not release-gating retests for this goal.

## Historical pre-publication status and release boundary

The signed publication receipt at the top supersedes the then-pending status below; the original unsigned seal, failures and review limits remain unchanged.

The root-owned final landscape/accessibility/Home route is complete within the receipt’s stated scope. The in-app reduced-motion preference toggle and a current automated pass in the opposite landscape direction remain outside that scope; earlier opposite-direction owner acceptance stays tied to its original runtime.

The owner conditionally authorized merge, Android signing, and Android/Windows itch publication after goal completion. At this receipt’s state, the goal is not complete, PR 18 is not merged, the Android artifact is not production-signed, and neither itch project is published. After the remaining root-owned integration is recorded, preserve the sealed Shipping APK and Windows payload through the authorized promotion, verify the signed APK’s certificate/alignment and unchanged application entries, retain final package hashes, then publish and verify the live itch artifacts. Do not rebuild merely to rename the already tested unsigned packages.

Eric’s independent audit has not been performed and is not self-certified by this packet. This report and the public aggregate receipt provide evidence and limits for that review. No release certification is claimed.
