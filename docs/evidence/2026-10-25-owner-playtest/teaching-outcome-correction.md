# Teach the result that actually occurred

After the encounter fix, an independent focused reproduction found that an
ordinary sword-hit stagger published `Learned`, displayed as “Well parried”.
It did not award a learned flag, but the feedback was false. Conversely a real
parry set its learned flag before prompt-opacity calculation and hid its own
success feedback.

`CombatTeaching` now requires the existing `CombatReaction::Parried` on the
actual stagger before publishing that success cue. Its existing stagger retains
the cue long enough to read; there is no new timer, event or animation. The cue
ends with that reaction and is suppressed by disabled/skipped teaching. It does
not extend safe practice, change damage, widen a defense window or alter the
owner-accepted guard/attack transitions.

The existing CombatFoundation gate first fails 7 of 764 assertions, including a
real ordinary sword hit and a real timed parry/riposte. Corrected result is
**766 assertions, zero failures, 8.54 s**, unchanged 30-second deadline. It also
checks source-reaction retention through the existing stagger and clearing on a
later ordinary hit; the room-edge encounter regression remains passing.
Original/corrected short logs are retained beside this note.

The startup APK identified in `startup-receipt.json` predates this separate
native correction and is not relabelled. Exact-head hosted Android compilation
and later physical feedback still need their own evidence. No phone/foreground
operation or save/quality change was made.

Audio/haptic manual revalidation required: **NO**; this changes truthful teaching
copy/visibility only, with no event, source, gain or playback change. The larger
four-skill practice, new combat-rule direction and one-time revival are distinct
unfinished work, not claimed complete by this correction.
