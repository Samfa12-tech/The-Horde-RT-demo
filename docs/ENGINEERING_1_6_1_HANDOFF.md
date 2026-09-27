# 1.6.1 recovery handoff

Updated 2026-09-27. Branch: `codex/horde-1.6.1-engineering-pass`.
Engineering work has resumed by explicit owner instruction; the goal is not complete. See [programme scope](ENGINEERING_1_6_1_PLAN.md)
and [latest lantern evidence](ENGINEERING_1_6_1_LANTERN_BENCHMARK_2026-09-20.md).

## Owner's quota and backup instruction

The owner explicitly removed the approximately 5% usage guard and resumed work
on September 20. The `horde-usage-pause-guard` automation is confirmed absent.
Do not restore either the quota pause or explicit-resume restriction from older
handoffs. Preserve the full scope, selected model and effort, and normal service
limits; do not redeem credits or publish without authorization. Keep GitHub current
after coherent reviewed slices, preserving unrelated files and verifying remote SHA.

### Latest owner steering for continued work

Restore current CI early: inspect/reconcile PR #15 conflicts non-destructively,
preserve accepted implementation/guidance, add scoped branch-push validation,
obtain fresh current-source/integration results, and update the PR description.
Ensure actual skinned smoke/semantic fixtures are covered despite the portable
workflow disabling Vulkan targets. Do not merge into main, force-push or rewrite
history. Finish Phase 2 admission and affected RT images without restarting
Phase 1 or endlessly chasing historical parallel-export ordering. Define a
separate arms-only contract for the dedicated viewmodel. Keep Shipping/Diagnostic
parity distinct from pipeline/compute parity; investigate backend pixel divergence
without loosening tolerance. Include live motion, not just frozen extreme poses.
Music, consent-based reporting, resource work and final gates remain in scope;
licensing, signing recovery and publication remain owner-controlled.

### Current Phase 3 checkpoint

**Anatomical mounting candidate:** explicit immutable simulation profile grounds
the world/view body under the player, with shared reachable sword/reward targets
and .10 m higher hand paths. Original torch depth, grip roll, weights, .44 lantern
scale and stretch bound stay unchanged. Eleven existing native poses pass;
three torch poses were repeated after restoring original torch depth. Combined
low/down, high/up and low/parry retain zero measured shared-seam separation.
Legacy control PNG is exact. Host tests and ARM64 build pass; current phone is
still `b5d486b2...` pending a separately recorded install/live run. The candidate
changes apparent framing and is not owner-accepted. See [evidence and next bounded
camera-look change](evidence/2026-09-27-anatomical-mount/README.md). Deeper look
must rotate the camera independently of the existing bounded held-pose pitch;
the old 20-degree centre-look limit cannot naturally show the legs beneath it.

**Explicit body remainder, not yet visually accepted:** opt-in five-region
WorldBody contract and native primary filtering retain connecting cloth/torso/legs
without duplicating viewmodel arms. Host admission, shader validation and six
focused CTests pass; legacy four-region PNGs remain byte-identical. High/up
captures pass, but low/down and low/parry fail the unchanged lantern-body pixel
gate. Native geometry places the camera behind the torso; investigate anatomical
camera/body mounting and reach, not another IK/weight search. Natural occlusion
is valid (owner clarification); zero lantern pixels alone is not a defect.
No production replacement or phone install. See [bounded evidence and remaining
gates](evidence/2026-09-27-body-remainder/README.md). CI at parent `363bc72` passed
push36281339999 / PR36281341324; that does not validate this later source.

Follow-up commits `01a6500` (remainder ownership), `53c786a` (paired Android
manifest staging) and `c339b6b` (native mount/reach probe) are pushed. `53c786a`
passes fresh branch/PR CI. New five-region ARM64 Debug APK `a19fc1cf...` builds,
has exact packaged assets and valid contained Mobile shaders, but is deliberately
not installed. Mount probe proves a body-only translation toward the Head joint
plane would exceed the current fixed-grip reach bound by 34 mm. Establish the
anatomical body/camera mount and compatible shared held pose together; do not
increase stretch or tune sleeves to conceal it. Full Phase 3 acceptance remains open.

