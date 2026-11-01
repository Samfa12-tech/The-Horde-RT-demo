# Owner playtest fixes — 2026-11-01

This bounded slice addresses the observed practice dead end, rope reach and quiet rescue speech. It is a private candidate requiring another playtest, not a 1.7 release.

## Changes

Normal application configuration selects sequential opening practice. Historical free-practice fixtures remain explicit. Skeleton A alone attacks during practice; B waits. Block uses normal attacks, parry uses yellow guard-break attacks, dodge uses red unblockable attacks, and guard attack returns to normal. Already-started attacks retain their kind until the next windup. Skills must be earned in order. Neither skeleton can be killed during protected practice. Accepted sword contacts still emit hit feedback and satisfy the guard-attack lesson at the health floor.

The existing four-skill Ready/Finish flow remains. If an unfinished opening lesson reaches the Keeper or exterior, the same shared owner gives a protected three-second warning, then removes both practice protection and its HUD at a safe attack boundary. Explicit Keeper dodge practice is preserved. This is not a revival handoff: normal revival remains OFF. The private clip/cue still needs distributable admission, actual platform readiness/recreation wiring and listening/natural-play acceptance before that sequence can be enabled. No audio asset was uploaded or changed.

Rope prompt and actual approach radius increase from 0.75m to 1.125m. Rope readiness, landing phase, support-height tolerance, swept capsule and coping obstruction checks remain. The larger radius does not authorize passing through the coping or attaching to an unavailable rope.

Both platforms now share rescue-dialogue spatial parameters. Only RescueFound/RescueRope use emitter gain0.60 and reference distance3m; other speech remains0.36/1m. Position, pan, obstruction,14m cutoff, voice-volume control, PCM and existing bed ducking are unchanged. The actual prior owner admission was −26.78dB spatial gain at Dialogue100%; at that distance this change adds approximately13.98dB. Original PCM peak0.939606 times maximum source gain0.60 is0.563764, retaining source headroom; this is not proof that every combined output mix is unclipped. Listening at default and100% remains required.

## Verification and limits

Four focused host suites pass: combat foundation, gated revival integration, dialogue ducking/spatial routing, and rescue traversal. The actual fixed-step four-skill scenario now explicitly uses the new sequential policy, reaches all four earned flags with accepted hits, and finishes at a safe boundary. Tests cover protected actors, one attacker/kind, accepted hits at1HP, bounded protection exit, added rope reach, outside-radius rejection, unavailable rope and existing through-coping rejection. The rope suite retains98,616+ cases and bounded movement steps. Historical free-practice tests still cover kill/next-attacker behavior under their old explicit configuration.

Windows Release application builds with the private1.7development labels. Android build/UI results and immutable artifacts are recorded in OWNER_FIXES.json. No candidate was automatically launched or installed. Save/load and physical outside-tomb protection handoff need owner acceptance; unit state tests do not substitute for that. No FPS claim, hosted CI or LFS transfer.

## Remaining work

- Full novice tutorial success/next presentation and natural-play acceptance, plus admitted lich cry/revival-to-live handoff.
- Physical protection/HUD checks after leaving the tomb, Skip, save/load and retry.
- Kit rescue voice listening at default/100%; rope approach on both ends and obstructed sides.
- Hold Attack to sheathe: continuous input/cancellation and proximity-redraw ownership are not implemented in this slice.
- Red ring side-angle readability: current geometry already faces the player, so no speculative orientation change was made. Owner saw yellow and red rings.
- Cold startup responsiveness: corrected complete packaging and diagnostic trace remain; prior stall cause is unproven, safe initialization pumping is not implemented.
- The treasury-road line follows the moss-covered waystone clue at F03 and refers to interpreting its markings. No road branch or mismatched replacement subtitle was invented; contextual clarity remains to review.
- Flask, asset licensing, reporting, public delivery and other release gates remain independently open. No issue was closed by these local changes; Hotstrike remains outside this slice.

Issue handoff (no closures): the owner reports that the earlier #35 tutorial/subtitle scope and #33 Kit grate duplicate may already be fixed. Verify their existing evidence/closure separately; do not duplicate those implementations. This new staged-skeleton practice and quiet rope speech work is distinct. #34 performance work remains paused and is not an FPS-based admission claim. #25 benchmark submission and the LIVE statistics website are required for 1.7; mocks do not complete that gate. Actual deployment/transmission authority and end-to-end delivery remain separate. #28 rope/mist shadows are explicitly deferred by the owner. Hotstrike is excluded.
