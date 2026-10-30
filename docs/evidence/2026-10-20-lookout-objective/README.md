# Player-opened lookout objective - 20 October 2026

The approved optional endpoint panel is now implemented on Android and Windows.
After earned completed-story lookout recovery, the ordinary pause menu offers
**Lookout objective**. It never opens on arrival. Continue exploring, Back/Escape
and controller cancel close it with neutral input; Return to menu uses the
existing branded RT entry path without resetting or saving the campaign.
The text says onward village travel belongs to a future chapter.
[Exact tested sources and artifacts](receipt.json).

Shared read-only eligibility requires a validated completed-forest boundary,
current ready Lookout, living grounded player near F04, completed grounded Kit
and no active final speech. The panel cannot complete or earn progress. Android
uses immutable surface/world identity from native publication and revokes stale
callbacks after lifecycle/menu replacement. Windows validates the actual native
source HWND/current identity and retires controls before dispatch. All exits
clear held movement/look/combat/dialogue commands; Windows repeat-key movement
and reset cannot leak through continuation.

| Validation | Actual result |
| --- | --- |
| Android UI | All 381 tests / 52 suites pass, zero failures/errors/skips. Nine new tests cover explicit opening, current identity, stale callbacks, neutral input, lifecycle, Back/Escape/controller and Return without save/reset/preferences. [Log](lookout-objective-java.log). |
| Shared policy/persistence | 1026 checkpoint checks and 145 play-session checks pass. Eligibility rejects dead/missing-boundary, active speech, distant and preparing-zone snapshots across 12 endpoint restores. [Run](lookout-objective-native-tests.log), [details](lookout-objective-native-tests-detail.log). |
| Windows native panel | Final 44 checks pass with a hidden WS_EX_NOACTIVATE/tool window, real native controls, command source validation, two DPI layouts, close/reopen/revocation/Return. Same foreground HWND at all sampled boundaries; parent never shown or observed foreground. [Rerun](lookout-objective-native-panel-tests-2.log), [HWND detail](lookout-objective-native-panel-tests-2-detail.log). This is native CPU UI evidence, not physical controller/visible RT acceptance. |
| Builds | Windows Release app and all four Android Debug ABIs pass. APK SHA a0f302c02707d673df5e448e4f1a9a471f7b19b177ce1546e5787efa9c6c60e1, 186149765 bytes. No new install or app launch. [Windows](lookout-objective-native-build-2.log), [Android](lookout-objective-android-build.log). |
| Fixture export | New explicit create-only exporter emits one 162-byte completed-lookout autosave using actual terrain, production checkpoint validation and restore. Repeat into occupied directory rejects exit2 with identical bytes. [Receipt](fixture-export.json). It is seeded noncombat recovery, not earned progression; never staged to phone. |

The first Windows panel test failed only foreground equality (42 checks, one
failure). It recorded no HWND identities, so the cause remains **unknown**.
The failed logs remain above. The corrected fixture explicitly prevents parent
activation, directly sends native parent notifications instead of simulated
mouse clicks, and records before/after HWNDs while retaining the strict continuity
assertion and adding a separate never-shown/never-foreground assertion. The
production keyboard/controller cancel path also sends the native command directly.
No failure was hidden or assertion weakened. This selected lane does not close
the previously recorded unrelated inventory/source-pin failures.

## Phone custody and remaining gates

Sam's direct instruction in this Horde thread was:

> mist off on mobile because when it is on, during the keeper fight, we also have those two lights on and all together it was pretty chuggy.

Saved mobile Mist Off therefore remains the owner-selected setting. A completed
read-only idle check matched the previous corrected-graphics receipt: saved Off,
pending false, exact settings SHA and all 12 campaign files/locks unchanged;
Debug PID absent. At Sam's unplug request the phone was immediately released:
no install, transfer, fixture staging or launch had started, and no further
phone operation followed. [Idle/custody receipt](phone-idle.json). Installed
APK remains c5a11bb2, not this new panel candidate.

The next local bounded slice is isolated hidden Windows **hardware-RT** lookout
Load/panel/Return acceptance using that create-only fixture. It must preserve
source record bytes and foreground custody. Physical exact-APK endpoint/player/
Kit placement, touch/controller/lifecycle and return-side acceptance remain
deferred until an authorized phone session. Natural earned saves/death/full-route,
combat contact/teaching, opening/geography/Kit acting and sustained warm performance
remain distinct acceptance requirements; no heavy outdoor content was added.

Audio/haptic manual revalidation required: **NO for this panel**. No semantic
event, audio/haptic asset/backend/gain or event-time data changes. The previous
endpoint restore listening gate remains open for absent completed VO replay and
appropriate lookout ambience.

The banked zero-mist shader exit remains about 18% lower GPU time in the cleared
tomb, not a Keeper two-light or sustained 30 FPS pass. Rejected trials and the
zero-frame inconclusive arm attempt stay rejected/inconclusive. No denied combat
action, Keeper timing, public push, release, merge or Reef operation occurred.
Work remains in the existing WP2 worktree and draft PR27.
