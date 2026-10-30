# Compact HUD: bounded physical-device check

Galaxy S26 Ultra **SM-S948B**, Android17/API37, effective560dpi, owner font1.7.
The already-built compact-HUD Debug APK `3c19e74b...` (194,997,073 bytes) was
installed temporarily after verified original APK/save/settings backup. This is
the exact APK from the earlier host layout checkpoint, not a newly built a088
runtime. No code, quality, personal preference or campaign policy was changed.

| Actual game capture | Observed placement |
| --- | --- |
| [Portrait,1440x3120](portrait-before.webp) | Lantern [852,2476,1048,2672], alongside Dodge; Parry/Swing in the lower row. |
| [Landscape,3120x1440](landscape-return.webp) | Lantern [2364,964,2560,1160], alongside Dodge; lower row below it. |

These are full actual phone screenshots, converted to lossless WebP with decoded
RGB identity checked. Original PNG hashes and all native accessibility rectangles
are in [the receipt](receipt.json); originals and logs remain local/ignored.
The translucent icon and full labels were visually inspected in both captures.
At3.5px/dp, lantern is56x56dp and current action targets80x56dp. No action
rectangles overlap; the dock leaves the lower-left movement area and broad
right-centre look area available. This does not establish thumb comfort or
sufficient primary-action prominence.

## Actual interaction and continuity

Fixture116 (`lantern-held-high`) was staged once in isolated test state. Normal
observed Resume/Continue UI exposed gameplay. Portrait Lower was tapped at the
verified icon: native pose0 ->1 ->2, contextual bits4 ->0 ->2, and accessible
label `LOWER. Lantern raised.` -> `RAISE. Lantern lowered.`.

Home, landscape rotation and a full surface replacement retained the same
process and lowered state, generation1 ->3, tick3718 retained across the rebind,
claimed chest4/light2 and reset/retry0. The Raise control returned through the
ordinary poll without restaging. Landscape Raise then produced pose2 ->3 ->0
and bits2 ->0 ->4. A750ms held/released right-centre look swipe outside every
recorded action rectangle changed the actual view without toggling the lantern.
No Swing/Parry/combat sequence was issued.

This successful bounded fixture check does not establish the cause/fix of the
original disappearing-control observation, a natural earned lantern journey,
all rotation directions, simultaneous movement/look, sustained performance,
subtitle/audio acceptance or owner thumb feel. A full portrait-return cycle was
not added inside the short window. Existing failures remain retained.

## Custody and release

Released **19:27:12.644 UTC**, ahead of19:29:30 deadline. Both original installed
APK hashes restored, all18 prior save/lock files and preference bytes unchanged,
original system settings (including auto-rotation and user rotation) restored,
no new save paths, no app PID/Horde rendering layer or owned install/capture/
transfer. Phone is safe to disconnect. No phone calls after release.

Sam's subsequent request is greater Swing/Parry prominence, followed by explicit
comparative mobile-game research before design. That research is being handled
separately. No merely enlarged-button follow-up was implemented or accepted.
Both current screenshot placement and this acceptance limit remain visible.

No merge, release, report transmission or phone performance claim. Audio/haptic
manual revalidation required: NO for this evidence-only checkpoint; existing
listening/feel acceptance remains open.
