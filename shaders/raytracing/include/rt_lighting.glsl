#include "rt_fire.glsl"

vec2 lightTuning(int group)
{
    if (group == kLightTorch)
        return vec2(controls.torchHueDegrees, controls.torchIntensityScale);
    if (group == kLightSkylight)
        return vec2(controls.skylightHueDegrees, controls.skylightIntensityScale);
    if (group == kLightPassage)
        return vec2(controls.passageHueDegrees, controls.passageIntensityScale);
    return vec2(controls.staffHueDegrees, controls.staffIntensityScale);
}

vec3 tunedLightColor(vec3 authoredColor, int group)
{
    vec2 tuning = lightTuning(group);
    float maximum = max(authoredColor.r, max(authoredColor.g, authoredColor.b));
    float intensityScale = tuning.y;
    if (maximum <= 0.0 || intensityScale <= 0.0)
    {
        return vec3(0.0);
    }
    if (abs(tuning.x) <= 0.0001)
    {
        return authoredColor * intensityScale;
    }

    float minimum = min(authoredColor.r, min(authoredColor.g, authoredColor.b));
    float chroma = maximum - minimum;
    float hue = 0.0;
    if (chroma > 0.000001)
    {
        hue = authoredColor.r >= maximum
            ? (authoredColor.g - authoredColor.b) / chroma
            : (authoredColor.g >= maximum
                ? 2.0 + (authoredColor.b - authoredColor.r) / chroma
                : 4.0 + (authoredColor.r - authoredColor.g) / chroma);
        hue = fract(hue / 6.0 + tuning.x / 360.0);
    }
    float saturation = chroma / maximum;
    vec3 phase = abs(fract(hue + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    vec3 rotated = maximum * mix(vec3(1.0), clamp(phase - 1.0, 0.0, 1.0), saturation);
    return rotated * intensityScale;
}

vec3 skyColor(vec3 d)
{
    float moonAlignment = max(dot(d, kMoonDirection), 0.0);
    float moonDisc = smoothstep(0.99945, 0.99988, moonAlignment);
    float moonHalo = pow(moonAlignment, 96.0);
    float horizon = smoothstep(-0.22, 0.45, d.y);
    vec3 sky = mix(vec3(0.015, 0.018, 0.024), vec3(0.04, 0.065, 0.105), horizon);
    sky += vec3(0.52, 0.62, 0.82) * moonDisc;
    sky += vec3(0.10, 0.14, 0.23) * moonHalo;
    sky += vec3(0.045, 0.018, 0.008) * smoothstep(-0.35, 0.15, -d.y);
    return tunedLightColor(sky, kLightSkylight);
}

float puddleMask(vec3 p)
{
    return (1.0 - smoothstep(0.0, 0.20, abs(p.x + 0.28)))
         * (1.0 - smoothstep(0.0, 2.1, abs(p.z - 0.4)));
}

#include "rt_hit_decode.glsl"
vec3 dielectricBeerLambert(vec3 attenuationColor, float pathLength,
                           float attenuationDistance);
float dielectricRayEpsilon(vec3 position, float interfaceDistance);
vec3 advanceDielectricRayOrigin(vec3 position, vec3 geometricNormal,
                                vec3 direction, float epsilon);

const int kMobileShadowInterfaces = 4;
const int kHighShadowInterfaces = 8;
const int kMobileShadowVolumes = 2;
const int kHighShadowVolumes = 4;

#if defined(HORDE_RT_VARIANT_QUALITY)
#define HORDE_RT_SHADOW_INTERFACE_CEILING kRtVariantShadowInterfaceBudget
#define HORDE_RT_SHADOW_VOLUME_CAPACITY kRtVariantShadowVolumeBudget
#else
#define HORDE_RT_SHADOW_INTERFACE_CEILING kHighShadowInterfaces
#define HORDE_RT_SHADOW_VOLUME_CAPACITY kHighShadowVolumes
#endif

struct ShadowHit
{
    bool hit;
    bool transparent;
    bool thinWall;
    bool certifiedClosedVolume;
    bool entering;
    float t;
    uint instance;
    uint material;
    float transmission;
    vec3 tint;
    vec3 attenuationColor;
    float attenuationDistance;
};

ShadowHit traceNearestShadowHit(vec3 origin, vec3 direction,
                                float maxDistance, uint mask,
                                float minimumDistance)
{
    ShadowHit result;
    result.hit = false;
    result.transparent = false;
    result.thinWall = false;
    result.certifiedClosedVolume = false;
    result.entering = true;
    result.t = maxDistance;
    result.instance = 0u;
    result.material = 0u;
    result.transmission = 0.0;
    result.tint = vec3(1.0);
    result.attenuationColor = vec3(1.0);
    result.attenuationDistance = 0.0;

    rayQueryEXT query;
    rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsNoOpaqueEXT, mask,
                          origin, max(minimumDistance, 0.000001), direction,
                          max(maxDistance, 0.000004));
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query, false) !=
            gl_RayQueryCandidateIntersectionTriangleEXT)
            continue;
        int candidateInstance = int(
            rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));
        int candidatePrimitive = rayQueryGetIntersectionPrimitiveIndexEXT(query, false);
        bool transparentWorldPane = candidateInstance == kWaterfallInstance;
        if (candidateInstance == 0)
        {
            int material = int(worldSurfaces.codes[candidatePrimitive] & 0xffu);
            transparentWorldPane = material == kMaterialClearGlass ||
                                   material == kMaterialWater;
        }
        if (!transparentWorldPane)
            rayQueryConfirmIntersectionEXT(query);
    }

    if (rayQueryGetIntersectionTypeEXT(query, true) ==
        gl_RayQueryCommittedIntersectionNoneEXT)
        return result;

    result.hit = true;
    result.t = rayQueryGetIntersectionTEXT(query, true);
    result.instance = uint(
        rayQueryGetIntersectionInstanceCustomIndexEXT(query, true));
    result.entering = rayQueryGetIntersectionFrontFaceEXT(query, true);
    RtInstanceMetadata instanceMetadata = rtInstances.values[result.instance];
    if ((instanceMetadata.flags & kRtInstanceFlagStaticPbr) == 0u)
        return result;
    uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(query, true);
    if (geometryIndex >= instanceMetadata.primitiveCount)
        return result;
    RtPrimitiveMetadata primitiveMetadata = rtPrimitives.values[
        instanceMetadata.primitiveBase + geometryIndex];
    RtMaterialGpu material = rtMaterials.values[primitiveMetadata.materialIndex];
    float transmission = clamp(
        material.metallicRoughnessOcclusionTransmission.w, 0.0, 1.0);
    float metallic = clamp(
        material.metallicRoughnessOcclusionTransmission.x, 0.0, 1.0);
    bool materialTransmits =
        (material.materialFlags.x & kRtMaterialFlagTransmission) != 0u &&
        transmission > 0.001 && metallic <= 0.5;
    if (!materialTransmits)
        return result;
    result.transparent = true;
    result.thinWall =
        (material.materialFlags.x & kRtMaterialFlagThinWall) != 0u;
    result.certifiedClosedVolume =
        (material.materialFlags.x & kRtMaterialFlagCertifiedClosedVolume) != 0u;
    result.material = primitiveMetadata.materialIndex;
    result.transmission = transmission;
    result.tint = clamp(material.baseColorFactor.rgb, vec3(0.0), vec3(1.0));
    result.attenuationColor = clamp(
        material.attenuationColor.rgb, vec3(0.0), vec3(1.0));
    result.attenuationDistance =
        material.iorThicknessAttenuationDistance.z;
    return result;
}

