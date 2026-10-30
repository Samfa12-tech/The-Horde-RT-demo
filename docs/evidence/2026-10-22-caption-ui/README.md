# Caption and lantern UI checkpoint - 22 October 2026

Source now keeps measured caption/control rectangles in one view hierarchy
coordinate space. It no longer subtracts a physical-screen origin from a
window-relative visible rectangle at a display cutout. Full text, scrolling,
explicit position, font scaling and accessibility remain available.

Owner's later instruction removes the visible dialogue Skip line button; native
dialogue timing and unrelated tutorial/menu recovery skip behaviours remain.
The lantern-pose button now uses an original32dp lantern/up-down pictogram in a
56dp touch region, with an accessible action/state label. Its existing Button,
listener, safe-area margins and controller prompts remain. Other contextual
OPEN/TAKE/CLIMB actions retain their existing presentation/semantics. The wider
circular/translucent redesign is deferred under the owner's19:35 clarification.
The [Briarhold reference](../../design/HORDE_1_7_UPSCALER_READINESS_2026_10_22.md#verified-briarhold-touch-reference)
was inspected read-only; nothing there was edited or launched.

## Validation and retained failure

Nine caption policy tests and two lantern control tests pass; the full Android
host lane passes **394 tests /55 suites, zero failures/errors/skips**. Android
arm64 Debug assembles with the existing private probe flags. This validates
source/contracts, not the final visual/touch result.

Earlier APK `712f070996124014832b9cf6bf4941e7fbac2d448fabfac926297be8d9d942a2`
showed a real complete Kit question at100% Auto/system font1.7 in landscape,
without the player-facing overflow diagnostic. That retained screenshot also
demonstrates residual control overlap and the large Skip label; it is not an
acceptance of the later coordinate/icon/no-Skip candidate.

Final UI APK
`fe3a51beb3a2d6385232dcccfdf8315785e969825e8a35a7de65be8894f46925`
matches the compatible installed update and public debug signer. A fresh
isolated launch of `chapter-kit-reunion`207 fails **before caption inspection**:
`reason=missing-terrain`, tick0, world generation2, terrain generation0,
sceneReady1, sceneTerrain0. The render loop then ends. Its150-second preparation
gate remains failed, with no retry/gate removal. The actual display was portrait
at this cold failure, despite the helper folder's landscape label. Native/render
sources are unchanged from the timing build; no UI or natural-game cause is
inferred. The earlier warm207 after203 worked, so cold versus warm terrain
admission requires a separate bounded diagnosis.

Corrected final caption/lantern appearance, portrait/landscape overlap and real
touch acceptance remain **pending**. Raw screenshots, errors, build logs,
source hashes and full Java XML remain ignored/local in
`reports/horde-outdoor-scale-20261022/` and
`reports/horde-phone-outdoor-baseline33-20261022-r1/`.

Phone release is verified at19:44 UTC: ordinary Back from diagnostics and Quit
through the fresh native menu, launcher foreground, no Horde SurfaceFlinger
layer, Activity stopped/destroyed, all helpers/capture/transfer/install finished.
All18 original save/lock hashes, settings bytes, font1.7 and rotation0 match;
zero new save paths. Production APK is unchanged. Debug may remain as an Android
cached PID with no active render/activity; no phone work is pending.

Audio manual revalidation required: **YES**, for the earlier outdoor obstruction
routing correction in `bb507d24`, not for these caption/icon edits. No listening
or haptic acceptance is claimed. The music/VO library transfer and continuous
speaker/listener anchoring are separate remaining work.
