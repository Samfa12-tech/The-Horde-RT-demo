# 1.6.2 UI refresh: a restrained dark-fantasy interface

**Execution update, 7 October 2026:** Use [the selected central hanging-lantern direction](design/ui/README.md) in the active 1.6.2 tomb goal. First build one functioning real-RT scene with accessible native controls, flicker/sway, Settings/More pan-left, Play fade-to-black and reduced-motion navigation; stop for owner visual/cost feedback before expanding the style. Graphics remains a separate explicit live preview with exact acknowledgement and normal 15-second confirmation. Loading follows actual loading state and uses a small spinner only. No Continue/save-slot invention, dialogue control, optional tomb dressing or adventure scope. [Current progress and remaining gates](ENGINEERING_1_6_2_TOMB_FINISH.md) distinguish implemented work from these targets; older proposals below remain supporting detail.

**Owner queue update, 8 October 2026:** Complete native Android controller
navigation and recovery in portrait and both landscape directions is now part
of this existing tomb goal. Meaningful controller input suppresses touch chrome
while essential HUD/context still respects saved interface preferences; connection
and drift cannot take over. Real controls, visible/retained focus, native dialogs,
sliders and scrolling preserve Graphics Use/Keep/Restore acknowledgement. The
host implementation and 213-test run are recorded in the [controller ledger](ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md).
The exact physical phone/controller/rotation/large-font/cutout matrix remains
pending; no support certification follows from those synthetic checks.

