# Owner playtest follow-up, 25 October 2026

Baseline: frozen `f27982cd1b03a2cab9dc822d39b8ea038c721d9e` Debug APK on
SM-S948B, SHA-256
`53af984fe8ce7fe275e2ea523c9fe7e2ccc7c433997c6ff708635f1cafa8224e`.
This is development evidence, not release or Shipping performance acceptance.

Sam accepts the held guard animation and attack-to-held-guard return from his
actual use. Preserve these transitions. Orientation, damage and wider combat
acceptance are not inferred from that feedback.

## Reported issues and evidence boundary

- Early startup Load/Resume can open before the menu is ready and stop responding,
  including Cancel. This is now the first functional priority. Trace real
  readiness and modal/event ownership; a decorative overlay alone is insufficient.
- The surviving skeleton reportedly stops after the other dies. Investigate
  encounter engagement separately from transient attacker-token ownership.
- Full hearts can be intentional during safe combat teaching. Establish actual
  teaching and hit state before changing damage policy or calling mitigation broken.
- Yellow parry remains primarily a text warning in the reviewed gameplay frames.
  Improve readable visual shape/motion within the existing normal/yellow/red
  authority; do not introduce a new attack mechanic or substitute colour alone.

The owner-authorised recording is private and excluded from Git. Only its game
content is relevant. Original and copied file hashes matched; the original was
preserved. No recording-derived frame gaps are treated as engine frame times,
and no audio listening acceptance is claimed. Phone access ended at
10:37:25.915 UTC with the frozen candidate retained and no save/settings writes.

## Loading presentation authority

Read the existing `origin/docs/horde-1.6.2-roadmap` references at
`docs/design/ui/README.md` and `docs/design/world/connected/README.md`, and the
current [selected UI references](../../design/ui/README.md). The accepted rule
is a small spinner and Loading, no progress bar/percentage, artificial wait or
forced text recital. Any motion respects reduced motion. This boot-before-menu
gate does not add loading screens to ordinary rope travel.

The inspected `horde-loading-orientation-concept-v1.webp` is a flattened
two-orientation concept with baked title, bar and tip. Its documentation states
it is not a runtime asset or a final geographical map. Do not ship that board or
pretend its baked bar is real progress. A final spoiler-safe illustrated map with
deliberate portrait/landscape composition, matching the actual world, remains an
artwork requirement separate from the immediate readiness correction. Existing
native menu identity and a plain responsive loading treatment can support the
functional correction without fabricating that asset.

## Validation still required

The [encounter correction](encounter-correction.md) is committed as `bf2dee1c`:
755 shared assertions and the production route pass; its original failure and
safe-practice/actual-damage distinction are retained. The
[startup correction](startup-correction.md) is `ca1826d9`: 483 Android tests
(six existing skips), four-ABI Debug build, lint and 201 asset-policy cases pass.
The [truthful outcome correction](teaching-outcome-correction.md) is `9dfc319c`:
766 shared assertions pass, including real hit/parry and retained encounter cases.

Cold physical startup, real encounter/feedback and rotation/Home acceptance
still require a new phone allocation; host tests do not establish those passes.
Visual rings are now specifically in-world and reserved for special attacks,
not a larger fixed HUD icon. See the
[explicit RT seam](../../design/HORDE_1_7_ATTACK_RING_SEAM_2026_10_25.md).
Full normal block/yellow guard-break and four-skill practice/one-time revival are
approved directions with a [bounded next plan](../../design/HORDE_1_7_COMBAT_PRACTICE_BRIDGE_2026_10_25.md),
not implemented by these defect fixes. Current damage/guard animation is preserved.

No replacement APK is installed during this offline follow-up. No public media,
release, merge or phone reconnection is authorised here. Future approved combat
policy work must keep its own tested checkpoint and save/ownership evidence.
