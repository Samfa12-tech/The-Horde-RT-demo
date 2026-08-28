# Task 8 — Reward interaction, carry, finale, and pendulum evidence

Date: 2026-08-28

## Delivered scope

- Added platform-free `InteractionState`, `ChestRewardSequence`, `FinaleSequence`, and `LanternPendulum` authorities. Rendering consumes immutable snapshots and does not advance reward, finale, or motion state.
- Appended stable `RewardChest`/`RewardLantern` entity identities and `ChestUnlocked`/`ChestOpened`/`LanternClaimed` semantic events. The existing event values remain unchanged.
- Added monotonic interact and held-light-pose command sequences through direct simulation input, the coherent two-slot mailbox, JNI, Android controls, Windows keyboard, and controller input. Every new edge is consumed once, including edges issued while interaction or pose changes are unavailable; commands are never buffered for later eligibility.
- Added the route-local chest progression `Locked -> ClosedUnlocked -> Opening -> LanternAvailable -> LanternClaimed`. A valid interaction requires at most 1.35 m distance and at most a 55-degree facing error. Opening lasts 1.20 s. The first valid interaction opens the chest and a second post-open interaction claims the lantern.
- Moved the post-reward clocks into `FinaleSequence`: a 0.65 s pickup raise and 1.25 s reveal precede the retained 4.50 s roof and 1.75 s dawn stages. `LichEncounter` owns defeat only. Automated contracts pin one `LichDefeated`, one `ChestUnlocked`, one `ChestOpened`, one `LanternClaimed`, and one delayed `FinaleCompleted` per completed route.
- Added shared `HeldLightKind` and `HeldLightPose` state, renderer adapter outputs, pause/reset/retry/import/Home-resume preservation, repeated ending-poll/RT-Lab ownership coverage, and deterministic checkpoints 116-119 for held-high, held-low, glass transmission, and motion extreme.

## Physical carry and character integration

- `LanternPendulum` advances only at the shared 60 Hz gameplay tick. Its state contains two hand-local angular components and velocities, prior authoritative pivot position/velocity, COM length, and explicit initialization/reset/import data.
- Semi-implicit Euler integration applies gravity/restoring torque, damping, actual hinge acceleration, and bounded movement/turn/strafe/dodge forcing. A soft 45-degree cone and hard 55-degree safety clamp bound all motion. Teleport, pause, lifecycle, retry, reset, and import seams clear inferred velocity; finite guards prevent NaN propagation.
- Behavioral tests cover rest convergence, forward-start lag, stop overshoot, strafe/turn response, bounded dodge, damping, both clamps, teleport reset, pause/Home-resume, finite output, and identical fixed-tick results under 30/60/120 Hz render delivery.
- The grip ring remains rigid to the authored hand socket. The lantern body, frame, six glass panes, flame mesh, true coloured light, reflection, and shadow all consume one pendulum body transform below the Task 7 `RewardLanternHingeSocket`/`Hinge` contract. The two ordered physical angular components provide bounded coupled front-face alignment; there is no camera attachment or sinusoidal render fake.
- The real skinned left arm follows high/low/transitioning IK targets and shared held-item wall retraction. Existing phone inward placement, sword edge-forward orientation, downcut/up-slice combo, right-hand grip, and route-floor authority remain covered by the expanded character and held-item tests.

## Renderer, controls, and budgets

- The generic production assets provide a hinged chest lid plus closed/open/reveal/claimed and held-high/held-low states. The retained budgets are 16 BLAS, at most 20 TLAS instances, 14 compact materials, 256 primitives, 3,938,688 vertex bytes, 506,892 index bytes, 1,568 material bytes, 640 metadata bytes, 156,587,312 texture bytes, and 1,032,704 combined static BLAS bytes.
- Android exposes contextual `INTERACT` and `RAISE`/`LOWER` controls with the enlarged-font compact-layout contract. Windows exposes `E`/`F`; controller gameplay uses A/Y while A retains menu selection semantics. Attack, parry, dodge, and pause/menu commands are not overloaded.
- Task 7 dielectric, generic-asset, floor, provenance, socket, texture-array, and licence contracts remain intact.

## Moving dielectric stress repair

