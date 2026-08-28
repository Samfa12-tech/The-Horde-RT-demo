# Task 8 — Reward interaction, carry, finale, and pendulum evidence

Date: 2026-08-28

## Delivered scope

- Added platform-free `InteractionState`, `ChestRewardSequence`, `FinaleSequence`, and `LanternPendulum` authorities. Rendering consumes immutable snapshots and does not advance reward, finale, or motion state.
- Appended stable `RewardChest`/`RewardLantern` entity identities and `ChestUnlocked`/`ChestOpened`/`LanternClaimed` semantic events without renumbering existing values. Monotonic interact and held-light-pose sequences travel through direct simulation, the coherent two-slot mailbox, JNI, Android, Windows keyboard, and controller input. Unavailable edges are consumed once and are never buffered.
- Added route-local `Locked -> ClosedUnlocked -> Opening -> LanternAvailable -> LanternClaimed` progression. Valid interaction is at most 1.35 m and 55 degrees; opening lasts 1.20 s; the next post-open interaction claims the lantern. One completed route emits exactly one each of `LichDefeated`, `ChestUnlocked`, `ChestOpened`, `LanternClaimed`, and delayed `FinaleCompleted`.
- Extracted finale clocks into `FinaleSequence`: 0.65 s pickup raise, 1.25 s reveal, existing 4.50 s roof and 1.75 s dawn. Reset, retry, checkpoint import, pause/Home-resume, ending polls, and RT Lab ownership are deterministic and do not duplicate events or inject motion.
- Added shared `HeldLightKind`/`HeldLightPose` state and deterministic checkpoints 116-133 for high, low, glass, motion extreme, the reachable carry volume, and both wall-compressed carry poses at the real `z=-10` fixture.

## Fix Round 1 corrections

- Separated production world/reward-asset existence from the developer inspection override. Standard production captures retain torch/sword masks `0x02`, the skinned-player route mask `0x14`, and primary player/arm pixels. Claimed-lantern frames keep the player, grip ring, and lantern body primary-visible instead of producing floating props.
- `PlayerRenderSlot` now exports the final skinned left-grip transform and refreshes it for same-tick checkpoint imports when animation state changes. The claimed ring is composed through the authored `GripRing`; `Hinge`, body, frame, glass, flame, light, reflection, and shadow derive below it. The source asset processor now bakes the ring's modelling rotation into vertices so `GripRing` and `Hinge` both have identity bases and their authored Y offset is the intended 97 mm. Runtime ring-to-final-grip error is 0 m and at most 0.000345 rad across checkpoints 116-131; ring-to-authoritative-grip error is at most 0.000008 m and 0 rad.
- Added owner-thread paused-input synchronization. Android resumes only after the owning thread acknowledges and consumes current attack/parry/reset/retry/interact/toggle sequences while paused. Deterministic stop-before-consume/resume interleavings pass for direct and mailbox delivery with no replay.
- Extended `LanternPendulum` with explicit torsion angle and angular velocity about the hanging axis. Torsion uses fixed-60-Hz restoring/damping and hand-basis/turn forcing, is bounded to a soft 15-degree and hard 20-degree limit with a 6 rad/s velocity cap, and participates in snapshots, checkpoint import, reset, teleport, pause/lifecycle, finite guards, and 30/60/120 delivery-invariance tests. It is composed with the two swing components; it is not inferred from those components or faked in rendering.
- Added a Robolectric SDK-34 rendered maximum-font-scale check at font scale 2.0 and a 320x568 viewport. It inflates and draws the real contextual controls, asserts bounds/no clipping/no overlap/no ellipsizing and non-empty rendered pixels. `includeFontPadding=false` and zero vertical text padding keep the compact buttons inside their authored bounds.
- Existing Task 5 inward phone-arm placement, 80-degree edge-forward sword, downcut/up-slice combo, wall retraction, and grip contracts remain green.

## Fix Round 2 corrections

- The shared reward carry target is now centred and moved far enough forward for the complete approximately one-metre authored lantern, while retaining shared wall retraction. High and low hand heights are 0.50 m and 0.26 m, remain 0.24 m apart, and use a reward-only left shoulder target; torch, sword, phone inward placement, combo, and wall-response targets are unchanged.
- The safe-frame test loads the actual runtime ring/body GLBs and projects every transformed authored AABB corner after `GripRing`, hinge, swing, and torsion composition. It covers high/low, rest, 45-degree cardinals, reachable 55-degree diagonals/opposites, torsion extremes, 16:9, 9:16, and 1440:3120. Rest AABBs fit inside +/-0.90 NDC; the complete ring retains a 14% margin. Worst reachable body coverage is high 51.6763% horizontal / 100% vertical and low 51.6763% / 97.4036%, while ring maximum absolute NDC is 0.686188 high and 0.394607 low.
- A direct same-tick regression proves a changed high-to-low imported animation refreshes the real `PlayerRenderSlot` and moves the final grip by at least 0.20 m. Repeating the identical low animation on that tick does not refresh and preserves the final grip within 1e-6 m.
- Checkpoint 130 retains an alternate camera but uses the covered positive diagonal swing so the complete illuminated cage is readable rather than hidden behind the arm/wall silhouette. This is secondary inspection framing; the shared runtime carry target and the aspect/extreme AABB contract provide the runtime safety guarantee.

