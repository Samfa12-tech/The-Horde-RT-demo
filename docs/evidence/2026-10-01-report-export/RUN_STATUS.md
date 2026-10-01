# Fresh integration CI and bounded Windows continuation

Runtime source69afe4e/shared3e0f9be. Evidence checkpoint
ca58f0956637f9063fe1cda42989ad5f74daa47d is pushed normally to both engineering
and isolated profile branches; original dirty engineering checkout remains untouched.
No main merge, force push, release or publication.

## Current-source CI

Both completed runs are **SUCCESS**, not old-job reruns:

- [Branch push36853938787](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36853938787).
- [PR integration36853945981](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36853945981).

Actual eight job logs were fetched and inspected locally. GCC55/55, Clang55/55,
MSVC57/57 and focused Vulkan CPU-host15/15 pass in **each** run. Newly included
`horde_rt_windows_playtest_report_ui_tests` passes0.61s push /0.60s PR. Compiler
and CPU-host fixtures do not certify physical RT presentation or performance.
Verbatim `host/ci-push-ca58f09.log`397,288B SHA-256
7F2E97F7740CD8FC03C8A847C9F05FB565B697D197E1F045330F1D1B6F0B4965;
`host/ci-pr-ca58f09.log`390,104B SHA-256
79C73DF634FC8766B2418DBB3FCF7F45CAB145645D6A38DF7369E6CC2B8E1493.

Fetched PR merge4aa5e0aca0cffc3fe395b05c4db4d191b756b563 has exact parents
bda1b99a62e1de883273dd21f267bcfc92bc5490 and
ca58f0956637f9063fe1cda42989ad5f74daa47d. Its tree
a29cbd6c9beabd5b208a86dfacc13be81b4a5145 equals the engineering head tree.
This is non-mutating integration validation, not permission to merge.

## Windows actual application / picker attempt

Exact built Debug executable SHA-256
C9FE9DF0D1B41E150C91430725AE5C563A092E41A8FE7F4A20F4435A0D9B4D7D.
The bounded smoke launches only this process, targets its PID/window class,
opens the actual Help/report command and verifies both consent choices off and
context control available. It explicitly selects category/impact/context/export
consent and opens the actual native save dialog. No owner application, private
directory-list text or clipboard is inspected or modified.

**Real Windows file-save gate remains INCONCLUSIVE.** The helper's expected
filename edit ID1152 is absent in this modern shell picker. First attempt stopped
before any destination/save action; its incomplete-dialog cleanup required killing
only the process it launched. That is not a crash/clean-exit pass. One bounded
second attempt waits10s for construction, but the expected edit remains absent;
inventory contains only control IDs/classes, no directory filenames. It cancels
the picker before closing the report/application and exits without forced cleanup.
Neither attempt chooses a path, writes a report or proves a SaveJson failure.
Retained negative receipts are under `windows/`; no successful mock is substituted.

One independently reviewed Luna follow-up switches identification to UI Automation,
restricted to that same owned PID/dialog and a uniquely labelled, visible/focusable
filename Edit. That single read identifies no such element; it stops before
selecting a destination/save and the owned process exits normally code0. Exact
EXE hash is unchanged; no JSON/ID/hash exists. This early metadata observation
does **not** establish that Windows lacks an accessible filename control.
`windows-real-save-uia-once.ps1` and its three logs are retained, no further
attempt or production edit. Total real-save attempts: **three incomplete**, zero
successful saves. Do not rerun them unchanged.

Startup capability report still says presentation pending/false while the
foreground form/picker blocks the application's first RT frame. This is not
mislabelled as a rendering defect or fresh RTX presentation acceptance.

**Next finite step:** capture settled picker accessibility metadata in the same
owned process (not an initial HWND/early snapshot), identify the actual filename
element without guessing, then perform exactly one approved-path/write check.
If reliable automation remains unavailable, retain this as an explicit desktop
interaction acceptance gap. Do not replay S26 saves, the performance matrix,
unchanged builds, or the already passing compiler/native-form fixtures to solve
this automation gap. Backend destination/authority and real remote delivery stay
separate and open.

The two synthetic S26 Download test JSONs were SHA-256 checked against the
archived exact bytes, then removed. Generated private picker scratch XML was
removed; no user files/app data were deleted. Final APK remains installed and
phone Home; S24 remains released with no operations after handback.

Audio/haptic manual:NO; no playback/event/asset or renderer source change.
