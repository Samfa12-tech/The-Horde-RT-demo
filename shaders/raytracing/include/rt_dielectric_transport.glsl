const int kMobileDielectricInterfaces = 4;
const int kHighDielectricInterfaces = 8;
const int kMobileDielectricVolumes = 2;
const int kHighDielectricVolumes = 4;

bool isGenericDielectric(HitInfo hit)
{
    return hit.hit && hit.transmission > 0.001 &&
        (hit.materialFlags & kRtMaterialFlagTransmission) != 0u;
}

vec3 roughDielectricDirection(vec3 idealDirection, vec3 surfaceNormal,
                              vec3 surfacePosition, float roughness)
{
    vec3 ideal = normalize(idealDirection);
    vec3 interfaceNormal = normalize(surfaceNormal);
    vec3 tangent = abs(interfaceNormal.y) < 0.92
        ? normalize(cross(interfaceNormal, vec3(0.0, 1.0, 0.0)))
        : normalize(cross(interfaceNormal, vec3(1.0, 0.0, 0.0)));
    vec3 bitangent = normalize(cross(interfaceNormal, tangent));
    float phase = fract(sin(dot(surfacePosition,
        vec3(12.9898, 78.233, 37.719))) * 43758.5453) * 6.2831853;
    float spread = clamp(roughness, 0.0, 1.0);
    vec3 perturbed = normalize(ideal +
        (tangent * cos(phase) + bitangent * sin(phase)) * spread * spread * 0.24);
    vec3 result = normalize(mix(ideal, perturbed, spread));
    // A microfacet lobe may approach the geometric tangent, but reflection,
    // TIR, and exit transmission cannot cross to the opposite side of the
    // ideal interface. Constrain only the tiny normal component; retain the
    // authored rough tangential spread and its energy partition.
    float idealSide = dot(ideal, interfaceNormal) >= 0.0 ? 1.0 : -1.0;
    float signedAlignment = dot(result, interfaceNormal) * idealSide;
    if (signedAlignment < 0.0001)
        result = normalize(result + interfaceNormal * idealSide *
                           (0.0001 - signedAlignment));
    return result;
}

vec3 dielectricOverflowFallback(vec3 direction, vec3 throughput)
{
    // Bounded and conspicuous in captures without becoming an emissive effect.
    return throughput * (skyColor(direction) * 0.12 + vec3(0.028, 0.009, 0.036));
}