## Fix Round 3 corrections

- Strengthened the shared authored-AABB safe-frame contract from the permissive Fix Round 2 threshold to at least 90% horizontal and vertical coverage across high/low, rest, 45-degree cardinals, reachable 55-degree diagonal/opposite swings, +/-20-degree torsion, 16:9, 9:16, and 1440:3120. The exact production GLBs now retain 93.5392% worst horizontal and 100% worst vertical coverage for both high and low; the complete ring stays within absolute NDC `x=0.146474`, with `y=0.433151` high and `0.250407` low. Open-space high/low targets remain 0.24 m apart.
- Reward carry depth is no longer an unconditional addition on top of wall retraction. `HeldItemKinematics` derives a smooth collision-clearance blend from the same wall-safe held-prop depth, then publishes the reward presentation yaw in the immutable simulation snapshot. `LanternPendulum` keeps raw swing/torsion and velocities authoritative for continuity but composes the collision-bound effective body transform during fixed-step simulation. Rendering consumes that final `worldFromBody` for the cage, six glass volumes, flame, coloured light, shadow, and reflection; there is no renderer-only wall transform, visibility mask, or camera attachment.
- At the real `z=-10` wall and camera/player position `z=-9.70`, the high rest/swing body minimum Z is `-9.94412/-9.94583`; low is `-9.94283/-9.94454`, leaving 39.17-40.88 mm beyond the required 15 mm wall-plane margin. Camera-origin clearance to body/glass/ring triangles is respectively 96.2772/255.794/102.733 mm worst high and 103.112/265.773/101.980 mm worst low. The hinge remains camera-side of the safety plane. This extreme-clearance pose deliberately compresses high/low separation and damps only the presented cone while preserving the raw pendulum state; open-space carry retains full pose separation and physical response.
- Added checkpoints 132 `lantern-wall-high` and 133 `lantern-wall-low`, which import the real wall fixture and exact authoritative body transforms. CPU checkpoint/import tests and Windows captures agree: both retain primary-visible skinned player, ring, and body, zero final-grip error, at most 11 micrometres renderer-authority position error, and zero strict glass failures.
- `StaticGltfAssetTests` now uses configuration/process-unique temporary roots and safe exact-root cleanup. Four concurrent Debug/Release processes pass without the prior shared-directory collision.

## Dielectric and reachable-volume contract

- Loader-issued material flag `1024` remains restricted to thick components that pass finite weld-domain, closed-manifold, consistent-winding, and outward signed-volume validation. Uncertified/open/thin/inward/nonmanifold components cannot use recovery.
- A certified closed stack that reaches an attributable ordinary/grazing terminal, interface/volume bound, or mismatched exit conservatively absorbs; it does not shade or transmit through the opaque cage. The ABI now records strict counters, intentional primary/shadow certified recovery, reason masks, primary visibility pixels, and final grip metrics in 176 bytes.
- All standard 13 views and reward/stress checkpoints 114-133 have zero strict failure/open-stack/mismatch/overflow counters. Intentional certified recovery is separately attributed as recovery events, not distinct pixels: primary 0-50 events and shadow 0-1897 events, with reason masks in `{0,32,33,288,289}`. The worst workload-normalised observation is 1897 recovery events / 518400 launched primary pixels (0.365934%); one launch can contribute more than one recovery event. The events remain bounded to loader-certified glass terminals. Re-enabling skinned hands/player geometry, strengthening portrait framing, and composing the wall-safe presentation did not create a strict transport failure.

## Exact Host, shader, and visual evidence

