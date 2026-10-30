# Terrain lighting attribution and retained opaque witnesses

Chapter integration remains incomplete. **Normal-play GPU residency saving is
zero.** This checkpoint answers the bright-ground observation with actual frozen
RT frames and strengthens the conservative geometry building block for future
resource eligibility. It changes no production shader, material, light tuning,
geometry, quality, capacity or budget.

## What lights the ground

Ground, trees and Kit use the shared opaque direct-light path and the same sRGB
base-color decoding. There is no terrain-only exposure. Their albedos, normals,
occlusion and visible light sources differ. The moon direction has an upward
component of 0.93014, so upward-facing ground receives substantially more direct
moonlight than many visible trunk faces. Whole-source-texture mean linear
luminance is 0.2483 for forest ground, 0.0574 for bark, 0.0173 for pine and 0.0314
for alder. Those are texture averages, not matched visible texels or measurements
of surface radiance. Kit also has an authored emissive contribution.

The legacy finale aperture selector is X/Z-only. Suppressing only its lighting
uniform leaves the physical open lid, rope, equipment, camera and materials
intact. At the landing it changes the shaft/rim and some nearby contributions;
the selected middle-ground region is pixel-identical on both backends. At the
second bend the **entire production/directional comparison is pixel-identical**.
Removing sky lighting darkens the broad ground dramatically. This attributes
the wide contrast to shared sky/material response; the aperture rule does not
explain the bright second-bend terrain. No receiver-height fix or global dimming
is adopted from this observation. The next visual adjustment should calibrate
the imported ground/bark material balance with matched views.

## Inspected native comparisons

Debug Diagnostic, RTX 5050 Laptop GPU, 960x540, 100% scale, High optical/water/fire
and Higher shadows. Each mode uses the same staged/frozen simulation and twelve
settling RT frames. This is image attribution, not sustained performance or
owner acceptance. Raw manifests identify cameras, posed meshes, masks, lights,
shader hashes and completed submissions.

| View/backend | Production | Directional sky replacing the aperture | Sky disabled |
| --- | --- | --- | --- |
| Landing/Pipeline | [Frame](native/pipeline/north-full/193-chapter-landing-north.png) | [Frame](native/pipeline/north-directional/193-chapter-landing-north.png) | [Frame](native/pipeline/north-off/193-chapter-landing-north.png) |
| Second bend/Pipeline | [Frame](native/pipeline/bend-full/202-chapter-forest-second-bend.png) | [Frame](native/pipeline/bend-directional/202-chapter-forest-second-bend.png) | [Frame](native/pipeline/bend-off/202-chapter-forest-second-bend.png) |
| Landing/QueryCompute | [Frame](native/query/north-full/193-chapter-landing-north.png) | [Frame](native/query/north-directional/193-chapter-landing-north.png) | [Frame](native/query/north-off/193-chapter-landing-north.png) |

All nine frames were opened. The default reunion frame is also byte-identical
to ec43baa2's capture. Cross-backend pixel identity is **not** claimed: the three
landing modes differ in 4,221/4,199/456 pixels, with mean absolute display-channel
differences below 0.0033 on the 0–255 scale. Both show the same attribution.

The Debug-only capture switch rejects ordinary launches, Release, missing or
duplicate values, unknown modes and conflicting moving/benchmark validations.
It never writes saved settings. The physical roof uses authoritative progression;
only the diagnostic lighting input is suppressed.

## Conservative visibility groundwork

`OpaqueQuadWitness` checks all 64 endpoint-box vertex pairs for an actual retained
axis-aligned rectangular two-triangle patch. This uses analytic intersection
extrema, not sampled visibility. Incomplete/nonplanar patches, stale generations,
wrong masks, crossing domains and insufficient numerical guards fail closed.
Tests cover both diagonals, all axes, boundary cases and an independent dense
oracle that checks accepted bounds without authorizing omission.

The native admission adapter reads actual world-baked vertices, indices,
primitive spans, material flags and Shared triangle provenance. It rejects
nonidentity instance transforms, mutable owners, potentially filtered/transparent
or emissive materials and omitted candidate triangles. The real prepared tomb
fixture supplies 182 retained quad witnesses and zero BoneWax witnesses.

These witnesses **do not close ray-origin domains or certify an unload**. The
immutable owner/current mask/generation must be pinned by its caller. The full
terrain support, shader spawn/tMin guards, transmission/reflection/bounce reach
and light-visibility origins remain required before normal GPU adoption. Burial
props stay resident. No normal resource omission or residency saving is enabled.

## Validation and current candidate

Debug and Release applications build. Six affected CTests pass in each; the final
two changed visibility/native-intake tests pass again after the numeric-guard
review, including 57 segment/admission checks. Failing red evidence is retained.
The Vulkan-host CI roster now builds/selects these new tests and the native tomb
dressing fixture. Existing route and 180-second torch coverage remain unchanged.

Android four-ABI Debug build/lint passes; fresh Java tests pass 286/286 across
45 suites, with zero failures/errors/skips. Actual APK inventory and development
certificate checks pass. Compared with the prepared ec43 candidate, only the four
native-library entry payloads change; Java, assets and the entry roster match.
Fresh compilation verifies all eight frozen shader keys and unchanged limits.

Exact local artifacts and source input hashes are in
[the receipt](source-and-artifact-receipt.json). They were built before the
checkpoint commit; the receipt records ec43baa2 as the base plus actual input
hashes, rather than claiming a post-commit rebuild. Current prepared phone APK:
`1b508d6ae48da7fedeac466c945fbd7d3ca076b12aeca8984ec30d7a32e35b72`,
183,124,186 bytes, versionCode10, unchanged development certificate. It is
**uninstalled**. Windows Debug SHA is
`c36b43fe4b15b8be26e4a3afba1009020742620a71e4d8e0707c8e348381dd1b`;
Release SHA is `4369152eb14fdf140d892776e6e29c1f8fbb4253612664a4a38d6ce032ef9879`.
Exact-commit CI results are reported on PR27 after the push.

Normal two-way GPU eligibility/ownership/retirement, sustained Shipping costs,
fresh phone route and owner acceptance remain open. Current/Higher and persisted
Lower-to-Current phone acceptance remain pending. Audio/haptic revalidation for
this diagnostic/helper slice: **NO**; the chapter's changed cue acceptance remains
pending. Easy Tree/canopy and mist issue28 remain deferred.

Next owner route: opening/guards → Keeper → claim → rope ascent → fresh lantern
raise/Kit → both bends/lookout → return/descent → second ascent. Note ground/tree
contrast, any transition pop/hitch, retained corpses, equipment/light ownership,
and dialogue/cue intelligibility at unchanged settings.
