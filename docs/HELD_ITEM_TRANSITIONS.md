# Shared held-item transition authority

`HeldItemState` now distinguishes body stow from hand attachment and the existing
world-owned torch trajectory. Draw, sheath, stow and restore advance once per
live fixed tick. Their initial authored durations are 0.40/0.36/0.30/0.36 seconds;
these are policy values, not measurements of an imported animation clip.

The midpoint changes attachment once, with an increasing semantic sequence and
exact tick. Completion retains that provenance. Duplicate requests do not restart
the transition. Reversal or interruption settles at whichever parent owns the
item at that tick. A torch drop interrupts before changing ownership and retains
its resolved detach transform. A completed edge is never emitted on later ticks.

The snapshot-safe state rejects inconsistent parent/kind/progress, non-finite
timing, regressed ticks and exhausted event sequences. Pause leaves progress
unchanged; reset cancels the transition without reusing semantic IDs. This adds
no campaign save format. Production startup, encounter requests, animated stow
poses and attachment-timed audio still need integration; this commit supplies
the shared authority and regression coverage only.

Windows Debug app and `horde_rt_held_item_transition_tests` pass in the combined
seven-suite run recorded in `post-reset-foundation-native-tests-20261007-03.log`.
Tests cover every transition, duplicate requests, pause, reversal before/after
attachment, same-tick attachment/completion, copied-state recovery, malformed
state, regressed ticks, reset and dropped-torch ownership. Actual device motion
and owner acceptance remain open.
