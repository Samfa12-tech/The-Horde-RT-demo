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
float visibilityMask(vec3 origin, vec3 direction, float maxDistance, uint mask)
{
    rayQueryEXT query;
    rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsNoOpaqueEXT, mask, origin, 0.0015, direction, max(maxDistance, 0.004));
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query, false) != gl_RayQueryCandidateIntersectionTriangleEXT)
        {
            continue;
        }
        int instance = int(rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));
        int primitive = rayQueryGetIntersectionPrimitiveIndexEXT(query, false);
        bool transparentWorldPane = instance == kWaterfallInstance;
        if (instance == 0)
        {
            int material = int(worldSurfaces.codes[primitive] & 0xffu);
            transparentWorldPane = material == kMaterialClearGlass || material == kMaterialWater;
        }
        if (!transparentWorldPane)
        {
            rayQueryConfirmIntersectionEXT(query);
        }
    }
    return rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

vec3 offsetRayOrigin(HitInfo h, vec3 direction)
{
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

void activeLocalLight(out vec3 position, out vec3 color, out float strength);

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
    float lightVisibility = visibility(offsetRayOrigin(h, lightDirection), lightDirection, lightDistance - 0.02);
    float diffuse = max(dot(h.normal, lightDirection), 0.0);
    float attenuation = 1.0 / (1.0 + lightDistance * lightDistance * 0.58);
    return h.base * (0.008 + localLightColor * diffuse * attenuation * lightVisibility * localLightStrength * 2.25);
}

vec3 currentTorchLightPosition()
{
    return rtHeldLight.value.positionStrength.xyz;
}

void activeLocalLight(out vec3 position, out vec3 color, out float strength)
{
    position = currentTorchLightPosition();
    color = tunedLightColor(vec3(1.0, 0.28, 0.055), kLightTorch);
    strength = rtHeldLight.value.positionStrength.w;
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
    localVisibility = localStrength > 0.001
        ? visibility(offsetRayOrigin(h, sampleDirection), sampleDirection,
                     sampleDistance - 0.02) : 0.0;
    if (dualVisibility && localStrength > 0.001)
    {
        vec3 otherVector = localPosition + areaOffsets[1 - sampleIndex] - h.position;
        float otherDistance = length(otherVector);
        vec3 otherDirection = otherVector / max(otherDistance, 0.001);
        localVisibility = 0.5 * (localVisibility + visibility(
            offsetRayOrigin(h, otherDirection), otherDirection, otherDistance - 0.02));
    }

    vec3 skyDirection;
    float skyDistance;
    vec3 skyRadiance;
    float skyGain;
    activeSkyLight(h.position, sampleIndex, skyDirection, skyDistance, skyRadiance, skyGain);
    skyVisibility = visibility(offsetRayOrigin(h, skyDirection), skyDirection,
                               skyDistance - 0.02) * skyGain;
    if (dualVisibility)
    {
        vec3 otherSkyDirection;
        float otherSkyDistance;
        vec3 otherSkyRadiance;
        float otherSkyGain;
        activeSkyLight(h.position, 1 - sampleIndex, otherSkyDirection,
                       otherSkyDistance, otherSkyRadiance, otherSkyGain);
        float otherVisibility = visibility(offsetRayOrigin(h, otherSkyDirection),
                                           otherSkyDirection,
                                           otherSkyDistance - 0.02) * otherSkyGain;
        skyVisibility = 0.5 * (skyVisibility + otherVisibility);
        skyRadiance = 0.5 * (skyRadiance + otherSkyRadiance);
    }

    float reflective = max(h.metallic, h.reflectivity);
    float localDiffuse = max(dot(h.normal, localDirection), 0.0);
    float localSpecular = pow(max(dot(reflect(-localDirection, h.normal),
                                      -rayDirection), 0.0),
                              mix(34.0, 5.0, reflective));
    float localAttenuation = 1.0 / (1.0 + localDistance * localDistance * 0.58);
    float flamePulse = 0.84 + 0.16 * sin(controls.time * 15.0);
    skyDiffuse = max(dot(h.normal, skyDirection), 0.0);
    vec3 skyHalf = normalize(skyDirection - rayDirection);
    float skySpecular = pow(max(dot(h.normal, skyHalf), 0.0),
                            mix(12.0, 96.0, reflective));
    float skyFresnel = pow(1.0 - max(dot(h.normal, -rayDirection), 0.0), 5.0);
    vec3 cold = tunedLightColor(vec3(0.15, 0.20, 0.28), kLightSkylight);

    vec3 color = h.base * vec3(0.025, 0.028, 0.032);
    color += h.base * localColor * localDiffuse * localAttenuation * localVisibility
        * localStrength * flamePulse * 2.65;
    color += localColor * localSpecular * localAttenuation * localVisibility
        * localStrength * (0.12 + reflective * 1.04);
    color += h.base * cold * skyDiffuse * skyVisibility * 0.10;
    color += h.base * skyRadiance * skyDiffuse * skyVisibility * 0.86;
    color += skyRadiance * skySpecular * skyVisibility
        * mix(0.025, 0.44, reflective) * (0.45 + 0.55 * skyFresnel);
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
    vec3 color = shadeOpaqueDirect(h, incoming, false, localVisibility,
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

vec3 shadeOpaquePrimary(HitInfo h, vec3 rayDirection);
