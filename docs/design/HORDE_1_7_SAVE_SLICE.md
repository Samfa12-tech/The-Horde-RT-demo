# Logical safe-checkpoint round trip and next storage boundary

**18 October update:** retention is now confirmed: exactly three slots, each
with a separate manual record and latest autosave; autosaving cannot replace
manual. The [real worker](../evidence/2026-10-18-campaign-worker/README.md) and
[full host save/load path](../evidence/2026-10-18-campaign-session/README.md)
are implemented and tested in disposable directories. Menus, owner-save roots,
platform input/camera/resource adapters and physical durability/lifecycle gates
remain open. Earlier unresolved-policy and stop-for-review wording below belongs
to its preserved pre-implementation snapshot; it is superseded by this update and
the current Project Decisions record.

The logical two-boundary slice below is implemented; the
[implementation and validation receipt](../evidence/2026-10-17-logical-save/README.md)
defines its supported state and remaining integration limits. It has no disk
or platform integration of its own. The subsequent
[typed codec and isolated storage adapter](../evidence/2026-10-18-campaign-storage/README.md)
now supplies bounded byte validation and failure-aware file publication;
platform durability acceptance, slot UI and automatic checkpoint coordination
remain open. The remainder
of this document preserves the design inspected at
`f3c494ee8b39c4f8503412ecd35361d927750a7a`, including its still-open storage policy.
This is the authorized fallback
from the [bounded shadow investigation](../evidence/2026-10-17-shadow-investigation/README.md).
The [accepted design section12](../references/horde-1.7/d37f1638/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#12-save-replay-and-recovery)
requires durable lower-claim, safe-exit and forest-end recovery. Three slots
are the current direction; manual-save retention policy remains unresolved. No arbitrary-position,
mid-climb, cloud-save or new inventory system is implied.

## Owners inspected before implementation

| Current owner | Reusable behavior | Missing save seam |
| --- | --- | --- |
| [GameSimulation](../../src/gameplay/simulation/GameSimulation.cpp), `RetryEncounter`, `ApplyShowcaseCheckpoint`, `ImportRewardCheckpointState` | Shared owner-thread retry, explicit fixture imports, resource-lease deferral and snapshot refresh | Versioned logical export/validated restore. Showcase import sets `legacyCombatCheckpoint_`; reward import returns to lower landing and resets traversal. Neither is a campaign loader. |
| [GameSimulationRescue](../../src/gameplay/simulation/GameSimulationRescue.cpp) and [RescueTraversal](../../src/gameplay/traversal/RescueTraversal.cpp) | Same-session reconstruction selects a safe side, retains claim/deployment and creates fresh traversal generations | Reconstruction depends on the live object, including rope nodes/timers. A fresh process needs safe logical facts, not a copied rope snapshot. |
| [SimulationSnapshot](../../src/gameplay/simulation/SimulationSnapshot.h) | Coherent read-only view of encounter, equipment, health, support, dialogue and companion facts | No closed persistent schema. It also contains transient combat/input/render-generation data that must not be serialized wholesale. |
| [ChapterDialogue](../../src/gameplay/dialogue/ChapterDialogue.h) and [dialogue integration](../../src/gameplay/simulation/GameSimulationDialogue.cpp) | Stable line IDs, once flags, bounded continuation queue and stale callback rejection; raised-lantern proof waits for a presented frame | Restore of logical story progress with fresh callback/input floors. Some flags are set when a line starts, not when it finishes; blindly persisting the public booleans can lose queued story responses. |
| [CompanionRouteFixture](../../src/gameplay/dialogue/CompanionRouteFixture.h) | Grounded path/wait state and fresh generation | Compatible grounded reconstruction at a checkpoint; waypoint indices and current animation clocks are not durable identities. |
| [Android preferences](../../android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java), [Windows settings](../../src/platform/windows/DiagnosticWindow.cpp) | Existing graphics/audio/controls/subtitles and RT Lab persistence | Separate campaign storage adapter, slot ownership and transactional writes. Reusing the settings blob would conflate different lifetimes. |

The pre-implementation search of `src`, `android/app/src/main` and `tests` found no campaign-save,
campaign-slot, SaveGame/LoadGame or autosave implementation. The sole autosave
match configures WebView password behavior. This source search supports the
gap alongside the inspected owners; it is not proof based only on filenames.

## Implemented logical milestone specification

Implement **CPU-only logical capture/restore for two authored safe boundaries**:
the claimed-lantern lower safe point and grounded exterior exit. Defer the
forest-end record until these round trips work. No disk worker, binary/JSON
codec, slot chooser, new menu, asset, renderer change or device installation is
part of this first slice. It can be tested with real fixed-step simulation and
the imported terrain through the existing native fixtures.

Proposed code ownership: new small `src/gameplay/campaign/` value/validation
types, owner-thread methods in `GameSimulation.h` and a focused
`GameSimulationCampaign.cpp`, narrow dialogue/traversal logical restore seams,
one `tests/CampaignCheckpointTests.cpp` target and its CMake/selected-host CI
registration. Do not make platform or renderer code mutate gameplay fields.
Keep `f3c494ee` diagnostics and existing frozen checkpoint behavior intact.

Use a closed, versioned `CampaignCheckpoint` value with a stable content revision
and authored checkpoint/zone IDs. Record coherent encounter victory/guard facts,
reward/lantern and existing sword facts, stable rescue/lid progression, safe
player position/orientation/support and recoverable health, plus Kit/story
progress. Validate enum/bit ranges, finite values, authored support/capsule
clearance and cross-field relationships before touching live state. The first
two IDs imply specific authored safe locations; they do not admit any position
that happens to be grounded. Slot IDs belong to the later storage envelope.

| Durable fact | Current source / required conversion |
| --- | --- |
| Tomb victory/reward | Guard entity states, `lich`, `chestReward` and `interaction`; preserve defeated facts together, never infer victory from the currently rendered actor slot. |
| Safe side and lid/rescue progression | `rescue.firstAscentCompleted`, actual safe phase/position and claim/deployment facts; opening currently comes from private `rescueOpeningSeconds_` and is exposed through the chapter's `lich.finaleSkylightOpenProgress`. Export a logical completed-lid fact at the safe boundary, not animation seconds. |
| Equipment | Actual lantern claim and existing sword state; restore through current ownership/socket paths. Do not serialize world matrices, grip blends, pendulum, attachment ticks or in-flight draw/stow. |
| Health | `playerVitals`; validate a recoverable live value/policy. Do not import death timers, transient protection or assume same-session reconstruction restores vitality. Verify alive/movement/action behavior after restore. |
| Story/Kit | Stable dialogue IDs and logical milestones, including proof pending/completed and required continuation. Persist no active audio stream, callback generation or elapsed sample clock. Rebuild Kit on actual current terrain at the checkpoint. |
| Night | Explicit chapter/night identity; derive chapter presentation without replaying the legacy finale's dawn/skylight fields. |

Capture only an alive, grounded, nontraversing safe checkpoint, with restored
equipment and current collision/destination readiness. Reject active static
resource leases and pending unsafe transitions. A fresh record must not claim
render readiness after load: validate/stage offside, prepare the destination,
then commit on the application/simulation owner thread with fresh world,
dialogue, companion and input identities. During preparation retain the last
safe session. For host tests, inject valid/invalid readiness through the existing
generation boundary rather than declaring every zone ready.

Do not reuse `AcknowledgeChapterPresentation` as a generic save receipt: today
it specifically requires a raised lantern and acknowledges the reunion proof.
No `lastConsumed*` counter, `chapterVisibleRaise_`, queue pointer, simulation tick,
GPU/OS handle, resource lease/generation or immunity timer goes into the record.
A pending proof must require a fresh manual raise and current-frame presentation
acknowledgement after restore; a completed proof must not replay its lesson.
Explicitly distinguish completed story responses from interrupted/pending
continuations so a load neither duplicates nor silently drops required lines.

## Finite acceptance for that slice

1. Round-trip lower claimed state through a newly constructed production
   simulation: same victory, reward and equipment; safe support; no duplicate
   claim/reward or legacy dawn. Legacy capture fixtures remain unchanged.
2. Round-trip safe exit with both pending and completed reunion proof: grounded
   player/Kit, retained lid/victory/lantern, coherent sockets/light, fresh raise
   required only when pending, and no stale completion/skip/input accepted.
3. Reject malformed/nonfinite pose, unknown/newer content/schema, contradictory
   facts, wrong support and active traversal without mutating the current
   session. Reject stale readiness/load tokens; failed preparation preserves it.
4. Test interrupted first ascent -> lower safe record; interrupted return
   descent -> last committed safe exterior record. Do not use mid-rope snapshots
   as checkpoints or count a merely proposed exterior boundary as committed.
5. Test restore health, duplicate callbacks/claims, reset during pending load and
   retained story continuation. Assert zero live attack/dodge/run/input debt,
   no repeated reward or completed rescue, and no renderer readiness invented.

Run the new host tests in Debug/Release, affected shared simulation/dialogue/
rescue/rig tests, and compile Android/Windows integration if owner APIs change.
Host round trips close **logical recovery only**. They do not close durable
storage, process death, physical RT, touch/audio or performance acceptance.

## Follow-on storage and the decision it needs

The typed codec and isolated file adapter in the receipt above implement the
storage portion of this specification in disposable test locations. No actual
owner-save root is selected. The separate
[session/request coordinator](../evidence/2026-10-18-campaign-save-coordinator/README.md)
now arbitrates fake asynchronous work; its real worker/slot integration is still
a follow-on. The adapter's exact publication and durability
outcomes must be respected rather than treating every failure as unchanged.

After logical recovery passes, add a separate bounded atomic file adapter and
active-slot commit coordinator. Build a candidate record, write/flush/replace
atomically, and advance the durable checkpoint only after the matching success
receipt. A failed/interrupted write keeps the previous valid record; stale
receipts from a replaced session/slot cannot advance it. Exercise injected
open/write/flush/replace failure, old/new/corrupt records, process interruption,
slot isolation and migration-source preservation before claiming durability.

Before implementing UI/storage overwrite policy, resolve whether **manual Save
stores the active slot's latest safe checkpoint**, or retains manual snapshots
separately from autosaves. Three independent slots remain the direction; this
slice chooses no destructive overwrite semantics. No owner interruption is
needed for logical recovery or an isolated failure-injected codec/file adapter.
Settings and RT Lab
unlock remain separate. The forest endpoint, platform process-death tests and
Save/Load/New Game menus then receive their own reviewable checkpoints.

The coordinator receipt is the current stop-for-review boundary. It includes a
short remaining-playability map; it does not authorize connecting real owner
paths, installing a device build or choosing manual/autosave retention.
