vec3 shadeThinWater(HitInfo h, vec3 rayDirection)
{
    vec3 baseNormal = waterBaseNormal(h.position, h.geometricNormal);
    vec3 geometricNormal = dot(baseNormal, -rayDirection) < 0.0
        ? -baseNormal : baseNormal;
    vec3 surfaceNormal = dot(h.normal, -rayDirection) < 0.0 ? -h.normal : h.normal;

    const float waterIor = 1.333;
    const float inverseWaterIor = 0.7501875;
    bool runoff = abs(geometricNormal.y) > 0.65;

    // Air -> water at the real primary interface.
    vec3 insideDirection = rayDirection;
    vec3 transmissionDirection = rayDirection;
    vec3 exitSurfaceNormal = surfaceNormal;
    vec3 exitPoint = h.position;
    float waterPathLength = 0.0;
    bool transmissionAvailable = true;
    if (controls.waterQuality >= 0.5)
    {
        insideDirection = refract(rayDirection, surfaceNormal, inverseWaterIor);
        if (dot(insideDirection, insideDirection) < 0.25)
        {
            insideDirection = rayDirection;
        }

        if (runoff)
        {
            // The catchment and runnel are a six-millimetre parallel film.
            waterPathLength = 0.006 /
                max(-dot(insideDirection, geometricNormal), 0.18);
            exitPoint += insideDirection * waterPathLength;
            exitSurfaceNormal = waterSurfaceNormal(
                exitPoint + vec3(-0.08, 0.0, 0.012), geometricNormal);
        }
        else
        {
            // The falling streams are closed tapered ellipses. Solve the actual
            // far interface instead of treating the front facet as a short slab;
            // that old mismatch collapsed the refracted view into a dark column.
            vec3 exitOutwardNormal = geometricNormal;
            if (waterStreamExit(h.position, insideDirection, exitPoint,
                                exitOutwardNormal, waterPathLength))
            {
                exitSurfaceNormal = waterSurfaceNormal(
                    exitPoint + vec3(0.0, -0.08, 0.0), exitOutwardNormal);
            }
            else
            {
                // Degenerate near-grazing rays keep a bounded thin-sheet
                // approximation instead of producing NaNs or an unbounded path.
                waterPathLength = 0.010 /
                    max(-dot(insideDirection, geometricNormal), 0.18);
                exitPoint += insideDirection * waterPathLength;
                exitSurfaceNormal = surfaceNormal;
            }
        }
        if (dot(exitSurfaceNormal, -insideDirection) < 0.0)
        {
            exitSurfaceNormal = -exitSurfaceNormal;
        }
        transmissionDirection = refract(insideDirection, exitSurfaceNormal, waterIor);
        if (dot(transmissionDirection, transmissionDirection) < 0.25)
        {
            // Total internal reflection has no transmitted path. High mode still
            // traces its one reflected ray; Mobile uses analytic reflection.
            transmissionAvailable = false;
        }
    }
    vec3 transmitted = vec3(0.0);
    if (transmissionAvailable)
    {
        HitInfo transmittedHit = traceScene(exitPoint + transmissionDirection * 0.004,
                                             transmissionDirection, 12.0, 0x23u, true);
        transmittedHit.t += h.t + waterPathLength;
        transmitted = shadeOpaqueSecondary(transmittedHit, transmissionDirection);
        transmitted *= exp(-vec3(0.060, 0.018, 0.008) * waterPathLength);
    }
    if (controls.waterQuality < 0.5)
    {
        return transmitted;
    }

    vec3 reflectionDirection = reflect(rayDirection, surfaceNormal);
    vec3 reflected = skyColor(reflectionDirection);
    if (controls.waterQuality >= 1.5)
    {
        HitInfo reflectedHit = traceScene(h.position + geometricNormal * 0.006,
                                          reflectionDirection, 12.0, 0x37u, false);
        reflectedHit.t += h.t;
        reflected = shadeOpaqueSecondary(reflectedHit, reflectionDirection);
    }

    float viewCosine = clamp(dot(surfaceNormal, -rayDirection), 0.0, 1.0);
    float fresnel = transmissionAvailable
        ? 0.0204 + 0.9796 * pow(1.0 - viewCosine, 5.0) : 1.0;
    vec3 localPosition;
    vec3 localColor;
    float localStrength;
    activeLocalLight(localPosition, localColor, localStrength);
    vec3 toLocal = localPosition - h.position;
    float localDistanceSquared = max(dot(toLocal, toLocal), 0.001);
    float localDistance = sqrt(localDistanceSquared);
    vec3 localDirection = toLocal / localDistance;
    vec3 localHalf = normalize(localDirection - rayDirection);
    float localHighlight = pow(max(dot(surfaceNormal, localHalf), 0.0), 72.0);
    // The six-millimetre catchment/runoff film is roughened by impact ripples
    // and moving cobbles underneath it. Give only that horizontal water a broad
    // view-dependent local-light lobe so Mobile's analytic reflection does not turn
    // the pool and drain runnel black at approach angles. This remains an
    // interface reflection, now shadowed by the same world/player geometry as
    // every ordinary direct-light sample.
    float runoffLocalHighlight = runoff
        ? pow(max(dot(surfaceNormal, localHalf), 0.0), 14.0) : 0.0;
    float localInterfaceVisibility = localStrength > 0.001
        ? visibility(offsetRayOrigin(h, localDirection), localDirection,
                     localDistance - 0.02) : 0.0;
    int skySample = int((gl_LaunchIDEXT.x + gl_LaunchIDEXT.y) & 1u);
    vec3 skyDirection;
    float skyDistance;
    vec3 skyRadiance;
    float skyGain;
    activeSkyLight(h.position, skySample, skyDirection, skyDistance,
                   skyRadiance, skyGain);
    float skyInterfaceVisibility = visibility(offsetRayOrigin(h, skyDirection),
                                              skyDirection, skyDistance - 0.02)
        * skyGain;
    float skyHighlight = pow(max(dot(surfaceNormal,
        normalize(skyDirection - rayDirection)), 0.0), 96.0);
    vec3 interfaceLight = localColor
        * (localHighlight * 3.2 + runoffLocalHighlight * 1.15)
        * localStrength * localInterfaceVisibility
        / (1.0 + localDistanceSquared * 0.58);
    interfaceLight += skyRadiance * skyHighlight * skyInterfaceVisibility * 0.10;
    if (runoff && h.position.x > -2.88)
    {
        float impactDistance = length(h.position.xz - vec2(-2.32, -15.26));
        float impactCrest = pow(0.5 + 0.5 * sin(
            impactDistance * 31.0 - controls.time * 6.2), 8.0)
            * (1.0 - smoothstep(0.07, 0.68, impactDistance));
        float impactCore = 1.0 - smoothstep(0.04, 0.22, impactDistance);
        vec3 impactLight = skyRadiance * skyInterfaceVisibility * 0.44
            + localColor * localStrength * localInterfaceVisibility * 0.24;
        interfaceLight += impactLight * (impactCrest * 0.12 + impactCore * 0.045);
    }
    float surfaceTurbulence = clamp(length(surfaceNormal - exitSurfaceNormal) * 5.0, 0.0, 1.0);
    vec3 surfaceRadiance = mix(transmitted, reflected, fresnel) + interfaceLight;
    // Entrained micro-bubbles are what make a real falling stream visible at
    // near-normal incidence. Keep this single-scattering term low and confined
    // to actual water geometry; the background remains dominant and refracted.
    float breakup = clamp(0.56
        + sin(h.position.y * 7.3 - controls.time * 5.1) * 0.27
        + sin(h.position.y * 13.7 + h.position.z * 17.0
              - controls.time * 8.4) * 0.17, 0.0, 1.0);
    float entrainedAir = runoff ? 0.004
        : 0.010 + pow(breakup, 3.0) * 0.028 + surfaceTurbulence * 0.010;
    vec3 scatteringRadiance = skyRadiance * skyInterfaceVisibility * 0.38
        + localColor * localStrength * localInterfaceVisibility * 0.18;
    return mix(surfaceRadiance, scatteringRadiance, entrainedAir);
}

