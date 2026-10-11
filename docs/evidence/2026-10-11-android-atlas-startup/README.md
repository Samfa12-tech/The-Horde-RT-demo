# Ordinary-startup atlas regression

The owner-approved Android terrain APK from source `278ea604` installed with
its existing development signature and preserved data, but failed ordinary
scene initialization before applying `rescue-journey-start`. Its SHA-256 is
`bce773bbe29bc94f4d47f6ca73504830c58993337495cbf6ffc702c2650f516e`.
On the authorized Samsung SM-S948B the selected backend was
RayTracingPipeline; `presented=0`. This is a failed physical startup, not device
acceptance. Foreground was verified before the owner-authorized error screenshot.
Raw device logs and the screenshot remain private local evidence.

Read-only hashing proved that the staged base-color array, installed APK entry
and repository payload were identical. The atlas has 18 base layers and 15
normal/ORM layers, while the ordinary native material roster owns 14 of each.
Chapter captures had tested the explicit chapter selection, missing the ordinary
scene that Android starts before checkpoint dispatch. The exact-count KTX guard
correctly rejected the mismatched ordinary upload.

## Correction and checks

Shared selection now explicitly uploads the original 14 leading rows for the
ordinary production scene. Menu/preview subsets, chapter aliases and terrain
append remain separate. The renderer validates native material counts against
that selection before uploading. Header, format, mip, alias and capacity guards
are unchanged. No atlas bytes, shader, quality or budget changed.

- Preserved failing regression: ordinary startup lacked a selection.
- Actual native integration caught an initial incorrect 11-layer normal/ORM
  assumption; it reports 14/14/14. Corrected against the actual roster and
  source atlas recipe; both failed local runs remain preserved.
- Debug focused tests: 3/3 PASS, 44.07 s. Release: 3/3 PASS, 6.51 s.
  These cover selection/negative cases, every actual Windows/ASTC mip and alias,
  and real native scene/material/resource ownership.
- Android Debug build, unit and lint tasks PASS (59 tasks; 283 Java tests,
  zero failures/errors/skips). Held-item/world/strict-ASTC package checks PASS.
- Ordinary ground fixture GPU captures: Pipeline and QueryCompute both complete
  with the unchanged strict capture checks. Actual frames opened and inspected:
  [Pipeline](ordinary-pipeline.png), [QueryCompute](ordinary-query.png).
  Both use Diagnostic/High, 960x540 at 100%, 12 settling frames. These prove
  ordinary upload/presentation on Windows, not Android or chapter completion.
- Parent `c6e2076c074b3fb9590c8a048cc4d6154b600587` exact CI passed all six jobs
  in [run 38109782834](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38109782834).
  Vulkan ran 36/36 tests, including all seven requested route/rescue tests.
  CI for this subsequent correction must be checked separately.

[Receipt](receipt.json) records exact source-file, executable/APK and shader
identities. Corrected APK SHA-256:
`eb50a06a1bfab9612f2a40fe87390c7e36567388cf540d16338417ad6944ba0e`.
Its physical verification is recorded separately after execution; prior owner
feedback does not automatically transfer. The larger chapter remains incomplete.

## Corrected physical startup

The owner approved the corrected exact APK separately. Install and installed-byte
verification succeeded with the same development signer/version, preserving data
and the release app. The owner reported "Game visible". Both focus and resumed
activity matched the Debug app before the authorized screenshot; no automated
input was sent. [Inspected portrait game frame](android-pipeline-start.png).

Fresh native evidence reports genuine Pipeline Diagnostic/Mobile presentation
at 720x1490 into 1440x2980, retaining the previously saved 50% render scale,
exposure 0.92 and water quality 1. No render-scale or quality override was sent.
The ordinary scene initialized, the rescue chapter rebuild completed, and the
chapter presented successfully. The first pipeline initialization took 15.70 s;
the checkpoint scene rebuild reused the compiled pair and took 297.60 ms.
These are startup observations, not sustained performance acceptance.

Fresh start records vitality/maxVitality 3/3, matching three visible hearts.
The diagnostic checkpoint has damage disabled; this is not an ordinary-run
combat acceptance or retrospective attribution of the older two-heart report.
Phone QueryCompute, full terrain traversal, lifecycle, audio/haptics and owner
visual acceptance remain pending. The larger chapter remains incomplete.