vec3 shadowTransmittanceMask(vec3 origin, vec3 direction,
                             float maxDistance, uint mask)
{
#if defined(HORDE_RT_VARIANT_QUALITY)
    const int interfaceBudget = kRtVariantShadowInterfaceBudget;
    const int volumeBudget = kRtVariantShadowVolumeBudget;
#else
    int interfaceBudget = controls.waterQuality >= 1.5
        ? kHighShadowInterfaces : kMobileShadowInterfaces;
    int volumeBudget = controls.waterQuality >= 1.5
        ? kHighShadowVolumes : kMobileShadowVolumes;
#endif
    int interfaceCount = 0;
    uint volumeInstances[HORDE_RT_SHADOW_VOLUME_CAPACITY];
    uint volumeMaterials[HORDE_RT_SHADOW_VOLUME_CAPACITY];
    uint volumeMaterialFlags[HORDE_RT_SHADOW_VOLUME_CAPACITY];
    float volumeEntryDistances[HORDE_RT_SHADOW_VOLUME_CAPACITY];
    vec4 volumeAttenuation[HORDE_RT_SHADOW_VOLUME_CAPACITY];
    int volumeDepth = 0;
    bool observedClosedVolumeEntry = false;
    vec3 transmittance = vec3(1.0);
    vec3 currentOrigin = origin;
    float travelledDistance = 0.0;
    float remainingDistance = max(maxDistance, 0.0);
    for (int traversal = 0; traversal <= HORDE_RT_SHADOW_INTERFACE_CEILING; ++traversal)
    {
        if (remainingDistance <= 0.0)
        {
            if (volumeDepth > 0)
                RT_DIAG_ADD(shadowFiniteEndpointVolumeCount,
                          uint(volumeDepth));
            for (int volumeIndex = 0; volumeIndex < volumeDepth; ++volumeIndex)
            {
                vec4 attenuation = volumeAttenuation[volumeIndex];
                transmittance *= dielectricBeerLambert(
                    attenuation.rgb,
                    max(maxDistance - volumeEntryDistances[volumeIndex], 0.0),
                    attenuation.a);
            }
            return clamp(transmittance, vec3(0.0), vec3(1.0));
        }
        float epsilon = dielectricRayEpsilon(currentOrigin, travelledDistance);
        float minimumDistance = interfaceCount == 0 ? 0.0015 : epsilon * 0.5;
        ShadowHit nearest = traceNearestShadowHit(
            currentOrigin, direction, remainingDistance, mask, minimumDistance);
        if (!nearest.hit)
        {
            // A shadow query is a finite segment to a sampled light. Open
            // media here mean that the light endpoint is inside each medium,
            // so retain their partial path attenuation without inventing an
            // exit surface beyond the endpoint.
            if (volumeDepth > 0)
                RT_DIAG_ADD(shadowFiniteEndpointVolumeCount,
                          uint(volumeDepth));
            for (int volumeIndex = 0; volumeIndex < volumeDepth; ++volumeIndex)
            {
                vec4 attenuation = volumeAttenuation[volumeIndex];
                transmittance *= dielectricBeerLambert(
                    attenuation.rgb,
                    max(maxDistance - volumeEntryDistances[volumeIndex], 0.0),
                    attenuation.a);
            }
            return clamp(transmittance, vec3(0.0), vec3(1.0));
        }
        if (!nearest.transparent)
        {
            bool everyOpenVolumeCertified = volumeDepth > 0;
            for (int volumeIndex = 0; volumeIndex < HORDE_RT_SHADOW_VOLUME_CAPACITY;
                 ++volumeIndex)
            {
                if (volumeIndex < volumeDepth)
                    everyOpenVolumeCertified = everyOpenVolumeCertified &&
                        (volumeMaterialFlags[volumeIndex] &
                         kRtMaterialFlagCertifiedClosedVolume) != 0u;
            }
            if (everyOpenVolumeCertified)
            {
                RT_DIAG_ADD(shadowCertifiedClosedVolumeRecoveryCount,
                          1u);
                RT_DIAG_OR(certifiedClosedVolumeRecoveryReasonMask,
                         32u);
            }
            return vec3(0.0);
        }
        if (interfaceCount >= interfaceBudget)
        {
            bool everyOpenVolumeCertified = nearest.certifiedClosedVolume;
            for (int volumeIndex = 0; volumeIndex < HORDE_RT_SHADOW_VOLUME_CAPACITY;
                 ++volumeIndex)
            {
                if (volumeIndex < volumeDepth)
                    everyOpenVolumeCertified = everyOpenVolumeCertified &&
                        (volumeMaterialFlags[volumeIndex] &
                         kRtMaterialFlagCertifiedClosedVolume) != 0u;
            }
            if (everyOpenVolumeCertified)
            {
                RT_DIAG_ADD(shadowCertifiedClosedVolumeRecoveryCount,
                          1u);
                RT_DIAG_OR(certifiedClosedVolumeRecoveryReasonMask,
                         64u);
                return vec3(0.0);
            }
            RT_DIAG_ADD(shadowOverflowCount, 1u);
            if (nearest.instance == 8u ||
                (volumeDepth > 0 && volumeInstances[volumeDepth - 1] == 8u))
                RT_DIAG_ADD(productionPaneStackFailureCount, 1u);
            return clamp(transmittance * vec3(0.08), vec3(0.0), vec3(1.0));
        }
        ++interfaceCount;
        float absoluteDistance = travelledDistance + nearest.t;
        if (nearest.thinWall)
        {
            transmittance *= nearest.transmission *
                mix(vec3(1.0), nearest.tint, 0.12);
        }
        else if (nearest.entering)
        {
            if (volumeDepth >= volumeBudget)
            {
                bool everyOpenVolumeCertified = nearest.certifiedClosedVolume;
                for (int volumeIndex = 0; volumeIndex < HORDE_RT_SHADOW_VOLUME_CAPACITY;
                     ++volumeIndex)
                {
                    if (volumeIndex < volumeDepth)
                        everyOpenVolumeCertified = everyOpenVolumeCertified &&
                            (volumeMaterialFlags[volumeIndex] &
                             kRtMaterialFlagCertifiedClosedVolume) != 0u;
                }
                if (everyOpenVolumeCertified)
                {
                    RT_DIAG_ADD(shadowCertifiedClosedVolumeRecoveryCount,
                              1u);
                    RT_DIAG_OR(certifiedClosedVolumeRecoveryReasonMask,
                             128u);
                    return vec3(0.0);
                }
                RT_DIAG_ADD(shadowOverflowCount, 1u);
                if (nearest.instance == 8u)
                    RT_DIAG_ADD(productionPaneStackFailureCount, 1u);
                return clamp(transmittance * vec3(0.08), vec3(0.0), vec3(1.0));
            }
            observedClosedVolumeEntry = true;
            volumeInstances[volumeDepth] = nearest.instance;
            volumeMaterials[volumeDepth] = nearest.material;
            volumeMaterialFlags[volumeDepth] = nearest.certifiedClosedVolume
                ? kRtMaterialFlagCertifiedClosedVolume : 0u;
            volumeEntryDistances[volumeDepth] = absoluteDistance;
            volumeAttenuation[volumeDepth] = vec4(
                nearest.attenuationColor, nearest.attenuationDistance);
            ++volumeDepth;
            transmittance *= nearest.transmission;
        }
        else
        {
            if (volumeDepth <= 0 && !observedClosedVolumeEntry)
            {
                // Surface and emissive samples can physically begin inside a
                // closed dielectric. Consecutive leading exits represent
                // nested origin-containing media. Account for each complete
                // origin-to-boundary path without fabricating a stack entry.
                transmittance *= nearest.transmission * dielectricBeerLambert(
                    nearest.attenuationColor, absoluteDistance,
                    nearest.attenuationDistance);
                RT_DIAG_ADD(shadowImplicitOriginExitCount, 1u);
            }
            else if (volumeDepth <= 0 ||
                volumeInstances[volumeDepth - 1] != nearest.instance ||
                volumeMaterials[volumeDepth - 1] != nearest.material)
            {
                bool everyOpenVolumeCertified = nearest.certifiedClosedVolume;
                for (int volumeIndex = 0; volumeIndex < HORDE_RT_SHADOW_VOLUME_CAPACITY;
                     ++volumeIndex)
                {
                    if (volumeIndex < volumeDepth)
                        everyOpenVolumeCertified = everyOpenVolumeCertified &&
                            (volumeMaterialFlags[volumeIndex] &
                             kRtMaterialFlagCertifiedClosedVolume) != 0u;
                }
                if (everyOpenVolumeCertified)
                {
                    // The certified manifold cannot leak energy through a
                    // mismatched grazing edge. Block the sample and count the
                    // finite-precision recovery without hiding invalid assets.
                    RT_DIAG_ADD(shadowCertifiedClosedVolumeRecoveryCount,
                              1u);
                    RT_DIAG_OR(certifiedClosedVolumeRecoveryReasonMask,
                             256u);
                    return vec3(0.0);
                }
                RT_DIAG_ADD(unclosedVolumeCount, 1u);
                RT_DIAG_ADD(shadowUnclosedVolumeCount, 1u);
                if (nearest.instance == 8u ||
                    (volumeDepth > 0 && volumeInstances[volumeDepth - 1] == 8u))
                    RT_DIAG_ADD(productionPaneStackFailureCount, 1u);
                RT_DIAG_ADD(shadowMismatchedExitCount, 1u);
                if (volumeDepth <= 0)
                    RT_DIAG_ADD(shadowMismatchEmptyCount, 1u);
                return clamp(transmittance * vec3(0.08), vec3(0.0), vec3(1.0));
            }
            else
            {
                --volumeDepth;
                vec4 attenuation = volumeAttenuation[volumeDepth];
                transmittance *= dielectricBeerLambert(
                    attenuation.rgb,
                    max(absoluteDistance - volumeEntryDistances[volumeDepth], 0.0),
                    attenuation.a);
            }
        }

        vec3 hitPosition = currentOrigin + direction * nearest.t;
        epsilon = dielectricRayEpsilon(hitPosition, absoluteDistance);
        currentOrigin = hitPosition + direction * epsilon;
        travelledDistance = absoluteDistance + epsilon;
        remainingDistance = max(maxDistance - travelledDistance, 0.0);
    }
    bool everyOpenVolumeCertified = volumeDepth > 0;
    for (int volumeIndex = 0; volumeIndex < HORDE_RT_SHADOW_VOLUME_CAPACITY; ++volumeIndex)
    {
        if (volumeIndex < volumeDepth)
            everyOpenVolumeCertified = everyOpenVolumeCertified &&
                (volumeMaterialFlags[volumeIndex] &
                 kRtMaterialFlagCertifiedClosedVolume) != 0u;
    }
    if (everyOpenVolumeCertified)
    {
        RT_DIAG_ADD(shadowCertifiedClosedVolumeRecoveryCount,
                  1u);
        RT_DIAG_OR(certifiedClosedVolumeRecoveryReasonMask,
                 512u);
        return vec3(0.0);
    }
    RT_DIAG_ADD(shadowOverflowCount, 1u);
    if (volumeDepth > 0 && volumeInstances[volumeDepth - 1] == 8u)
        RT_DIAG_ADD(productionPaneStackFailureCount, 1u);
    return clamp(transmittance * vec3(0.08), vec3(0.0), vec3(1.0));
}