**Exact seam repair candidate:** opt-in `--reconcile-sleeve-seams` transfers the
accepted view endpoint weights to 355 world vertices at 198 matched Body/NearFace
edges. Fresh native low/down, high/up and low/parry now have zero measured shared
edge separation; view GLB and all three posed view uploads remain byte-identical.
Default exports remain exact; helper 11/11, gauntlet 6/6, C++ paired admission and
three RT captures pass. World candidate `2385fbb8...` is retained with its unchanged
view in [reproducible evidence](evidence/2026-09-27-player-seam-repair/README.md).
This is NOT a complete armpit/cuff repair: first-person connecting-cloth/body
ownership and actual cuff geometry remain open. No phone install or production
asset replacement; phone stays on `b5d486b2...`. Next fix must address anatomical
surface partition/visibility without exposing duplicate world arms or discarding
connecting faces by an arbitrary weight threshold. CI at preceding `acc73b9`
passed push36279842949 / PR36279844890.

**Latest installed follow-up:** ARM64 Debug `b5d486b2...`, runtime source
`f227b64`, passed exact install hash, 13-waypoint replay, six selected captures
and Home/resume on SM-S948B. This now puts yellow-floor removal, 12% lantern
scale and reward-carry parry guard on the phone. Retained 50-second live video
exercises low/high parry, swings and carry/look/movement transitions; owner
acceptance and the wrist/armpit/body/mirror issues remain open. Glass failures
remain 1–3 transport overflows in lantern captures. Phone app is Menu-paused
and Home-backgrounded. [Evidence](evidence/2026-09-27-phone-parry-followup/README.md).

Wrist investigation corrected a false contour match before asset edits: the
52/97-vertex open gauntlet loops are finger/grip openings, not the cuff. Mapping
through the verified source affine identifies the actual closed +X cuff, where
a small local cloth underlap is being evaluated. Do not bridge to the rejected
loops or retry global sleeve caps. Torso/legs cannot simply reuse all NearFace:
that material retains arm-weighted coat panels. A correct primary body partition
must preserve the real shoulder boundary and exclude duplicated arm geometry;
arbitrary bone-weight thresholds alone do not establish that contract.

The paired native dump now demonstrates posed disagreement at coincident
bind-space seam endpoints (low/down left wrist max100.75mm, upper seam49.24mm).
198/278 viewmodel sleeve boundary edges exactly adjoin retained NearFace cloth;
unmatched edges may include subdivision T-junctions, not necessarily true holes.
The cuff-strip worker exported no candidate; a strip before paired seam/cloth
ownership repair risks overlap. Next step is a bounded offline partition/seam
repair, not more IK tuning or blanket caps. [Exact paired evidence and tests](evidence/2026-09-27-player-boundary/README.md).
The installed phone is unchanged. CI for `e584ba5` passed push36278751279 and
PR36278752909.

**Newest owner follow-up:** `fce8b40c...` is quite good; sword swings no longer
hit the lantern (owner-confirmed). Remaining: low-carry parry hits the arm;
mirror swing arm penetrates torso (lower priority); reduce carried lantern
10–15% (lead selected12% candidate); low+look-down exposes inner bicep/armpit;
high+look-up exposes wrist join. Preserve this direction, with targeted
combined-pose captures and topology/mount fixes, not another broad arm rewrite.
The yellow-wash contributor is isolated: unoccluded bay-light colour was added
after surface shadowing/fog. Its removal restores dark hand backs in matched
RTX captures without geometry changes, while the torch control is pixel-identical.
[Evidence](evidence/2026-09-27-yellow-wash/README.md). The newer `b5d486b2...`
phone candidate above now includes this fix; final owner lighting acceptance
remains open. Full look-down body visibility and all later phases remain.

Lighting fix `178b332` is pushed with green current-source CI. The next small
candidate sets shared lantern scale .50 -> .44 (12%), preserving GripRing and
scaled Flame/Light sockets; focused host3/3 and native high/low captures pass.
It is now on the phone in `b5d486b2...`. Existing pendulum angular calibration is unchanged;
size is an owner-requested art change, not a measured optimization. Exact
low-parry, low+look-down and high+look-up development poses now use IDs144–146.
Parry stages one real command at the ninth60Hz tick (active time.11s); Android
camera pose restoration now covers136–146. Host2/2, Android units19/19 and
34 actual capture-guard positive/negative cases pass; native combined captures
succeed. These are diagnostic additions, not physical acceptance. Read-only
inspection finds open sleeve shoulder/wrist contours with intact UV/PBR data;
separate ForeArm sleeve and Hand gauntlet deformation makes a local wrist
overlap/bridge worth investigating. Rejected global caps remain rejected.

