# Plain save-slot menu

New Game now shows strong Slot1/2/3 headings, a single Empty line when both save
types are genuinely empty, and a plainly labelled New game action. Occupied slots
retain useful location/heart information and explicit Manual save / Autosave
labels. Continue only opens the existing load chooser; it does not pick a record,
invent recency or resume automatically. Loading still waits for its existing
owner/rebase/presentation gates and explicit Resume.

Progress, empty/unreadable/error and cancellation messages use player language.
An unconfirmed save remains explicitly unconfirmed. An occupied-slot New Game
confirmation still explains that its manual save is kept and future autosaves
will replace that slot's autosave. Manual replacement separately warns that the
autosave stays. Keep existing saves remains the initially focused choice.
No save schema, IO, record selection, native resource or retry policy changed.
A captured page action now rejects detached controls after Continue navigation.

The two existing focused suites pass44tests. The first capture compile error
(unsupported Files.writeString in the Android test API) and first after-run's
stale unavailable-message assertion are retained under ignored reports; no native
configuration/readiness or safety assertion was removed. The ordinary integration
assertions still run when optional image output is disabled.

## Bounded visual comparison

These are **host-rendered actual Android widgets**, not phone screenshots or an
RT background. Before uses frozen6dbc43fd plus the test-only capture hook; after
uses the revised UI. The [capture receipt](capture-receipt.json) pins bytes and
scope. Both screenshots use the same288px panel; after provenance reports the
fixture resource display320x470 despite411x801/804x381 viewports. They support
copy/hierarchy comparison, not physical responsive-layout acceptance. No forced
scroll or image crop is used; title bounds remain inside the actual scroller.

| State/view | Before | After |
| --- | --- | --- |
| Empty, portrait | [before](before/android-widget-fixture-empty-411x801.png) | [after](after/android-widget-fixture-empty-411x801.png) |
| Occupied, portrait | [before](before/android-widget-fixture-occupied-411x801.png) | [after](after/android-widget-fixture-occupied-411x801.png) |
| Empty, landscape | [before](before/android-widget-fixture-empty-804x381.png) | [after](after/android-widget-fixture-empty-804x381.png) |
| Occupied, landscape | [before](before/android-widget-fixture-occupied-804x381.png) | [after](after/android-widget-fixture-occupied-804x381.png) |

The frozen6dbc43fd phone APK does not contain these changes. Horde released its
device lease before this source work; no new phone calls, installation or capture
were made. Physical portrait/landscape, controller and owner readability checks
remain pending on a newly allocated current candidate. APK build/lint and exact
new-head CI are separate from these focused tests.
