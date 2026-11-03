# Owner follow-up: bounded input and lesson fixes

Private development checkpoint based on b592c38a. This is not a completed natural tutorial or release candidate.

## Implemented

The shared dodge lesson accepts spatial evasion of a nearby committed red attack as well as the existing real invulnerability contact. A new accepted dodge must begin within 1.7 m of the unobstructed attacker, during its last 0.55 s of windup or active phase. The player must actually move at least 0.25 m during that dodge, clear the actual 1.55 m damage reach, and remain clear when the attack becomes active. Random distant rolls, early rolls, blocked movement, non-red attacks, cancellation and Skip cannot award this witness. Damage windows and hit resolution are unchanged.

Sequential practice now briefly acknowledges accepted outcomes and failed contacts, with the next lesson and a clear-reach dodge hint. Existing actual parried-stagger feedback, imminent timing cues, optional slowdown, Ready/Skip and accessibility behaviour remain. Two new semantic cue values fit the existing four-bit Android transport. The Windows ANSI prompt uses ASCII punctuation, fixing its literal UTF-8 em dash display corruption.

Normal Windows R no longer immediately resets the route. Controls text and the native menu's displayed R shortcut are removed; deliberate existing menu commands remain.

Windows missing-profile graphics now select the existing Platform Defaults (particles LOW), rather than always constructing a historical migration with particles OFF. Real legacy display preferences retain their migration, and valid explicit OFF/Standard settings remain. Dialogue defaults to 100% on both platforms; existing saved values, including mute, remain authoritative. Android explicit Reset also uses 100%.

## Checks and limits

Six focused host suites pass: combat foundation (1,843 checks), gated revival integration, Windows combat prompt, graphics settings, real Windows INI persistence, and desktop controller input. New production-waterfall probes earn dodge by actual backward movement with both keyboard and analogue-controller axes and zero invulnerability contacts. New finite observer probes reject remote, blocked, early, non-red, cancelled and skipped cases. INI probes verify absent-profile LOW and retained saved OFF/Standard. The historical old-chamber lesson fixture cannot move under current authored body clearance, so its invulnerability-only witness is retained separately from the new production-arena movement evidence; no collision rule was weakened.

Android CombatTeachingPreferencesTest passes 18 tests and CombatTeachingUiSchedulingTest passes six. Local native application builds and artifact hashes are recorded in OWNER_FOLLOWUP.json. No application was launched and no phone was installed or driven. All 151 parked owner files match the supplied SHA-256 baseline.

Audio/haptic manual revalidation required: YES, because fresh/reset Dialogue gain changed. Automated checks do not establish listening, touch, controller feel or RT presentation.

## Exact remaining work

- The owner's natural four-skill -> protected practice kills -> audible lich cue -> visible revival -> clear live-damage handoff is still incomplete. This checkpoint does not remove the existing Finish flow or pretend an automatic live-damage exit satisfies it. Normal revival remains OFF.
- Normal Release Windows cry playback is compiled out by `_DEBUG`; its only loader is a hash-checked private debug_audio WAV behind HORDE_DEBUG_LICH_REVIVAL_AUDIO. Neither normal platform caller registers ConfigureSkeletonRevivalPresentation. The authored clip/private cue needs legitimate asset admission and normal readiness/recreation/audio integration, plus listening and natural-play acceptance. No substitute cue or LFS download was introduced.
- Controller prompt source issue remains unproven physically: XInput/WinMM button/stick input sets controller glyphs, then any keyboard key, left click or nonzero captured mouse movement sets keyboard glyphs. The presenter recalculates text each refresh. Controller-specific prompt text is tested; no speculative input-ownership change is claimed as a fix.
- Standard fireflies remain sparse existing pockets at (-29,-19), (-12,1), (-7,22), four motes each, with 12 m visibility, Standard quality, showcase rescue route and admitted terrain-height prerequisites. Owner forest visibility complaint remains open; no effect budget was increased.
- Hold Attack to sheathe, cold-start responsiveness, red-ring side-angle readability and waystone/treasury-road contextual clarity remain open. No new town branch, ring rotation or loading-spinner concealment was added.
- Physical Skip, save/load/retry and natural handoff acceptance remain open. No issue closure, merge, release, signing, paid asset work, hosted CI or live transmission occurred.