Low-parry pose candidate now moves only the sword-side guard (X -.16 -> .08,
blade inward -.62 -> -.28 for reward carry). Matched exact-upload diagnostics
prove baseline472 right-arm/left-sleeve crossings and307 sword/left-sleeve
crossings become zero broadphase pairs, with unchanged assets/left Grip.
The1098-pose shared-kinematics regression RED/GREENs; normal torch parry is
unchanged. [Reproducible native evidence](evidence/2026-09-27-parry-guard/README.md).
Do not declare live/phone acceptance until the next coherent APK is exercised.

**September 27 implementation/evidence checkpoint:** pushed `ed6e4bb` (default75),
`bf0334b` (shared sword carry lane), `e706f61` (bounded carry bend), `54493cf`
(single grip-anchored +10% gauntlet candidate). Current runtime-source CI push
`36273603275` and PR `36273605905` pass: 44 portable +11 Vulkan-enabled CPU
contracts; PR15 is draft, mergeable/CLEAN. Exact isolated APK `fce8b40c...`
passes phone replay/eight captures/Home-resume and has retained unfrozen live
motion video. It is installed and paused via Menu, not a paused engineering
goal. [Causes/checks](ENGINEERING_1_6_1_LIVE_ARM_FOLLOWUP_2026-09-27.md) and
[phone artifact/owner matrix](evidence/2026-09-27-live-arm-followup/README.md).
The three owner visual gates remain open pending feedback. **New owner note:**
lantern yellow wash lights hand/arm backs and sword implausibly; investigate
emitter placement, real shadow visibility/ownership and indirect/emissive/fog
contribution before final lantern/Phase3 acceptance. No proven cause or shader
fix yet. Do not mask this by darkening materials. Explicit look-down body,
glass correctness/performance and the wider programme still remain open.

**Latest owner live feedback (September 27):** the installed modelled candidate
is now pretty good and substantially improved. Preserve that direction; do not
restart IK or a broad arm rewrite. Remaining acceptance issues are (1) sword /
lantern / arm separation through the complete attack, (2) natural lantern-arm
compliance during walking/look/attacks/high-low and lantern motion, and (3)
slightly undersized hands relative to forearms. Diagnose and change each at its
own gameplay-pose or asset/mount layer; no capture-only prop movement, hidden
geometry or unrestricted roll/weight search. Continuous live motion and owner
feedback supersede relying solely on the eight frozen poses. The explicit
look-down torso/legs and nonduplicating visibility requirement still stands.
Read-only ADB matches installed APK `51b257c0...` on exact `SM-S948B`.

Owner explicitly requests default RT scale **75%** for current phone gameplay;
preserve saved choices and benchmark overrides, and do not claim a rendering
optimisation from this requested default. After Phase 3 acceptance, investigate
a bounded Android render-scale resize preserving resolution-independent assets,
BLAS/TLAS, pipelines, textures and state. Current device-idle/full-scene recreate
pause is owner-observed; preserve lifecycle/evidence and measure before/after.

Solver-first checkpoint `69014cf` is pushed/remote-verified. The documented
negative projection defect RED/GREENs in both the pure helper and actual
skinned solver using one independent analytic GLB control. Pinned ozz agrees;
Debug/Release focused tests pass 11/11. Current rig ratios do not encounter this
case and all eight native portrait images remain byte-identical after the fix:
**not the proven cause of the visual defect**. See the
[solver isolation](ENGINEERING_1_6_1_ARM_SOLVER_ISOLATION_2026-09-23.md).
The connected SM-S948B has now run the unchanged isolated APK: installed-byte
identity, replay, eight captures and Home/resume pass after fixing stale attack
capture expectations. Preserve 4/2 high/low lantern volume-budget overflows and
the visibly unaccepted art. [Phone baseline](evidence/2026-09-23-arm-phone-baseline/README.md).
Next: labelled anatomical mounting and production sleeve-layer isolation,
then explicit nonduplicating look-down body geometry and live/owner gates.
No further roll/weight sweep, broad arm rewrite or glass diversion.

