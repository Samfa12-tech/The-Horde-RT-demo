# Combat and ASTC Phone Validation - 2026-07-14

## Outcome

The exact combat/ASTC debug build passed its primary-target runtime and performance gate on the Samsung `SM-S948B`.

- Fresh uninstall/install succeeded; the first launch was cold and completed in 480 ms.
- The Adreno 840 selected `RayTracingPipeline` at Vulkan 1.4.295 and dispatched at `1440x2812`.
- Logcat reported `PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)`.
- Logcat reported `RT frame reached Android swapchain presentation.`
- The app-private runtime directory contained the skeleton plus the three expected KTX2 arrays and no raw `.rgba` material arrays.
- The capability report honestly recorded `RT scene status: Presented via swapchain` and `RT scene presented: yes`; the unimplemented in-app FPS field remains `N/A`.

## Performance gate

SurfaceFlinger presentation timestamps were cleared after the live Horde `SurfaceView` existed. Statistics use 127 consecutive actual-present timestamps, yielding exactly 126 intervals.

- First sample: 12.500 ms median, 16.667 ms p95, 14.087 ms average, approximately 80.0 FPS median.
- Second warm sample at Android thermal status 2: 12.500 ms median, 16.667 ms p95, 13.690 ms average, approximately 80.0 FPS median.
- Result: pass. Both samples remain comfortably above the 50 FPS median gate.

The phone began the validation session at thermal status 0, but repeated launch, capture, and input work raised it to status 2 by the end of the first measured sample. The second status-2 result is retained as useful warm evidence rather than presenting the run as perfectly thermally cold.

## Controls and visual evidence

- Scripted left-side movement, right-side look, and `SWING` input left the same native process alive with no renderer or Android runtime error.
- The swing capture visibly shows the sword moving independently of the torch.
- The current Android exposure preserves dark corridor depth, warm fire, readable PBR floor texture, and a warm rather than cyan presentation.
- At the device's accessibility font scale of 1.7, the informational HUD occupies a large part of the upper screen. Treat a compact/collapsible HUD presentation as part of the next readability pass; do not change the user's system font setting.

Evidence:

- `docs/validation/horde_combat_astc_phone_2026-07-14.png`
- `docs/validation/horde_swing_phone_2026-07-14.png`
- `docs/validation/horde_enemy_dead_phone_2026-07-14.png`

## Decision

The compressed material and one-enemy combat foundation is cleared for a focused feel/readability pass. Tune the existing HUD, sword timing/reach, enemy attack telegraph, hit/death readability, and damage feedback before adding another enemy, wider AI, block/dodge, or the textured sword LOD.
