# Touch HUD backing checkpoint

23 October 2026. Existing WP2 worktree, source base `4beff836`; implementation
and this receipt are committed together locally. No phone use or public push.

## Verified starting state

- Visible Skip line was already removed. The actual dialogue panel contains
  only its scrollable caption content; the nine caption tests retain that gate.
  Dialogue timing, tutorial recovery and accessibility settings are unchanged.
- The lantern already used a 32 dp raise/lower vector icon inside a 56 dp target,
  with an accessible action plus raised/lowered state. Those semantics remain.
- Gameplay controls still used the opaque iron plaque. Although the face was
  passed the saved opacity, its solid lower/rim plate covered the world beneath.
  A native pixel control confirms that old centre remained alpha 255 at 70%.

## Change

`TouchButtonDrawable` supplies one reusable, stateful native backing for gameplay
and the paused controls preview. It has **no opaque underlay**: the face actually
honours the saved 55-95% opacity. Default 70%, stronger backing 95%, preference
keys and saved values are unchanged. Text stays opaque, with distinct pressed,
disabled, focused/selected and activated treatments. There is no animation.

The lantern backing is circular. Swing/parry/dodge/context controls keep their
readable action text and use 16 dp rounded corners. They are not new icon-only
combat circles: long rope/chest instructions still need their text box. Native
View bounds, safe margins, listeners, keyboard/controller mapping and the
existing press/cancel/accessibility-click semantics are unchanged. Transparent
corners retain the original rectangular hit area, at least 48 dp.

Menus retain their established plaque style. No scene, asset, shader, save,
lighting or deferred forest work is changed.

## Before/after evidence

These are **emulated native Android component renders**, Robolectric API 34 with
native Skia, 1.7 system font, explicit safe margins. They compare the retained
old production plaque factory with the new gameplay factory using identical
actual XML/native layout targets and labels. The striped backdrop is synthetic
and deliberately exposes transparency. Unconfigured startup/menu chrome is
hidden. They are not running-game/phone screenshots or new RT presentation proof.

| Layout | Before | After |
| --- | --- | --- |
| Portrait 360 x 640 | [Old backing](before-360x640.png) | [New backing](after-360x640.png) |
| Landscape 800 x 360 | [Old backing](before-800x360.png) | [New backing](after-800x360.png) |

All four final images were visually inspected. The label boxes and touch target
rectangles match exactly. Earlier full semicircular text caps left little backing
behind long words. A 12 dp padding trial then wrapped the interaction label and
failed the new full-label assertion; it was withdrawn. Final text padding/width
is unchanged, with gentler corners. The [failure log](withdrawn-padding-failure.txt)
and earlier local images remain; no existing gate was weakened.

## Checks and build

- Six-class focused selection passes: **31 tests**, including native pixel/state
  and bounds/action checks, existing interface font/compact/scale matrix,
  contextual layout, captions, lantern state and touch cancellation. Saved
  opacity centres at 55/70/95%, clear corners, focus/press/disable/raised state
  differences and opaque disabled labels are checked.
- Final component-only fixture rerun passes all three tests after hiding
  unconfigured chrome. This changes no production code.
- Ordinary **arm64 Debug** APK builds offline in 35 s; no profiling overrides.
  APK 178,942,282 bytes, SHA-256
  `8be1c9272b2ce840083904f25f13b61e85cc13c5cf890920b3a1883f28b77228`.
  Build only: not installed or physically accepted. The earlier production-stage
  APK remains frozen separately and is not replaced as evidence.

[Receipt and canonical source/image hashes](receipt.json),
[focused output](focused-tests.txt), [final component output](component-tests.txt)
and [build output](assemble-debug.txt). Binary APK stays ignored. No game FPS,
full-HUD visual acceptance or actual-device contrast/touch claim is made.
Audio/haptic manual revalidation required: **NO**; those inputs and paths did
not change.

## Optional reference blocker and next step

Read-only `git status`/`git grep` against
`C:\Users\sam_s\Documents\Briarhold` returned filesystem **Permission denied**.
The optional reference was not read; no retry or alternate access was attempted.
This did not block the independent Horde change and is not claimed as a verified
Briarhold match.

Next acceptance: inspect the backing/labels over actual moving RT scenes in both
orientations and check touch feel/contrast on a phone at owner settings. Existing
stronger backing remains available. The phone stays released for this checkpoint;
no renderer, install, capture or transfer was started. This local checkpoint
awaits later backup authorization; the explicit GitHub grant ended at `9bd46a8f`.
