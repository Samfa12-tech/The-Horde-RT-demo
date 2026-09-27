# Accepted modelled player: production integration

Source: engineering branch, changes on `ad023f3da85b6a5a6a2200fd843a762eba700556`.
These records were produced with the promotion diff uncommitted; the enclosing
promotion commit records that diff. No public release or main merge is implied.

## Change and authority

Admit the owner-accepted world `f2c3f62b2696c4630309b1d0b0ecb366151054fb48f7c0c6bcc6956980ff81eb`
and unchanged view `6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b`.
Five WorldBody regions and two Viewmodel regions have separate manifests,
geometry and dynamic GPU ownership; the existing Body/Gauntlet atlas is shared.
Both consume the gameplay-owned anatomical profile and solved animation/grips.
Normal gameplay, showcase, replay and glass use the dedicated modelled route.
Only explicit `player-body-*` / `player-fallback-*` comparisons select old routes.
No new asset design, IK tuning, camera adjustment or lighting/shader change.

## Startup failure reproduced and fixed

Ordinary Debug APK `0bea94649d67856a36db576258023b2422d88403f890eb091b468f31b3f291a9`
installed successfully but did not present RT. Run145531 timed out. App logcat:
`Dedicated modelled RT viewmodel requested without its validated runtime geometry.`
[Owner-observed screen](phone/startup-failure.png) was captured before restart.

The APK contained the pair, but MainActivity copied it to the native files root
only for candidate builds. Move manifest/GLB copying into unconditional production
staging, retain candidate-only provenance receipt staging, and fail explicitly
if required production assets cannot stage. A package-inventory regression check
failed before this fix and passes after it. No app data was cleared; the separately
accepted `.debug.viewmodel` application was untouched.

## Current evidence

| Check | Actual result |
| --- | --- |
| Vulkan-enabled Windows Debug player/semantic/socket/gameplay selection | 15/15 pass; 38.85 s |
| Normal-route Windows native RT captures | All 13 standard checkpoints pass, AnatomicalBody, dedicated ownership |
| Accepted four native arm poses vs promoted normal defaults | All four PNGs byte-identical (141,145,146,144) |
| Android ordinary ARM64 Debug + Java unit tests | Build passes; 19 tests, zero failures/errors/skips |
| Android unsigned ARM64 Release | Build passes; not installed or published |
| Exact Debug/Release APK viewmodel inventory | Current GLB/manifest bytes match; no processing JSON packaged |
| Packaged Debug/Shipping Mobile SPIR-V | Pipeline and compute containment, spirv-val/dis pass |
| Post-promotion offline regeneration | Two clean world/view/manifest exports; all six hashes exactly match accepted reference |
| SM-S948B ordinary Debug run150451 | Exact APK pullback, 13/13 replay, 3 captures, Home/resume pass at75% |

Fixed Debug APK: `14927941cc7e7596943b0a2092c01ae1ff30277d376697e170c1d44381057688`.
Unsigned Release APK: `d93a1084c8e08c6f7da2a9a7bac5f854ab9c7a47aeef5d193e097d153f1907b5`.
Both are retained in `C:/Dev/tmp/horde-modelled-production-android-20260927-b`.
Windows standard-capture executable: `0af9d37b6cdab15967124a28143b2241be2e241a7955609d43fa1afa06dfddfe`.
Native standard images/logs: `C:/Dev/tmp/horde-modelled-production-standard-20260927-b`.
Four comparison poses: `C:/Dev/tmp/horde-modelled-production-native-20260927-a`,
compared with `C:/Dev/tmp/horde-segmented-seams-native-20260927-b`. Its staged EXE
was later replaced for a capture-guard correction; do not use that staging folder
as the exact executable receipt for the earlier four images.

Phone captures show the normal opening arms/props, world-body mirror silhouette,
and separated low-parry carry. Dark mirror/cloth limit fine visual inspection;
this is integration evidence, not a new subjective acceptance claim. Existing
owner acceptance/live-motion evidence remains in the segmented-seam report.
All three states report AnatomicalBody, modelled-viewmodel, dedicated ownership,
60 Hz skin cadence and zero reported socket error. Strict ASTC and native RT
swapchain presentation pass; dispatch1080x2235. No timed benchmark was requested.

Shipping ELF `d64617655d424768fae88dc0a80153de7ab220c99337c3d562feb8bf4e68f788`
contains four Mobile modules: pipeline125253/54796 words, compute125318/54859.
All have zero atomic instructions and no diagnostic binding22. This is compiled
Shipping evidence, not Shipping/Diagnostic or pipeline/compute image parity.

## Test contract reconciliation

The old socket test permuted four materials after admission grew to five,
indexing `remap[4]`. Its MSVC array-bounds dialog was a genuine fixture defect.
The fixed named-size test checks indices and all120 permutations, retaining four
runtime atlas layers. Debug CRT assertions report to stderr without disabling
bounds checks.

Primary camera metrics now skin the actual two-part viewmodel from the shared
solved pose, using the renderer's anatomical root/basis. World-body boot grounding
remains separate. Nonempty sampled-triangle counts and finite clearance prevent
an omitted primary stream from passing on infinity. Live180-tick wall approach
passes with minimum clearance0.27312m and finite geometry.

Projection clips triangles against the near and four screen planes, rather than
clamping projected vertices. Synthetic inside/outside/behind/clipped/duplicate
tests cover this. The96x54 union footprint measures consecutive silhouette change;
the unchanged0.19 continuity bound passes (high0.0516975,low0.0239198). Summed
triangle area deliberately remains a separate conservative geometry-inflation
guard (same-carry wall/open ratio1.75 and absolute1.70 ceiling); it is not a
viewport percentage. Physical50mm clearance, deformation and individual-triangle
bounds remain.

Two obsolete full-body/view-relative assertions are explicitly superseded:
cross-carry torch-vs-raised-lantern summed-area ratio is not a like-for-like pose
comparison; every peripheral vertex being40mm ahead of the eye is inappropriate
for anatomical shoulders outside the view. Use same-carry bounds and zero visible
near-plane-crossing area plus actual50mm triangle clearance instead. No backend
pixel tolerance or native-image gate was relaxed. Earlier intermediate smoke
logs with omitted primary geometry are not acceptance evidence; the retained
final log includes triangle-count/finite guards and actual viewmodel skinning.

## Remaining gates

Phase3 normal-route integration is verified; preserve accepted geometry. Glass
is still open: low-parry reports one transport/volume-budget overflow, shadow0.
Prior high/look-up overflow evidence is not erased. No matched performance gain,
live glass correctness, backend parity, S24/S25 support or final release matrix
is claimed. Continue focused Phase4 glass work, then justified resource/resize,
music/separate volume, consent reporting and final-candidate validation.

Audio/haptic manual revalidation required: **YES, separately pending for the
anatomical gameplay mounting profile's event-time listener/source implications**.
The staging fix and topology admission add no new feedback changes. Visual owner
acceptance does not substitute for that pending exact-candidate feedback check.
