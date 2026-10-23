# Sword/shield combat hierarchy prototype

Swing and Parry now read as the two primary actions: an authored ivory/bronze
sword and shield, circular 80/76 dp default targets and stronger rims over the
existing user-controlled translucent backing. Lantern remains 56 dp and visually
subordinate. Dodge retains its full label. This is a bounded prototype awaiting
phone/owner acceptance, not a blanket touch-HUD completion.

| Native component render | Bright background | Dark background |
| --- | --- | --- |
| Portrait 411 × 801 dp | [Bright](portrait-bright.png) | [Dark](portrait-dark.png) |
| Landscape 804 × 381 dp | [Bright](landscape-bright.png) | [Dark](landscape-dark.png) |

These are Robolectric API 34 native-Skia component renders at font scale 1.7, not RT game
or phone screenshots. Controlled Slate backing is used for all component buttons;
production Swing retains its existing darker Charcoal backing. Both variants were
visually inspected. The [prior compact layout's actual phone captures](../2026-10-23-compact-hud-phone/README.md)
remain distinct; the new prototype was never installed after the phone’s 19:27:12
release.

## Why this arrangement

The owner's request was greater primary-action prominence, followed by comparative
research rather than simple scaling. Parent research supplied official
[Call of Duty: Mobile controls](https://blog.activision.com/es/call-of-duty/2019-10/Getting-a-Grip-on-the-Call-of-Duty-Mobile-Controls)
and [Blades Arena](https://elderscrolls.bethesda.net/en-US/news/6k35P5RS5vdln7ZY1lZpqG/blades-arena-beginners-guide)
references. Distinct primary actions, subordinate utilities and explicit movement/
look separation informed this prototype. Sizes are our proposed Android dp values,
not measurements copied from those games. No attack mechanics or artwork is copied;
the two vectors are original source-authored Horde assets. GDC slides were not
available and no claim relies on them.

Portrait places shield up/left of the sword anchor, with lantern below the shield;
landscape uses a shallower combat arc and a bounded utility row. Narrow windows
retain a right-column fallback. Actual available parent height and safe top/bottom
insets bound short-window context placement. Normal phone targets prefer 72–80 dp
before the existing scale preference; constrained short windows fit the available
space without dropping below 48 dp. All actual View bounds are reserved, with no
invisible hit extension into the look lane or left movement half.

Primary faces use symbols; full localized Swing/Parry accessibility actions and
native enabled/pressed/focused feedback remain. The existing Controls/settings
preview retains full scaled text with the corresponding glyph. Glyphs are cached
until their resource/size changes. No new customization, preference write, gameplay
rule, listener, controller/keyboard mapping or motion animation was added.

## Checks and retained failures

- Full Debug build, all Android units and lint pass in 151 s: **419 discovered,
  413 pass, six existing Windows symbolic-link assumptions skip**, zero failures/
  errors. Lint has zero errors and 76 warnings; no threshold or exclusion changed.
- Focused native layout/opacity/safe-area/preview/input checks pass. Matrix covers
  320/411/804dp widths, portrait/landscape, font scales 1/1.7/2, compact and scales 85/100/110.
  Added 320 × 320 resizable cases include side/top/bottom insets and full context text.
- Actual Android ViewGroup dispatch uses the existing gameplay listeners. A look
  drag entering Swing does not attack; Swing released over Parry and the reverse
  keep their starting owner, exactly one edge each; cancellation releases pressure.
  This is host dispatch evidence, not JNI gameplay or physical multitouch acceptance.
- First-run fixture failure is retained: its post-style setup overwrote complete
  accessibility with empty visible icon text. The fixture now uses the full resource
  labels; the production label assertion was preserved.
- Read-only review found short-window Interact clipping. The new regression
  reproduces top=-12px before the fit correction; the 110 percent review witness was
  -34px. Both evidence and new regression remain; no bounds/overlap assertion was
  weakened. Prior fixed-grid alignment assertions were intentionally replaced by
  the new arc hierarchy while preserving/strengthening their safety intent.

[Exact source/APK/log/image identities and limitations](receipt.json). APK 200,205,652
bytes, SHA-256 `bed3f5b79998f2e341277eb8c7e59a36bc40346a800345d26acba2e0a481f858`.
This build includes current accepted source since the older phone APK; its size
change is not attributed solely to HUD code. It remains local and uninstalled.
No phone/performance, release/merge or owner comfort claim. Audio/haptic manual
revalidation required: NO; existing sound/feel acceptance remains separate.
