# Horde 1.7 recovery decisions for morning - 20 October 2026

Status: **policy pending**. This record collects the bounded choices left by the
accepted [master 10.5 and saves section 12](../references/horde-1.7/d37f1638/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md).
No later accepted recovery policy or current consumable implementation was found
in the repository audit at `cbfc13f6`. No healing rule, balance or save format is
changed by this record.

## Constraints already settled

Authoritative health starts at three hearts. Hearts are a HUD representation;
maximum upgrades/fractional damage and the later healing economy remain separate
balance choices. Recovery should use grounded objects or safe places. Passive
regeneration, floating heart pickups, mana, hunger, crafting and compulsory
farming are not implied. Checkpoint activation does not automatically heal/refill.

Save retention is decided: three independent slots, each with separate manual
and latest-auto records, at supported safe boundaries. Loading must preserve the
chosen health/resource state without duplicate consumables or repeat grants.
Dead/unavailable use, duplicate commands, damage ordering, interruption and
resource consumption need shared simulation rules and explicit tests.

## Exact choices needed

| Decision | Choice to settle before implementation |
| --- | --- |
| First method and availability | Limited flask, authored safe-rest interaction, or another bounded method. Specify the existing chapter milestone/location at which it first becomes usable. |
| Capacity and amount | Initial capacity/charges and hearts restored per use; whether a full-health use is rejected or consumes a charge. Keep maximum health at three unless separately chosen. |
| Refill/rest trigger | Name exact safe places or actions, repeatability and any cost. Decide whether a refill is independent of checkpoint activation. A generic “safe area” needs an authored eligibility rule. |
| Use and interruption | Safe-only or usable during combat; action duration; allowed movement/attacks/parries/climbing; damage interruption; exact point when health is restored and a charge is consumed. |
| Death and retry | Which committed health/charges return on death/load/retry, and whether the existing encounter resets described below remain. Loading an earlier save must not manufacture a new grant. |
| Save migration and entitlement | Fields for charges/capacity/refill identity, old-save initial entitlement and one-time ownership. Specify whether old records begin empty, receive a single grant at a safe milestone, or use another explicit migration. |

These choices remain pending for morning. No candidate quantity, use timing,
automatic refill or checkpoint heal is selected here.

## Recommended package for Sam to approve

**Proposal only, not an agreed rule or implementation.** The master section 10.5
already points toward a limited flask and deliberate rest. The quantities,
timings and new Rest interaction below are recommendations, not recovered
agreement. Existing three-heart health and exact checkpoint restoration remain
unchanged until Sam chooses a policy.

Player-facing proposal: carry one drink that restores one heart. Stand still to
drink; an interrupted drink costs nothing. After claiming the lantern, deliberately
rest at the lower rope landing to recover fully and refill. Walking over a
checkpoint or loading a save gives no free healing.

| Decision | Recommended first slice |
| --- | --- |
| Method and availability | One carried flask, filled on New Game and usable before the first fight. No pickup quest, upgrade tree or new art requirement. |
| Capacity and amount | One charge; restore one heart, capped at three. Full-health, empty, dead or unavailable use is rejected without cost. |
| Refill/rest | Add an explicit two-second Rest action within one metre of the existing lower rope landing, after Keeper defeat and lantern claim. Require grounded, ready terrain and no combat, climb or active dialogue. Completion restores three hearts and one charge; repeatable and free. This proposes a new interaction at an existing location, not an already-authored refill prop. |
| Use/interruption | Drink for 1.25 seconds while grounded and stationary; looking is allowed. Movement, attack, parry, dodge, climb, damage, menu, pause or lifecycle interruption cancels without cost. Damage/cancellation wins a completion-tick tie. Consume and heal atomically once on completion; no invulnerability. |
| Death/retry | Load and post-claim death restore committed health/charges exactly. Retain the existing explicit pre-claim encounter retry's three-heart reset and propose one charge for that deliberately restarted attempt. Ordinary encounter selection must not refill the flask. |
| Migration/entitlement | Version the save payload to store charges; capacity stays fixed at one. Older records retain their exact health and migrate with an empty flask. Starter fill occurs only on New Game; an eligible deliberate Rest can refill old saves. Retain originals until successful replacement, and reject unsupported newer formats. |

This gives one recoverable mistake and one clear recovery place without a wider
resource economy. Implementation would still require an explicit input/control,
shared interruption ordering and save-version/duplicate-grant tests. In
particular, old pre-claim records must have a viable explicit retry path; migration
must not quietly grant a drink on every load. No part of this proposal has been
implemented or admitted by the later phone acceptance pass.

## Existing behavior that the decision must account for

[`PlayerVitals`](../../src/gameplay/ShowcaseGameplay.h) currently owns fixed
maximum three, damage, `ResetForEncounter()` and exact
`RestoreCampaignVitality(1..3)`. It has no general heal, flask, capacity-upgrade or
refill API. [`UpdateEncounters`](../../src/gameplay/simulation/GameSimulation.cpp)
currently resets vitality on selected-encounter changes before Keeper reveal.
Explicit pre-claim encounter retry also resets to three through the existing
checkpoint path. Post-claim rescue reconstruction preserves live vitality.
Adding a recovery method must account for those established resets explicitly.

[`Checkpoint`](../../src/gameplay/campaign/CampaignCheckpoint.h) stores current
vitality. [`Codec`](../../src/gameplay/campaign/CampaignCheckpointCodec.cpp)
currently uses format/schema one, content revisions one/two and a fixed 142-byte
payload; it has no resource/capacity/refill fields. A flask needs a deliberately
versioned migration and duplicate-grant tests. Current capture/restore retain
actual saved vitality; safe-rest rules would still need to distinguish live
recovery from loading an existing record.

## Independent accepted work that can continue

The [rescue/forest music checkpoint](../evidence/2026-10-20-chapter-music/README.md)
now implements shared chapter state selection; actual listening remains open.
The [local tomb/rope checkpoint](../evidence/2026-10-20-local-ambience/README.md)
now implements sparse wet-room drips and grip-travel rope contacts with finite
shared voice ownership; actual listening remains open. Master 11's gentle dialogue
ducking/smooth recovery is now implemented by the [shared envelope checkpoint](../evidence/2026-10-20-dialogue-ducking/README.md).
Finite cues stop for Kit; music/waterfall consume one transient shared multiplier.
Existing forest/pond calls and waterfall audio are already implemented.

Nearby debris reaction remains conditional on measured cost; the six chips are
currently static. A player-facing debris detail option is absent. A later slice
can use the existing primitive/material owner and preserve collision/saves,
without an unbounded physics system or heavy outdoor content. This conditional
detail work is lower priority than recovery and coherent story/audio behavior;
the option/omission requirement depends on unacceptable measured cost.

Natural full-route/return, earned saves/death Load, first-time teaching/contact,
controller/touch/listening and warm Keeper/exterior performance with mobile Mist
Off remain physical acceptance gates. Missing hardware permission does not turn
them into host passes or require guessing a recovery policy.
