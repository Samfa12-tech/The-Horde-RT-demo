# S26 interactive benchmark FPS counter

Finite display/cancel/reset admission, not a performance experiment. Exact
SM-S948B Debug APK35776e1c is documented with post-install byte identity in the
[report-UI record](../../2026-10-02-s26-report-ui/README.md). Existing native
benchmark and unchanged counter source2cb19de; no automation run-ID intent,
renderer change, reduced resolution, sampling change or rebuilt artifact.

Actual paused-menu Run benchmark launches a short prefix. `fps-first.png` reads
12.1FPS / mean82.3ms / last60frames / waypoint1/13 opening. `fps-second.png`
reads14.9FPS / mean67.0ms / last60frames / waypoint3/13 shadow. Back cancels
before route completion; `fps-cancel-ui.xml` shows only the paused menu.
One fresh short restart (`fps-restart.png`) reads12.8FPS / mean78.2ms /
last**8**frames / waypoint0/13 opening: the rolling window resets, not stale60.
Second Back returns to menu (`fps-restart-cancel-ui.xml`), then phone Home.

Visual inspection: FPS, mean frame time and rolling count remain readable;
third-line long checkpoint label is intentionally ellipsized. The caption says
RT loop / not displayHz, avoiding a refresh-rate claim. Display-rounded reciprocal
values are consistent. These Debug snippets are neither matched Shipping evidence
nor a completed full benchmark, a sustained-performance acceptance or an overhead
measurement. Do not replace earlier performance evidence with them.

The live UIAutomator dump could not obtain idle. No stale XML or invented numeric
hierarchy is used; screenshots supply the actual changing display evidence.
Both paused-menu dumps required successful fresh writes before pulling and contain
the Horde Debug package only. No personal-screen capture or audio audition.

Phone rows complete; next unfinished row is Windows interactive display. Counter
on/off overhead remains unmeasured. No repetition of these phone checks without
a source/validity change. Audio/haptic manual revalidation required:NO.