vec3 shadeOpaquePrimary(HitInfo h, vec3 rayDirection)
{
    if (!h.hit)
    {
        vec3 nightSky = skyColor(rayDirection);
        bool cameraInFinale = controls.cameraX >= -36.9 && controls.cameraX <= -30.5 &&
                              controls.cameraZ >= -18.4 && controls.cameraZ <= -12.0;
        float dawn = cameraInFinale ? smoothstep(0.0, 1.0, controls.finaleDawnReveal) : 0.0;
        vec3 dawnSky = tunedLightColor(
            mix(vec3(0.34, 0.14, 0.09), vec3(1.05, 0.52, 0.22),
                smoothstep(-0.12, 0.52, rayDirection.y)),
            kLightSkylight);
        return mix(nightSky, dawnSky, dawn);
    }
    if (h.emissive > 0.0)
    {
        return h.base * h.emissive;
    }

    bool leanWorkload = controls.workloadPreset < 0.5;
    bool maxWorkload = controls.workloadPreset >= 1.5;
    float reflective = max(h.metallic, h.reflectivity);
    float localVisibility;
    float skyVisibility;
    float skyDiffuse;
    vec3 localLightColor;
    float localLightStrength;
    vec3 color = shadeOpaqueDirect(h, rayDirection, maxWorkload,
                                   localVisibility, skyVisibility, skyDiffuse,
                                   localLightColor, localLightStrength);
    // Keep the one RT bounce deterministic. The previous time-varying hemisphere sample was the main source of shimmer.
    vec3 bounce = vec3(0.0);
    if (!leanWorkload)
    {
        vec3 bounceDirection = reflective > 0.38
            ? reflect(rayDirection, h.normal)
            : normalize(h.normal + vec3(0.18, 0.58, -0.34));
        bool wantsPlayerReflection = reflective > 0.38 ||
            (h.instance == 0 && h.material == kMaterialWetCobble && puddleMask(h.position) > 0.05);
        uint bounceMask = wantsPlayerReflection ? 0x37u : 0x23u;
        HitInfo bounceHit = traceScene(offsetRayOrigin(h, bounceDirection), bounceDirection,
                                       12.0, bounceMask, false);
        bounce = bounceSample(bounceHit, bounceDirection, wantsPlayerReflection);
    }

    color += h.base * bounce * (0.10 + reflective * 0.16);
    float reflectionGain = h.instance == 0 && h.material == kMaterialMirror ? 0.82 : 0.32;
    color += bounce * reflective * reflectionGain;

    if (h.instance == 0 && h.material == kMaterialWetCobble)
    {
        float puddle = puddleMask(h.position);
        color += localLightColor * puddle * localVisibility * localLightStrength * 0.28;
        color += bounce * puddle * 0.46;
    }

    float fog = 1.0 - exp(-h.t * h.t * 0.012 * controls.fogDensityScale);
    vec3 fogColor = tunedLightColor(
        mix(vec3(0.028, 0.034, 0.046), vec3(0.052, 0.064, 0.086),
            skyVisibility * skyDiffuse * 0.55),
        kLightSkylight);
    color = mix(color, fogColor, fog);
    // This close-range haze belongs to the selected local emitter. Keeping the
    // old unconditional warm term made the lantern-off skylight chamber brown
    // and also contaminated every authored bay colour.
    color += localLightColor * 0.16 * clamp(localLightStrength, 0.0, 1.0) * exp(-h.t * 0.24);
    return color;
}

