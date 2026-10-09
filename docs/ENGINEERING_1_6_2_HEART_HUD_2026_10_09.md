# Heart-only health HUD — 9 October 2026

Owner request: remove the visible `VITALITY X/3` caption, keep full red hearts and transparent empty hearts, and allow the presentation to grow with future maximum health.

Android and Windows now draw only hearts. Lost points retain an outline with no fill. Accessible current/max text remains available; Android publishes both values in one atomic read from the shared simulation snapshot. The icon count follows `maxVitality`, wrapping into additional rows when necessary. Current health, damage, death and retry rules remain unchanged at three points; no upgrade or healing mechanic is added.

## Exact subject

- Runtime source: `7766ea3a992e2e758c5edaaee1b1d753d660c987`; tree: `3710e061fc1ee53099e22fec8465dcf18325d72e`.
- Four-ABI Debug 516f1ace49dedab29ed266a0b1b7ef92e9bcbbc855818be81ce36a492dabb748: `516f1ace49dedab29ed266a0b1b7ef92e9bcbbc855818be81ce36a492dabb748`, 138462962 bytes. Installed-base pullback matches on SM-S948B / Android 16.
- Windows Debug 04ded713f1240e66e0aeb86b88624b20c81afdbafb08b93795f377149751884f: `04ded713f1240e66e0aeb86b88624b20c81afdbafb08b93795f377149751884f`.
- All 94 runtime assets are byte-identical to the accepted grate package.

## Checks and inspection

All 243 Android tests in 38 classes pass, with no failures/errors/skips; lint and four-ABI Debug build pass. Existing heart tests now check red full interiors, transparent lost-point interiors, visible outlines, seven-heart wrapping, changing maximum at unchanged current health, and accessible current/max text. Windows Debug compilation and existing simulation-gameplay, showcase-gameplay and desktop-controller host tests pass (3/3).

Normal Play on the exact phone package shows three red hearts without visible text. The owned native hierarchy reports empty visible HUD text and accessible `Vitality 3 of 3`. The image below is a plain pixel crop of the retained 1440×3120 screenshot, not a mockup. [Sanitized artifact/evidence receipt](evidence/2026-10-09-heart-hud/receipt.json).

![Actual phone HUD](evidence/2026-10-09-heart-hud/phone-hearts-crop.png)

Saved game settings and the accepted menu mix are byte-identical before/after. Android's ActivityThread IDS launch bookkeeping changes separately; it is not a game preference. The owned app is stopped after inspection.

## Limits

Hollow damage states and future larger maxima are automated bitmap/HUD evidence; this brief phone check does not repeat physical damage/death, orientation/controller or spoken-screen-reader acceptance. Windows appearance was compiled, not physically inspected for this checkpoint. Earlier accepted route, combat, equipment, controller, audio and grate evidence retains its original source/package identity.

Audio/haptic manual revalidation required: **NO** — only HUD presentation and read-only current/max publication change. No gameplay events, playback or haptic semantics change. Sustained 30 FPS, matched 50/40/33 comparisons, remaining final-candidate gates and Eric's independent audit remain open; this is not release approval.

Current runtime CI at this evidence checkpoint: 10 success, 2 in_progress across 12 jobs. Documentation CI after the evidence commit is a separate source head.