Follow-up `c7bb1c1` is pushed and passes fresh branch/PR CI
`35827593090` / `35827596896`: 44 portable, 11 Vulkan CPU-host, six mirror,
ten resolver and 24 capture guard tests (logs inspected). PR #15 is draft/clean.
Fresh ARM64 Debug also builds. Exact APK `51b257c0...` passes the physical phone
matrix in run `20260923-164314` after one preserved 120-second replay timeout
(12/13 waypoints); successful rerun used the existing 300-second timeout. Seven
images are pixel-identical to the old APK, forward differs by 1 in two pixels.
The new math fix still does not resolve arm art; overflows stay 4/2. See the
phone record for exact hashes and limits. No app/test worker remains active
after this checkpoint; the installed isolated candidate is unaccepted.

Owner's subsequent research steering supersedes the next-step suggestions below:
freeze the current mesh/roll candidates; no broad rewrite or unrestricted weight/
roll search. First reproduce the signed-projection IK counterexample, distinguish
the helper from the actual skinned solver, and test one independent analytic arm
fixture. Use pinned ozz-animation as a solver reference, not a new runtime
dependency. Repair only demonstrated defects in mounting, IK, LBS or topology.
Visible torso/legs on look-down is an explicit remaining requirement, with no
duplicate primary/secondary arms or primary head. Current modelled masks do not
provide it. Existing eight-pose captures, live transitions/retraction and owner
phone acceptance remain gates. The phone is now connected as `SM-S948B`; older
"no device" statements below describe their historical runs, not current access.

Latest surface work: [sleeve closure checkpoint](evidence/2026-09-23-viewmodel-sleeve-closure/README.md).
The adjacent-cloth recovery attempt worsened the rim and was removed. Closure
preserved normals/gauntlet data and passed edge/paired-pose checks, but the later
intersection check **rejects** both candidates (221/191 cap/cloth triangle pairs).
The processor now fails before exporting this source; four Blender fixtures pass,
including rejection of crossing panels despite closed edges. Sleeve retopology
was the proposed next step before the owner's solver/control-first steering;
do not begin it before that isolation. Deliberately open garment cuffs are valid; edge closure alone is not the
goal. Preserve gauntlets/rig/RT ownership. Neither tracked GLB nor the earlier
Android candidate was replaced. Fitting also retains two normal failures.
The preceding portrait checkpoint `6f282bb` is pushed; fresh push/PR CI
`35810546291` / `35810550084` passed 44 portable, 10 Vulkan-host, six mirror and
ten APK-resolver tests (logs inspected). Do not reuse that CI for later tool edits.

The corrected-hand candidate now has an isolated **Android Debug** package, not a
production switch: `.debug.viewmodel` defaults to the real modelled route for live
play/replay and carries its matched world counterpart. Normal Debug/Release remain
unchanged. ARM64 Debug and unsigned Release isolation builds pass; a fresh targeted
Java run passes 4/4, and the APK resolver passes 10/10. Package/source/hash checks,
exact local artifact, pending eight-pose/live-motion matrix and limitations are in
the [Android candidate record](ENGINEERING_1_6_1_VIEWMODEL_ANDROID_CANDIDATE_2026-09-23.md).
ADB currently exposes no device; no install or new phone acceptance occurred.
Do not use the stale conventional Debug APK path: follow Gradle's artifact listing
or the immutable recorded artifact. Accepted runtime GLBs remain unchanged.
Implementation checkpoint `ed78452567682563e071cc117870eb6e00c57da5` is pushed
and remote-verified. Its push/PR CI `35808891150` / `35808893925` passed 44/44
portable, 10/10 Vulkan CPU-host, six mirror and ten APK-resolver tests, with logs
inspected. This later handoff-only edit changes no implementation.

