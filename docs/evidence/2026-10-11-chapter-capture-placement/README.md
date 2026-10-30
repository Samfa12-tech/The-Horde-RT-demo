# Authored capture placement repair

Source parent: `5b8d7760f5547c06326648fe71bee7714a454938`. This is a diagnostic repair, not chapter completion or a passing held-lantern capture receipt.

The shared development helper staged its camera, then imported reward state twice. Rescue-enabled reward reconstruction correctly returned to the lower safe side, but that overwrote the diagnostic camera and placed the frozen lantern pendulum at the wrong hinge. Eight capture poses reproduced sixteen failed camera/hinge assertions before this change. A private placement policy available only to the existing friend staging helper now preserves the authored position/support before resolving the hand and pendulum. The public import API and its safe recovery policy are unchanged. No extra simulation step, command or feedback is introduced.

## Validation

- Debug: development-static-asset and rescue-journey integration tests, 2/2 PASS (16.54 s).
- Release: the same two tests, 2/2 PASS (2.15 s).
- Additional Debug route foundation, vertical support, rescue rig attachment and rescue adversarial tests: 4/4 PASS (53.54 s).
- Each of eight reward poses checks exact camera, support, hinge, pendulum transform/history, tick count and empty feedback; a subsequent public import must still return to the lower landing.
- Windows Debug application built. Exact executable/shader identities and actual backend are in [receipt.json](receipt.json).
- Shader source, embedded binaries, thresholds, quality and capture assertions are unchanged.

## Inspected native frames: strict gate still FAIL

Two hidden, input-free Pipeline captures were opened and inspected: [held-high corridor](116-lantern-held-high.png) and [held-high chest](135-lantern-chest-held-high.png). They now show the authored corridor/chest views and correctly placed lantern. Both runs remain `complete:false`; their errors are preserved in the receipt. Earlier failed captures remain retained locally.

The remaining failures are separately attributable: the production configuration starts these historical fixtures with the sword stowed, while the pixel policy requires a sword; the shader's torch counter counts metadata identity 1, which is also used by permanent Keeper wall-torch instances. The chest image visibly contains a legitimate wall torch. This does not establish a duplicate held torch and does not justify removing world geometry or relaxing the assertion. Correct fixture equipment state and unambiguous per-instance diagnostic evidence still need resolution. QueryCompute was not rerun for this diagnostic-only change after the Pipeline gate exposed these remaining issues; Android/device acceptance is NOT RUN.

No gameplay/voice/haptic semantics changed by this repair. Existing owner feedback remains scoped to its tested builds. Terrain, visible Kit, real zone resource handoff and the full chapter visual gallery remain unfinished.
