# Isolated modelled arm follow-up — SM-S948B, September 27

In-progress Phase 3 evidence, **not owner acceptance or Shipping performance**.

- Installed ARM64 Debug APK SHA-256:
  `fce8b40cb0a80b403f49edf4318b91ed9147c69a0d99da68269e31578c8229c9`.
- App: `com.samfa12.hordelanternrt.debug.viewmodel`, version 1.6.1-debug/code9.
- Native/Java build at `e706f61`, plus the offline .099 asset candidate whose
  processor/recipe is committed in `54493cf`. The latter changes only offline
  scripts/documentation; runtime source is identical. The runner records
  `54493cf` with a dirty tree because unrelated `.superpowers` scratch remains.
- Build succeeded via `:app:assembleDebug`, candidate directory
  `C:/Dev/tmp/horde-gauntlet-proportion-20260927-a`, ARM64-only override.
  The current Gradle redirect resolved the APK; conventional output was not
  assumed. Installed bytes match. No data was cleared or production app replaced.
- Immutable local APK:
  `C:/Dev/tmp/horde-android-arm-followup-20260927-a/HordeLanternRT-1.6.1-viewmodel-debug-arm64.apk`.
- Exact device: Galaxy S26 Ultra **SM-S948B**, Android16/API36. This does not
  certify S24/S25, signed Release, Shipping/Diagnostic or compute/pipeline parity.

## Automated and live evidence

`run-20260927-073831` passed 13-waypoint replay, eight modelled-route captures,
strict ASTC, honest hardware RayTracingPipeline presentation and Home/resume.
Retained state JSONs, summary and validation report describe those frozen
captures. High/low glass still records **3/1 transport overflows**: open failure
diagnostics, not an accepted physical-glass result or an improvement claim.

`live-motion.mp4` is a separate **unfrozen physical-phone screen recording**,
45.021111 seconds, SHA-256
`e5d446a626e7a71c9d888476752e534da5e5cc81b2e4985a881b3562a926afdb`.
720x1560 is the recording encode size, **not** the RT render extent. The game
remained at explicitly requested 75% (1080x2235), Mobile/Diagnostic, authored
scene settings. Saved render preference50 was preserved; default75 applies to
unset/reset preferences and is not used as an optimization claim.

The existing high-carry checkpoint's 480-frame measurement finished before
motion recording. The post-finale card was dismissed with Continue. Looking
was restored with ordinary right-side touch; the Java debug checkpoint helper
does not set yaw/pitch for development IDs136–143, so leaving its frozen mode
otherwise reuses the prior touch pose. That staging limitation remains open.
No runtime geometry was hidden or moved solely for this recording.

Ordinary ADB touchscreen input exercised sword attacks with high carry, walking
and looking, Lower, low-carry attacks, return movement/look, Raise and further
attacks. The full video is retained, including idle gaps. Reviewed 1-second
overview samples and 8Hz close samples of high/low attack sequences show no
visible sword/cage crossover, maintain distinct hands, and show the lantern
and arm responding to movement and high/low transitions. This is bounded live
coverage, not proof for every possible pose or a subjective smoothness pass.
The host full-arc geometry sweep is documented in the linked engineering note.

The first attempted recording was **invalid for acceptance**: the finale card
interrupted play and subsequent input touched RT Lab controls. It remains local
as `invalid-ui-interrupted.mp4`. Restore Authored was used before the retained
recording. It is not an A/B run and must not be mixed into performance evidence.

Diagnostic warm high-carry setup measured window averages83.613/83.514/83.589ms;
live logged windows reached roughly84–99ms while recording. These are CPU
window averages, not per-frame medians, and are not matched before/after or
Shipping results. The heavy lantern performance gate plainly remains open.

## Owner check

The new candidate is left in-game, paused through Menu to avoid heating the
phone. Resume the **Horde RT Viewmodel Candidate** app:

1. While holding the lantern high, tap Swing twice for the down/up combo;
   repeat while walking and looking. Check blade, sword hand and both arms.
2. Lower the lantern, repeat the combo and movement, then Raise during motion.
   Check secure grip, elbow/wrist compliance and lantern swing.
3. Judge gauntlet size against cuffs/forearms from normal forward and modest
   up/down viewpoints. This is the single +10% candidate, not a roll/weight sweep.

All three owner acceptance issues remain open pending feedback. Look-down
torso/legs and final nonduplicating visibility also remain open. Production
block arms are not retired. Fast render-scale resize work follows Phase3
acceptance; it was not started here. Audio/haptic manual revalidation required:
**NO**, because event-time inputs, cues and feedback transport were unchanged.

The owner's subsequent yellow-wash observation adds an open lantern-lighting
correctness check: the back of hands/arms and sword should not receive direct
lantern light through occluding geometry. These images/video do **not** accept
that lighting. Emitter/shadow/indirect-medium causes remain to be isolated.

See [causes, changes and host evidence](../../ENGINEERING_1_6_1_LIVE_ARM_FOLLOWUP_2026-09-27.md).
