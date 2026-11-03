# Windows pause cadence and Standard diagnostics

This slice follows Android entry/save checkpoint `290456df910b3425b971365cafb3b5375d6429b1`.

## Result

The pause policy required foreground ownership. AltTab while already paused therefore exited the 500 ms paused cadence into the unrestricted Showcase loop. Paused Showcase now retains its bounded cadence in the background. Focus changes, output loss, explicit wake and surface generation changes still trigger immediate work; unpause exits the cadence. Entry/Preview keep their separate background behavior. The existing owner loop continues refreshing the simulation time anchor while waiting.

Standard benchmark window application now captures each mutation's return value and immediate Win32 LastError before another API can overwrite it. Original placement/style/client observations are recorded independently. Fixed-client monitor fit and windowed-entry availability have explicit messages. Apply and restore mutation traces are emitted under `HORDE_BENCHMARK_WINDOW`; failed apply/restore messages retain the trace. Every original restoration mutation is still attempted independently. Owning RT acknowledgement, exact output/quality matching, cancellation and restoration gates remain in force.

The hidden HWND fixture applied a 1280x720 borderless client and restored the exact original rectangle/styles without showing a window or taking foreground. Invalid HWND calls failed closed and retained error 1400. Raw `SetMenu(window,nullptr)` returned 1/error 0 here: absent-menu failure was not reproduced and no speculative SetMenu behavior change was retained.

**Standard remains an actual RT runtime gate.** No normal Standard apply/measure/cancel/restore run was authorized or launched during this source/headless-fixture slice. These results do not identify the owner's earlier target-specific Standard failure or turn Custom into a Standard waiver. The next coordinated run can now distinguish failing Win32 calls, monitor/entry availability, exact client mismatch and subsequent RT acknowledgement failure.

## Focused checks

- Pause cadence and Windows benchmark launch/session tests passed.
- Hidden benchmark window API fixture passed, including invalid-handle error retention and foreground preservation.
- Waterfall route tests passed with tutorial ON (one active walking actor, second idle), tutorial OFF (authored distinct pair gait), and legacy contact policy. Existing placement, leash, sword draw, reset/retry and historical capture checks remain.
- Spatial audio smoke now checks both platform consumers call the shared `ChapterDialogueSpatial` policy with the frozen snapshot/domain. Functional speech checks retain world panning/range and rescue audibility; existing exact line/generation matrix ownership and no-resubmission checks remain.
- Rebuilt SimulationGameplay and CombatFoundation were green without changing Keeper assertions. The reported Keeper failure did not reproduce; three accepted hits, death/reward order and historical profiles were not weakened.
- Windows production Release executable compiled successfully. No Android rebuild was needed for this Windows/test-only slice.

This is not a foreground/device acceptance pass, release package, catalog activation or report submission. New source requires final artifact freeze and accurate catalog recapture before combined owner Send validation. Ordinary public revival/flask admission, manual audiovisual/gameplay checks, Android focus/touch callback evidence, final version assignment/Play Console check, release signing and publication remain separate gates.