// Phone-safe ordered shadow transport for the production path. A shadow ray
// may cross several closed panes sequentially, but only one volume may be open
// at a time. That matches the asset contract for the lantern and fixture while
// making nested glass-on-glass terminate conservatively instead of carrying a
// dynamically indexed volume stack in every raygen invocation. Mobile and
// High retain the same entry/exit, thickness and Beer-Lambert model; High only
// raises the ordered interface ceiling.
vec3 compactShadowTransmittanceMask(vec3 origin, vec3 direction,
                                    float maxDistance, uint mask)
{
#if defined(HORDE_RT_VARIANT_QUALITY)
    const int interfaceBudget = kRtVariantShadowInterfaceBudget;
#else
    int interfaceBudget = controls.waterQuality >= 1.5
        ? kHighShadowInterfaces : kMobileShadowInterfaces;
#endif
    int interfaceCount = 0;
    bool volumeOpen = false;
    bool observedClosedVolumeEntry = false;
    bool volumeCertified = false;
    uint volumeInstance = 0u;
    uint volumeMaterial = 0u;
    float volumeEntryDistance = 0.0;
    vec3 volumeAttenuationColor = vec3(1.0);
    float volumeAttenuationDistance = 0.0;
    vec3 transmittance = vec3(1.0);
    vec3 currentOrigin = origin;
    float travelledDistance = 0.0;
    float remainingDistance = max(maxDistance, 0.0);

    for (int traversal = 0; traversal <= HORDE_RT_SHADOW_INTERFACE_CEILING; ++traversal)
    {
        if (remainingDistance <= 0.0)
        {
            if (volumeOpen)
            {
                transmittance *= dielectricBeerLambert(
                    volumeAttenuationColor,
                    max(maxDistance - volumeEntryDistance, 0.0),
                    volumeAttenuationDistance);
                RT_DIAG_ADD(shadowFiniteEndpointVolumeCount,
                          1u);
            }
            return clamp(transmittance, vec3(0.0), vec3(1.0));
        }

        float epsilon = dielectricRayEpsilon(currentOrigin, travelledDistance);
        float minimumDistance = interfaceCount == 0 ? 0.0015 : epsilon * 0.5;
        ShadowHit nearest = traceNearestShadowHit(
            currentOrigin, direction, remainingDistance, mask, minimumDistance);
        if (!nearest.hit)
        {
            if (volumeOpen)
            {
                transmittance *= dielectricBeerLambert(
                    volumeAttenuationColor,
                    max(maxDistance - volumeEntryDistance, 0.0),
                    volumeAttenuationDistance);
                RT_DIAG_ADD(shadowFiniteEndpointVolumeCount,
                          1u);
            }
            return clamp(transmittance, vec3(0.0), vec3(1.0));
        }
        if (!nearest.transparent)
        {
            if (volumeOpen && volumeCertified)
            {
                RT_DIAG_ADD(shadowCertifiedClosedVolumeRecoveryCount,
                    1u);
                RT_DIAG_OR(certifiedClosedVolumeRecoveryReasonMask,
                    32u);
            }
            return vec3(0.0);
        }
        if (interfaceCount >= interfaceBudget)
        {
            RT_DIAG_ADD(shadowOverflowCount, 1u);
            if (nearest.instance == 8u || (volumeOpen && volumeInstance == 8u))
                RT_DIAG_ADD(productionPaneStackFailureCount, 1u);
            return clamp(transmittance * vec3(0.08), vec3(0.0), vec3(1.0));
        }
        ++interfaceCount;

        float absoluteDistance = travelledDistance + nearest.t;
        if (nearest.thinWall)
        {
            transmittance *= nearest.transmission *
                mix(vec3(1.0), nearest.tint, 0.12);
        }
        else if (nearest.entering)
        {
            // The runtime asset contract forbids overlapping closed panes.
            // Treat a nested entry as a bounded invalid-stack recovery.
            if (volumeOpen)
            {
                RT_DIAG_ADD(shadowOverflowCount, 1u);
                if (nearest.instance == 8u || volumeInstance == 8u)
                    RT_DIAG_ADD(productionPaneStackFailureCount,
                        1u);
                return vec3(0.0);
            }
            observedClosedVolumeEntry = true;
            volumeOpen = true;
            volumeCertified = nearest.certifiedClosedVolume;
            volumeInstance = nearest.instance;
            volumeMaterial = nearest.material;
            volumeEntryDistance = absoluteDistance;
            volumeAttenuationColor = nearest.attenuationColor;
            volumeAttenuationDistance = nearest.attenuationDistance;
            transmittance *= nearest.transmission;
        }
        else if (!volumeOpen && !observedClosedVolumeEntry)
        {
            // The sampled light segment begins inside this closed medium.
            transmittance *= nearest.transmission * dielectricBeerLambert(
                nearest.attenuationColor, absoluteDistance,
                nearest.attenuationDistance);
            RT_DIAG_ADD(shadowImplicitOriginExitCount,
                      1u);
        }
        else if (!volumeOpen || volumeInstance != nearest.instance ||
                 volumeMaterial != nearest.material)
        {
            if (volumeCertified && nearest.certifiedClosedVolume)
            {
                RT_DIAG_ADD(shadowCertifiedClosedVolumeRecoveryCount,
                    1u);
                RT_DIAG_OR(certifiedClosedVolumeRecoveryReasonMask,
                    256u);
                return vec3(0.0);
            }
            RT_DIAG_ADD(unclosedVolumeCount, 1u);
            RT_DIAG_ADD(shadowUnclosedVolumeCount, 1u);
            RT_DIAG_ADD(shadowMismatchedExitCount, 1u);
            if (!volumeOpen)
                RT_DIAG_ADD(shadowMismatchEmptyCount, 1u);
            if (nearest.instance == 8u || (volumeOpen && volumeInstance == 8u))
                RT_DIAG_ADD(productionPaneStackFailureCount, 1u);
            return clamp(transmittance * vec3(0.08), vec3(0.0), vec3(1.0));
        }
        else
        {
            transmittance *= dielectricBeerLambert(
                volumeAttenuationColor,
                max(absoluteDistance - volumeEntryDistance, 0.0),
                volumeAttenuationDistance);
            volumeOpen = false;
            volumeCertified = false;
        }

        if (max(transmittance.r, max(transmittance.g, transmittance.b)) <= 0.001)
            return vec3(0.0);

        vec3 hitPosition = currentOrigin + direction * nearest.t;
        epsilon = dielectricRayEpsilon(hitPosition, absoluteDistance);
        currentOrigin = hitPosition + direction * epsilon;
        travelledDistance = absoluteDistance + epsilon;
        remainingDistance = max(maxDistance - travelledDistance, 0.0);
    }

    RT_DIAG_ADD(shadowOverflowCount, 1u);
    if (volumeOpen && volumeInstance == 8u)
        RT_DIAG_ADD(productionPaneStackFailureCount, 1u);
    return clamp(transmittance * vec3(0.08), vec3(0.0), vec3(1.0));
}

