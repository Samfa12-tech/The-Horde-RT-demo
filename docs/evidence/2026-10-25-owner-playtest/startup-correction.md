# Boot readiness and responsive loading

Reported on the frozen `f27982cd` phone candidate: Load/Resume could be reached
before startup finished, then did not respond, including Cancel. The phone was
released before this correction; its precise JNI stall has not been retested.

## Reproduced defect and implementation

Three original actual-widget host tests fail on the original code: pending Play
opens the chooser, an old surface's presented state unlocks a replacement, and
early pause Resume/Load bypass readiness. The original entry availability method
ignored its state argument. Initial native probing and the complete asset-copy
roster also ran synchronously before the first menu draw.

`StartupAssetWork` now serializes that existing probe/cache/report preparation on
one process-wide worker. The UI never joins it. Activity epoch/attempt/lifetime
checks reject obsolete results, replacement writers cannot overlap, and native
surface start waits for required assets. Optional diagnostic-report IO cannot
invalidate otherwise prepared assets. Native Vulkan lifecycle and simulation
remain on their existing owner; no gameplay work moved to the worker.

The boot card is opaque and only admits ordinary menu interaction after the
current surface reports ready and its actual RT Entry presentation is published.
Old generation state, early/direct callbacks and stale Retry cannot bypass it.
Return to Menu publishes its native Entry request before waiting; native resets
the old presentation flag synchronously. Existing campaign slot/storage rules
and the opening-benchmark recovery owner are preserved.

Loading uses the current Horde identity, a small spinner, and plain text. Reduced
motion is static. There is no percentage, fake wait or loading screen on ordinary
rope travel. Retry/error and local Cancel/Back remain available without launching
a synchronous diagnostic reprobe. Immediate start rejection and runtime failure
use the same recovery card. Finished map artwork is still separate; the archived
flattened concept board was inspected and is not shipped as runtime art.

## Checks and retained corrections

- Original baseline: 3/3 readiness tests fail, retained in
  [original failure summary](startup-original-failure.json).
- First expanded group: 119 tests, six failures retained. Corrections include the
  real Return-to-Entry circular wait and stale Back-dialog ownership; prepared
  lifecycle fixtures now explicitly declare their already-staged state without
  removing start/retire/recreation assertions.
- Corrected group: 122 tests, zero failures/skips. Held worker, failure/retry,
  serialization, background/destroyed completion, asset-ready start, stale
  presentation, failed-start and no-synchronous-reprobe paths are covered.
- Full Android roster: 483 tests, zero failures/errors, six existing skips.
  The first full run retained one stale source-text anchor in WorldAssetStaging;
  it now targets the worker method while retaining exact asset roster/order.
- Debug assemble for all four ABIs and lint pass in the same corrected run,
  2m44s; lint zero errors and the same 83 warnings.
- The separate PowerShell asset-policy source anchors were likewise updated to
  the worker method/publication call; every existing missing/duplicate/wrong-path
  and late-staging rejection remains. All **201 cases pass**; the original anchor
  failure is retained alongside the corrected result and checkpoint receipt.

Actual-widget host renders check 320×568 and 640×320 at 200% text with synthetic
safe insets. Cancel and Retry remain reachable through the existing scroll owner.
Reviewed representative Loading/error top and action views show readable labels;
these are Android widget fixtures, not phone screenshots or proof of RT startup.
The full eight-view local set and metrics remain under ignored reports.

The APK contains the new worker in DEX, all four native libraries and the same
185 byte-identical runtime assets as the frozen candidate. No source/private
recording/scream draft is added to it. APK SHA-256:
`12711f634f9bb1e291156ea2a42fabf0002f960c9c37dd2f1fb6cd3620c368ea`,
206,289,547 bytes. It was built from `bf2dee1c` plus the recorded startup patch;
its source closure and final checkpoint identity are recorded separately.
This APK predates the separately tested correction to false parry-success
wording; it is not silently relabelled as a later complete combat candidate.

## Acceptance boundary

The new APK is **not installed**. `f27982cd` remains on the released phone. Cold
physical startup with early/repeated taps, slow/failure recovery, rotation and
Home-return still require an explicitly allocated device session. Host mocks do
not establish the exact phone deadlock cause or physical RT responsiveness.
Windows startup was not changed or newly foreground-tested. No settings, save
schema, quality default, audio gain or guard animation changed here.

Audio/haptic manual revalidation required: **NO** for this startup-only slice;
its semantic audio/event routing is unchanged. The separate encounter correction
retains its own feedback acceptance requirement.