- Qualified implementation head: `51516ebcad21442c1f0afb81a6da3f8198948778`; tree: `dad42dd2823b6556b5817e3eef09c30bacf9efaf`.
- Fresh independent `build/task-8-fix-round3-51516eb/debug` and `/release` trees passed Debug 30/30 in 142.90 s and Release 30/30 in 66.98 s. A separate four-process concurrent Debug/Release static-asset run also passed after the unique-temporary-root fix.
- Generated ABI freshness, generic/legacy shader freshness, and embedded-SPIR-V equality passed. ABI definition SHA-256: `9295d677a88c47b52321b1d749207b2024edf617dd2486074f1bad7903100332`. Generic SPIR-V: `6714d10905d3e20f9b546a86a202861672146d32e964f9bfd6c112f09bc815f9`; legacy SPIR-V: `6e60d08555173dfe24fddb670b67074040446bf95b082a966c10e499c1411cfb`.
- The NVIDIA GeForce RTX 5050 Laptop GPU honestly presented fresh 960x540, render-scale-1.0 evidence in `reports/task-8-fix-round3-51516eb`. Standard 13-view aggregate median was 12.59715 ms and mean 34.147221 ms; RT-command-buffer average was 1.753339 ms across 155 samples. Reward/stress/wall medians were 11.69985-13.7455 ms.
- Every exact final PNG was visually inspected, including contact sheets and full-resolution 132/133. Twelve standard views are byte-identical to Fix Round 2. Standard opening still shows real skinned arms with torch and sword, not floating props; `finale-roof` intentionally changes with the new shared carry while retaining player/ring/body pixels. Checkpoints 114/115 show the chest/reward asset and glass inspection; 116-119 show distinct high/low/glass/extreme poses with the hand contacting the ring and the body below it; 120-131 show readable open-space cardinal/diagonal/torsion/alternate-camera motion while the ring stays hand-rigid. The near-wall 132/133 views are intentionally close and compressed rather than open-space beauty views, but both visibly retain player, ring, cage, coherent light, and wall context without camera/body/glass inversion.
- The deterministic Windows capture path is fixed at 960x540 and ADB exposed no phone, so no rendered portrait frame is claimed. Portrait evidence is the production-GLB corner projection across both required portrait aspects and every reachable pose; exact rendered portrait/device review remains part of Task 9.
- Exact hashes, masks, pixel counts, grip metrics, timings, transport counters/reasons, shader metrics, asset hashes, and resource totals are recorded in `docs/evidence/TASK_8_REWARD_LANTERN_WINDOWS_CAPTURES_2026-08-28.json`. These deterministic Host samples establish workload identity and bounded behavior, not sustained-phone performance.

## Android build and package evidence

- Clean `testDebugUnitTest assembleDebug assembleRelease lintDebug lintRelease` passed in 2 min 5 s (116 tasks; 114 executed). The SDK-34/font-scale-2.0/320x568 rendered max-font test passed 1/1 in 5.177 s.
- Debug APK: 85,968,829 bytes; SHA-256 `83eacf45e7a4500125a1ad63ef3d93896faecf568b1d93f940b46e457800e7c6`.
- Unsigned Release APK: 84,073,108 bytes; SHA-256 `7fe1394d802412e9209e96f95846614299cf4c0f0cc590d20202923c851de60e`.
- Both APKs passed the runtime asset/licence package contract, four-ABI presence, no shared libc++ packaging, and 16 KiB ZIP alignment. All twelve Release ELF `LOAD` segments are aligned to `0x4000`; `git lfs fsck` passed.
- `adb devices -l` exposed zero devices. No install, pullback, strict-ASTC runtime selection, honest phone RT presentation, touch/controller feel, Home-resume, performance, thermal, or phone visual evidence is claimed. There is no new device evidence, so the compatibility record is unchanged. Exact `SM-S948B` validation remains the Task 9 hard gate.

## Commits and owner gates

Original Task 8 commits: `4849fb2`, `381c060`, `3e714a2`, `95f49d0`, `8f0c6b9`, `e1fde27`, `6dca5e4`, and `e29e16a`.

Fix Round 1 commits:

1. `f9d8c71` — `fix: preserve reward player visibility and grip`
2. `0ededb7` — `fix: acknowledge paused input before Android resume`
3. `2f64ef5` — `feat: add deterministic lantern torsion`
4. `1d04525` — `test: render contextual controls at maximum font`
5. `964efc4` — `test: guard reward fix integration seams`
6. `c2834e5` — `fix: keep claimed lantern below final grip`
7. `64cf333` — `fix: keep skinned arms in production reward world`
8. `3f383e8` — `test: enforce player and grip visibility through lantern sweep`

Fix Round 2 commits:

1. `0256b50` — `fix: keep claimed lantern inside shared safe frame`
2. `938116d` — `fix: improve alternate lantern inspection framing`

Fix Round 3 commits:

1. `d8611f8` — `fix: keep claimed lantern safe in portrait and at walls`
2. `eac8c89` — `test: isolate static asset fixtures per process`
3. `0bf808e` — `fix: preserve inward reward grip within arm reach`
4. `c29b19e` — `test: capture wall-safe reward carry`
5. `51516eb` — `fix: publish wall-safe lantern presentation`

No signing, version change, publication, upload, deployment, Meshy use, or replacement asset generation occurred. Automated and deterministic Windows evidence does not replace owner interaction, carry-motion, art, audio/haptic, or exact-phone review.

Audio/haptic manual revalidation required: YES — delaying FinaleCompleted and appending chest/claim semantic events changes gameplay-event timing/traffic even though no new cue or haptic is planned; the exact candidate therefore needs an owner feedback pass.