Latest owner correction: the source hand is **Right**, not Left. The historical
left-source classification is withdrawn. New `--gauntlet-source-hand Right`
candidate exports mirror Left and preserve Right, with correct winding/UV-corner
association; no bone-label swap or accepted asset replacement. The UV correction
reduces unique viewmodel vertices to 15,855 without removing triangles. Six pure
geometry tests and paired admission smoke pass; two native corrected-chirality
captures complete. Anatomical mounting remains unaccepted; freeze roll values
until the new layered investigation establishes the hand-to-grip frame.
The earlier 90/180 and 105/145 roll experiments used the superseded chirality and
are not accepted fixes. A new Right-source 105/145 candidate is now packaged for
investigation, not admitted. Optional sleeve-envelope fitting still leaves ragged
boundaries and is not packaged. See the latest section of the hand/roll record.

The owner's hand-orientation report is investigated in the
[hand/roll record](ENGINEERING_1_6_1_HAND_ORIENTATION_2026-09-23.md).
There is no literal Left/Right chain swap; an isolated authored Grip-roll
candidate changes the bad cuff direction, but is not admitted. Exact Debug CPU
upload capture proves substantial sleeve deformation before GPU decoding. A
continuous elbow-weight candidate now stays below 3x maximum-triangle stretch in
all eight frozen poses, but residual fold/normal and anatomical gates remain.
World-space ray checks and native isolation corrected a mistaken attribution:
large rectangular panels are gallery swatches/a wall mirror, not sleeves. Do not
change that scene geometry as an arm fix. Exact Debug OBJ now includes the actual
instance transform; no Shipping readback was added. All isolation overrides were
removed, the restored native image is byte-identical, and the normal missing-arms
capture gate correctly failed the no-viewmodel experiment. Finish anatomical
mounting, sleeve shape and live motion before phone acceptance.
Both accepted GLBs remain byte-identical, and a fresh default offline export
reproduces the accepted viewmodel. No new phone evidence or production switch.
`f584be2` branch/PR runs `35802962127` / `35802965472` passed 44/44 portable and
10/10 Vulkan CPU-host tests (logs inspected); newer changes need their own CI.

`6b219832514b3e36bda3792ad6639119894675c2` is pushed and remote-verified.
Fresh branch/PR CI `35799096847` / `35799100216` passes 44/44 portable and 10/10
Vulkan CPU-host tests; PR #15 is MERGEABLE/CLEAN. This handoff-only follow-up
changes no runtime. New commits are `34b27b4` (non-no-op stale-hash fixture),
`d8b1b7f` (native GPU geometry ownership), and `6b21983` (current shader/checkpoint
validation, including fresh failure-driven repairs). Earlier CPU/asset slices
`6404976`, `6a93eb7`, `fff4649`, `051fcde` remain accepted foundations.

World/viewmodel now have independent vertex buffers, BLAS/scratch ownership,
shared solved gameplay/IK/grip pose, named geometry roles, shared texture domains,
and fixed bindings 23/24. Slot 20 is a new primary-only instance; normal gameplay
still uses block arms. See [Phase 3 record](ENGINEERING_1_6_1_VIEWMODEL_2026-09-23.md)
and [nine native captures](evidence/2026-09-23-viewmodel-rt/README.md).

The candidate is under `assets/models/player/viewmodel/`; its SHA-256 is
`1e3b041ee7aa896a84fe462c182f6012d026b4a577b2d4ddf59d764b823f2538`.
That tracked candidate is rendered in opt-in Windows development checkpoints and
remains outside normal Android packaging; the separate corrected candidate app is
described above. The unchanged Phase 2 world-image tolerance passes, 13/13 MSVC
focused checks pass (plus the final inventory regression), both sets of eight
SPIR-V modules validate, and unsigned Android Shipping/Mobile builds all four
ABIs. Broader Windows non-Vulkan validation passed 56/59 initially, then all three
failed tests passed focused repair reruns; do not call that one clean 59/59 run.
The missing-optional-asset native check also passes: world-body presentation works,
and explicit viewmodel requests fail clearly without a full-body fallback.
Severe sleeve/shoulder presentation, live motion, retraction, exact phone and owner
acceptance remain open. One raised-lantern transport overflow is preserved as an
open finding, not hidden by capture success. Next: refine the model/skin presentation,
package and validate the candidate, and then obtain owner phone acceptance before
retiring block arms. Static sleeves have bounded indices/55 mm maximum bind-pose
edges; exact posed CPU geometry now confirms sleeve deformation (see the newer
hand/roll record), without establishing every visible defect's cause. Do not redo
Phase 2 or restore a quota pause. Audio/haptic manual revalidation required: NO
for this slice; feedback semantics/playback did not change. No phone installation,
paid generation, signing/licence change or publication occurred.

