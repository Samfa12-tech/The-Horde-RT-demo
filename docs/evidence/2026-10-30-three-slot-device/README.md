# Three-slot device and Windows cursor acceptance

This is interactive acceptance evidence for the frozen packages below, not a
new gameplay implementation or full-chapter pass. Source checkpoint before this
receipt: `6078e19105159cd98856576dfcf51dac872565ce`; its
[CI run 41025879954](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/41025879954)
passed all six jobs. No runtime source was changed during these checks.

## Windows

Frozen R8 executable SHA256
`38144dd9d9176b2d4af16e9bf0e12034452438332231ff55eab803669c5c4e85`,
with the previously recorded 184 staged assets and isolated state root. Its
source is `3815ce2c` plus the committed `bdb660c2` keyboard-hook correction,
not a clean build of this later receipt.

The current explicitly authorized retry succeeded. Load Slot 3 entered the RT
outdoor scene. Click/drag mouse-look changed the camera; Escape opened Pause
with a visible pointer; mouse selection opened Load, Back returned, and Resume
returned to gameplay. A second acquisition/turn, Alt+Tab focus loss, return and
Pause left a visible pointer and usable Controls dialog. The owned window was
closed with Alt+F4 and its absence verified.

The published-build hidden-cursor symptom did not reproduce in this sequence.
No cursor fix was made, and this does not prove the older published binary or
every focus transition is correct. Computer Use screenshots include a pointer
overlay; menu selection and observed camera motion provide the interaction
evidence, not a new claim about every OS cursor-counter state. The prior
physical-Escape cancellation remains a separate, stopped attempt.

## Android

Connected device **SM-S948B**, Android 17. Only
`com.samfa12.hordelanternrt.debug` was updated in place, versionCode 10 /
`1.6.2-debug`. Installed APK SHA256:
`aeace11f5e0d07f2bf531a893fc54d543ce6676f0ba0a7d5e0ec3b276264d8fb`.
The frozen four-ABI package includes the `d618199f` copy changes; per-ABI
source stamps remain in the [earlier receipt](../2026-10-30-three-slot-saves/receipt.json).
The original and candidate signing certificate digests matched. Original debug
APK and files/preferences were backed up privately before installation.

All tests used `campaign_test_namespace=slots_20261030_r1`. Initially empty
storage showed three empty slots without dates. Three synthetic records were
then copied into that namespace only: an untimed manual and dated auto record
in Slot 1, and an untimed two-heart Lower Chamber record in Slot 2. Slot 3
remained empty. Fixture file dates were deliberately 2030; the dated record's
embedded UTC seconds were `1793346195`. The phone, configured for `en-AU` and
`Australia/Sydney`, showed **30 Oct 2026 6:43 pm**, not the copied file date.
The undated record honestly showed **Date unavailable**.

Observed checks:

- Portrait three-slot selection, missing-date and empty-slot states.
- Landscape direct Load of Slot 1 into the RT Safe Exit scene with three hearts,
  without a separate Resume-loaded-game step. The touch-aware Raise Lantern
  instruction was visible in the rendered scene.
- All three Save actions and Back were reachable by scrolling in landscape.
  The existing **150% system font** was preserved: this intentionally uses the
  vertical, scrollable layout rather than the three-column small-font layout.
  The initially suspected rotation defect was not established; no speculative
  layout patch was made. Rotation with the menu open retained usable content.
- Save into empty Slot 3 produced a 177-byte dated record and **Saved.** status.
  Cancel in its subsequent overwrite confirmation preserved all four test
  record hashes exactly. Confirming Replace save changed only Slot 3's bytes
  and timestamp (`1793351141` to `1793351213`, displayed 8:05 to 8:06 pm).
- Cold process restart retained the final date. Continue Slot 3, followed by
  ordinary Home/background and return, reached the same RT outdoor scene with
  three hearts. Back then opened a responsive Pause menu. No second Load or
  Resume-loaded-game action was required.
- The benchmark pace explanation was present in More; no benchmark or live
  report submission was run.

Android input was ADB-injected single-pointer/key input. It is not owner
two-thumb feel, physical-controller acceptance, natural checkpoint earning,
full chapter traversal, combat balance, sustained performance or ring/flask
admission. UIAutomator could not obtain an idle tree during continuously
updating gameplay; actual screenshots and the subsequent responsive Pause
menu establish the visual/interaction observations. No FPS claim is made.

## Custody and next gates

At **2026-10-30 09:09:46 UTC**, the owned debug process was stopped and Home
restored. Temporary rotation lock was restored to its original **free** mode;
system font remained 1.5. No brightness/network/security settings were changed.
Production and actor-worker APK hashes were unchanged. The original default
debug campaign save and lock remained byte-exact; schema-4 test writes stayed
inside the named test namespace. Per Sam's explicit clarification, the tested
debug candidate remains installed; old backups were retained, with no rollback
or data clearing. No owned monitor, transfer, window or device test remains.
Phone and Windows foreground are released.

Private raw XML/screenshots, hashes and custody receipts remain under ignored
`reports/phone-three-slots-20261030/`. No device serial, private preferences,
original APK or owner saves are included in this receipt. Next acceptance is
natural chapter/campaign progression and real input feel on a fixed build;
the two original field crashes and other documented 1.7 gates remain open.
No merge, publication, deployment or feature enablement occurred.

Subsequent [owner observations](../2026-10-30-owner-teaching-followup/README.md)
confirm real menu cursor visibility, Alt+Tab and Slot 3 save/load on the same
R8 artifact. The earlier overlay limitation remains a limit of that automated
attempt, not an unresolved absence of any owner cursor observation. Separate
Keeper-audio and early-teaching findings are tracked in that follow-up.