**Planning date:** 2 October 2026  
**Planning snapshot status (2 October):** Proposed design and implementation handoff, requested by the owner; that planning snapshot claimed no implementation or acceptance. Current source and owner feedback are tracked by the execution records above.
**Milestone:** After 1.6.1 is completed, accepted, merged and released; before 1.7 expansion. See [ROADMAP](ROADMAP.md#16-2--engine-readiness-and-demo-polish).

## Recommendation

Make the interface feel like equipment carried into the tomb: dark iron edges, worn leather/slate surfaces, pale readable lettering and small aged-brass/lantern-amber highlights. Keep ornament at the edges, not over the world or behind labels. Give the existing controls a deliberate shape, hierarchy and responsive state language rather than surrounding every control with a heavy fantasy frame.

**Transparency already exists.** Preserve it. The improvement is better use of space, readability, thumb reach and consistent materials; adjustable opacity is a possible extension of the current implementation, not a claim that translucency is new. A dark-fantasy theme must not make the controls darker, smaller or harder to understand.

This pulls the existing-demo HUD/menu portion of the older [1.7 UI brief](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#10-on-theme-touch-controls-hud-and-menus) forward. 1.7 reuses the accepted system and adds its own traversal, dialogue/subtitles and chapter-specific surfaces.

## Evidence and limits

This is a source-grounded design proposal with one supplied screenshot reference, **not a completed live UX/accessibility audit**.

- Inspected the owner's supplied PC image `image(8).png` from 2 October: a playable opening-room view with a long dark technical-status strip at the upper left and a separate centred vitality box. The game world remains the main view, but the technical strip has substantial visual weight relative to its everyday gameplay value. The image does not show entry/pause menus or Android buttons; its executable/settings identity is unverified.
- Android source at engineering checkpoint [9931cf18](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/9931cf18fc36369ca6efbe2b785d15f877bf6d31): [layout](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/9931cf18fc36369ca6efbe2b785d15f877bf6d31/android/app/src/main/res/layout/activity_main.xml) and [MainActivity](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/9931cf18fc36369ca6efbe2b785d15f877bf6d31/android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java). The HUD uses translucent `#B30D0B09` backings; menu and combat buttons also have alpha-bearing fills. Existing menus already use amber borders and serif headings. Build on that foundation.
- The XML has 104×72 dp Swing/Parry rectangles at the lower right, with 128×60 dp contextual interaction/light controls above; Menu is 72×48 dp. These are source dimensions, not a measurement of physical thumb comfort. Contextual controls already appear according to gameplay state. Preserve that useful behaviour.
- Inspected Settings includes Music/SFX, sensitivity, render scale, water quality, haptics, Show HUD and reset. No exposed opacity/layout editor was found in that inspected method. Do not confuse existing translucency with an existing adjustable-opacity preference. Re-audit the final released configuration, theme resources and both platforms before adding any setting.
- Windows [DiagnosticWindow.cpp](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/9931cf18fc36369ca6efbe2b785d15f877bf6d31/src/platform/windows/DiagnosticWindow.cpp) supplies the status/vitality text, native controls, DPI layout and menus. Preserve platform-native behaviour rather than importing a browser UI framework.
- The [Graphics plan](ROADMAP.md#graphics-menu-and-measured-quality-choices) remains authority for settings semantics, real renderer choices and measurements. This document owns presentation and navigation, not a second graphics configuration system.

**Planning snapshot limits (2 October):** fresh Android HUD and menus, Windows menus, pressed/disabled/focus states, controller navigation, physical touch reach, screen-reader behaviour, lifecycle transitions and performance had not been inspected. The active tomb goal now records subsequent exact-build evidence and owner feedback; remaining physical/accessibility/performance gates stay explicit. Old screenshots and source-only dimensions are not current visual passes.

## 1. Visual language and hierarchy

### Materials and typography

- Thin etched-iron outline with a restrained brass inner highlight; very low-frequency leather/slate texture only on larger menu panels. Avoid rust/noise across small controls.
- Proposed starting palette: charcoal `#151719`, elevated slate `#24272A`, parchment text `#F2E9D8`, muted text `#C9C4B8`, brass `#CFA96A`, danger `#D96F65`. These are provisional art tokens, not certified contrast pairs; test composited results.
- Retain a restrained serif for large headings and a clear licensed sans-serif for labels, values and help. No blackletter at small sizes, no invented runic labels.
- One consistent family of crisp scalable silhouettes: sword for Swing, shield/blade for Parry, hand for Interact, recognisable lantern for Raise/Lower. Keep short real-text labels; do not require memorising icons or change the accepted lantern identity.
- A small etched corner motif can connect menus to the game's architecture. No ornamental full-screen frame, perpetual particles, flickering labels or constantly pulsing gold.

### Gameplay HUD

Keep vitality compact, near an edge, using three distinct segments plus readable current/maximum text. Do not invent stamina or mana. Minimise routine RT/version status into a small optional/collapsible status affordance after startup; keep failure, unsupported-device and diagnostic states truthful and discoverable. Do not hide a meaningful runtime failure to achieve a cleaner screenshot.

Protect the centre and lower-middle playfield. Keep attack cues, chest prompts and held items visible. No permanent objective panel or journal added without gameplay need. Reuse contextual interaction/lantern controls with stable slots; changing labels must not cause another action to jump under a held finger.

## 2. Hands-first controls

**Default behaviour stays familiar:** left-side movement and right-side look; existing swing, press-down parry, dodge and contextual action semantics remain unchanged. This is not permission for tap-to-move, new combat timing or a mandatory fixed joystick.

- Replace the visually heavy block arrangement with an intentional lower-right thumb arc: Swing as the main control, Parry immediately adjacent and clearly distinct, contextual action/light controls just above. Prototype against the existing layout before selecting exact coordinates. Preserve access to right-side look without forcing awkward reaches.
- Use approximately 64–72 dp primary-action hit areas as the initial prototype target, never below 48×48 dp for interactive controls. Current large rectangles are not automatically too large; choose final dimensions from actual touch trials. Visible medallions can be smaller than hit areas; decorative corners must not steal touches.
- Start with at least 8 dp between non-overlapping hit regions, then verify accidental activation and slide-off behaviour. Honour cutouts, system-gesture insets and display orientation. Do not place Parry on a screen edge just to free visual space.
- Offer Compact/Comfortable presentation, a bounded control scale, and a preview/reset for this UI group. Audit existing settings first, reuse keys where applicable and preserve users' saved choices.
- Evaluate an **opt-in** mirrored left-handed arrangement after default-layout validation. Mirror the action cluster and move/look ownership together, with explicit preview/help; do not merely put action buttons on top of the existing left movement zone. Default mapping does not change. If safe coexistence cannot be proved, defer this preset rather than ship conflicting input regions.
- Defer a free drag-to-position editor. Bounded presets, scale and safe-area handling cover this slice without creating overlapping or unreachable controls.

### Transparency and feedback

Retain the current translucent presentation. If adding opacity adjustment, change backing/ornament opacity separately from label/icon/focus readability. Preview it against bright stone/flame and near-black rooms; offer a stronger-contrast backing. Never fade the whole control until its label is illegible. Transparency must not change hit testing, release pointer ownership or pass a consumed touch into camera look.

Use immediate press feedback: a small inset/edge change and brighter symbol, not a delayed flourish. Selected and focused states have a visible outline; unavailable controls retain readable shape/label and explain why where necessary. Colour is supplementary. No animation may delay parry or create a second action on click after down-edge activation. Decorative transitions are brief and disabled by reduced-motion preference.

## 3. Menus as part of the game

Use the same iron/brass framing and text hierarchy across entry/main, pause, Settings, controls/help, death/retry, ending, credits and the outer RT Lab frame. A restrained static scrim is enough; **no expensive live blur**.

- Entry: clear Start/Continue where applicable, then Settings and secondary choices.
- Pause: Resume first, with Settings/Controls easy to reach; destructive restart/quit remains visually separated and retains its existing confirmation behaviour.
- Settings: Graphics, Audio, Controls and Interface/Accessibility sections. Integrate the roadmap's actual Graphics contract: supported presets/Custom, effective values, tradeoffs, apply/revert and unavailable reasons. Never display attractive but nonfunctional toggles.
- Preserve separate existing Music/SFX controls and their real values. Dialogue/subtitle systems remain 1.7 dependencies; do not expose dead controls now.
- Preserve reports, consent/preview/cancel, benchmark, updates, More by Samfa12, diagnostics, provenance and error routes. Dense technical reports need readable structure, not ornamental redesign. System file pickers remain native.
- Keyboard, mouse and supported controllers get predictable focus order, visible focus, sliders that can be adjusted without a pointer, confirm/back and scrolling. Android Back and Windows Escape restore the correct prior surface. Do not hide contextual actions when touch chrome is suppressed for another input device.
- Opening a menu gates game/camera input; resuming clears stale held actions. Preserve single-surface ownership so finale polling cannot replace Settings, RT Lab or report screens.

## 4. Accessibility and implementation budget

Target 4.5:1 normal text and 3:1 large text/meaningful control boundaries against their **actual composite background**. Test contrast in motion as well as stills; provide stronger backing when translucent defaults cannot maintain it. Never claim compliance from a mockup.

Retain real labels, accessibility names/roles/value announcements and platform text scaling. Test Android font scales 1.0, 1.3, 1.7 and 2.0, and Windows DPI 100%, 150% and 200%. Reflow or scroll instead of clipping or shrinking labels indefinitely. No changes to the user's system font settings.

Share design tokens/specification and a small icon family across existing Android/Windows native implementations. Extract focused helpers only where needed; no new UI engine, web overlay or renderer rewrite. Conventional 2D controls are allowed and do not substitute for the RT world.

**Proposed initial limits, to measure rather than claim as achieved:** one reusable icon/ornament atlas of at most 1024×1024 RGBA-equivalent (4 MiB decoded base level; account separately for mips, fonts, copies and retained caches); no continuous scene readback, full-screen blur, scene relighting or per-frame texture upload. Prefer vector/native drawing for simple frames and icons. Record actual total UI memory and CPU/GPU cost.

Use matched baseline/candidate runs to isolate idle HUD, active multi-touch and open-menu costs. Aim for no sustained median/p95 frame-time regression beyond measured run-to-run noise; treat any repeatable increase above 0.5 ms as a review trigger, not an automatic allowance. Existing frame pacing problems do not excuse extra UI cost. Do not pay for ornament by reducing scene resolution/quality. Simplify ornament if the budget is not met.

## 5. Artwork decision

**Image generation is optional, not required.** Native shapes and cleaned vector icons can deliver the core refresh. If the basic direction still lacks identity after a small real-UI prototype, separately propose one small reference/ornament sheet for approval. Generated material is reference or edge decoration only, with recorded rights/source and a cleaned runtime derivative. No baked text, generated screenshot presented as working UI, large background image covering the world, or new paid work implied by this plan.

## 6. Implementation sequence and acceptance

1. **Baseline capture and inventory.** After the release gate, pin the actual accepted build/source and configuration. Capture current phone/PC gameplay, entry, pause, Settings, controls, death/retry, ending and reporting/error surfaces. Record current transparency, HUD settings, input ownership and focus behaviour. Mark unavailable states explicitly.
2. **Tokens and a small real-UI slice.** Implement shared visual specification through existing native layers: one action cluster, vitality, pause and one Graphics settings page. Review matching before/after captures on actual devices before applying decoration everywhere. No generated board counts as acceptance.
3. **Complete theme and bounded customisation.** Extend accepted styling across the existing menu inventory; add only audited-missing opacity/scale/preset options, persistence and scoped reset. Integrate the Graphics workstream rather than duplicating it. Evaluate mirrored layout separately.
4. **Input/accessibility/performance gate.** Automated semantic tests plus live multi-touch, desktop navigation and composited screenshots. Fix regressions; record accepted/deferred options and measurements.
5. **Integrated 1.6.2 review.** Recheck with the other graphics/demo-polish slices and preserve the accepted configuration as 1.7's starting UI.

Required evidence/checks:

| Area | Acceptance evidence |
|---|---|
| Visual | UI-on before/after at matching camera/settings in dark tomb, bright flame/stone, lantern close-up and busy combat. Capture default/minimum/maximum opacity and Compact/Comfortable variants; centre/lower-middle remain clear. |
| Screens | Entry, pause, Graphics, Audio, Controls/Interface, death/retry, ending, credits, report/consent and error states; explicitly label any unavailable capture. |
| Touch | Move + look + Swing/Parry concurrently; pointer handoff, rapid alternating actions, slide off, cancellation, extra finger, menu opening and resume. No lost/stuck/duplicate actions, accidental neighbour hits or look leakage. Check actual comfortable thumb reach with the owner. |
| Context | Chest locked/opening/claim and lantern Raise/Lower remain truthful and stable; no control moves beneath a held finger. |
| Layout/accessibility | Supported aspect ratios/orientations, safe areas, font/DPI scales, real labels and value announcements, contrast, reduced motion and non-colour-only state cues. |
| Desktop | Mouse, keyboard and supported controller navigation, focus return, Back/Escape, sliders, scrolling, mixed-input transitions and resizing. |
| Settings/lifecycle | Existing values migrate intact; explicit UI reset is scoped; relaunch, retry, pause, Home/resume and interrupted apply/revert preserve usable state. Graphics UI displays real effective settings. |
| Cost | Exact candidate/build, hardware/backend, resolution/settings, thermal/load context, actual UI memory and matched sustained CPU/GPU/pacing evidence. Missing counters/devices remain gaps. |

Use the exact Android/Windows targets required by the accepted release matrix, including the Galaxy S26 Ultra and Windows RTX target; do not infer S24/S25 behaviour from S26 or desktop. Keep scene-only captures distinct from UI-on evidence.

**Completion:** controls and menus visibly belong to Horde, the player can see and control the game at least as well as before, settings are truthful, and evidence supports the result. A pretty screenshot without input/lifecycle checks is incomplete.

**Current change boundary:** documentation only; no code, generation, spending, merge or release.  
**Audio/haptic manual revalidation required: NO for this document.** Later pure decoration keeps that boundary; changed feedback/input/audio behaviour requires the applicable manual checks.