bool shadowSegmentIntersectsBounds(vec3 origin, vec3 direction,
                                   float maxDistance,
                                   vec3 boundsMin, vec3 boundsMax)
{
    vec3 safeDirection = sign(direction + vec3(0.0000001)) *
        max(abs(direction), vec3(0.0001));
    vec3 first = (boundsMin - origin) / safeDirection;
    vec3 second = (boundsMax - origin) / safeDirection;
    vec3 nearPlane = min(first, second);
    vec3 farPlane = max(first, second);
    float intervalStart = max(max(nearPlane.x, nearPlane.y), nearPlane.z);
    float intervalEnd = min(min(farPlane.x, farPlane.y), farPlane.z);
    return intervalEnd > max(intervalStart, 0.0) &&
           intervalStart < maxDistance;
}

bool shadowSegmentCrossesTransparentWorld(vec3 origin, vec3 direction,
                                          float maxDistance)
{
    // Procedural world geometry currently shares one BLAS geometry and is
    // therefore marked opaque at build time. These conservative world-space
    // bounds cover every clear roof pane, catchment/runoff sheet, and the full
    // tunable waterfall envelope. Only segments that can touch those surfaces
    // need NoOpaque candidate filtering; elsewhere Vulkan can commit opaque
    // stone/metal in hardware and expose only imported dielectric geometry.
    return shadowSegmentIntersectsBounds(
               origin, direction, maxDistance,
               vec3(-0.80, 1.27, -5.30), vec3(0.70, 1.40, -3.30)) ||
           shadowSegmentIntersectsBounds(
               origin, direction, maxDistance,
               vec3(-5.45, -1.02, -16.90), vec3(-1.75, 2.60, -13.50));
}