### Historical quota checkpoint (superseded status)

Engineering work through `614a0e2daea00f4eec3ce02792cbc1af3317ec5c` is pushed and
remote-verified; this final handoff-only checkpoint follows it on the same branch.
Measurement-only baseline `186068a4adcb223d3096eee779fcf2b607117503` is also
remote-verified and clean. Phase 2 commits since the Phase 1 gate:

- `4063f90`: shared typed four-way contract (3/3 targeted MSVC tests).
- `d26f1c9`: manifest/loader enforcement (5/5 targeted MSVC tests).
- `ef8fdc0`: canonical texture groups (4/4 targeted MSVC tests; four-ABI Android build).
- `81908e1`: actual skinned malformed/reordered fixtures (final 2/2 MSVC tests).
- `614a0e2`: reproducible single-thread export runner, evidence and metadata reconciliation.

No worker/build/benchmark remains active. No phone action, release publication,
signing change, source-asset replacement or licence change occurred in this slice.
Four pre-existing untracked scratch paths remain deliberately uncommitted. Curated
evidence and reproducible sources are backed up; local APKs/build caches/generated
validation GLBs are not a full workstation backup. Fresh GCC/CI, generated-runtime
admission and affected RT image/device checks remain open; do not mark Phase 2 green.

## Accepted work and current boundaries

- Native hardware RayQuery compatibility backend is implemented alongside the
  preferred RT pipeline. Exact S24/S25 acceptance is still open; S26 does not
  certify those devices.
- Completion-owned CPU/GPU evidence, Shipping/Diagnostic and Mobile/High variants,
  full-route reports, Release-safe automation and isolated development-signed
  Android benchmark packaging are implemented.
- Five additional lantern workloads now cover four frozen production-geometry
  views and the live reveal sequence. The historical route never claimed the
  lantern. Shared, Windows and Android commits: `6335049`, `efb04d1`, `7ccb753`.
- Windows Release completed all five cases; 5/5 focused host tests and 11 Android
  benchmark tests passed. Fresh Windows High shader parity passes 15 image pairs,
  including held glass, with existing RGB tolerance 3. This is Debug-host image
  evidence, not Release timing or Mobile-driver equivalence.
- Exact SM-S948B Shipping/Mobile artifact `0f30a7…d80a95` completed all five
  600-frame measurements at unchanged 100%/1440x2980. Medians were 100.86–146.95 ms.
  These expose the expensive workload; they are not an optimization result.
  Full hashes, conditions and raw report copies are linked in the evidence guide.
- Home interruption rejected stale reports but exposed temporary scene retention.
  `6576950` restores gameplay in the owning cancellation path. Cleanup artifact
  `faf42c…fc0eb1` was byte-matched on device, interrupted, relaunched and visually
  checked at the normal spawn/torch scene. Do not assign the earlier timings to it.
- Latest implementation/documentation head before this recovery checkpoint:
  `89b20bfcd9709e26a83e30c78bee0b57b8ef3628`, pushed and remote-verified.

## Next work

September 20 follow-on: [fresh Android route A/B](ENGINEERING_1_6_1_ANDROID_RELEASE_ABBA_2026-09-20.md)
is complete at matched 76% settings, with whole-run thermal context and a new
clean-source candidate `167ce8b` / APK `02112a…e24bc`. All four 1,838-frame runs
completed; −0.37% mean-of-run-medians is not a demonstrated gain given thermal
drift. The 15-file [recovery bundle](evidence/2026-09-20-android-route/README.md)
is tracked. `.benchmark` now contains that clean candidate, restored to 100%;
stable remains untouched at 76%, phone Home, no active sampler/benchmark.
Quota check after subsequent lantern measurement: 12% remaining; no pause threshold reached.

The narrow source-baseline harness is now implemented and pushed through
**`186068a`** on `codex/horde-160-lantern-baseline`. Worktree:
`C:\Dev\tmp\horde-160-lantern-baseline`; host build:
`C:\Dev\tmp\horde-160-harness-build`. Core2/2 MSVC tests, four-ABI Android build,
three Java policy tests and exact old shader/51-asset package proof passed.
It does not change the old renderer, shader bytes, assets or gameplay implementation.
Do not merge the old-source branch into engineering or publish its test package.

