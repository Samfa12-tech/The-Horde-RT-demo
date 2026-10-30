# Chapter-earned checkpoints through real host persistence

The existing natural-input chapter journey now continues through real manual/
autosave publication, fresh production-session reload and return-descent recovery.
This is a **test-only integration proof**. No gameplay, checkpoint policy, schema,
asset, CMake registration or platform adapter is changed by this slice.

## What is actually joined

The existing `NormalBoundaryJourney` walks the real imported chapter terrain,
defeats both guards and the Keeper using ordinary sword command edges, opens the
chest, claims the lantern and completes the physical rope ascent. It already
proved fixed-tick lower/upper latches, canonical safe respawn anchors, retained
held movement and multi-tick frame delivery. The new checks connect those earned
facts to the real `PlaySession` -> coordinator -> worker -> file codec/storage
path, rather than stopping at a latched in-memory value.

The earned path uses no `Seed`, `Record`, checkpoint import, authoritative player
pose, direct victory/reward mutation or arbitrary-position teleport. Normal save
projection still chooses the existing authored lower/exterior anchors; it does
not move the live player while earning progress. As before, this CPU test disables
damage and tutorial teaching, so it proves progression/persistence rather than
combat balance, player survival or teaching acceptance.

| Joined case | Verified invariant |
| --- | --- |
| Earned lower autosave | Real worker reaches success; stored record is LowerClaimed at the canonical lower anchor with an actual publication receipt |
| Earned lower manual | Real write in Slot 1; bytes equal the earned lower autosave |
| Fresh lower reload | A new production-configured simulation and PlaySession load the actual manual file; complete schema-1 record re-encodes byte-for-byte, victory/lantern/one rope and health survive, old action/held-input debt is consumed and the lower Climb prompt is usable |
| Earned upper autosave | Actual ascent plus continued held movement yields a new safe exterior record; earlier lower manual bytes remain unchanged |
| Fresh upper reload | Actual earned autosave loads outside with a usable Descend prompt, after explicit CPU Dungeon/terrain readiness publication |
| Interrupted return | One second of real ordinary-input descent with facade observation/polling retains the committed upper boundary and unchanged upper record; no mid-rope save is published |
| Recovery after interruption | Another new production session reloads that upper disk record on its safe exterior side, then ordinary interaction/fixed ticks complete real descent and landing with victory/claim/deployment retained exactly once |
| Isolation | No records appear in the other two slots; the immutable lower manual remains intact |

Each fresh load validates a recapture of the actual restored simulation against
all 162 earned bytes, not only a menu summary. It admits the real imported CPU
terrain and collision, including resident Dungeon readiness. This explicit host
readiness injection is not a GPU residency or presentation claim. The final
full-descent continuation does not observe/publish its new lower boundary through
the facade; it verifies motor continuation without adding another autosave case.

## Checks and exact evidence

Changed source: `tests/CampaignPlaySessionTests.cpp`, SHA-256
`112538a330d4ee2aa60b8d44119873a617a96c4428edc6f3839b7c205f131ba6`.
Source parent: `0a9d75882f6e64fc2f2b57465733bb63d6feb825`, with the frozen test
change and contemporaneous shared-source/library hashes recorded in
[validation.json](validation.json). The standalone implementation manifest is
[implementation-files.json](implementation-files.json).

Focused target `horde_rt_campaign_play_session_tests` built and passed in both
configurations: **106 checks, zero failures** each; CTest **1/1 Debug, 17.24 s** and
**1/1 Release, 3.73 s**. The existing **30-second timeout** and all earlier checks
remain intact. CTest source count/ordinal is not used as an acceptance shortcut.
No broader rerun was needed for this test-only addition.

Initial restricted Debug/Release MSBuild launches stalled with only their version
line, before compiler/test output. Only those two known sessions were stopped
with Ctrl+C; their logs remain separate. Standard local builds/tests then ran
sequentially with the required execution permission. This was an environment
spawn blockage, not an automatic-review denial or a failed gameplay assertion.
The first runnable Debug/Release attempts both passed. No global process was
killed and no test/deadline was weakened. Build, native output and executable
hashes are retained in the receipt; full logs stay ignored in `reports/`.

Audio/haptic manual revalidation required: **NO** for this test-only addition;
no runtime listener, cue, playback, feedback or damage/death behavior changes.

## Remaining gates

Fresh production objects here are not an OS process-death test. The earlier
[phone fixture/restart receipt](../2026-10-18-android-campaign-readiness-followup/README.md)
remains bound to its own APK and seeded records; it does not become physical
chapter-earned evidence through this host pass.

Still open: earning/persisting these checkpoints through normal platform play,
full Android/Windows RT route and equipment presentation, actual active-IO
interruption and power-loss/fault recovery, post-exit death recovery through the
player-facing policy, normal forest traversal/endpoint recovery, sustained
performance and owner controls/audio/visual/comfort acceptance. Schema 1 still
supports exactly LowerClaimed and SafeExit; no new forest endpoint or migration
policy is introduced. No owner data, app directory, device, desktop or public
service was touched by this slice.
