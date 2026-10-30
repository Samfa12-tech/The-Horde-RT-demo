# Kit admission review — keep normal motion OFF

Superseded admission recommendation: the [matched natural-walk follow-up](../2026-10-24-kit-natural-corner/README.md)
records the visible corner and confirms a turn-delay regression. The hint-first
proposal below is withdrawn; its original measurements remain preserved.

The shared normal-actor candidate functions through a bounded actual RT journey, but is not ready for ordinary players: the preserved six-metre escort rule has no adequate waiting/rejoin cue. The first-corner route screenshots also leave Kit outside the unchanged camera view, so those cannot establish a visible join. No gameplay motion, speed, tether, collision, checkpoint or timing policy was changed.

## What passed

- Existing isolated normal-actor join sequence: 515 checks, 123 actual RT images, 32.5 seconds; all 123 images byte-identical to the previously reviewed retiming reference. Pause/reset identities pass, no separate inspection actor. Sampled three-metre views retain coherent turn-to-walk appearance; dark lighting limits fine contact judgment.
- One hidden input-driven RT route: 17 checks, zero failures, 1,724 presented frames, 157.516 seconds. Starts from the declared post-Keeper lower seed (victory/claim seeded, ascent/proof/forest unset), then actual ascent, fresh presented manual lantern proof, escorted route, grounded endpoint, durable autosave/manual slot two, return/descent and reload of this run's endpoint. Not a natural Keeper victory, death test, listening or phone acceptance.
- Every outdoor rendered candidate snapshot must match the normal actor's published motion and have no inspection actor. A narrow Debug-only flag --route-kit-turn-start is accepted only with the existing strictly isolated --route-validate-hidden fixture. Normal application configuration and saves remain unchanged/OFF. Existing 360-second/6,000-frame bounds and 420-second wrapper bound remain.
- The pre-existing input driver deliberately pauses when Kit is behind by over three metres. This run proves completion with that pacing, not that an uninformed player will do so. No checkpoint flags are awarded by the driver.
- All 151 parked owner files, original fixture bytes, owner campaign files and settings remained unchanged. HWND stayed hidden/nonactivating; no phone or foreground activation.

## Actual visual review and cue gap

The paired normal-actor poster shows the previously reviewed supported turns and first walking pose. Actual route departure shows Kit turn from facing the player to facing travel before walking. Raised lantern/near camera obscure the lower body, so it is not a close foot-contact acceptance. Both first-corner snapshots contain the correct completed RT/state receipt but Kit is outside the view; retain them as a camera-coverage limitation, not a visual pass. JPEG posters are labelled scaled review derivatives; original PNG identities and full raw receipts remain in reports and receipt.json. No synthetic scene or image enhancement was used.

ChapterDialogue.h declares forest.wait / “I'll wait here” but excludes it from recoverable active lines and never triggers it. ForestWaystone's “Hold it there” is a once-only story beat triggered by player proximity; ReunionDepart's “Come on” does not explain stopping for Kit. No player-facing consumer of CompanionWait::PlayerLagging was found. The symmetric limit therefore silently stops Kit if the player runs ahead during a turn.

**Next smallest action:** add one restrained transient escort hint using existing guidance space and shared state: wait/stay with Kit when ahead, rejoin/catch up when behind, clear on recovery. Do not add a button, change the six-metre policy, trigger unrelated story VO or complete progress. Then review that cue with one route-camera witness that actually includes Kit at the corner. This pass only recommends it. Casual 0.75 s versus urgent 0.50 s preference is preserved; no timing selection was added.

## Why the 30-second Debug gate remains open

The registered test, with only flushed phase markers added, times out at 30.02 s. It has earned the endpoint at 19.229 s, begun loaded return at 21.250 s and reported no failed assertion. The identical finite executable run outside CTest completes all 145 checks at 37.067 s: loaded return starts at 21.193 s, consumes 15.832 s / 3,082 fixed ticks and completes at 37.025 s. Earlier terrain/storage/earned journey and recovery consume the remaining budget. This demonstrates aggregate Debug workload exceeding the total gate, rather than a nonterminating route; it does not attribute a hot function or CPU busy/wait split. The ordinary configuration keeps the turn candidate OFF in both runs. The registered 30-second deadline is unchanged and still failing; supplementary completion is not a gate pass. No repository-wide test run or timeout increase was made.

Previous exact WIP e955af54 CI40068443694 is terminal success across all six jobs. The subsequent review checkpoint needs its own exact-head CI; hosted success does not close the local Debug gate or the stated player/visual gaps.
