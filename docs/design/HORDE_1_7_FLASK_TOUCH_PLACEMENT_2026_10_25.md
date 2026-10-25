# Flask touch placement: bounded feasibility, not admitted

The recording candidate remains frozen at f27982cd. This source-only review
made no code, layout, device, renderer or budget change. Flask remains OFF.

One additional 56 dp utility target can fit between the existing actions without
moving or shrinking combat, lantern, Dodge or Interact. Source equations reproduce
the retained 6dbc43fd portrait XML bounds at 3.5 px/dp. These are source-derived
rectangles, not fresh widget/device or thumb-comfort acceptance:

| HUD parent and safe inset | Proposed rectangle in logical dp |
| --- | --- |
| 320 x 568, bottom 48 | [172,372] to [228,428] |
| 640 x 320, right 48 | [356,248] to [412,304] |

The candidate has an 8 dp gap from existing controls. Landscape offers credible
lower-dock space. Portrait places it left of Parry and above the lantern, close
to the right-middle thumb look area. Bounds/non-overlap alone cannot accept it.
Shrinking to 48 dp does not resolve that concern. Current tests do not define an
authoritative reserved-look rectangle; they reserve movement and broadly constrain
control height. No new visibility or touch system is justified by this finding.

The smallest next option is a host-rendered review of this specific extra slot,
then owner reach/look review before any normal HUD admission. If the portrait slot
intercepts steering, a deliberately opened secondary utility tray is a concrete
alternative requiring an explicit interaction-design decision. It has more input
and lifecycle work and is not implicitly approved or implemented here. No existing
combat or lantern action should silently disappear to make room.

If Drink and Rest share a reviewed slot, the visible named action and surface
identity must latch at DOWN. Revalidate that same action and reject stale
eligibility; never substitute Drink for Rest or vice versa. Preserve pointer
ownership, cancellation, opacity and one edge per press. H/Up stays Drink and
J/Down stays Rest. UI adds no healing/hold timer; the shared simulation remains
authoritative. This describes a possible control contract, not implemented UI.

The separate [right-hand presentation proposal](HORDE_1_7_FLASK_PRESENTATION_SEAM_2026_10_25.md)
still needs review. Its one generic utility owner would change ordinary metadata
28 to 29, total TLAS storage 104 to 105, static assets 13 to 14 and ordinary BLAS
ceiling 21 to 22. Those changes remain unapplied under the explicit no-budget-
increase instruction. Primitive/material headroom and actual allocation/frame
cost must be established before admission. No spare compatible owner or honest
physical upgrade pickup has been established, and no existing sword/light owner
was sacrificed as a workaround.
