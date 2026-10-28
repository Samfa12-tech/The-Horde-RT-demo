# Windows local statistics review: native interaction, 28 October 2026

The actual Win32 form passed a bounded synthetic interaction fixture at 120 DPI
(125%). This is local review/export evidence, not benchmark results or delivery.
The shared form was extracted unchanged from the accepted `469e0cc9` source;
renderer setup and default gameplay were not changed. Native provenance now
includes the compiled `.inl` fragments.

The fixture creates only its own non-activating window. It exercises production
controls, window procedure, typed preparation, UTF-8 conversion and handlers.
Clipboard APIs are replaced with owned memory, and the picker boundary with
cancel/checked-write destinations in a fresh test directory. Owner clipboard,
files, settings, running games, phone and network are untouched.

## Verified

- Unchecked consent blocks preparation/export; switching private/public resets
  consent, hardware, cooling, prepared bytes and saved state. Stale commands
  cannot export the previous audience. Explicit hardware inclusion is exercised.
- Private/public preparation, exact Unicode/UTF-8 Copy, failure then explicit
  retry, cancelled Save then explicit retry, immutable report identity/bytes,
  checked local writes/readback, and duplicate-Save prevention pass.
- Native tab-order query, fixed export footer, scrolling, close and font cleanup
  pass. The window remains non-foreground throughout.
- The normal window is 800x850 physical pixels at this host DPI. The 640x480
  capture **simulates a smaller monitor work area in the fixture's minimum-size
  response only**; it changes neither the real monitor nor product sizing rules.
  Actual client layout/actions remain the production implementation.

Six selected Release CTests passed in 3.20 seconds: summary reporting, native
summary interaction (30-second deadline), 40 source contracts, build identity,
desktop controller mapping and input-publication trace. The latter two use
synthetic inputs, not physical controller/keyboard operation. The Windows game
also compiled successfully; it was not launched.

The reviewed captures show readable consent/description, exact JSON and export
controls. The small capture is deliberately scrolled to its lower content.

![Public local preview, synthetic data](public-native.png)

![Private local review, simulated small work area](private-small-workarea.png)

## Failures retained and limits

The first compile needed the Windows SDK COM header for GDI+. A later test-only
rectangle name collided with the SDK `small` macro. An ordinary sandbox build
made no progress beyond file-glob verification and was stopped; the authorized
normal build-cache escalation completed, with no security setting change.

The first native run requested 640x480 but Windows correctly enforced the
production minimum, invalidating its scroll expectation. The next fixture
incorrectly left the synthetic model empty while opting into hardware, and the
real label validator rejected it. The fixture now supplies the production
Windows capture's explicit `Unknown` model. No product validation was relaxed.
Original failed logs/captures remain privately under ignored `reports/` and
`build/`; their hashes are in [the receipt](receipt.json).

Still pending: real OS clipboard/picker, modal owner/focus restoration and Escape,
physical keyboard/controller interaction, actual multi-monitor DPI transitions,
Windows RT gameplay, Android acceptance, private service delivery and the public
stats service/page. No reports were sent/published. This fixture cannot certify
any of those. Audio/haptic manual revalidation required: **NO**, no relevant
event, cue or playback changes.

PR conflicts were diagnosed separately [read-only](pr27-conflicts.md); no merge
or resolution was performed.