[Held-high source-baseline A/B](ENGINEERING_1_6_1_LANTERN_ABBA_2026-09-20.md)
completed at unchanged100%: A134.387/B130.6903/B130.5626/A134.578ms. The observed
−2.867% mean-of-run-medians difference is small, not a material glass optimization.
The checked scene-region pixels match exactly. Full report/hash/thermal limitations
are retained; both originals needed to reproduce pixel comparison are Git-LFS-backed.
This plus the preceding variant/counter/runtime/route evidence closes the Phase1
measurement-foundation gate; its explicit decision is in the report. Broader
glass correctness/performance, exact S24/S25 and final release gates remain open.

Installed `.baseline` is **`84104e3` / APK `5f9253…c4fee2`**. New live-reveal
runner alignment at `186068a` built as APK `4cbd08…5a3461`, but is **not installed**.
No timings belong to the later artifact. All four accepted held-high runs used
fresh-process `am start -S -W`; without `-S` one attempt only brought a task forward
and was excluded. Baseline/candidate remain100%, stable76%; phone Home, no sampler,
build, benchmark or worker remains active.

1. **Phase 2 is technically accepted at `56cd539`:** four-way manifests/processor/
   loaders/atlas agree; actual static/skinned addressing is checked before upload;
   reproducible `e8737f10…450fd` runtime is admitted. Five paired native RTX pose
   images are byte-identical. Fresh push/PR CI each pass 43/43 portable plus 8/8
   Vulkan-host player tests; MSVC and four-ABI Android packaging also pass. See
   [phase gate](ENGINEERING_1_6_1_PLAYER_CONTRACT_2026-09-20.md) and
   [image evidence](evidence/2026-09-23-player-admission/README.md). No new phone or
   subjective arm acceptance is claimed. Main documentation conflicts were resolved
   in `d3a7225`, current-source CI restored, and PR #15 is clean/mergeable.
2. **Phase 3 in progress:** the reproducible arms asset, explicit two-part role,
   shared solved-pose CPU seam and dedicated GPU viewmodel/world-body ownership are
   implemented. Anatomical/surface/live-motion and owner-device acceptance remain open.
   Keep gameplay-owned IK/grip authority and independent small dynamic resources;
   no full-body primary-ray switch, overlays or permanent procedural block arms.
3. Physical glass correctness/performance remains open. Diagnostic held-high still
   records one overflow/pane-stack/primary-volume-budget event; secondary glass
   termination is also visible. Preserve these failures until actually fixed.
   Expand genuinely matched measurement to remaining views/live reveal before
   accepting later optimizations; no more telemetry framework is needed.
4. Complete justified memory/pacing/CI/docs, supplied adaptive Pocket Chordsmith
   music and separate volume, and Briarhold-derived reporting. Only then run the
   final comprehensive cross-platform/device/release matrix. No publication authorised.

## Recovery and device state

- Engineering checkout: `.worktrees/horde-1.6.1-engineering-pass` beneath the saved
  project. No active test run or worker remains from this checkpoint. The phone is
  returned Home; `.benchmark` contains the clean `167ce8b` artifact described above.
  Stable/Debug apps and
  their data were not replaced or cleared. Phone permission is already granted;
  do not repeat Briarhold coordination merely because context was compacted.
- Curated report evidence is now tracked in [the recovery bundle](evidence/2026-09-20-lantern/README.md).
  Compiled binaries/APKs, raw PNGs, build caches and unreviewed scratch are still
  local-only; GitHub is not a full workstation backup. Rebuild/recapture from source
  where necessary, preserving the distinction from the original exact evidence.
- Local `.superpowers/.../progress.md` contains extended implementation history.
  Four unrelated untracked scratch paths remain; do not blindly stage or delete them.
- A delegated path-quoting mistake overwrote two old capture files and created one
  under `C:\Users\sam_s\Documents\the`. It was disclosed; no unverified restoration
  or deletion was attempted. Details are in the lantern evidence document.
- Hotstrike licensing/distribution and owner signing recovery remain owner-controlled.