vec3 shadePrimary(HitInfo h, vec3 rayDirection)
{
    if (!h.hit || h.emissive > 0.0)
    {
        return shadeOpaquePrimary(h, rayDirection);
    }

    bool isThinGlass = h.instance == 0 && h.material == kMaterialClearGlass;
    if (isThinGlass)
    {
        // This showcase pane is intentionally modelled as thin transmission: the
        // outgoing direction remains parallel, while an RT query supplies what is
        // genuinely behind the glass. Schlick Fresnel adds a readable reflection
        // without pretending this single surface is a thick dielectric volume.
        HitInfo transmittedHit = traceScene(h.position + rayDirection * 0.012,
                                            rayDirection, 10000.0, 0x23u, false);
        vec3 transmitted = transmittedHit.hit
            ? bounceSample(transmittedHit, rayDirection, false) : skyColor(rayDirection);
        vec3 tint = vec3(0.82, 0.94, 1.0);
        float viewCosine = clamp(dot(h.geometricNormal, -rayDirection), 0.0, 1.0);
        float fresnel = 0.04 + 0.96 * pow(1.0 - viewCosine, 5.0);
        vec3 reflected = skyColor(reflect(rayDirection, h.geometricNormal));
        float transmission = 0.88;
        return mix(transmitted * tint * transmission, reflected, fresnel);
    }

    bool isThinWater = h.material == kMaterialWater;
    if (isThinWater)
    {
        return shadeThinWater(h, rayDirection);
    }

    return shadeOpaquePrimary(h, rayDirection);
}
