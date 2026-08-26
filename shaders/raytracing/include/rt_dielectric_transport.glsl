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
    vec3 tangent = abs(surfaceNormal.y) < 0.92
        ? normalize(cross(surfaceNormal, vec3(0.0, 1.0, 0.0)))
        : normalize(cross(surfaceNormal, vec3(1.0, 0.0, 0.0)));
    vec3 bitangent = normalize(cross(surfaceNormal, tangent));
    float phase = fract(sin(dot(surfacePosition,
        vec3(12.9898, 78.233, 37.719))) * 43758.5453) * 6.2831853;
    float spread = clamp(roughness, 0.0, 1.0);
    vec3 perturbed = normalize(idealDirection +
        (tangent * cos(phase) + bitangent * sin(phase)) * spread * spread * 0.24);
    return normalize(mix(idealDirection, perturbed, spread));
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
    reflectedHit.t += firstHit.t;
    vec3 reflected;
    if (isGenericDielectric(reflectedHit) ||
        (reflectedHit.hit && reflectedHit.material == kMaterialWater))
    {
        atomicAdd(rtDielectricDiagnostics.value.secondaryDielectricRejectCount, 1u);
        reflected = skyColor(reflectionDirection) * 0.18 +
            (reflectedHit.hit ? reflectedHit.base * 0.12 : vec3(0.0));
    }
    else
    {
        reflected = shadeTerminalOpaqueEmissive(reflectedHit, reflectionDirection);
    }
    float reflectedLocalDistance = reflectedHit.hit
        ? max(reflectedHit.t - firstHit.t, 0.0) : 12.0;
    vec4 reflectedFire = integrateFireEmitters(
        firstHit.position + reflectionDirection * 0.004,
        reflectionDirection, min(reflectedLocalDistance, 12.0), true);
    reflected = reflected * reflectedFire.a + reflectedFire.rgb;

    uint volumeMaterials[kHighDielectricVolumes];
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
                atomicAdd(rtDielectricDiagnostics.value.unclosedVolumeCount, 1u);
                atomicAdd(rtDielectricDiagnostics.value.primaryUnclosedVolumeCount, 1u);
                transmitted = dielectricOverflowFallback(
                    transmissionDirection, throughput);
            }
            else
            {
                currentHit.t = totalDistance;
                transmitted = throughput *
                    shadeTerminalOpaqueEmissive(currentHit, transmissionDirection);
            }
            terminalResolved = true;
            break;
        }
        if (interfaceIndex >= interfaceBudget)
        {
            overflowed = true;
            break;
        }

        bool thinWall = (currentHit.materialFlags & kRtMaterialFlagThinWall) != 0u;
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
            transmissionDirection = roughDielectricDirection(
                reflect(transmissionDirection, orientedNormal), orientedNormal,
                currentHit.position, currentHit.roughness);
        }
        else
        {
            if (!thinWall)
            {
                if (entering)
                {
                    if (volumeDepth >= volumeBudget)
                    {
                        overflowed = true;
                        break;
                    }
                    volumeMaterials[volumeDepth] = uint(currentHit.material);
                    volumeIors[volumeDepth] = currentHit.ior;
                    volumeAttenuation[volumeDepth] = vec4(
                        currentHit.attenuationColor,
                        currentHit.attenuationDistance);
                    ++volumeDepth;
                }
                else
                {
                    if (volumeDepth <= 0 ||
                        volumeMaterials[volumeDepth - 1] != uint(currentHit.material))
                    {
                        overflowed = true;
                        break;
                    }
                    --volumeDepth;
                }
            }
            transmissionDirection = roughDielectricDirection(
                idealTransmission, -orientedNormal,
                currentHit.position, currentHit.roughness);
            throughput *= clamp(currentHit.transmission, 0.0, 1.0) *
                (1.0 - fresnel);
        }

        float epsilon = dielectricRayEpsilon(
            currentHit.position, totalDistance);
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
        overflowed = true;
        atomicAdd(rtDielectricDiagnostics.value.transportOverflowCount, 1u);
        transmitted = dielectricOverflowFallback(transmissionDirection, throughput);
    }
    // Roughness changes the same bounded reflection/transmission response; it
    // never switches to a screen sample or adds a recursive glossy path.
    return reflected * firstFresnel + transmitted;
}