- The production lantern exposed finite-precision terminal cases at reachable high/low and pendulum angles. Camera-only composition changes were rejected because the dynamic prop is visible from arbitrary player views.
- Runtime loading now issues append-only material flag `1024` only when every thick-material component passes finite weld-domain, closed-manifold, consistent-winding, and outward signed-volume checks. Open, inward, nonmanifold, thin-wall, or otherwise uncertified geometry cannot enter the recovery path.
- A certified closed stack that reaches an ordinary/grazing terminal, interface/volume bound, or mismatched exit conservatively absorbs instead of shading or transmitting through its opaque cage. The strict open-stack/failure counters remain zero. Separate primary/shadow recovery counters and a reason mask distinguish intentional certified recovery from a strict transport failure; ABI size remains 144 bytes.
- Checkpoints 120-131 deterministically sweep the reachable high/low forward, backward, left, right, hard-diagonal, and alternate-camera volume. All 12 sweep views have zero strict failure/open/mismatch/overflow counters. Intentional certified recovery ranges from 0-9 primary and 0-21 shadow samples per view, with reason masks limited to `0`, `32`, `33`, and `289`. The fix therefore covers the live reachable volume rather than one camera angle.

## Exact Host and Windows evidence

- Final implementation head `e29e16adf4350ccb418e095417172db74b77128e` and tree `41e29a392f56c68c71ae9f497d7d3d763dafdf1c` passed a fresh `build/task-8-final-e29e16a`: Debug 30/30 in 137.84 s and Release 30/30 in 72.51 s.
- Generic and legacy shader freshness, embedded-SPIR-V equality, and generated ABI freshness passed. The ABI definition SHA-256 is `4b15639690968cc21fd1b20dbf8deaca2f67cc9467019c83c9f258a1a1849067`; generic/legacy SPIR-V hashes are `f8eb622d9dd6add57158b346e9ff358feb1eb6d4f28995506260cb651f718326` and `18399de853a49861a156dc97e8108fe3c6d01e36686aa4a994389c04f42eee1c`.
- The RTX 5050 Laptop GPU honestly presented and was visually inspected at 960x540, render scale 1.0. The fresh standard 13-view route is complete with every strict and certified-recovery counter zero; its aggregate 12-sample median is 11.3547 ms and mean is 26.25054 ms.
- The 18 reward/stress views, including Task 7 regressions 114/115, required Task 8 views 116-119, and reachable-volume sweep 120-131, honestly presented and were visually inspected. Every strict failure/open-stack/mismatch/overflow counter is zero. Required-view medians are 12.3552 ms high, 12.6980 ms low, 12.13105 ms glass transmission, and 11.7491 ms motion extreme. The 12-view sweep medians span 11.6864-12.98305 ms.
- Exact PNG hashes, shader metrics, resource totals, every recovery count/reason mask, and the standard route hashes are tracked in `docs/evidence/TASK_8_REWARD_LANTERN_WINDOWS_CAPTURES_2026-08-28.json`. These short deterministic samples establish workload identity and bounded Host behavior; they are not a sustained-phone performance claim.

## Android build and package evidence

- A clean `assembleDebug assembleRelease lintDebug` passed in 1 min 35 s, and a separate `lintRelease` passed in 2 s. The package contains all four Android ABIs.
- Debug APK: 85,905,461 bytes, SHA-256 `2c2d41a54f1fb24a21e015a940ce5c12f936d2a370fa0d47de8346ce5635e66f`.
- Unsigned Release APK: 84,009,452 bytes, SHA-256 `65e342c9723cf6c847f898d119fb77d3f391bc455df612e0278996905655ae7f`.
- The held-item runtime asset/licence package contract, chest/lantern credit strings, ZIP alignment, and all twelve ELF `LOAD` segments at 16 KiB alignment passed.
- `adb devices -l` exposed no device. No install, APK pullback, runtime ASTC selection, honest phone RT presentation, touch/controller feel, Home/resume, performance, thermal, or phone visual evidence is claimed. No new Android-device evidence exists, so the compatibility record was not changed. Exact `SM-S948B` validation remains a hard Task 9 gate.

## Commits and remaining owner gates

1. `4849fb2` — `test: add reward interaction and command sequence contracts`
2. `381c060` — `feat: add reward interaction and finale authorities`
3. `3e714a2` — `feat: add deterministic reward lantern motion`
4. `95f49d0` — `fix: reset lantern motion across lifecycle seams`
5. `8f0c6b9` — `feat: render and control the claimed reward lantern`
6. `e1fde27` — `fix: preserve frozen lantern capture state`
7. `6dca5e4` — `test: pin reward transport identifiers`
8. `e29e16a` — `fix: harden certified lantern glass traversal`

No signing, version change, publication, upload, deployment, or Meshy generation occurred. Automated and deterministic Windows visual evidence does not replace owner interaction, carry-motion, art, audio/haptic, or exact-phone review.

Audio/haptic manual revalidation required: YES — delaying FinaleCompleted and appending chest/claim semantic events changes gameplay-event timing/traffic even though no new cue or haptic is planned; the exact candidate therefore needs an owner feedback pass.