vec3 shadeBoundedDielectric(HitInfo firstHit, vec3 rayDirection)
{
    bool highQuality = controls.waterQuality >= 1.5;
    int interfaceBudget = highQuality
        ? kHighDielectricInterfaces : kMobileDielectricInterfaces;
    int volumeBudget = highQuality
        ? kHighDielectricVolumes : kMobileDielectricVolumes;

    vec3 firstOutward = normalize(firstHit.geometricNormal);
    vec3 firstNormal = dot(rayDirection, firstOutward) < 0.0
        ? firstOutward : -firstOutward;
    float firstCosine = clamp(dot(firstNormal, -rayDirection), 0.0, 1.0);
    float firstFresnel = dielectricEffectiveFresnel(
        firstCosine, 1.0, firstHit.ior, firstHit.roughness);

    vec3 reflectionDirection = roughDielectricDirection(
        reflect(rayDirection, firstNormal), firstNormal,
        firstHit.position, firstHit.roughness);
    float reflectionEpsilon = dielectricRayEpsilon(
        firstHit.position, firstHit.t);
    HitInfo reflectedHit = traceScene(
        advanceDielectricRayOrigin(firstHit.position, firstOutward,
                                   reflectionDirection, reflectionEpsilon),
        reflectionDirection, 12.0, 0x37u, reflectionEpsilon * 0.5,
        false, false);
    float reflectedLocalDistance = reflectedHit.t;
    reflectedHit.t += firstHit.t;
    vec3 reflected;
    if (isGenericDielectric(reflectedHit) ||
        (reflectedHit.hit && reflectedHit.material == kMaterialWater))
    {
        atomicAdd(rtDielectricDiagnostics.value.secondaryDielectricTerminalCount, 1u);
        // This is the reusable one-reflection terminal approximation, never
        // an opaque reclassification or a rejected dielectric hit. Attribute
        // both endpoints so captures distinguish a pane-origin reflection
        // from the dielectric surface it subsequently encountered.
        if (firstHit.instance == 8u)
            atomicAdd(rtDielectricDiagnostics.value.productionPaneSecondaryOriginCount, 1u);
        if (reflectedHit.instance == 8u)
            atomicAdd(rtDielectricDiagnostics.value.productionPaneSecondaryTerminalCount, 1u);
        if (isGenericDielectric(reflectedHit) &&
            firstHit.instance == 8u && reflectedHit.instance == 8u &&
            firstHit.instance == reflectedHit.instance &&
            firstHit.material == reflectedHit.material)
        {
            atomicAdd(rtDielectricDiagnostics.value.productionPaneSecondarySameMediumCount, 1u);
            if (reflectedLocalDistance <= reflectionEpsilon * 8.0)
                atomicAdd(rtDielectricDiagnostics.value.secondaryNearSelfHitCount, 1u);
        }
        else if (firstHit.instance == 8u || reflectedHit.instance == 8u)
            atomicAdd(rtDielectricDiagnostics.value.productionPaneSecondaryDifferentMediumCount, 1u);
        reflected = skyColor(reflectionDirection) * 0.18 +
            (reflectedHit.hit ? reflectedHit.base * 0.12 : vec3(0.0));
    }
    else
    {
        reflected = shadeTerminalOpaqueEmissive(reflectedHit, reflectionDirection);
    }
    reflectedLocalDistance = reflectedHit.hit
        ? max(reflectedHit.t - firstHit.t, 0.0) : 12.0;
    vec4 reflectedFire = integrateFireEmitters(
        firstHit.position + reflectionDirection * 0.004,
        reflectionDirection, min(reflectedLocalDistance, 12.0), true);
    reflected = reflected * reflectedFire.a + reflectedFire.rgb;

    uint volumeMaterials[kHighDielectricVolumes];
    uint volumeInstances[kHighDielectricVolumes];
    uint volumeMaterialFlags[kHighDielectricVolumes];
    float volumeIors[kHighDielectricVolumes];
    vec4 volumeAttenuation[kHighDielectricVolumes];
    int volumeDepth = 0;
    vec3 throughput = vec3(1.0);
    vec3 transmissionDirection = rayDirection;
    HitInfo currentHit = firstHit;
    float totalDistance = firstHit.t;
    vec3 transmitted = vec3(0.0);
    bool terminalResolved = false;
    bool overflowed = false;
    bool touchedProductionPane = false;
    int tirSinceLastTransition = 0;

    for (int interfaceIndex = 0; interfaceIndex <= kHighDielectricInterfaces;
         ++interfaceIndex)
    {
        float segmentLength = currentHit.t;
        if (interfaceIndex > 0 && volumeDepth > 0)
        {
            vec4 attenuation = volumeAttenuation[volumeDepth - 1];
            throughput *= dielectricBeerLambert(
                attenuation.rgb, segmentLength, attenuation.a);
        }
        if (!isGenericDielectric(currentHit))
        {
            if (volumeDepth > 0)
            {
                bool everyOpenVolumeCertified = true;
                for (int volumeIndex = 0; volumeIndex < kHighDielectricVolumes;
                     ++volumeIndex)
                {
                    if (volumeIndex < volumeDepth)
                        everyOpenVolumeCertified = everyOpenVolumeCertified &&
                            (volumeMaterialFlags[volumeIndex] &
                             kRtMaterialFlagCertifiedClosedVolume) != 0u;
                }
                if (everyOpenVolumeCertified)
                {
                    // The loader proved every open component is an outward
                    // closed manifold. A grazing numerical terminal may not
                    // shade through its cage: conservatively absorb remaining
                    // energy and attribute the bounded recovery separately.
                    atomicAdd(rtDielectricDiagnostics.value.primaryCertifiedClosedVolumeRecoveryCount,
                              1u);
                    atomicOr(rtDielectricDiagnostics.value.certifiedClosedVolumeRecoveryReasonMask,
                             1u);
                    transmitted = vec3(0.0);
                    terminalResolved = true;
                    break;
                }
                atomicAdd(rtDielectricDiagnostics.value.unclosedVolumeCount, 1u);
                atomicAdd(rtDielectricDiagnostics.value.primaryUnclosedVolumeCount, 1u);
                if (currentHit.hit)
                {
                    atomicAdd(rtDielectricDiagnostics.value.primaryOpenOpaqueCount, 1u);
                    if (tirSinceLastTransition > 0)
                        atomicAdd(rtDielectricDiagnostics.value.primaryOpenOpaqueAfterTirCount,
                                  1u);
                    atomicOr(rtDielectricDiagnostics.value.primaryOpenOpaqueTerminalInstanceMask,
                             1u << min(uint(currentHit.instance), 31u));
                    atomicOr(rtDielectricDiagnostics.value.primaryOpenOpaqueVolumeInstanceMask,
                             1u << min(volumeInstances[volumeDepth - 1], 31u));
                    atomicOr(rtDielectricDiagnostics.value.primaryOpenOpaqueTerminalMaterialMask,
                             1u << (uint(currentHit.material) & 31u));
                    if (currentHit.instance == int(volumeInstances[volumeDepth - 1]) &&
                        uint(currentHit.material) != volumeMaterials[volumeDepth - 1])
                        atomicAdd(rtDielectricDiagnostics.value.primaryOpenOpaqueSameInstanceDifferentMaterialCount,
                                  1u);
                }
                else
                    atomicAdd(rtDielectricDiagnostics.value.primaryOpenMissCount, 1u);
                overflowed = true;
            }
            else
            {
                currentHit.t = totalDistance;
                transmitted = throughput *
                    shadeTerminalOpaqueEmissive(currentHit, transmissionDirection);
            }
            terminalResolved = volumeDepth == 0;
            break;
        }
        if (interfaceIndex >= interfaceBudget)
        {
            if (volumeDepth > 0 && tirSinceLastTransition > 0)
            {
                // Repeated TIR leaves the remaining transmission energy
                // physically trapped in the closed volume. At the fixed
                // Mobile/High interface bound, conservatively absorb it; the
                // path has not overflowed or escaped an open stack.
                atomicAdd(rtDielectricDiagnostics.value.primaryTirTerminationCount, 1u);
                transmitted = vec3(0.0);
                terminalResolved = true;
            }
            else if (volumeDepth > 0)
            {
                bool everyOpenVolumeCertified = true;
                for (int volumeIndex = 0; volumeIndex < kHighDielectricVolumes;
                     ++volumeIndex)
                {
                    if (volumeIndex < volumeDepth)
                        everyOpenVolumeCertified = everyOpenVolumeCertified &&
                            (volumeMaterialFlags[volumeIndex] &
                             kRtMaterialFlagCertifiedClosedVolume) != 0u;
                }
                if (everyOpenVolumeCertified)
                {
                    atomicAdd(rtDielectricDiagnostics.value.primaryCertifiedClosedVolumeRecoveryCount,
                              1u);
                    atomicOr(rtDielectricDiagnostics.value.certifiedClosedVolumeRecoveryReasonMask,
                             2u);
                    transmitted = vec3(0.0);
                    terminalResolved = true;
                    break;
                }
                atomicAdd(rtDielectricDiagnostics.value.primaryInterfaceBudgetCount, 1u);
                atomicAdd(rtDielectricDiagnostics.value.primaryInterfaceBudgetOpenVolumeCount, 1u);
                overflowed = true;
            }
            else
            {
                atomicAdd(rtDielectricDiagnostics.value.primaryInterfaceBudgetCount, 1u);
                atomicAdd(rtDielectricDiagnostics.value.primaryInterfaceBudgetClosedVolumeCount, 1u);
                overflowed = true;
            }
            break;
        }

        bool thinWall = (currentHit.materialFlags & kRtMaterialFlagThinWall) != 0u;
        touchedProductionPane = touchedProductionPane || currentHit.instance == 8u;
        vec3 outwardNormal = normalize(currentHit.geometricNormal);
        bool entering = dot(transmissionDirection, outwardNormal) < 0.0;
        vec3 orientedNormal = entering ? outwardNormal : -outwardNormal;
        float incidentIor = volumeDepth > 0 ? volumeIors[volumeDepth - 1] : 1.0;
        float transmittedIor = entering ? currentHit.ior
            : (volumeDepth > 1 ? volumeIors[volumeDepth - 2] : 1.0);
        float cosine = clamp(dot(orientedNormal, -transmissionDirection), 0.0, 1.0);
        float fresnel = dielectricEffectiveFresnel(
            cosine, incidentIor, transmittedIor, currentHit.roughness);
        vec3 idealTransmission = thinWall
            ? transmissionDirection
            : refract(transmissionDirection, orientedNormal,
                      incidentIor / max(transmittedIor, 1.0));

        if (!thinWall && dot(idealTransmission, idealTransmission) < 0.25)
        {
            // TIR consumes one interface but does not mutate the medium stack.
            atomicAdd(rtDielectricDiagnostics.value.primaryTirCount, 1u);
            ++tirSinceLastTransition;
            transmissionDirection = roughDielectricDirection(
                reflect(transmissionDirection, orientedNormal), orientedNormal,
                currentHit.position, currentHit.roughness);
        }
        else
        {
            if (!thinWall)
            {
                tirSinceLastTransition = 0;
                if (entering)
                {
                    if (volumeDepth >= volumeBudget)
                    {
                        bool everyOpenVolumeCertified =
                            (currentHit.materialFlags &
                             kRtMaterialFlagCertifiedClosedVolume) != 0u;
                        for (int volumeIndex = 0;
                             volumeIndex < kHighDielectricVolumes; ++volumeIndex)
                        {
                            if (volumeIndex < volumeDepth)
                                everyOpenVolumeCertified = everyOpenVolumeCertified &&
                                    (volumeMaterialFlags[volumeIndex] &
                                     kRtMaterialFlagCertifiedClosedVolume) != 0u;
                        }
                        if (everyOpenVolumeCertified)
                        {
                            atomicAdd(rtDielectricDiagnostics.value.primaryCertifiedClosedVolumeRecoveryCount,
                                      1u);
                            atomicOr(rtDielectricDiagnostics.value.certifiedClosedVolumeRecoveryReasonMask,
                                     4u);
                            transmitted = vec3(0.0);
                            terminalResolved = true;
                            break;
                        }
                        atomicAdd(rtDielectricDiagnostics.value.primaryVolumeBudgetCount, 1u);
                        overflowed = true;
                        break;
                    }
                    volumeMaterials[volumeDepth] = uint(currentHit.material);
                    volumeInstances[volumeDepth] = currentHit.instance;
                    volumeMaterialFlags[volumeDepth] = currentHit.materialFlags;
                    volumeIors[volumeDepth] = currentHit.ior;
                    volumeAttenuation[volumeDepth] = vec4(
                        currentHit.attenuationColor,
                        currentHit.attenuationDistance);
                    ++volumeDepth;
                }
                else
                {
                    if (volumeDepth <= 0 ||
                        volumeMaterials[volumeDepth - 1] != uint(currentHit.material) ||
                        volumeInstances[volumeDepth - 1] != currentHit.instance)
                    {
                        bool everyOpenVolumeCertified =
                            (currentHit.materialFlags &
                             kRtMaterialFlagCertifiedClosedVolume) != 0u;
                        for (int volumeIndex = 0;
                             volumeIndex < kHighDielectricVolumes; ++volumeIndex)
                        {
                            if (volumeIndex < volumeDepth)
                                everyOpenVolumeCertified = everyOpenVolumeCertified &&
                                    (volumeMaterialFlags[volumeIndex] &
                                     kRtMaterialFlagCertifiedClosedVolume) != 0u;
                        }
                        if (everyOpenVolumeCertified)
                        {
                            atomicAdd(rtDielectricDiagnostics.value.primaryCertifiedClosedVolumeRecoveryCount,
                                      1u);
                            atomicOr(rtDielectricDiagnostics.value.certifiedClosedVolumeRecoveryReasonMask,
                                     8u);
                            transmitted = vec3(0.0);
                            terminalResolved = true;
                            break;
                        }
                        atomicAdd(rtDielectricDiagnostics.value.primaryMismatchedExitCount, 1u);
                        overflowed = true;
                        break;
                    }
                    --volumeDepth;
                }
            }
            // Thin sheets scatter at their sole interface. A thick closed
            // volume must first traverse to the paired exit geometrically:
            // scattering on entry can jump a 7 mm pane into its cage before
            // Mobile has enough interfaces to close the stack. Apply the
            // same deterministic microfacet perturbation at the physical
            // exit instead, after the paired instance/material pop. This
            // bounded two-interface approximation retains rough directional
            // transmission while it cannot escape a volume before its exit.
            vec3 roughTransmission = roughDielectricDirection(
                idealTransmission, orientedNormal,
                currentHit.position, currentHit.roughness);
            transmissionDirection = (!thinWall && entering)
                ? normalize(idealTransmission) : roughTransmission;
            throughput *= clamp(currentHit.transmission, 0.0, 1.0) *
                (1.0 - fresnel);
        }

        float epsilon = dielectricRayEpsilon(currentHit.position, totalDistance);
        vec3 nextOrigin = advanceDielectricRayOrigin(
            currentHit.position, outwardNormal, transmissionDirection, epsilon);
        float advancedDistance = max(
            dot(nextOrigin - currentHit.position, transmissionDirection), 0.0);
        HitInfo nextHit = traceScene(
            nextOrigin, transmissionDirection, 10000.0, 0x23u,
            epsilon * 0.5, false, false);
        nextHit.t += advancedDistance;
        totalDistance += nextHit.t;
        currentHit = nextHit;
    }

    if (!terminalResolved)
    {
        bool everyOpenVolumeCertified = volumeDepth > 0;
        for (int volumeIndex = 0; volumeIndex < kHighDielectricVolumes;
             ++volumeIndex)
        {
            if (volumeIndex < volumeDepth)
                everyOpenVolumeCertified = everyOpenVolumeCertified &&
                    (volumeMaterialFlags[volumeIndex] &
                     kRtMaterialFlagCertifiedClosedVolume) != 0u;
        }
        if (everyOpenVolumeCertified)
        {
            atomicAdd(rtDielectricDiagnostics.value.primaryCertifiedClosedVolumeRecoveryCount,
                      1u);
            atomicOr(rtDielectricDiagnostics.value.certifiedClosedVolumeRecoveryReasonMask,
                     16u);
            transmitted = vec3(0.0);
        }
        else
        {
            overflowed = true;
            atomicAdd(rtDielectricDiagnostics.value.transportOverflowCount, 1u);
            if (touchedProductionPane)
                atomicAdd(rtDielectricDiagnostics.value.productionPaneStackFailureCount, 1u);
            transmitted = dielectricOverflowFallback(transmissionDirection, throughput);
        }
    }
    // Roughness changes the same bounded reflection/transmission response; it
    // never switches to a screen sample or adds a recursive glossy path.
    return reflected * firstFresnel + transmitted;
}
