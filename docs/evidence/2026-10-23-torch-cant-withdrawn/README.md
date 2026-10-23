# Torch cant trial withdrawn — 23 October 2026

**No runtime change is admitted.** Canting and tucking the carried Rag torch
contains its complete mesh, flame, light samples and actual gauntlet in the
tested rooms, but hides the hand/torch at retained wall witnesses and introduces
a discontinuity while turning. The accepted source and compiled tests are
restored to `4eeafcb64edefcd0f4a60413300753006c155a24`. The earlier whole-hand
translation rejection remains [separately retained](../2026-10-22-torch-wall-diagnosis/README.md).

## What was tried and observed

One shared gameplay pose candidate rotates the Grip frame and tucks the hand,
then fits a conservative envelope into the existing room union. It includes
actual Rag vertices, the complete flame box, Light and all four shadow sample
endpoints. It changes neither illumination strength nor renderer quality.
The corrected variant replaces its overly broad circular overhead footprint
with an oriented rectangle and attempts to limit an infeasible depth preference.
Both versions remain withdrawn.

| Check | Accepted control | First trial | Corrected trial |
| --- | --- | --- | --- |
| Independent horizontal mesh/flame/light containment, 375 poses | Fails at walls and turns | Passes | Passes |
| Actual rig: 60 poses, including square/wide aspects, three pitches and two phases | 52 failures across containment, framing and a roof witness | 10 failures | 6 failures, all required wall framing |
| Left gauntlet **and** torch positive projected area at 10 named framing poses | Wall poses fail | Wall poses fail | Wall poses fail |
| Maximum hand change per 0.01 radian wall-turn sample | 2.601 mm | 129.444 mm | **124.327 mm** |
| Inherited low-lintel bounded-lowering check, maximum below 0.8 m | Passes | **0.820403 m: fails** | **0.819779 m: fails** |
| Ordinary centre calibrated hand target | Original | Unchanged | Unchanged |

The final-rig subset loads and skins the actual player and held torch, rather
than inferring the gauntlet from a target point. It checks physical containment,
the unchanged 30 mm roof gap, Grip error/orientation and shared socket ownership.
Projection uses the actual shader frustum; this CPU check does not prove native
visibility or usable presentation. All 60 physical cases pass in the corrected
trial, but six required wall framing cases still fail. No assertion, deadline,
capture admission rule or quality setting was relaxed.

Three fresh native Windows RT sequences each capture the same 49 poses. Camera,
simulation tick, shader/settings and actual uploaded emitter power match for
every triple. All 147 images have verified SHA-256, owning completed submissions
and successful RT presentation/readback. See [the ledger](matched-witnesses.csv),
[summary](capture-summary.json) and per-version process receipts.

Inspection of control/corrected images confirms:

- Diag13 and diag21 change from nearly black wall views to lit stone.
- Diag11 loses its previously visible torch. The gauntlet is absent in the
  original too; zero primary arm pixels were never accepted as a hand pass.
- The ordinary centre view retains its visible torch/hand composition.

These are hidden diagnostic, frozen-look captures on the RTX 5050 Laptop GPU,
960×540 internal/output, High shader/fire, Higher shadows, Mist On and Dust High.
Movement between views uses current simulation inputs, without imported
completion flags. They do not prove physical controls, a natural journey,
phone behavior, continuous-motion quality or performance. No timing win is claimed.

## Retained failures and disposition

The six original/failing logs are retained verbatim. Both complete runtime/test
patches are compressed here; each passes `git apply --check` against the accepted
source. They preserve the new independent witnesses and helper tests for review
and reproduction, **not as passing compiled acceptance**. No inherited test was
deleted or weakened. Source hashes are in [source-identities.json](source-identities.json).
The temporary capture include is removed, and the accepted Windows app is rebuilt.

The restored, unchanged transition, clearance, actual final-rig and socket CTest
registrations pass **4/4 in 168.77 seconds**, including the full final-rig test
in 133.15 seconds. [Results](restored-tests.txt) and
[restoration/build/custody receipt](checkpoint.json). These passes describe the
accepted implementation; they do not close the archived failing wall witnesses.

The corrected trial also detects a real 0.579 mm roof-gap discrepancy from the
Flame socket's small forward offset during upward pitch. Its correction closes
that new physical witness, but is withdrawn with this trial; it is not banked as
an accepted fix. The original witness and failure remain.

Review identifies two further mechanisms without claiming an implemented fix:
repeating fractional overhead anticipation compounds the lowering response, and
a global depth cap can select a distant room's bound. The fixed tangent also
points the torch head behind the camera at diag11 (Grip depth 0.155 m, Light
depth −0.146 m). Shortening anticipation merely to recover framing is unsupported.
Stop this fixed-cant trial rather than tune away the failed witnesses.

Raw PNGs, full native ledgers, binaries and build logs remain ignored under
`reports/torch-clearance-refinement-20261022/`. The retained probe writes JSON
objects spanning two lines; the ledger parser decodes a JSON stream, rather
than assuming one object per line. No raw images or owner saves enter Git.

Next bounded torch work needs a pose that jointly preserves physical clearance,
front-of-camera hand/torch framing and continuous turns, retaining diag11/13/21
and the ordinary centre pose. Resolve forward-facing orientation feasibility
before another runtime candidate; do not repeat the withdrawn translation or
fixed tangent unchanged. The owner's phone clip cause remains unverified.

CI at the accepted source is **incomplete**: [run 39871548947](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39871548947)
has five successful jobs and a cancelled MSVC job at its unchanged 15-minute cap.
Windows main CTest passed 169/169, but six subsequent offline steps were skipped.
That does not make all required hosted checks green. Debug route's 30-second gate,
PR #27 conflicts, natural Keeper/journey/recovery, endpoint listening, final touch
UI and sustained warmed-phone performance remain open. All merges remain unapplied.

No phone operations, merge or release occurred. Audio/haptic manual revalidation
required: **NO** for this evidence-only checkpoint; existing listening acceptance
is still pending.