// One ray query can visit every candidate interface on a finite light segment.
// Opaque candidates are committed normally; transmissive candidates instead
// contribute bounded per-interface Fresnel-independent transmission, tint and
// half of the material's validated closed-volume thickness. A closed pane's
// entry and exit therefore accumulate its complete Beer-Lambert path without
// relaunching a query or keeping a per-ray dynamic stack. Primary camera
// transport still measures exact geometric entry/exit distance and refraction.
vec3 boundedShadowTransmittanceMask(vec3 origin, vec3 direction,
                                    float maxDistance, uint mask)
{
#if defined(HORDE_RT_VARIANT_QUALITY)
    const int interfaceBudget = kRtVariantShadowInterfaceBudget;
#else
    int interfaceBudget = controls.waterQuality >= 1.5
        ? kHighShadowInterfaces : kMobileShadowInterfaces;
#endif
    int interfaceCount = 0;
    bool overflow = false;
    bool productionPaneOverflow = false;
    vec3 transmittance = vec3(1.0);

    rayQueryEXT query;
    uint shadowFlags = shadowSegmentCrossesTransparentWorld(
        origin, direction, maxDistance) ? gl_RayFlagsNoOpaqueEXT : 0u;
    rayQueryInitializeEXT(query, topLevelAS, shadowFlags, mask,
                          origin, 0.0015, direction,
                          max(maxDistance, 0.004));
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query, false) !=
            gl_RayQueryCandidateIntersectionTriangleEXT)
            continue;

        uint instance = uint(
            rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));
        int primitive = rayQueryGetIntersectionPrimitiveIndexEXT(query, false);
        bool transparentWorldPane = int(instance) == kWaterfallInstance;
        if (instance == 0u)
        {
            int material = int(worldSurfaces.codes[primitive] & 0xffu);
            transparentWorldPane = material == kMaterialClearGlass ||
                                   material == kMaterialWater;
        }
        if (transparentWorldPane)
            continue;

        bool materialTransmits = false;
        RtMaterialGpu material;
        RtInstanceMetadata instanceMetadata = rtInstances.values[instance];
        if ((instanceMetadata.flags & kRtInstanceFlagStaticPbr) != 0u)
        {
            uint geometryIndex =
                rayQueryGetIntersectionGeometryIndexEXT(query, false);
            if (geometryIndex < instanceMetadata.primitiveCount)
            {
                RtPrimitiveMetadata primitiveMetadata = rtPrimitives.values[
                    instanceMetadata.primitiveBase + geometryIndex];
                material = rtMaterials.values[primitiveMetadata.materialIndex];
                float transmission = clamp(
                    material.metallicRoughnessOcclusionTransmission.w,
                    0.0, 1.0);
                float metallic = clamp(
                    material.metallicRoughnessOcclusionTransmission.x,
                    0.0, 1.0);
                materialTransmits =
                    (material.materialFlags.x &
                     kRtMaterialFlagTransmission) != 0u &&
                    transmission > 0.001 && metallic <= 0.5;
            }
        }

        if (!materialTransmits)
        {
            rayQueryConfirmIntersectionEXT(query);
            continue;
        }

        if (interfaceCount >= interfaceBudget)
        {
            overflow = true;
            productionPaneOverflow = productionPaneOverflow || instance == 8u;
            continue;
        }
        ++interfaceCount;

        float transmission = clamp(
            material.metallicRoughnessOcclusionTransmission.w, 0.0, 1.0);
        vec3 tint = clamp(material.baseColorFactor.rgb,
                          vec3(0.0), vec3(1.0));
        transmittance *= transmission * mix(vec3(1.0), tint, 0.12);
        if ((material.materialFlags.x & kRtMaterialFlagThinWall) == 0u)
        {
            float closedThickness = max(
                material.iorThicknessAttenuationDistance.y, 0.0);
            transmittance *= dielectricBeerLambert(
                clamp(material.attenuationColor.rgb,
                      vec3(0.0), vec3(1.0)),
                closedThickness * 0.5,
                material.iorThicknessAttenuationDistance.z);
        }
    }

    if (rayQueryGetIntersectionTypeEXT(query, true) !=
        gl_RayQueryCommittedIntersectionNoneEXT)
        return vec3(0.0);
    if (overflow)
    {
        RT_DIAG_ADD(shadowOverflowCount, 1u);
        if (productionPaneOverflow)
            RT_DIAG_ADD(productionPaneStackFailureCount,
                1u);
        transmittance *= vec3(0.08);
    }
    return clamp(transmittance, vec3(0.0), vec3(1.0));
}

float visibilityMask(vec3 origin, vec3 direction, float maxDistance, uint mask)
{
    rayQueryEXT query;
    rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsNoOpaqueEXT, mask,
                          origin, 0.0015, direction, max(maxDistance, 0.004));
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query, false) !=
            gl_RayQueryCandidateIntersectionTriangleEXT)
            continue;
        int instance = int(rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));
        int primitive = rayQueryGetIntersectionPrimitiveIndexEXT(query, false);
        bool transparentWorldPane = instance == kWaterfallInstance;
        if (instance == 0)
        {
            int material = int(worldSurfaces.codes[primitive] & 0xffu);
            transparentWorldPane = material == kMaterialClearGlass ||
                                   material == kMaterialWater;
        }
        if (!transparentWorldPane) rayQueryConfirmIntersectionEXT(query);
    }
    return rayQueryGetIntersectionTypeEXT(query, true) ==
        gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

vec3 sceneShadowTransmittanceMask(vec3 origin, vec3 direction,
                                  float maxDistance, uint mask)
{
    if (!genericTransmissionEnabled())
    {
        return vec3(visibilityMask(origin, direction, maxDistance, mask));
    }
    return boundedShadowTransmittanceMask(
        origin, direction, maxDistance, mask);
}

