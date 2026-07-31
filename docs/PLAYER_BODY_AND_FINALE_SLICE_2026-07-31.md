# Player Body and Finale Slice - 2026-07-31

Status: host-validated on Windows RTX and directly device-validated on exact model `SM-S948B` with the same clean Debug APK recorded below. Automated phone evidence covers installation, performance, the full checkpoint/replay/capture/lifecycle gate, live movement frames, the completed dawn UI, Continue, and Begin Again. Human judgment of the new gait feel remains separate.

## Scope

This slice substantially improves the procedural RT player body, gives walking a readable articulated gait, and turns the lich death into a complete ending:

- layered travelling coat with tapered chest and skirt, shoulder pieces, collar, belt, buckle, diagonal strap, and split tails;
- bevel-ended six-sided limb geometry in place of the former box-like shared limb;
- smoothed opposed stride, pelvis bob and sway, torso counter-twist, knee bend, foot lift, and heel-to-toe roll;
- sequential lich fall, physical skylight opening, 1.75-second moonlight-to-warm-dawn RT reveal, and a contextual ending;
- Continue in the Ruin, Begin Again, and Quit actions on Android and Windows.

No external art was imported, so `ASSET_LICENSES.md` did not require a change.

## Body implementation

The richer coat is generated inside the existing player-body BLAS. Stable primitive ranges let the ray-generation shader give brass, leather, and cloth sections distinct materials without adding textures or another runtime asset path.

The shared player-limb BLAS is now a compact bevel-ended capsule-like mesh. Existing TLAS instances articulate that mesh into:

- two upper arms and two forearms, still driven by the established two-bone grip IK;
- pelvis, two thighs, two shins, and two boots;
- the existing selectively visible head/reflection geometry.

The live pose is evaluated once per frame and applied consistently to the body axes, held grip, lower limbs, and reflected head. Movement drives a smoothed visual amount so starts and stops do not snap. Windows and Android use the same gait cadence.

First-person visibility remains deliberate:

- articulated arms, pelvis, legs, and boots use mask `0x04` and remain available to lower-screen primary rays and player-shadow queries;
- the detailed close-range coat uses mask `0x10`, keeping it in mirror/wet-surface reflection rays without putting the camera inside a near-field torso slab;
- the camera origin remains outside the body.

This preserves the established 18 TLAS instances and eight scene BLAS objects. It does not add a second active skinned enemy, another frame in flight, or a new per-frame TLAS ownership path.

## Walking model

`EvaluateLowerBodyPose` now produces:

- opposed left/right stride;
- double-step pelvis bob;
- lateral pelvis sway;
- counter-rotating torso twist;
- per-foot lift;
- per-knee bend;
- per-foot toe roll.

The gait is driven by actual resolved movement, not input alone. When collision prevents displacement the visual walk amount decays toward idle. Gameplay smoke tests cover idle, opposed limbs, foot lift, knee flexion, half-cycle opposition, and reset behavior.

## Ending flow

The ending is a shared gameplay state machine:

1. `LichFalling`
2. `SkylightOpening`
3. `DawnRevealed`
4. `Complete`

Delta time is consumed sequentially across phase boundaries, so a long frame cannot skip the physical roof or dawn transition. The existing `finale-roof` Debug checkpoint intentionally remains before completion, preserving the canonical twelve-checkpoint contract.

The ray-generation shader receives `finaleDawnReveal` and transitions both the miss sky and skylight radiance from cold moonlight to warm dawn while the frame continues through `vkCmdTraceRaysKHR` and honest swapchain presentation.

At `Complete`, Android and Windows pause gameplay and show:

- `DAWN RETURNS`
- `THE LAST LANTERN HAS DONE ITS WORK`
- short context explaining that the lich held the ruin in unending night;
- Continue in the Ruin;
- Begin Again;
- Quit.

Continue dismisses the ending and leaves the player in the completed ruin. Begin Again performs the normal full reset. Back on Android acts as Continue while the ending is visible. Debug F11 on Windows is an authoring shortcut to preview the completed finale; it does not add a checkpoint or alter release gameplay.

## Preserved renderer and gameplay guardrails

- Native Vulkan hardware ray tracing remains mandatory.
- Every captured scene frame is still dispatched with `vkCmdTraceRaysKHR` and copied through the swapchain.
- `rtScene.presented` semantics are unchanged.
- Presentation-format-driven red/blue output swapping is unchanged.
- Phone-safe `rayQueryEXT` shading remains inside raygen; no recursive phone pipeline was introduced.
- One frame remains in flight for the host-written held-prop instance data.
- Sequential skeleton/lich selection and the one-active-skinned-enemy limit remain unchanged.
- No block/dodge, broader AI, simultaneous enemies, or unrelated gameplay was added.

## Host validation

Windows:

- Debug build: passed.
- Release build: passed.
- Debug CTest: 7/7 passed.
- Release CTest: 7/7 passed.
- Fresh deterministic route capture: `reports/body-finale-windows-captures/run-20260731-175602`.
- Manifest: complete, 12/12 checkpoints, 144 timing samples, all captures `honestlyPresentedRtFrame: true`.
- GPU: `NVIDIA GeForce RTX 5050 Laptop GPU`.
- Overall median: `6.0548 ms`; overall mean: `6.506207 ms`.
- Embedded raygen SHA-256 in the capture manifest: `59BF3CE8503C772D96BDC65CEC70E6917090062DF5CBEF4170805A2E1FD8CDF5`.

