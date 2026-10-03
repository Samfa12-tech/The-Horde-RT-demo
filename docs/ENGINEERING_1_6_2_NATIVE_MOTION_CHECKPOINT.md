# Native motion evidence checkpoint

Debug-only Windows motion adapter source is frozen for the lead's coordinated
application build. Four literal scenarios use the sole production simulation,
ordinary measured frame delta, normal input/command publication, normal RT
record/submit/present and the existing fence/final-idle completion coordinator.
The scenario seeds checkpoint0/4/8 once before audio startup. Automated output
is muted and does not load/write user settings or progression; it does not
constitute changed-audio or owner visual acceptance.

`--validate-native-motion ABS_FRESH_DIR --motion-scenario SCENARIO` admits only
the four declared scenarios and optional explicit compute-backend selection.
Release rejects this Debug mode. Conflicting modes, orphan/malformed arguments,
more than16 entries and nonordinary absolute paths fail before output admission.
The run fails on external focus, resize, menu or configuration interruption.
Reports contain actual executable identity, bounded captures and exact joined
simulation/submitted/completed RT rows. Captures occur on stage changes and
approximately every2 actual simulation seconds, at most64; wall budget120s.

Independent source review found and verified fixes for: querying the next frame
slot after RenderFrame advanced its cursor; assuming numerical final-idle drain
order meant the last ledger row owned the newest output; and linking the portable
CPU fixture to a Vulkan-only target. The adapter now caches the submitted slot
before rendering, finds every field of the actual submitted identity plus surface
generation after drain, and binds each PNG to that exact row. Actual retry drains
accepted old-scope work before advancing the existing Retry lifecycle once and
requiring a newly completed current-scope presentation.

The isolated MSVC Debug launch fixture built with `/W4 /WX /permissive-` and passed
with zero failures; scoped diff checks passed. The scenario owner separately
reports12 production CPU schedules at15/60/120Hz passed, including strict scope
and retirement negatives. The lead's first full Debug compile found one adapter
reference to a nonexistent Windows resize field; it was replaced by the actual
owning swapchain handle/scene epoch plus the actual client-extent guard. Only the
declared real Retry may rebase the captured epoch. Coordinated rebuild is pending.
These are CPU/source evidence, not runtime RT results.
The lead still must build Debug and Release, run affected policy/CPU checks and
collect exact-build actual GPU scenario evidence. The ordinary driver idle and
image-readback calls retain production blocking behavior: a bounded external
owned-PID deadline remains required, explicitly recorded in the manifest.

Files: `WindowsMotionEvidenceLaunch.h`, `WindowsMotionEvidenceValidation.inl`,
targeted `DiagnosticWindow.cpp` integration, portable launch/scenario fixtures
and existing policy-fence host boundary. No shader, asset, Java or device edits
belong to this motion adapter slice.
