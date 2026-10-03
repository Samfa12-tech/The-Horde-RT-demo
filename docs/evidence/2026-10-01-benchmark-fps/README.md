# Interactive benchmark render-loop counter

Current continuation: the [finite live record](LIVE_STATUS.md) supersedes the
October1 no-device statement below. Exact S26 update/cancel/restart/reset display
rows pass on the retained October2 Debug APK. Windows interactive display and
matched observer overhead remain open; the counter stays off in unattended runs.

Small shared counter within `ShowcaseBenchmarkRun`, no new telemetry framework.
Interactive Windows/Android benchmarks opt in. It displays the mean of the last
up to60 valid successfully RT-presented native render-loop intervals and its
inverse FPS, explicitly **not display Hz**. Includes warmup without admitting
warmup into reports; resets at lap/restart/invalid timing, vanishes on cancel/end.
UI text refreshes after the first valid interval and at approximately500ms of
completed intervals. Android's existing poll can impose additional UI latency.
Three-line native status areas avoid hiding the counter behind the old ellipsis.

Default/off, unattended Windows, Android run-ID automation and every frozen
lantern case collect/format no live counter. Original persisted statistics and
report bytes are unchanged; the enabled/disabled deterministic twin test proves
that contract. This is usability, not an engine speedup or display-pacing proof.

Fresh MSVC Debug/Release real RT app links; focused benchmark/lantern/launch/report
tests4/4 PASS (6.84s/3.90s). Android Debug four-ABI build/lint SUCCESS1m8s,53 tasks;
lint0errors/38warnings. No phone install, live HUD usability, actual interactive
Windows UI, or observer-overhead acceptance claim. Initial wrong app target,
missing auxiliary test executables (Not Run, not assertions) and missing Java
ViewGroup import failures are retained alongside corrected passes.

Owner steering now prioritises the separately tracked staged Mobile RT experiment.
Counter live interaction/exit/cancel and matched observer-overhead checks remain
open; do not enable it in phone performance experiments before those checks.
Audio/haptic manual revalidation required:NO: no audio routing or semantic change.