vec3 offsetRayOrigin(HitInfo h, vec3 direction)
{
    bool genericTransmissionActive = genericTransmissionEnabled();
    if (genericTransmissionActive)
    {
        return advanceDielectricRayOrigin(
            h.position, h.geometricNormal, direction,
            dielectricRayEpsilon(h.position, h.t));
    }
    float side = dot(h.geometricNormal, direction) >= 0.0 ? 1.0 : -1.0;
    return h.position + h.geometricNormal * side * 0.004;
}

float visibility(vec3 origin, vec3 direction, float maxDistance)
{
    // Held prop meshes stay excluded from their own direct-light estimate. The
    // world uses bit 0x01 and the separate player body uses bit 0x04. Visibility
    // is a world-space property: never change the caster set based on screen Y,
    // because the same water or stone receiver must keep its shadow when the
    // camera, placement, or reflected/refracted view changes.
    // Instance 16 is reflection-only for primary rays, but participates here so
    // the player's head casts real shadows. Instance 17 is the moving roof slab.
    return visibilityMask(origin, direction, maxDistance, 0x35u);
}

float transparentVisibility(vec3 origin, vec3 direction, float maxDistance)
{
    vec3 transmittance = sceneShadowTransmittanceMask(
        origin, direction, maxDistance, 0x35u);
    return dot(transmittance, vec3(0.2126, 0.7152, 0.0722));
}

const int kShadowSampleCapacity = 4;

mat4 transparentTransmittanceBatch(vec3 origins[kShadowSampleCapacity],
                                   vec3 directions[kShadowSampleCapacity],
                                   float distances[kShadowSampleCapacity],
                                   bvec4 enabled)
{
    // glslang inlines every rayQuery helper into raygen. Keeping the four
    // direct-light samples in one bounded loop preserves the exact queries
    // while avoiding separately expanded copies of the material traversal.
    mat4 result = mat4(0.0);
    for (int shadowSample = 0; shadowSample < kShadowSampleCapacity; ++shadowSample)
    {
        if (!enabled[shadowSample]) continue;
        result[shadowSample] = vec4(sceneShadowTransmittanceMask(
            origins[shadowSample], directions[shadowSample],
            distances[shadowSample], 0x35u), 1.0);
    }
    return result;
}

void activeLocalLight(out vec3 position, out vec3 color, out float strength);
vec3 fireEmitterDirectLighting(HitInfo h, vec3 rayDirection, bool dualVisibility,
                               bool allowAnalyticSpecular);

vec3 bounceSample(HitInfo h, vec3 incoming, bool lightAwareMirror)
{
    if (!h.hit)
    {
        // A single bounce may see the sky, but it must not turn the torch's indirect light blue.
        return skyColor(incoming) * 0.22;
    }
    if (h.emissive > 0.0)
    {
        return h.base * h.emissive;
    }
    if (h.material == kMaterialWater)
    {
        // Water secondary hits deliberately stop here. Primary water shading
        // owns its one reflection and one transmission query, so this keeps
        // water-on-water paths bounded and deterministic on the phone.
        return skyColor(incoming) * 0.18 + h.base * 0.35;
    }
    if (!lightAwareMirror)
    {
        return h.base * (0.11 + 0.16 * max(dot(h.normal, kMoonDirection), 0.0));
    }

    // A mirror bounce must share the scene's current darkness. The former
    // constant moon term made reflected characters look studio-lit even after
    // the lantern had gone out. Use only a very low ambient floor plus the same
    // currently active local emitter, with one bounded visibility query.
    vec3 localLightPosition;
    vec3 localLightColor;
    float localLightStrength;
    activeLocalLight(localLightPosition, localLightColor, localLightStrength);

    vec3 toLight = localLightPosition - h.position;
    float lightDistance = length(toLight);
    vec3 lightDirection = toLight / max(lightDistance, 0.001);
    bool genericTransmissionActive = genericTransmissionEnabled();
    // Passage/staff lights may be absent while the held fire emitter remains
    // active. Do not launch a second shadow ray toward the zero placeholder;
    // the emitter query below still supplies the physically coherent coloured
    // light for the reflected hit.
    vec3 lightTransmittance = localLightStrength > 0.001
        ? sceneShadowTransmittanceMask(
            offsetRayOrigin(h, lightDirection), lightDirection,
            lightDistance - 0.02, 0x35u)
        : vec3(0.0);
    float lightVisibility = lightTransmittance.x;
    float diffuse = max(dot(h.normal, lightDirection), 0.0);
    float attenuation = 1.0 / (1.0 + lightDistance * lightDistance * 0.58);
    vec3 authoredDirect;
    if (!genericTransmissionActive)
    {
        authoredDirect = h.base *
            (0.008 + localLightColor * diffuse * attenuation * lightVisibility *
             localLightStrength * 2.25);
    }
    else
    {
        authoredDirect = h.base *
            (0.008 + localLightColor * lightTransmittance * diffuse * attenuation *
             localLightStrength * 2.25);
    }
    return authoredDirect + fireEmitterDirectLighting(h, incoming, false, false);
}

void activeLocalLight(out vec3 position, out vec3 color, out float strength)
{
    position = vec3(0.0);
    color = vec3(0.0);
    strength = 0.0;
    if (controls.guidanceLightStrength > 0.001)
    {
        position = vec3(controls.staffX, controls.staffY, controls.staffZ);
        color = tunedLightColor(vec3(1.0, 0.48, 0.12), kLightPassage);
        strength = controls.guidanceLightStrength;
        return;
    }
    if (controls.cameraX <= -8.5 && controls.cameraX >= -28.5 &&
        controls.cameraZ >= -16.8 && controls.cameraZ <= -13.6)
    {
        float bayX = controls.cameraX > -13.5 ? -11.0 :
                     (controls.cameraX > -18.5 ? -16.0 :
                     (controls.cameraX > -23.5 ? -21.0 : -26.0));
        position = vec3(bayX, 0.67, -13.98);
        vec3 authoredColor = bayX > -13.5 ? vec3(1.0, 0.42, 0.06) :
                             (bayX > -18.5 ? vec3(0.055, 0.22, 1.0) :
                             (bayX > -23.5 ? vec3(1.0, 0.045, 0.018) :
                                             vec3(0.16, 0.78, 0.22)));
        color = tunedLightColor(authoredColor, kLightPassage);
        strength = 1.0;
    }
    if (controls.enemyKind > 0.5 && controls.staffLightStrength > 0.001)
    {
        position = vec3(controls.staffX, controls.staffY, controls.staffZ);
        color = tunedLightColor(vec3(0.58, 0.10, 1.0), kLightStaff);
        strength = controls.staffLightStrength;
    }
}

