# Kit turn/start admission diagnosis — WIP, normal OFF

The current source is an attempted normal-path integration, not admitted gameplay.
Normal ProductionGameSimulationConfig.companionTurnStart remains false.

## Retained failures

- host-tests-r1-failed.log and host-diagnostics-r2.log retain the original two failures.
- Endpoint movement fixture: restored checkpoint yaw is -pi/2, but a new input defaults to zero. The required neutral camera-rebase/resume acknowledgement was never satisfied; snapshot stayed paused, input publication was zero and there was no player movement. Correct test setup through the existing resume barrier, not runtime policy. Assert actual unpaused movement before judging turning.
- Earned route: ordinary 1.9 m/s player walked away during stationary turns; first player-ahead tether stop was 6.01319599 m, final separation 22.6437855 m. Kit remained grounded at waypoint 7; latest boundary correctly remained SafeExit. This result demonstrates no controller deadlock or terrain rejection; it does demonstrate a playability cost if a player does not know to remain with Kit. An incomplete companion route correctly confers no checkpoint entitlement. It is a real added pacing cost interacting with an inherited symmetric distance limit.

## Design and smallest bounded check

Accepted Kit plan calls for leading and safe waiting, not teleport/catch-up. Shared turn/start scope lines 74–91 retain the six-metre tether, 1.8 m/s ceiling, support and progression. The pre-existing Windows natural route driver explicitly waits if Kit is behind by over 3 m. No source establishes automatic catch-up outside six metres.

Keep the original unpaced inputs as a negative witness in an opt-in host diagnosis: assert no endpoint and no translation while outside the tether; backtrack through normal movement to the stranded corner; then use the same deliberate escort input policy as the native route driver. Require actual resumed Kit travel, completed grounded route and every original endpoint/save/reload assertion. Baseline production journey remains unchanged. This tests recoverability and expected escort pacing; it does not conceal the unpaced regression or prove that players understand when to wait. No runtime tether/speed/checkpoint change is proposed.

## Owner timing direction

Sam reviewed the actual clip and likes both coordinated timings for urgency versus casual. Preserve the authored 0.75 s and faster 0.50 s full-exchange options. Later admission can latch fast timing when setting off/catching up and slower timing for relaxed facing/conversation at BeginTurn, keeping the same duration through each supported exchange. Reassess intent only at the planted boundary. No broad emotion system, mid-step rate oscillation or universal replacement of the slower option. This diagnosis does not implement timing selection.

## Still required

Normal actor actual-RT joins/continuous turns and natural route/save recovery, Release/platform staging, honest input/escort UX judgment and any physical acceptance remain open. Host shared-simulation movement is not RT presentation or phone play. WIP backup/CI does not admit the feature.

## Diagnostic setup failures retained separately

The first added recovery check required at least one waiting tick. Actual backtracking gave Kit a head start, so all old endpoint/save/reload checks passed but this new expectation failed (r4). The second required the player/Kit separation always stay at most six metres; Kit was 6.21924 m ahead after backtracking, so that also rejected a legitimately recoverable situation (r5). Neither expectation expresses the existing contract. The final diagnostic checks the exact rule: for every accepted Kit translation during recovery, this tick's updated player position must be within six metres of the pre-translation Kit root. The original r1/r2 failures, both new diagnostic failures and the final result remain separate. No prior route/checkpoint assertion or deadline was removed.

## WIP integration inventory

- Rename the explicit shared flag to companionTurnStart/turnStartEnabled; constructors and normal development remain OFF.
- Stage one append-only owned runtime derivative (original seven clips plus two turn clips), shared named binding roster and normal-actor pose cache identity. The optional actor worker remains OFF; original geometry and source remain unchanged.
- Android/Windows package inventories replace the original duplicate with the derivative, preserving manifest/textures. Asset policy: 201 checks pass. Native intake: original seven clips/35 sampled poses and 61 named binding checks pass.
- The hidden controller path is prepared to exercise the normal actor and reject use of a separate inspection actor. It has not run on this WIP. The earlier retiming RT evidence remains evidence of its own build.
- Existing generic round-trip probe waits for a real continuous one-second walking witness rather than assuming proof+one second implies Kit has departed; unchanged phase/whole-run budgets remain.

The Debug application was rebuilt successfully after restoring normal OFF, so the local executable now matches that safeguard. This is compilation evidence only; no application render validation was run on this WIP. No install, owner launch, foreground activation or phone operation occurred.

## Executed results at this WIP

- Corrected endpoint/locomotion CTest: 2,342 checks, zero failures, 4.44 seconds. Resume, actual movement, live turn, pause and canonical save/load checks execute; normal development remains OFF.
- Broader Debug CampaignPlaySession CTest: unchanged 30-second deadline reached at 30.01 seconds. Retained as unresolved; no retry of its unchanged workload and no deadline change. This is not a pass.
- Opt-in earned-route diagnostic r6: 94 checks, zero failures, 48.484 seconds. The original unpaced workload produces 22.6438 m separation and retains SafeExit. Ordinary backtracking resumes Kit; he reaches grounded waypoint 8 at (14.2604, 36.1628). Every accepted recovery translation obeys the exact six-metre rule. Returning gives Kit a head start (maximum separation 6.21924 m with Kit ahead), so deliberate escort input needs zero extra waiting ticks. All original endpoint eligibility, durable autosave/manual preservation, reload and return checks execute. No checkpoint is imported into this journey; host presentation acknowledgement is explicitly not successful GPU presentation. The wider diagnostic takes longer than the ordinary CTest gate and does not close that gate.
- Original r1/r2 failures and new r4/r5 setup failures remain included. Those are not runtime corrections or evidence of a successful original unpaced journey.

Next bounded step: review the normal-actor RT route/joins with correctly paced escort inputs and implement the separately recorded casual/urgent exchange-time latch only after that correctness checkpoint. Do not enable normal motion solely from the host recovery result. Clarify waiting guidance as a player-facing acceptance concern; no expanded tether, teleport, faster catch-up motor or artificial checkpoint is warranted.

The final normal-OFF Debug application build passed. No owned renderer, install, transfer or phone operation was started. The WIP is safe to back up but remains unadmitted.
