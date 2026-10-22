# Render-slot source-contract diagnosis - 22 October 2026

Base `cc20af8ff8270bb86bd7b7126d73fc1558b9c8c6`, existing WP2 worktree and draft
PR 27. All eight reproduced failures are stale textual expectations introduced
by the earlier regional-water change `b2d548a22cc44a58c92c290018f5bf211f2a8d87`.
No production regression is demonstrated by these failures. Production C++,
GLSL, JNI, assets, quality settings and save behavior are unchanged here.

## Diagnosis and preserved intent

| Failing expectation | Current implementation and correction |
| --- | --- |
| Six literal normal-code branches | Shader uses equivalent axis/parity arithmetic. The fixture now compiles the actual production decoder through a small test-only C++ scalar/vector shim, then checks all six independent expected signed axes with packed material/high bits present. CPU enum/face-winding checks remain. A source binding rejects stale generated arithmetic. |
| Water transmission always adds medium distance | Thin-film tracing starts after a separate exit distance; regional-water bed-hit distance already includes travel through the medium. Always adding the latter would double count it. Four numeric cases execute the actual production accumulation statement: film 6.006/11 m and regional 9/5 m. Existing reflected local/accumulated-distance checks remain unchanged. |
| Android snapshot declaration without survey isolation | The actual `RenderFrame` takes authored defaults for isolated surveys and one mutex-protected snapshot for ordinary RT frames. The fixture now checks that choice, one snapshot within this function, reuse for movement/preview/gameplay, and reset within the actual route-reset JNI function. The diagnostic-report snapshot elsewhere is valid and is not counted as a second frame read. |

[Provenance](provenance.json) checks each old expression at the introducing
commit's parent, its removal/new behavior in that commit, and current production
hashes. [Baseline output](baseline-test.txt) retains the exact six normal, one
water-distance and one Android failure from a fresh unchanged build.

CMake harvests the two bounded GLSL fragments verbatim into a target-private
generated test header. Both input shaders are configure dependencies; missing
extraction anchors fail configuration. The source binding also rejects ordinary
source changes followed by running an old test executable without rebuilding.
The fixture does not ship an alternate renderer or runtime normal table.

Six negative source mutations reject a changed decoder, restored regional
distance double counting, lost live Lab control, lost survey isolation, a second
frame snapshot, and removed route reset despite other JNI resets remaining.
The existing 2,000-iteration writer/reader coherent-tuple exercise is retained.

## Actual validation

- Fresh baseline build succeeds; the unchanged render-slot fixture fails its
  eight retained source assertions.
- Corrected Release selection passes **3/3**: CharacterRenderSlotSmoke,
  ChapterRegionalWaterTests and DielectricMathTests. Regional water reports
  3,047 checks/zero failures; dielectric math includes 24,576 contact-grid checks.
  [Final output](host-tests.txt), [fixture build](final-build.txt).
- Windows Release application compiles/links after the test-only CMake change.
  [Adjacent/application build](adjacent-build.txt).
- Read-only integration review found and confirmed correction of the first
  candidate's Android scope mistake: it inspected outer `SwapchainRenderLoop`
  instead of actual `RenderFrame`. [Original failure](scope-fixture-failure.txt)
  is retained; the existing outer-loop telemetry guards are unchanged.
- [Receipt](receipt.json) pins the source, generated fixture, binaries and logs.
  No binaries or new captures are committed. No phone operation, owner save or
  preference access occurred. Canonical owner-file hashes remain unchanged.

These are host arithmetic and source-integration contracts, not SPIR-V execution,
RT presentation, Android execution, or new visual/performance acceptance.
Extraction uses textual anchors/first matches; future duplicated or commented
definitions would require revisiting this seam. No such ambiguity exists in the
current inspected source. A fresh hosted full programme was not run here.

Audio/haptic manual revalidation required: **NO** for this test-only change.
Actual endpoint/listening acceptance for the earlier Kit anchoring `cc20af8f`
remains pending; this checkpoint does not alter or accept its runtime audio.

## Remaining checkpoint

The selected eight source failures are resolved locally; prior red outputs stay
historical evidence. Other hosted checkout/LFS, generated-bound/encoding and
fixture failures require their own exact-source diagnosis. Next inspect those
remaining hosted receipts before resolving PR conflicts safely. PR 27 remains
draft/conflicting, the Debug route timeout remains open, and no timeout or
acceptance gate was relaxed. No merge, release, deployment or phone reconnection.