vec3 fireEmitterDirectLighting(HitInfo h, vec3 rayDirection, bool dualVisibility,
                               bool allowAnalyticSpecular)
{
    const vec3 areaOffsets[2] = vec3[2](vec3(-0.075, 0.03, -0.045),
                                        vec3(0.070, 0.10, 0.060));
    int sampleIndex = int((gl_LaunchIDEXT.x + gl_LaunchIDEXT.y) & 1u);
    float reflective = max(h.metallic, h.reflectivity);
    bool genericTransmissionActive = genericTransmissionEnabled();
    vec3 result = vec3(0.0);
    for (uint emitterIndex = 0u; emitterIndex < kRtActiveFireEmitterCapacity;
         ++emitterIndex)
    {
        RtFireEmitterGpu emitter = rtFireEmitters.values[emitterIndex];
        float strength = emitter.lightPositionStrength.w;
        if (emitter.identity.x == 0u || strength <= 0.001) continue;

        vec3 lightPosition = emitter.lightPositionStrength.xyz;
        vec3 lightColor = tunedLightColor(emitter.colourIntensity.rgb, kLightTorch);
        vec3 toLight = lightPosition - h.position;
        float lightDistance = length(toLight);
        vec3 lightDirection = toLight / max(lightDistance, 0.001);
        vec3 sampleVector = lightPosition + areaOffsets[sampleIndex] - h.position;
        float sampleDistance = length(sampleVector);
        vec3 sampleDirection = sampleVector / max(sampleDistance, 0.001);
        float attenuation = 1.0 / (1.0 + lightDistance * lightDistance * 0.58);
        float diffuse = max(dot(h.normal, lightDirection), 0.0);
        float specular = allowAnalyticSpecular
            ? pow(max(dot(reflect(-lightDirection, h.normal),
                          -rayDirection), 0.0),
                  mix(34.0, 5.0, reflective))
            : 0.0;
        // A shadow ray cannot affect a mathematically zero lighting term.
        // Rejecting that work before traversal is exact (not a visibility
        // approximation) and matters for the three orthogonal corridor faces.
        if (diffuse <= 0.0 && specular <= 0.0)
            continue;
        vec3 lightTransmittance = sceneShadowTransmittanceMask(
            offsetRayOrigin(h, sampleDirection), sampleDirection,
            sampleDistance - 0.02, 0x35u);
        if (dualVisibility)
        {
            vec3 otherVector = lightPosition + areaOffsets[1 - sampleIndex] - h.position;
            float otherDistance = length(otherVector);
            vec3 otherDirection = otherVector / max(otherDistance, 0.001);
            lightTransmittance = 0.5 * (lightTransmittance +
                sceneShadowTransmittanceMask(
                    offsetRayOrigin(h, otherDirection), otherDirection,
                    otherDistance - 0.02, 0x35u));
        }
        float lightVisibility = lightTransmittance.x;
        if (!genericTransmissionActive)
        {
            result += h.base * lightColor * diffuse * attenuation * lightVisibility *
                      strength * 2.65;
        }
        else
        {
            result += h.base * lightColor * lightTransmittance * diffuse * attenuation *
                      strength * 2.65;
        }
        if (allowAnalyticSpecular)
        {
            if (!genericTransmissionActive)
            {
                result += lightColor * specular * attenuation * lightVisibility *
                          strength * (0.12 + reflective * 1.04);
            }
            else
            {
                result += lightColor * lightTransmittance * specular * attenuation *
                          strength * (0.12 + reflective * 1.04);
            }
        }
    }
    return result;
}

void activeSkyLight(vec3 surfacePosition, int sampleIndex, out vec3 direction,
                    out float distance, out vec3 radiance, out float gain)
{
    bool inSkylightChamber = surfacePosition.x >= -8.5 && surfacePosition.x <= -2.5 &&
                             surfacePosition.z >= -18.0 && surfacePosition.z <= -12.4;
    bool inFinaleChamber = surfacePosition.x >= -36.9 && surfacePosition.x <= -30.5 &&
                           surfacePosition.z >= -18.4 && surfacePosition.z <= -12.0;
    bool finaleActive = inFinaleChamber && controls.finaleSkylightOpen > 0.001;
    vec3 target = finaleActive
        ? (sampleIndex == 0 ? vec3(-34.35, 2.76, -16.02) : vec3(-33.05, 2.76, -14.38))
        : vec3(0.0);
    vec3 toSky = target - surfacePosition;
    // The moon is an ordinary directional emitter. Roof slabs, apertures,
    // characters and moving geometry decide visibility through the shared RT
    // query; no room-coordinate light target is allowed to bypass them. The
    // finale remains a bounded authored dawn aperture while its roof animates.
    distance = finaleActive ? length(toSky) : 18.0;
    direction = finaleActive ? toSky / max(distance, 0.001) : kMoonDirection;
    gain = finaleActive ? smoothstep(0.0, 0.88, controls.finaleSkylightOpen) : 1.0;
    vec3 finaleRadiance = tunedLightColor(
        mix(vec3(0.38, 0.52, 0.76), vec3(1.12, 0.54, 0.23),
            smoothstep(0.0, 1.0, controls.finaleDawnReveal)),
        kLightSkylight);
    radiance = finaleActive ? finaleRadiance : tunedLightColor(
        inSkylightChamber ? vec3(0.28, 0.44, 0.72) : vec3(0.26, 0.34, 0.48),
        kLightSkylight);
}

