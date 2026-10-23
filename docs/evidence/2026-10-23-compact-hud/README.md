# Compact deliberate-action dock

The owner identified the lantern as intercepting the steering/look thumb. Parent
review of actual `b08aef65` phone captures confirmed that portrait put it in the
movement half and landscape left it above the action cluster in the look area.
This checkpoint groups deliberate actions at the lower right and corrects the
window-to-HUD inset conversion. It changes no input listener, action timing,
dialogue, controller/keyboard mapping, saved setting or campaign state.

## Placement and coordinate basis

Default primary targets change from 104 × 72 to **80 × 56 dp**, compact from
88 × 64 to **72 × 52 dp**, retaining saved scale 85–110% and a 48 dp minimum.
The **56 × 56 dp** circular lantern sits next to Dodge above Parry/Swing.
Spacing is 8 dp, right edge 12 dp and bottom 16 dp plus residual safe insets.
Interact retains its context label/padding and reserved row above the dock; hiding
it never moves combat actions. At narrow widths and large fonts, complete primary
labels cause a single-column primary arrangement, with lantern beside Dodge.
Label widths use the existing 10 sp autosize minimum; the controls preview retains
its existing nonshrinking 17 sp accessibility presentation and new action order.

`HudSafeArea` intersects the window safe rectangle with the actual HUD parent in
screen coordinates. A cutout/status inset already excluded by the parent is not
subtracted again. Gameplay controls, top anchors and RT status use this same
residual basis, refreshed after root/inset changes. Debug logs record changes as
`HordeHudLayout parent=... insets=...`. This addresses the demonstrated coordinate
ambiguity; its physical rotation result is **not yet verified**.

## Evidence and checks

These are **Robolectric API34 native-Skia component renders**, 1.7 system font,
default control preferences and no physical safe inset. The striped background
is deliberately synthetic. They are not running-game or phone screenshots.

| Orientation | Candidate component |
| --- | --- |
| Portrait, 411 × 801 dp | [Native layout](component-411x801.png) |
| Landscape, 804 × 381 dp | [Native layout](component-804x381.png) |

Both were visually inspected. The new matrix checks 320/411/804 dp widths, font
1/1.7/2, compact on/off and scale85/100/110: at least48 dp targets, complete
one-line primary labels, right-half ownership, bounded dock and lantern adjacency.
The inset fixture checks actual prior phone parent rectangles, already-excluded
cutouts, residual navigation insets and portrait-return stability. Existing
context/layout, caption, opacity/state and input/lifecycle tests remain.

Full Android unit/build/lint checkpoint: **417 discovered, 411 pass, six existing
Windows symbolic-link assumptions skip, no failures/errors**, 58 s combined.
After an evidence-export-only test change, the component matrix rerun passes in
10 s. Ordinary default-ABI Debug APK is194,997,073 bytes, SHA-256
`3c19e74b1e34369e3208068e4c9dc19337d3d4298d0ef2365a09ed0314a4ded3`.
No native runtime, shader, asset or quality-setting change is included.

Retained iterations: changing Interact padding wrapped its label; it was restored.
A narrow high-font Swing label then failed; a label-width floor and bounded
stacked arrangement corrected that failure. No wrapping, overlap or minimum-size
assertion was weakened. [Receipt](receipt.json) retains log/source/image hashes;
local full logs and binaries stay ignored under `reports/` and `android/app/build`.

## Actual phone baseline and device gap

The actual earlier `b08aef65` portrait and landscape full-rebind captures are in
Library as `libfile_535d15ea60f4819183e80cd12241dcfd` and
`libfile_da91d0a1910481919f5128dd5cc677dc`. Parent visually reviewed them.
[Their exact bounds](before-phone-bounds.json) remain distinct from component
renders. At560 dpi the portrait-return lantern was [502,2224,698,2420] on a
1440-wide screen, and landscape [2350,376,2546,572] above the other actions.

That baseline batch staged fixture116 **once**, then rotated the same process and
resumed through full surface rebind in both orientations without restaging.
Native claimed chest/light/pose and Java contextual eligibility recovered each
time. It did **not reproduce** the original disappearing-control failure and
does not prove naturally earned equipment or close that retained observation.
All18 prior save/lock files, preference bytes, original APK hashes and rotation
were verified restored; release was10:15:24UTC, with no owned renderer/transfer.

For this candidate, fresh ADB first verified SM-S948B, no Debug PID and launcher
foreground. Before the first snapshot, the next command returned
`adb.exe: device 'R5GL219SZGK' not found`. The model-read guard failed before any
snapshot directory, installation, setting write, launch, input or transfer.
**No candidate phone operation started.** Actual after phone captures,
rotation/inset continuity, caption/Interact clearance in moving play and owner
thumb comfort remain pending. Do not infer them from automated nonoverlap.

## Follow-up

Finish terminal exact-head CI for the separately backed-up Windows selection
checkpoint, preserving the15-minute gate. Next approved feature/performance slice:
one lower-detail world-body candidate with first-person arms unchanged, followed
independently by Kit with face/silhouette preserved. Originals, deterministic
pose/ownership and actual look-down/shadow/reflection evidence are required;
geometry is not assumed to explain measured RT dispatch cost. Deferred forest,
parked chimney, pending recovery policy, PR conflicts and release gates stay open.
