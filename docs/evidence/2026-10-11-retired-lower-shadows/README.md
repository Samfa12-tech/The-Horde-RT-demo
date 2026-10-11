# Retire Lower shadows — issue 23

The owner requested removal of the broken Lower tier in
[issue 23](https://github.com/Samfa12-tech/The-Horde-RT-demo/issues/23).
This supersedes the original intake's proposal to repair and compare all three
tiers. The released 1.6.2 artifacts and historical reports are unchanged.
The original Android blackout and its physical cause are not newly certified
here; neither the owner's preview FPS readings nor this change establish a
sustained performance benefit.

## Behavior

- Windows and Android expose **Current / Higher** only. Reduced Effects uses
  Current. Fire Low remains independently available.
- Numeric identities remain Current=1 / Higher=2. Saved confirmed and interrupted
  Lower=0 settings recover as Current, preserving the rest of the settings tuple
  and existing schema migrations. Recovery reads do not rewrite preferences;
  explicit confirmation writes the supported value.
- Live edits, persistence writes, Android presented acknowledgements and native
  GPU-quality admission reject 0 and unknown modes. Historical telemetry can
  still describe old Lower evidence; it cannot select the retired render path.
- The shader's centre-sample Lower branch is removed. Current's authored spatial
  alternation, Higher's sample counts, secondary visibility, transport, light
  energy and all frozen budgets remain unchanged.
- A pre-existing isolated preview-capture mismatch was reproduced: the saved
  default requested Low dust while the initial renderer intent was Off. Initial
  dust intent now derives from that same saved tuple. No capture assertion was
  bypassed, and normal saved preferences retain their ordinary load path.

## Inspected Windows hardware RT

Both supported backends completed all 17 frozen preview captures and ordinary
Apply/Revert/Keep transactions. These four images were opened and inspected:

| Backend | Current | Higher |
|---|---|---|
| Pipeline | ![Pipeline Current](pipeline-current.png) | ![Pipeline Higher](pipeline-higher.png) |
| QueryCompute | ![Query Current](query-current.png) | ![Query Higher](query-higher.png) |

The matched preview retains lit masonry, physical glass and coherent surface
shadows in both settings. Current → Higher → Current returns **zero changed image
channels** on each backend. This is same-candidate restoration evidence, not a
before/after comparison with the shipped Android defect. The preview contains
no Keeper mist volume; its Mist transactions validate uploaded state only.

Captures use Diagnostic High, 960×540 at 100%, the same frozen glass-camera pose
at tick 120, and twelve settling frames. No owner gameplay input, preference
writes, device installation or audio playback occurred. Exact executable,
shader, pose, settings and package identities are in [receipt.json](receipt.json).

## Validation

- New retirement tests failed against the starting source, then passed.
- Windows Debug: **12/12** affected CTests, 29.75 s. Release: **11/11**, 15.68 s.
  Includes settings/migration, preview adapter, UI, resource inventory and the
  unchanged original character/torch fixture and deadline.
- Android Debug: four-ABI build, **284 Java tests, zero failures/errors/skips**,
  lint, strict ASTC/world/held-item and closed exact-roster package checks pass.
  Packaged arm64 bytes match the stripped library; the containment scanner
  validates its exact Diagnostic/Mobile Pipeline and QueryCompute shader modules.
  The APK is **not installed**; physical phone acceptance is pending.
- All eight Pipeline and eight QueryCompute variants regenerated and validated.
  All four registered shader gates pass across the corrected runs, including
  fresh compilation, unchanged budget negatives, malformed input and publication
  rollback checks. The final artifact gate takes 93.12 s.
- Shader instruction counts fall by 18 in each generic variant and 298/310 in
  Mobile/High opaque variants. Static query sites and sample budgets are unchanged.
  This is compiled-cost evidence, **not a measured FPS claim**.
- Failed runs are retained: sandbox INI/Gradle access restrictions; the old
  preview dust mismatch; a Java negative fixture initially assigning its unchanged
  shadow value; and shader witnesses pinned to the previous binaries. Corrected
  witnesses retain exact hashes and independent fresh compilation, not weaker
  assertions. No production budget or timeout was raised.

## Remaining acceptance

On the next authorized Windows/Android candidate: open Graphics, verify only
Current/Higher appear, compare the torch/lantern alcove and nearby walls, then
Use/Keep, reopen, Restore and pause/resume. An old Lower preference should recover
to Current while keeping resolution, glass, fire and audio choices. Repeat on
both supported Android RT backends where available. Sustained performance and
owner phone visual/lifecycle acceptance remain pending. No audio semantics
changed. Mist issue 28 and the broader unfinished chapter remain separate.
