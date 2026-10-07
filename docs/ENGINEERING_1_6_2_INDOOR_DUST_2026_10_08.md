# Optional indoor dust prototype, 8 October 2026

The owner expanded the active 1.6.2 scope with a bounded indoor-dust prototype.
This specifically supersedes older optional-dressing exclusions for this feature;
all other tomb/adventure/release boundaries remain. The supplied research brief
reviewed 62b6d40e; the actual clean starting checkout was ea5329bb on the existing
PR18 handoff branch. Existing guard, combat, mist and physical-device evidence is
preserved under its own exact identities. No phone work is authorized while the
owner is away carrying that device.

## Bounded core

`scene/atmosphere/IndoorDust.h` provides cosmetic, absolute-simulation-time world
positions, validated authored bounds/seed/drift/density and box/ellipsoid shapes.
The level data authors two small, inset tomb zones. Admission is at most two of
four authored zones, in stable-ID order. Low generates 16 motes per admitted zone;
Standard 32, with a global capacity64. These are experiment limits, not measured
mobile budgets. Conservative camera-projected 32x18 tile lists contain at most
four candidates each. Overflow is counted and rejects additional references;
there is no room-wide all-particles-per-pixel traversal. Movement does not reseed
or translate motes with the camera. Pause uses the same simulation timestamp.

Off is the fresh/saved migration default. The core Off branch skips authoring
validation, particle generation and tile projection, leaves upload bytes alone,
and reports zero owned work. Host tests pass for these properties, deterministic
motion, invalid data/capacity, overlap admission and camera independence.

## Integration and acceptance gates

The shared RT prototype is integrated for Pipeline and RayQueryCompute.
It uses primary depth, linear radiance composition and actual bounded existing
light visibility, without particle geometry/lights or a raster pass. Primary-only
transport is an explicit initial limitation: dust behind glass/water and reflected
dust are deferred. Keep authored zones away from the transmission/mirror proof
spaces; do not broaden those sightlines without secondary-view review.

The first unrolled shader probe exceeded the existing29 ray-query-site driver
safeguard (36). It is rejected and retained privately. A bounded candidate/source
loop probe meets that safeguard: Mobile opaque27, High opaque29, generic2 sites.
Measured changed-source static size is +31,612B opaque / +21,096B generic. Updated
ceilings use exact measured values, with no spare headroom; Shipping still has
zero atomics/no binding22. Static code growth is not dynamic cost acceptance or a
phone pipeline/presentation pass.

The core checkpoint9455c237 has all12 push/PR CI jobs passing in runs
37692764564 /37692771232. Changed-source Windows compilation and all4 Android
native ABI assembly pass. Ten affected native tests pass, including cache pause,
Off, projection, persistence, owning-completion, resources and fixture admission.
Android unit tests pass182/182 after the Debug inspection extension.
ABI/shader freshness and remaining exact integration checks are being completed.
Independent read-only source review found no consequential core/resource/shader
bug; it is not runtime visual validation. Desktop images/motion/cost and the new
phone comparison are still pending. Nothing here closes
owner appearance, Android motion, sustained thermal/performance or mobile
acceptance. Default Off remains authoritative until those decisions.

No shaft layer is implemented. Shafts stay deferred behind a clear motes visual
and cost go/no-go review, including mobile evidence. Existing ground Mist is
unchanged; any future continuous shaft would obey Mist Off. Dust motes are an
independently explained option and may be on with Mist Off.

Audio/haptic manual revalidation required: NO for this cosmetic slice; gameplay,
event timing and playback inputs are unchanged. Final 1.6.2 combat/audio/haptic,
quality-profile, owner and Eric audit gates retain their existing status.

The owner has now returned. Read-only device identification confirms the intended
SM-S948B / Android16 / API36. New device evidence must name the exact new APK;
none of the build/source results above is a phone dust presentation or cost pass.
The immutable renderer candidate will add requested/uploaded/completed Dust tier
and bounded CPU work to diagnostic stills and actual Dust tier to benchmark rows,
motion rows and consented summaries. Missing historical Dust evidence remains
absent, rather than being relabelled Off. Existing consent remains required.

Retained intermediate failures include the obsolete all-quality-bits Mist source
assertion, old frozen shader snapshot pins and stale compatibility includes.
Mist now gates only its own bit, with a negative mutation test rejecting use of
Dust bits. Compatibility artifacts and exact literal snapshot pins were refreshed
from compiled/validated current shared source. Hard driver/query/Shipping limits
remain. A manifest check correctly rejected concurrent source edits; it passes
when edits are stopped. The first Android Debug admission fixture caught mutually
inconsistent motion/checkpoint flags. Root review then corrected scale ordering:
Dust override must precede the existing explicit Debug scale request. These
corrections preserve settings; no phone test is implied by the host results.

All20 unique affected native/contract checks now pass: the10-test integrated
suite, eight unaffected-by-follow-up ABI/evidence/shader checks, the corrected
frozen manifest check, and the fully corrected artifact/compatibility check.
The consented Dust summary regression additionally passes after its metadata
change. Final Windows build and final4-ABI Android assembly pass from stable
runtime sources; Android Java182/182 remains current. A wrong per-command newline
setting produced a false CRLF whitespace report; ordinary repository settings
are checked before committing. No source content was changed to address that
invocation error. Exact runtime artifacts and visual evidence follow below when
sealed; the build checks still do not close sustained/device/owner gates.