vec3 shadeOpaqueDirect(HitInfo h, vec3 rayDirection, bool dualVisibility,
                       bool allowAnalyticFireSpecular,
                       out float localVisibility, out float skyVisibility,
                       out float skyDiffuse, out vec3 localColor,
                       out float localStrength)
{
    vec3 localPosition;
    activeLocalLight(localPosition, localColor, localStrength);
    vec3 localVector = localPosition - h.position;
    float localDistance = length(localVector);
    vec3 localDirection = localVector / max(localDistance, 0.001);
    const vec3 areaOffsets[2] = vec3[2](vec3(-0.075, 0.03, -0.045),
                                        vec3(0.070, 0.10, 0.060));
    int sampleIndex = int((gl_LaunchIDEXT.x + gl_LaunchIDEXT.y) & 1u);
    vec3 sampleVector = localPosition + areaOffsets[sampleIndex] - h.position;
    float sampleDistance = length(sampleVector);
    vec3 sampleDirection = sampleVector / max(sampleDistance, 0.001);
    bool genericTransmissionActive = genericTransmissionEnabled();
    vec3 localTransmittance = vec3(0.0);
    vec3 skyTransmittance = vec3(0.0);
    vec3 skyDirection;
    float skyDistance;
    vec3 skyRadiance;
    float skyGain;
    vec3 otherDirection = sampleDirection;
    float otherDistance = sampleDistance;
    if (dualVisibility && localStrength > 0.001)
    {
        vec3 otherVector = localPosition + areaOffsets[1 - sampleIndex] - h.position;
        otherDistance = length(otherVector);
        otherDirection = otherVector / max(otherDistance, 0.001);
    }
    activeSkyLight(h.position, sampleIndex, skyDirection, skyDistance,
                   skyRadiance, skyGain);
    vec3 otherSkyDirection = skyDirection;
    float otherSkyDistance = skyDistance;
    float otherSkyGain = skyGain;
    if (dualVisibility)
    {
        vec3 otherSkyRadiance;
        activeSkyLight(h.position, 1 - sampleIndex, otherSkyDirection,
                       otherSkyDistance, otherSkyRadiance, otherSkyGain);
        skyRadiance = 0.5 * (skyRadiance + otherSkyRadiance);
    }
    float reflective = max(h.metallic, h.reflectivity);
    float localDiffuse = max(dot(h.normal, localDirection), 0.0);
    float localSpecular = pow(max(dot(reflect(-localDirection, h.normal),
                                      -rayDirection), 0.0),
                              mix(34.0, 5.0, reflective));
    float localAttenuation = 1.0 / (1.0 + localDistance * localDistance * 0.58);
    skyDiffuse = max(dot(h.normal, skyDirection), 0.0);
    vec3 skyHalf = normalize(skyDirection - rayDirection);
    float skySpecular = pow(max(dot(h.normal, skyHalf), 0.0),
                            mix(12.0, 96.0, reflective));
    bool localContributes = localStrength > 0.001 &&
        (localDiffuse > 0.0 || localSpecular > 0.0);
    bool skyContributes = (skyGain > 0.0 || otherSkyGain > 0.0) &&
        (skyDiffuse > 0.0 || skySpecular > 0.0);
    vec3 visibilityOrigins[kShadowSampleCapacity] = vec3[kShadowSampleCapacity](
        offsetRayOrigin(h, sampleDirection), offsetRayOrigin(h, otherDirection),
        offsetRayOrigin(h, skyDirection), offsetRayOrigin(h, otherSkyDirection));
    vec3 visibilityDirections[kShadowSampleCapacity] = vec3[kShadowSampleCapacity](
        sampleDirection, otherDirection, skyDirection, otherSkyDirection);
    float visibilityDistances[kShadowSampleCapacity] = float[kShadowSampleCapacity](
        sampleDistance - 0.02, otherDistance - 0.02,
        skyDistance - 0.02, otherSkyDistance - 0.02);
    mat4 transmittanceSamples = transparentTransmittanceBatch(
        visibilityOrigins, visibilityDirections, visibilityDistances,
        bvec4(localContributes,
              dualVisibility && localContributes,
              skyContributes, dualVisibility && skyContributes));
    localTransmittance = dualVisibility
        ? 0.5 * (transmittanceSamples[0].xyz + transmittanceSamples[1].xyz)
        : transmittanceSamples[0].xyz;
    skyTransmittance = dualVisibility
        ? 0.5 * (transmittanceSamples[2].xyz * skyGain +
                 transmittanceSamples[3].xyz * otherSkyGain)
        : transmittanceSamples[2].xyz * skyGain;
    localVisibility = !genericTransmissionActive
        ? localTransmittance.x
        : dot(localTransmittance, vec3(0.2126, 0.7152, 0.0722));
    skyVisibility = !genericTransmissionActive
        ? skyTransmittance.x
        : dot(skyTransmittance, vec3(0.2126, 0.7152, 0.0722));

    float skyFresnel = pow(1.0 - max(dot(h.normal, -rayDirection), 0.0), 5.0);
    vec3 cold = tunedLightColor(vec3(0.15, 0.20, 0.28), kLightSkylight);

    vec3 color = h.base * vec3(0.025, 0.028, 0.032);
    color += fireEmitterDirectLighting(
        h, rayDirection, dualVisibility, allowAnalyticFireSpecular);
    if (!genericTransmissionActive)
    {
        color += h.base * localColor * localDiffuse * localAttenuation * localVisibility
            * localStrength * 2.65;
        color += localColor * localSpecular * localAttenuation * localVisibility
            * localStrength * (0.12 + reflective * 1.04);
        color += h.base * cold * skyDiffuse * skyVisibility * 0.10;
        color += h.base * skyRadiance * skyDiffuse * skyVisibility * 0.86;
        color += skyRadiance * skySpecular * skyVisibility
            * mix(0.025, 0.44, reflective) * (0.45 + 0.55 * skyFresnel);
    }
    else
    {
        color += h.base * localColor * localTransmittance * localDiffuse * localAttenuation
            * localStrength * 2.65;
        color += localColor * localTransmittance * localSpecular * localAttenuation
            * localStrength * (0.12 + reflective * 1.04);
        color += h.base * cold * skyTransmittance * skyDiffuse * 0.10;
        color += h.base * skyRadiance * skyTransmittance * skyDiffuse * 0.86;
        color += skyRadiance * skyTransmittance * skySpecular
            * mix(0.025, 0.44, reflective) * (0.45 + 0.55 * skyFresnel);
    }
    return color;
}

vec3 shadeOpaqueSecondary(HitInfo h, vec3 incoming)
{
    // This is the terminal radiance evaluation for a reflected water ray. It
    // shares the ordinary material and active-light BRDF plus real shadow rays,
    // but never traces another reflection, refraction or indirect bounce.
    if (!h.hit)
    {
        return skyColor(incoming);
    }
    if (h.emissive > 0.0)
    {
        return h.base * h.emissive;
    }
    if (h.material == kMaterialWater)
    {
        return skyColor(reflect(incoming, h.geometricNormal)) * 0.06 + vec3(0.008);
    }
    float localVisibility;
    float skyVisibility;
    float skyDiffuse;
    vec3 localColor;
    float localStrength;
    vec3 color = shadeOpaqueDirect(h, incoming, false, false, localVisibility,
                                   skyVisibility, skyDiffuse, localColor,
                                   localStrength);
    float fog = 1.0 - exp(-h.t * h.t * 0.012 * controls.fogDensityScale);
    vec3 fogColor = tunedLightColor(
        mix(vec3(0.028, 0.034, 0.046), vec3(0.052, 0.064, 0.086),
            skyVisibility * skyDiffuse * 0.55),
        kLightSkylight);
    color = mix(color, fogColor, fog);
    color += localColor * 0.16 * clamp(localStrength, 0.0, 1.0) * exp(-h.t * 0.24);
    return color;
}

vec3 shadeTerminalOpaqueEmissive(HitInfo h, vec3 incoming)
{
    // Generic terminal path: opaque/emissive lighting is evaluated once and
    // may cast visibility rays, but it never owns another glossy transport ray.
    return shadeOpaqueSecondary(h, incoming);
}

vec3 shadeOpaquePrimary(HitInfo h, vec3 rayDirection);
