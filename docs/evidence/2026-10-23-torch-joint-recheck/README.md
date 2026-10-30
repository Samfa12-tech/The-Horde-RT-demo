# Current torch clearance and framing recheck

**The defect still reproduces; no pose change is admitted.** Five fresh Windows
Debug hardware-RT captures at source `dbf09d34` retain the current wall, lintel and
centre witnesses. This closes the question of whether the archived defect still
exists, not its acceptance requirement or the cause of the owner's phone clip.

| Current witness | Actual uploaded light / visible result |
| --- | --- |
| [Already-lit right diagonal, 11](diag-11.png) | Light X 1.768524 m, power 1.501050. Torch remains visible; primary arm count is zero. |
| [Right dark wall, 13](diag-13.png) | Light X 1.922479 m exceeds the physical wall at X 1.85 m and rear shell at 1.92 m, despite active power 1.477493. Stone is nearly black. |
| [Left dark wall, 21](diag-21.png) | Light X -1.918602 m is beyond the -1.85 m wall, power 1.518976. Stone is nearly black. |
| Lintel view, 40 | Torch remains visible, 237 primary arm pixels; this image alone does not establish full flame/roof clearance. |
| Centre view, 48 | Visible hand and torch, 2,371 primary arm pixels. |

All five were inspected. Each has a successfully presented RT-produced frame,
owning completed submission, successful storage-image readback and verified image
hash. This hidden, input-driven diagnostic follows current simulation movement
and advances the retained look sequence without importing progression flags.
Only five angles are captured. They retain the old control's simulation ticks;
there is no continuous-motion, physical-controls, performance or phone claim.
Settings are 960 × 540 internal/output, RayTracingPipeline, High shader/fire,
Higher shadows, Mist On and Dust High on the RTX 5050 Laptop GPU.

## Why another simple offset is not admitted

The [whole-hand translation](../2026-10-22-torch-wall-diagnosis/README.md) and
[fixed cant/tuck](../2026-10-23-torch-cant-withdrawn/README.md) remain withdrawn:
clearance was bought with lost framing and, in the latter, abrupt turning.
Production kinematics are byte-identical to that retained accepted control.

A small analytic check also rejects **world-upright plus screen-contained grip
centre** as an easy replacement at witness 13. It uses the actual shader slopes
(`aspect/1.22`, `0.74/1.22`), the 1.35 m roof, full flame top at 0.965 m from Grip,
0.15 m flame half-width and 30 mm gaps. Keeping that upright grip below the roof
requires at least 0.4362 m view depth. Even the favourable frustum corner needs
0.1040 m wallward displacement, while only 0.0709 m remains. See the
[explicit bounds and assumptions](upright-grip-centre-bounds.json).

This is **not** a proof that partly visible fingers, every upright rig solution,
or a tilted/adaptive pose is impossible. It establishes why a translation or
upright-origin shortcut lacks sufficient evidence. A useful next torch candidate
needs joint orientation, actual hand/torch projection and wall/roof constraints,
then continuous-turn validation. No new runtime solver, light relocation,
illumination compensation, weakened gate or repeated rejected trial was added.
The bounded pass ends here; approved opening-menu benchmark work can progress
independently.

## Build, custody and evidence

The five-view probe completed in 28.734 s with exit 0. Its temporary include was
removed byte for byte and the accepted Windows executable rebuilt successfully.
The original settings-file hash is unchanged; capture mode attaches no campaign
session. No owner save, source asset, phone, quality setting or held-item runtime
change was made. No renderer remains running from this pass.

An initial wrong CMake target failed without compilation. A sandboxed correct-target
build stalled during worker startup; its own session was cancelled and all owned
processes verified stopped before the successful normal-access build. Logs are
retained locally rather than presented as passing checks. Unchanged gameplay tests
were not rerun solely for this evidence-only checkpoint.

[Exact source, executable, frame and restoration identities](receipt.json).
Three small original RT images are retained here; the five raw captures, full
ledgers and logs remain in ignored `reports/torch-joint-review-20261023/`.
Audio/haptic manual revalidation required: **NO**, because runtime is unchanged.
