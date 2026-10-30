# Windows campaign menu and RT load handoff

Windows now exposes New Game slot selection from Enter, Load from Entry > More,
pause and death, and manual Save from pause. The native panel presents the three
manual and three latest-auto records, separate record states, occupied-record
confirmation, cancellation, failure messages, read-only recheck and explicit
loaded-game Resume. It uses the existing shared worker, schema and slot policy.
Normal records live in `FOLDERID_LocalAppData/HordeLanternRT/campaign-v1`.
Storage initialization failure retains a usable New Game chooser with a warning;
it does not silently disable Enter or promise that later saves succeeded.

IO runs on the existing bounded worker. The application thread retains the old
simulation and scene during load preparation, prepares complete RT resources
offside when required, and proves GPU/presentation retirement before cutover.
After commit it consumes old command/held-input debt, rebases the absolute camera
through a newer neutral paused publication, and waits for a matching fresh RT
presentation before enabling Resume. Output recreation invalidates old surface
identities and reissues the pending handoff. Presentation fences remain usable
after draining. New Game constructs its replacement before retargeting the slot.

The former Android-only load barrier is now shared, with an Android compatibility
alias. Its logic and existing phase values are retained. The native menu never
performs IO, mutates simulation, claims GPU readiness, or activates its owner.

## Actual hidden Windows run

The corrected run passed **26/26** checks on NVIDIA GeForce RTX 5050 Laptop GPU,
driver 610.47, Vulkan 1.4.341, using `RayTracingPipeline` and the normal 100%
render scale (1232 x 803). The exact Debug executable is pinned in
[validation.json](validation.json); it is `aaa3147d…f4ccfe`.
[hidden-rt.json](hidden-rt.json) is the unmodified runtime receipt.

The unshown HWND used real `WM_COMMAND` handlers, the actual worker/file adapter,
simulation restore and Vulkan RT swapchain presentation. It verified entry Load and same-batch Choose/Back cancellation both before any
active game and after a retained successful Load,
precommit cancellation, saved one-heart/camera/input adoption, paused result and
explicit Resume; a real injected worker read failure preserved the current game
and displayed a native failure; confirmed manual Save changed only Slot 3 manual
while Slot 1 remained active. The [six before/after hashes](record-isolation.json)
show the other five records unchanged, all still 162 bytes.

A diagnostic position in the **production** waterfall encounter then allowed
ordinary shared combat damage to kill the player. The actual death card's Load
could cancel back to that dead game, or load the saved two-heart record through
a fresh RT frame and explicit alive Resume. Finally the diagnostic reopened the
entry origin and exercised its real New Game slot/confirmation/owner path,
fresh RT frame, selected active slot and doorway-facing alive opening.

The owner remained hidden and the foreground HWND was unchanged. No foreground,
cursor capture, audio, owner settings, owner saves or device actions were used.
The RT storage image was read back and inspected: the opened lower rescue view
and modeled held lantern/sword agree with the loaded checkpoint. The PNG remains
under ignored reports; its hash is pinned, not a new generated Git artifact.
The capability report in that directory is the **initial probe**, before these
frames; its initial `rtScene.presented` field is not a post-load status report.

The first run is retained in [first-hidden-run.json](first-hidden-run.json).
Its other 15 checks passed, but its diagnostic death setup used the old encounter
coordinates, so the production player did not die. It supplies no death-origin
acceptance. Correcting the diagnostic coordinates did not alter gameplay damage.

The [intermediate run](intermediate-hidden-run.json) passed 24 checks before the
same-batch queued-cancellation cases were added. Its exact binary remains distinct
from the final 26-check build. The final readback has the same SHA-256 as the
inspected intermediate image.

## Checks and limits

The native chooser's hidden HWND fixture passed 43 checks in both Debug and
Release. Both complete Windows application configurations build. The affected
load barrier, scene adapter, desktop-controller, entry-handoff and pause-cadence
selection passes 6/6 in Debug and 6/6 in Release. The initial controller source
contract failed on the extended pause expression; its correction preserves all
existing RT Lab/overlay checks and additionally requires campaign pause and a
visible chooser. Both failed logs and final passes are pinned in the receipt. Android death Load
has [82 affected Java host passes and fresh four-ABI builds](../2026-10-18-android-death-load/README.md).
The [earned-route proof](../2026-10-18-earned-campaign-recovery/README.md) separately
joins ordinary chapter progression to real disk records and fresh-session return
descent; it must not be confused with these seeded native records.

Reproduction, from this existing checkout: create an empty direct reports child
named `horde-campaign-fixtures-*`, export six records with
`horde_rt_campaign_play_session_tests.exe --export-fixtures <absolute-directory>`,
then run Debug `HordeLanternRT.exe --campaign-validate-hidden --campaign-test-root
<absolute-directory>`. The validator rejects Release and non-fixture roots;
precommit cancellation uses an explicit preparation hold to remove fast-IO races.
Use a fresh fixture directory for each run. No record cleanup or owner migration
is part of this workflow.

This closes a bounded **hidden native UI/input/IO/RT handoff** checkpoint. It does
not certify visible-desktop physical input, naturally earned Windows/device
saves, OS process death, physical power loss, Android death Load on the new APK,
audio/controller/comfort, forest-end recovery, sustained performance or all 1.7.
No render scale, quality, shader, asset, save schema or retention change was made.
