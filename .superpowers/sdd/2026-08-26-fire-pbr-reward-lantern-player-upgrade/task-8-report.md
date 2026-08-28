# Task 8 — Reward interaction, carry, finale, and pendulum evidence

Date: 2026-08-28

## Delivered scope

- Added platform-free `InteractionState`, `ChestRewardSequence`, `FinaleSequence`, and `LanternPendulum` authorities. Rendering consumes immutable snapshots and does not advance reward, finale, or motion state.
- Appended stable `RewardChest`/`RewardLantern` entity identities and `ChestUnlocked`/`ChestOpened`/`LanternClaimed` semantic events without renumbering existing values. Monotonic interact and held-light-pose sequences travel through direct simulation, the coherent two-slot mailbox, JNI, Android, Windows keyboard, and controller input. Unavailable edges are consumed once and are never buffered.
- Added route-local `Locked -> ClosedUnlocked -> Opening -> LanternAvailable -> LanternClaimed` progression. Valid interaction is at most 1.35 m and 55 degrees; opening lasts 1.20 s; the next post-open interaction claims the lantern. One completed route emits exactly one each of `LichDefeated`, `ChestUnlocked`, `ChestOpened`, `LanternClaimed`, and delayed `FinaleCompleted`.
- Extracted finale clocks into `FinaleSequence`: 0.65 s pickup raise, 1.25 s reveal, existing 4.50 s roof and 1.75 s dawn. Reset, retry, checkpoint import, pause/Home-resume, ending polls, and RT Lab ownership are deterministic and do not duplicate events or inject motion.
- Added shared `HeldLightKind`/`HeldLightPose` state and deterministic checkpoints 116-131 for high, low, glass, motion extreme, and the reachable carry volume.

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

## Dielectric and reachable-volume contract

- Loader-issued material flag `1024` remains restricted to thick components that pass finite weld-domain, closed-manifold, consistent-winding, and outward signed-volume validation. Uncertified/open/thin/inward/nonmanifold components cannot use recovery.
- A certified closed stack that reaches an attributable ordinary/grazing terminal, interface/volume bound, or mismatched exit conservatively absorbs; it does not shade or transmit through the opaque cage. The ABI now records strict counters, intentional primary/shadow certified recovery, reason masks, primary visibility pixels, and final grip metrics in 176 bytes.
- All standard 13 views and reward/stress checkpoints 114-131 have zero strict failure/open-stack/mismatch/overflow counters. Intentional certified recovery is separately attributed as recovery events, not distinct pixels: primary 0-51 events and shadow 0-5425 events, with reason masks in `{0,1,32,33,288,289}`. The worst workload-normalised observation is 5425 recovery events / 518400 launched primary pixels (1.04649%); one launch can contribute more than one recovery event. The events remain bounded to loader-certified glass terminals. Re-enabling skinned hands/player geometry and moving the shared carry target did not create a strict transport failure.

## Exact Host, shader, and visual evidence

- Qualified implementation head: `938116de3561f97b35e9329f41f7bbf56922f7c7`; tree: `ef2ee5e0efde2139661d3270bb9f1341cf9d5a44`.
- Fresh independent `build/task-8-fix-round2-938116d/debug` and `/release` trees passed Debug 30/30 in 125.89 s and Release 30/30 in 74.10 s.
- Generated ABI freshness, generic/legacy shader freshness, and embedded-SPIR-V equality passed. ABI definition SHA-256: `9295d677a88c47b52321b1d749207b2024edf617dd2486074f1bad7903100332`. Generic SPIR-V: `6714d10905d3e20f9b546a86a202861672146d32e964f9bfd6c112f09bc815f9`; legacy SPIR-V: `6e60d08555173dfe24fddb670b67074040446bf95b082a966c10e499c1411cfb`.
- The NVIDIA GeForce RTX 5050 Laptop GPU honestly presented fresh 960x540, render-scale-1.0 evidence in `reports/task-8-fix-round2-938116d`. Standard 13-view aggregate median was 12.3004 ms and mean 16.719876 ms; RT-command-buffer average was 1.472193 ms across 155 samples. Reward/stress medians were 12.0076-14.9856 ms.
- Every exact final PNG was visually inspected. Twelve standard views are byte-identical to Fix Round 1. Standard opening still shows real skinned arms with torch and sword, not floating props; `finale-roof` intentionally changes to the shared safe carry and now shows the complete lantern instead of the earlier bottom-clipped body. Checkpoints 114/115 show the chest/reward asset and glass inspection; 116-119 show distinct high/low/glass/extreme poses with the hand contacting the ring and the body below it; 120-131 show readable cardinal/diagonal/torsion/alternate-camera motion while the ring stays hand-rigid.
- Exact hashes, masks, pixel counts, grip metrics, timings, transport counters/reasons, shader metrics, asset hashes, and resource totals are recorded in `docs/evidence/TASK_8_REWARD_LANTERN_WINDOWS_CAPTURES_2026-08-28.json`. These deterministic Host samples establish workload identity and bounded behavior, not sustained-phone performance.

## Android build and package evidence

- Clean `testDebugUnitTest assembleDebug assembleRelease lintDebug lintRelease` passed in 2 min 1 s (116 tasks; 114 executed). The rendered max-font test passed 1/1 in 5.427 s.
- Debug APK: 85,967,341 bytes; SHA-256 `9adc91f26941c2246c2eb0124c3b1729834b9273b29a3aa0ecf241aa9f75892b`.
- Unsigned Release APK: 84,071,604 bytes; SHA-256 `bc46164a14b1174ac46037391f2a7b42765e4ec9bfa558e7e2e0291a98ba4ec0`.
- Runtime asset/licence package checks, four-ABI presence, ZIP alignment, and all twelve ELF `LOAD` segments at 16 KiB alignment passed.
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

No signing, version change, publication, upload, deployment, Meshy use, or replacement asset generation occurred. Automated and deterministic Windows evidence does not replace owner interaction, carry-motion, art, audio/haptic, or exact-phone review.

Audio/haptic manual revalidation required: YES — delaying FinaleCompleted and appending chest/claim semantic events changes gameplay-event timing/traffic even though no new cue or haptic is planned; the exact candidate therefore needs an owner feedback pass.
