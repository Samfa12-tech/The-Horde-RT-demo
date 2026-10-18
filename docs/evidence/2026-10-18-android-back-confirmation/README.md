# Android Back exit confirmation

Back at the modern entry front page now opens a native Stay/Quit confirmation.
The legacy first-menu Back boundary uses the same dialog instead of immediately
calling `finishAndRemoveTask`. Stay is focused by default, including controller
confirmation. Back, cancellation and an outside dismissal keep the current menu.
Only an explicit Quit tap on the current, showing dialog at a still-valid exit
boundary can finish the Activity. Pause/destroy dismiss and revoke the dialog.

Existing Back priority remains: report decisions, graphics preview/restore,
Interface/settings, pending Play cancellation, RT Lab, diagnostics, entry side
pages, death/ending handling, benchmark handling and gameplay pause/resume.
The change does not modify settings, confirm a pending graphics trial, discard
its recovery marker, or connect Save/Load. Existing explicit Quit buttons and
automation/startup failure exits are outside this Back-only change.

## Host reproduction and checks

The added production `MainActivity` Back test first ran against unchanged Java:
one test failed because the Activity was already finishing before confirmation.
The baseline Gradle exit was 1, with the JUnit XML and raw output retained.

Tests exercise the real Activity without `onCreate`/native rendering, using the
existing Robolectric SDK 34/JNI shadow boundary. New cases cover explicit exit,
Stay focus, repeated Back, cancel and a queued old positive click, real navigation
to Settings before a stale click, controller cancel/default confirm without
combat input leakage, entry-side/pending-Play return, gameplay pause/resume,
onPause revocation, and graphics preview return with a pending marker and normal
restore-ACK state preserved.

Two intermediate failures are retained honestly. The first asserted native
negative-button dismissal before its posted message ran; the host test now
drains that message before checking dismissal. The next exposed a real stale-tap
window: cancellation hid the dialog before `onDismiss` cleared its owner field.
The positive handler now also requires `dialog.isShowing()`. No assertion was
removed or narrowed to hide the failure.

From `android/`, baseline selection:

```powershell
.\gradlew.bat --offline --no-daemon --max-workers=2 testDebugUnitTest --tests com.samfa12.hordelanternrt.AndroidControllerIntegrationTest.backAtFirstMenuRequiresExplicitExitConfirmation --console=plain
```

Affected suite:

```powershell
.\gradlew.bat --offline --no-daemon --max-workers=2 testDebugUnitTest --tests com.samfa12.hordelanternrt.AndroidControllerIntegrationTest --tests com.samfa12.hordelanternrt.GraphicsPreviewOptionsTest --tests com.samfa12.hordelanternrt.GraphicsMenuLifecycleTest --tests com.samfa12.hordelanternrt.MainActivityPlaytestReportLifecycleTest --console=plain
```

Final observed results and tested hashes are in [receipt.json](receipt.json).
Raw Gradle output and JUnit XML are local ignored evidence under
`reports/2026-10-18-android-back-confirmation/`.

No native Android build, phone use, APK installation, owner save access or release
action was performed by this slice. Current-build physical Back/gesture/dialog
focus/touch, background-return and controller interaction remain device acceptance
work. Production Java compilation and Robolectric checks do not close those gates.
