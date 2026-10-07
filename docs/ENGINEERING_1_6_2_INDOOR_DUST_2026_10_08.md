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

The shared RT prototype is being integrated for Pipeline and RayQueryCompute.
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

Settings migration/Use/Keep/Restore, exact owning-submission ACK, lifecycle and
resource tests, shader freshness, integrated builds, desktop images/motion/cost
and all affected regression checks are still being completed. Nothing here closes
owner appearance, Android motion, sustained thermal/performance or mobile
acceptance. Default Off remains authoritative until those decisions.

No shaft layer is implemented. Shafts stay deferred behind a clear motes visual
and cost go/no-go review, including mobile evidence. Existing ground Mist is
unchanged; any future continuous shaft would obey Mist Off. Dust motes are an
independently explained option and may be on with Mist Off.

Audio/haptic manual revalidation required: NO for this cosmetic slice; gameplay,
event timing and playback inputs are unchanged. Final 1.6.2 combat/audio/haptic,
quality-profile, owner and Eric audit gates retain their existing status.
