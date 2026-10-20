# Android single Apply/save - local 1.7 checkpoint

Continues a050d779 in the existing WP2 worktree and draft PR27. The approved
[feature inventory](../../design/HORDE_1_7_FEATURE_STATUS_2026_10_18.md) identified
separate Use/Keep as an unimplemented player-facing 1.7 simplification. Existing
saves, crouch, subtitles and paused rendering already have implementations.

Ordinary Graphics, authored preview and Image now offer Apply, Restore and Back.
One Apply requests the complete clicked settings tuple, then saves once after a
matching serial, surface generation, requested/effective tuple and successful
hardware-RT presentation. Native confirmation remains required. Merely editing a
draft or comparing preview choices cannot save. Apply disables during the change;
Back, Restore, native failure, surface replacement and backgrounding revoke its
save intent. Failed storage retains recovery metadata and restores the previous
saved native settings. An old failure receipt cannot cancel a newer pending Apply. If native Restore
returns no request, the direct fallback retains its own serial and rebases the
edit session only after that exact saved-settings RT receipt, reopening Apply.
Help and accessibility text describe this flow. Three-button docks retain native
48dp minimum targets, large-font reflow and persistent Restore/Back controls.

Mobile fresh/reset Mist Off remains the owner's selected policy. This change
preserves existing explicit saved choices, Windows defaults, both Keeper lights,
RT backend/quality, assets and gameplay. No new performance result is claimed.

## Evidence and remaining acceptance

[Exact source/build/check receipt](receipt.json): Android Debug assembles for all
four configured ABIs; all 369 Java checks in 51 suites pass, with no skips. The
90 graphics/preferences/controller checks cover full-tuple negative receipts,
transient previews, one-time persistence, stale clicks, native/storage rejection,
Restore, surface changes, background cancellation, focus and accessible layouts.
JNI is replaced in host UI checks; they do not prove physical RT presentation.

Two intermediate failures were obsolete UI assertions: an image-dock index still
expected four actions; the broader controller route still looked for Use/Keep.
Both were corrected to the approved three-action behavior. Review also caught a
zero-request Restore fallback that could leave Apply disabled; the correction and
old-serial/wrong-requested rejection regression pass. Saved-versus-draft label
assertions remain covered. No safety gate was removed. The existing independently classified native inventory/source-pin
failures were not rerun or relabeled as passes by this Android-only milestone.

The new APK is compile-only, uninstalled and unlaunched. No phone commands,
preparations, combat inputs, public push, release or Reef operations occurred.
Owner assets/saves and canonical-folder changes remain untouched. The previously
denied combat action and inconclusive Keeper mist trial remain stopped.

Next acceptance step, when physical testing resumes: use this exact APK to change
Mist Off with Apply, verify the Saved value after its RT presentation, then check
Back/Restore and Home/resume during an outstanding change. Repeat touch/controller
and large-font navigation on hardware. These gates remain open; older phone
receipts do not cover this UI. Afterward return to the recorded critical-path
combat/progression and warm-performance gates, without adding heavy outdoor art.

Audio/haptic manual revalidation required: NO. This slice changes graphics menu
transaction controls and help, with no gameplay feedback, cues, gains or events.
