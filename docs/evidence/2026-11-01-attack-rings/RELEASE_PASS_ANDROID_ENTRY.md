# Android entry configuration and save destination checkpoint

This source checkpoint starts from `d0bb5d7eb770ce11b638d028b1b34466f7f78618`.

## Changes

- Normal Android New Game reconstructed the simulation using the production config, dropping the private candidate's admitted revival flags. Startup, world replacement, and first/subsequent New Game now call the same application simulation factory. Retry and campaign load preserve that configuration. Public builds still default to revival OFF; private admission and presentation readiness remain separate gates.
- New Game previously accepted corrupt or unreadable save destinations even though persistence deliberately refuses to overwrite those records. Both manual and auto destinations must now be known empty or valid. Android controls, detached callbacks, and the native Android/Windows command owners enforce this before replacing the current game or preparing new resources. The chooser explains that files were kept and asks the player to choose another slot. Valid-save confirmation and load fallback remain available. No corrupt bytes are deleted or quarantined.

## Checks actually run

- Four focused native CTests passed: `horde_rt_application_config_0_tests`, `horde_rt_application_config_1_tests`, `horde_rt_campaign_play_session_tests`, and `horde_rt_windows_campaign_menu_tests`.
- Configuration tests exercise public OFF/private ON, first and second New Game, Retry, route reset and committed load using typed restore readiness and synthetic support geometry. These host fixtures do not certify audiovisual assets or device presentation.
- Persistence/menu checks reject corrupt destinations, preserve corrupt and valid record bytes, retain independent usable slots and prevent a forged disabled Windows choice from dispatching.
- Android `CampaignMenuStateTest` (13) and `CampaignMenuIntegrationTest` (49) passed. The old integration expectation that an unreadable autosave could confirm New Game was corrected; valid-save overwrite confirmation remains tested after that destination becomes empty.
- Windows `horde_rt_diagnostic_window` Release build passed with the existing private/isolated acceptance configuration. Android `externalNativeBuildDebug` for arm64-v8a passed with the existing private runtime directory. No APK was packaged or installed for this slice; no application or device was launched.
- `git diff --check` passed. The 151 parked-file baseline is checked separately before backup.

## Remaining gates and scope

This is a source/build checkpoint, not a release or owner interaction pass. Existing catalog captures describe the previous Send artifacts and cannot certify newly rebuilt binaries. Freeze the final repaired artifacts before catalog activation/recapture.

Pending audit slices: already-paused Windows Showcase background cadence, individual Win32 Standard benchmark apply diagnostics and restoration, legitimate updates to the Keeper/waterfall/spatial-audio source suites. Version proposal remains separate: 1.7.0/code 11 is locally unassigned, with Play Console availability unverified.

Device touch/focus and owner visual/audio checks remain pending, including ground-aware footsteps on dirt/grass and other surfaces, the complete lesson/kill/cry/revival/live-damage transition, fireflies, bone particles, lich goo, sheath, red ring readability and treasury/waystone context. No report was sent and no backend/site/catalog deployment was performed here.