Headed visual evidence:

- Walking/look-down view: `reports/body-finale-windows-live/run-20260731-175141/walking-look-down-accepted.png`
  - SHA-256: `576AECDB5344FDE1E1750EE9C0A996F88DEC3C6FF6B765292835D609CD10CD11`
  - verified articulated forearms framing the route without a near-camera torso slab.
- Completed ending: `reports/body-finale-windows-ending/run-20260731-173636/ending-overlay.png`
  - SHA-256: `A3C505C361BFFEE53ED0F93593D5B0E34DA240841A17D6758BE8C118C0682CD6`
  - verified warm RT dawn, ending copy, and Continue/Begin Again/Quit controls;
  - captured before the final body-only visibility/material refinement, which did not change the finale state, dawn lighting, or ending UI.
- The final Debug build was re-run after that refinement: the exact ending title and all three action labels were present, and Continue restored the normal title and `ENTER THE RUIN / RESUME` action.

Shader generation:

- `shaders/raytracing/minimal.rgen`: 35,775 bytes; SHA-256 `152819128A56F1CA31BD82ADEFCBD38A4A49EB55259D69D6E00D5605D714BB95`.
- `src/vulkan/raytracing/MinimalRayGenShader.inc`: 255,515 bytes; SHA-256 `59BF3CE8503C772D96BDC65CEC70E6917090062DF5CBEF4170805A2E1FD8CDF5`.

Android host build:

- `clean assembleDebug assembleRelease lintRelease`: passed across the four configured ABIs.
- Debug APK: 56,395,923 bytes; SHA-256 `A9FE35932548F38E067C39EF604739AAA7EACD12005D119A88C0B77E2D7FA81E`.
- Unsigned Release APK: 56,126,974 bytes; SHA-256 `A72A1B6624251126D1A98FBD2925DD73579B9359C5F0C818F8146CE4B268D733`.
- Release lint report: `android/app/build/reports/lint-results-release.html`.

## Android device validation

The exact Debug APK above was installed and validated on exact model `SM-S948B`, Android 16 / API 36. The device selected `RayTracingPipeline` on `Adreno (TM) 840`, exposed acceleration structures, ray-tracing pipeline, ray query, and buffer device address, and retained honest RT swapchain presentation.

Standard gate: `reports/android-showcase-runs/run-20260731-221231`

- Result: PASS with no recorded failures.
- APK SHA-256: `A9FE35932548F38E067C39EF604739AAA7EACD12005D119A88C0B77E2D7FA81E`.
- Five 75% medians of three 120-frame window averages:
  - opening `11.402 ms`;
  - worst bend `7.767 ms`;
  - skylight `7.096 ms`;
  - green `11.209 ms`;
  - lich `15.727 ms`.
- The ordered run began at 28.9 C / thermal status 0 and ended at 35.4 C / status 1. Every checkpoint remained below the runner's enforced 20 ms gate.
- The 13-waypoint collision replay completed in the finale with `presented: true` and one active skinned enemy.
- All 12 scene-only captures recorded 12 stable frames and `presented: true`.
- Home/resume recreated the surface and regained honest RT presentation.
- Capture manifest SHA-256: `1266D7EECA90A9223342FD7D8EDD476B70236B79F20099A82BF6CA4D5958D47D`.

Because the lich was measured last in the ordered run, it was repeated after cooling the phone to thermal status 0:

- `reports/android-showcase-runs/run-20260731-222749`: `13.723 ms` median at 36.3 C;
- `reports/android-showcase-runs/run-20260731-222831`: `13.600 ms` median at 36.7 C.

Both isolated runs retained presentation and passed the standard gate; the repeat also restored the older sub-13.7 ms reference. Timing remains CPU wall-clock through `vkQueuePresentKHR`, not Vulkan GPU timestamps.

Focused body/finale evidence: `reports/android-body-finale-runs/run-20260731-221525`

- A live deterministic replay state was captured at animation time `7.5708`, waypoint 6, with `presented: true`; two non-identical phone frames preserve the moving route and first-person body.
- A separate touch-driven look/walk screenshot verified the articulated first-person limbs remained readable without a near-camera torso slab.
- The `finale-roof` checkpoint completed three honestly presented 120-frame windows at `8.326 / 8.326 / 8.327 ms`, then normal simulation advanced through the RT dawn into the real Android ending.
- The accessibility hierarchy and screenshot verified `DAWN RETURNS`, the contextual lore, Continue in the Ruin, Begin Again, and Quit Demo.
- Ending screenshot SHA-256: `10407C3B9E3A7CF90DA7C297F37544E65CF50DEE12489AFC656E2087CDFD27F9`.
- Tapping the real Continue button dismissed the ending and restored active RT controls.
- After a clean activity restart, tapping the real Begin Again button reset to active play with 3/3 vitality.
- Repeated focused finale runs eventually raised the phone to 43.1 C / thermal status 3. This is retained as stress context, not attributed to the standard gate or to a specific timing window.

Evidence boundary: these are direct automated exact-device checks, including ADB-driven touch actions and headed screenshot inspection. They do not replace human judgment of gait feel, touch comfort, or perceived audio. The earlier owner-reported vitality/haptics/audio pass still applies only to its previously installed build.