# Gated flask owner and confirmed recovery

This is an implementation checkpoint behind **default-OFF**
`GameSimulationConfig::healingFlask1_7`. No production flask input, prop, upgrade
pickup or hand binding is enabled. Normal capture/restore keeps its earlier
schema1/2 behavior; enabled diagnostic simulations own exact schema3 resources.

One shared fixed-step owner consumes distinct Drink/Rest sequences, cancels for
competing intent before combat consumption, and resolves healing after actual
damage/death and dialogue. Cancellation/damage wins the completion tick. The
same snapshot publishes resources and exact half-heart healing; there is no
feedback-queue inference, new immunity or renderer timer. Active dodge is
explicitly unavailable even when collision produces zero movement.

New Game starts two full charges; only explicit pre-claim Retry refills current
capacity. Generic encounter/checkpoint selection and timing reset do not refill.
Legacy1/2 restores exact prior health/progress with empty base resources; enabled
schema3 restores exact entitlement/charges. Actual-terrain tests exercise the
three safe anchors. A qualified lookout recapture updates retained health and
resources; backtracking does not downgrade its endpoint ownership.

## Confirmed records, not live recovery

Post-claim Retry publishes a monotonic request without healing/relocation.
PlaySession retains a private current-game/current-slot record only after an
actual successful save acknowledgment or adopted load from Session's coordinator.
Logical candidates, writes still pending/failed/uncertain and another slot's
manual save do not become its retry state. A no-record request fails visibly
rather than granting resources. New Game/reset clears ownership and cancels
old-game tickets. Cancellation retains the prior confirmed record for that game.

The exact immutable acknowledged bytes pass the existing Begin/Prepare/Commit
terrain, input-rebase and retirement path. Cached retry is explicitly marked
`fromConfirmedRecord`; it performs no new filesystem IO and claims no new flush.
Ordinary manual loads still adopt their actual read receipt. Successful Rest
publishes one completion generation, allowing one changed same-ID checkpoint
autosave; repeated polling cannot cause a write loop.

Platform adapters now activate that owner operation once through their existing
load barriers. Android skips the legacy camera for confirmed retry, adopts a
fresh owner serial, and waits for neutral input acknowledgment, Alive and owning
RT presentation. Windows uses the same retirement/rebase/presentation gates.
Precommit cancellation does not resurrect a queued request; postcommit lifecycle
keeps the existing loaded-game barrier. A logical restore is not RT evidence.
The feature remains disabled pending actual-path and presentation admission.

## Local evidence and retained failure

- First build passed. Initial owner selection: coordinator and existing campaign
  restore passed; flask contact setup failed before establishing damage contacts.
  The unchanged damage/completion-tie assertion was not reached.
- The corrected fixture uses the established opening melee setup, rather than
  stationary waiting in the production waterfall setup without a range witness.
  A separate wounded, blocked-wall dodge case guards the demonstrated eligibility
  gap. Corrected flask simulation and synthetic coordinator selection passes2/2
  (9.98s and0.04s). Original [failure](first-owner-tests.log) and
  [corrected result](corrected-owner-tests.log) remain distinct.
- Existing mailbox stress now includes both utility counters:125,000 publications,
  four readers, unchanged30-second deadline; passes3.57s. No platform input
  functionality or physical feel is inferred from it.
- Windows application/adapter compiles. Shared combat foundation and Android
  neutral-rebase checks pass2/2. The Android roster passes464tests, zero failures,
  six existing skips, including five confirmed-retry flow/error/ack/Home tests.
  Four-ABI assembly and lint pass; the final native publication-order adjustment
  (retry routing before the release-published life phase) is rebuilt separately.
  These do not close actual RT load/return or physical lifecycle gates.
- New independent `horde_rt_flask_confirmed_retry_tests` compiles. It covers actual
  record acknowledgment, failed writes, exact fractional health/charges, another
  slot's manual save, legacy bytes, same-ID Rest, F04 and reset ownership.
  **Its real-file execution is pending hosted CI.** Local publication previously
  failed at `SetFileInformationByHandle(FileRenameInfoEx)` with
  ERROR_ACCESS_DENIED5 on disposable `<system temp>/horde-campaign-storage-test-<suffix>/slot-a.sav`.
  That denied operation was not retried or redirected. Synthetic coordinator
  tests exercise typed success/failure acknowledgments without filesystem writes.

Raw build/test logs stay under ignored `reports/held-guard-20261025/`.
Prior backed-up `2f461da4` CI40174689046 passed all six jobs; this newer owner
checkpoint needs its own exact-head CI. No owner saves/settings or151 parked
asset files were changed. No phone operation occurred.

The [concrete presentation seam](../../design/HORDE_1_7_FLASK_PRESENTATION_SEAM_2026_10_25.md)
needs review before capacity/mesh integration. Platform controls/HUD, right-hand
stow/drink/reacquire joins, the F03 upgrade and physical/balance acceptance remain
unfinished. This checkpoint is not a player-ready flask feature.

## Hosted confirmation after the initial checkpoint

Exact source `11c136590ba0db2261e62409e2c693eb7699d537` completed all six jobs in
[CI40178298329](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40178298329).
The MSVC log explicitly records `horde_rt_flask_confirmed_retry_tests` passing
in0.37seconds and `horde_rt_flask_simulation_tests` in0.75seconds, with all158
selected Windows tests passing. The real-file confirmed-record fixture above
therefore has hosted execution evidence; it is no longer merely compiled.
The local ERROR_ACCESS_DENIED remains a local environment boundary and was not
retried or bypassed. Hosted file/owner checks do not establish actual RT return,
phone lifecycle, flask presentation or balance acceptance. The feature stays OFF.

## Named keyboard/controller inputs, still disabled

H / D-pad Up requests Drink; J / D-pad Down requests Rest. R remains the existing
route reset. Both actions require the current owner's explicit eligibility,
distinct monotonic sequences and a live/focused surface; menu navigation keeps
its original Up/Down behavior. Native simulation still owns all timers, resources
and consumption-time checks. Repeats, release/cancellation, hat/key coalescing and
stale/malformed publication are covered by the focused fixtures. Windows input
and load-barrier tests pass2/2; the three relevant Android suites pass55tests.
These are input/host checks, not flask animation or device admission.

No new touch targets were placed: the accepted compact portrait action dock has
no proven room for two additional utilities without moving combat controls or
intercepting steering. The required flask prop/upgrade/presentation seam also
remains open. `healingFlask1_7` stays false and the new controls remain invisible
and inactive in the normal phone candidate. Phone preparation/acceptance is a
separate bounded task; no new device pass is inferred here.
