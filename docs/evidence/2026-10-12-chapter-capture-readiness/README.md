# Chapter capture readiness — owner-requested pause

12 October 2026. Implementation parent `dc0d17296b6649b6a4b67c7541a9e71306c42001`.
This checkpoint fixes development capture admission and inspection framing;
it does not complete the chapter or implement normal-play GPU streaming.
The owner paused the goal and removed the phone. No further phone action is due.

## Cause and bounded correction

The first physical landing request falsely reported `capture-ready` while the
snapshot stayed at X0/Z1.85/tick0. Its image was the opening showroom, not the
requested chapter landing. Java resolved the canonical name, but native scene
selection enabled the authored chapter only for live-start id192. Native staging
also ignored a failed simulation-stage result. The rejected image, state and
admission failure are preserved under `android/rejected-false-readiness/`.

All23 canonical chapter selections now request the actual authored scene.
Readiness is published after the selected simulation generation exists. Capture
and benchmark admission require staging success; failure propagates instead of
publishing a ready label. Returning to legacy selections exits chapter mode.
Host tests reject missing and stale readiness and check actual landing camera,
progression and equipment. The initial RED run had23 failures; GREEN passes.

The prior portal camera at Z-5.1 failed the strict primary-arm pixel guard.
The retained uploaded/skinned geometry and exact shader-frustum reconstruction
show the low-ceiling carry puts the arm below the view. Changing only the
inspection camera to Z-4.9 admits2,310 clipped left-arm triangles. Production
IK, carry, light sockets, FOV and the RT guard are unchanged. Tests preserve both
the positive inspection view and the negative original close view. Failed-frame
diagnostics now retain uploaded geometry and pose before rejecting a capture.
Large OBJ witnesses remain locally under `reports/chapter-primary-pose-diagnostic-portal/`.

## Actual Windows inspection

Both freshly opened native captures are complete, 960x540 at100% scale,
Diagnostic/High/Higher, RTX5050 Laptop GPU, frozen yaw0/pitch-0.15, X0/Z-4.9.
Each records2,151 primary player pixels. The torch, lower left gauntlet, bevelled
stone portal and tunnel are visible. These are framed inspection images, not
proof of every reverse portal, corner or the complete route. The original
rejected close image is retained beside them, not compared as an identical view.

| Backend | Twelve-frame GPU average | Manifest |
| --- | ---: | --- |
| Pipeline | 4.935288ms | [identity and counters](windows/pipeline/capture-manifest.json) |
| QueryCompute | 4.707872ms | [identity and counters](windows/query/capture-manifest.json) |

These bounded samples are not sustained performance acceptance. Exact shader
hashes and completed submission/resource identities are in each manifest.

![Reframed portal — Pipeline](windows/pipeline/198-chapter-stone-portal-front.png)
![Reframed portal — QueryCompute](windows/query/198-chapter-stone-portal-front.png)

## Physical Android scope

Owner-authorized SM-S948B, separate Debug app, unchanged development signer and
versionCode10; release untouched. Installed bytes were identified once before
replacement. Last installed prototype APK is
`0374dd8d4c367ff21bf586a6424e9c2f9733b6a3abecbccff9de2f43c0005a0c`.
It contains the staging fix before the final legacy-exit guard and portal reframe.
It is not claimed to be the final checkpoint APK. Exact original/changed identities,
signatures, four state records and screenshots are retained under `android/`.

Four actual landing captures use identical X-33.7/Z-12.8/yaw0/pitch-0.15/tick922.
They prove chapter progression, 3/3 health and genuine presented frames in both
hardware RT backends. All four screenshots were opened. They show the shaft/rim,
rope stake, grave markers, tree and terrain; the removed Lower-tier artifact is
not present in this view. Owner visual acceptance, movement and complete route
remain pending. Current uses1 local/1 sky sample; Mobile Higher uses2/2.

Original saved scale40% is preserved:576x1192 internal into1440x3120 portrait.
Only shadow tier was varied. Stored legacy Lower(0) rendered as Current(1).
Preferences still contained0 until a settings confirmation; physical confirmation
and persisted normalization were not tested. Every case restored the original
preferences and stopped Debug. All phone work ended before the pause request.

| Actual selection | Backend | Last GPU sample | Device-local bytes |
| --- | --- | ---: | ---: |
| Lower→Current | Pipeline | 54.636665ms | 166076608 |
| Higher | Pipeline | 67.906509ms | 166076608 |
| Current | QueryCompute | 55.481926ms | 166076096 |
| Higher | QueryCompute | 64.751301ms | 166076096 |

Twelve stable frames per case do not establish sustained/mobile budget acceptance.
All four primary-arm counters are0 in this portrait Low-lantern pose: these phone
captures are not held-equipment/arm visibility acceptance. No automated gameplay
input, listening/haptic or lifecycle acceptance is claimed.
[Exact matched results](android/matched-shadow-receipt.json).

## Validation and current artifacts

Debug and Release each pass the five affected CTests across the retained initial
three passes and final two corrected reruns. The first five-test runs failed the
new fixture count and the structural test's old `void` marker; both failures are
preserved. The original1,475 torch sweep poses and180-second policy are unchanged.
Final Debug torch160.87s plus slot smoke6.00s; Release15.93s plus3.32s.
Android four native ABIs, assemble/lint and286 Java tests pass, zero failures/errors/
skips. Runtime exact-byte/ASTC and held-item package checks pass for the final APK.
No shaders, quality settings, capacities or production budgets changed.

Final APK, prepared but not installed:
`24aaf0b5894e57a38804f39875c06fbf34b801dfd0c83997b76c8aea8fa9233e`
(183,031,354 bytes), immutable local copy
`reports/phone-candidate-24aaf0b5894e/horde-debug.apk`.
Debug EXE `395312f3c673bfe67ac683537fa7d0e8830cb6bc0ff9528b80faa55b607440e1`;
Release EXE `54ab35c71c34b2c8d81d8aec05b99f5b07777f099a8385dbc4443d1e2dda3c09`.
[Source/artifact receipt](candidate-receipt.json) identifies exact built source
bytes. Parent dc0d1729 passed all six CI jobs in run38214769328; new commit CI
must be reported separately after push, not inferred from that parent.

## Resume boundary

The chapter remains incomplete. Normal-play GPU residency saving is still ZERO.
Two-way preparation/commit/retirement with conservative ray/light contributor
retention, full final route/gallery, sustained performance, final-phone lifecycle,
listening/haptics and owner playtest acceptance remain open. Current resources
are shared/resident; diagnostic owner replacement does not establish streaming.
Base conflicts remain; no main merge/history rewrite is requested.
Easy Tree canopy work and mist issue28 remain deferred. Untracked historical
source/backup assets remain untouched. Resume normal-play ownership work and full
journey validation only when the owner resumes the goal.
