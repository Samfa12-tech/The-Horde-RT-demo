uint fireHash(uint value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    value ^= value >> 16u;
    return value;
}

float fireHash01(uint seed, uint lattice)
{
    return float(fireHash(seed ^ (lattice * 0x9e3779b9u)) & 0x00ffffffu) / 16777215.0;
}

float fireValueNoise(uint seed, float domain)
{
    float base = floor(domain);
    float blend = fract(domain);
    blend = blend * blend * (3.0 - 2.0 * blend);
    uint lattice = uint(max(base, 0.0));
    return mix(fireHash01(seed, lattice), fireHash01(seed, lattice + 1u), blend) * 2.0 - 1.0;
}

bool findFireEmitter(uint stableId, out RtFireEmitterGpu emitter)
{
    for (uint index = 0u; index < kRtActiveFireEmitterCapacity; ++index)
    {
        if (rtFireEmitters.values[index].identity.x == stableId)
        {
            emitter = rtFireEmitters.values[index];
            return true;
        }
    }
    emitter = rtFireEmitters.values[0];
    emitter.lightPositionStrength.w = 0.0;
    emitter.colourIntensity.w = 0.0;
    return false;
}

vec3 fireLocalToWorld(RtFireEmitterGpu emitter, vec3 localPosition)
{
    return emitter.worldFromLocal0.xyz * localPosition.x
         + emitter.worldFromLocal1.xyz * localPosition.y
         + emitter.worldFromLocal2.xyz * localPosition.z
         + emitter.worldFromLocal3.xyz;
}

vec3 fireWorldToLocalPoint(RtFireEmitterGpu emitter, vec3 worldPosition)
{
    vec3 relative = worldPosition - emitter.worldFromLocal3.xyz;
    return vec3(dot(relative, emitter.worldFromLocal0.xyz),
                dot(relative, emitter.worldFromLocal1.xyz),
                dot(relative, emitter.worldFromLocal2.xyz));
}

vec3 fireWorldToLocalDirection(RtFireEmitterGpu emitter, vec3 worldDirection)
{
    return vec3(dot(worldDirection, emitter.worldFromLocal0.xyz),
                dot(worldDirection, emitter.worldFromLocal1.xyz),
                dot(worldDirection, emitter.worldFromLocal2.xyz));
}

bool fireSphereInterval(vec3 rayOrigin, vec3 rayDirection, vec3 centre,
                        float radius, float maximumDistance,
                        out float nearDistance, out float farDistance)
{
    vec3 toOrigin = rayOrigin - centre;
    float projection = dot(toOrigin, rayDirection);
    float discriminant = projection * projection -
        (dot(toOrigin, toOrigin) - radius * radius);
    if (discriminant <= 0.0)
    {
        nearDistance = 0.0;
        farDistance = 0.0;
        return false;
    }
    float root = sqrt(discriminant);
    nearDistance = max(0.0, -projection - root);
    farDistance = min(maximumDistance, -projection + root);
    return farDistance > nearDistance;
}

vec4 integrateOneFireEmitter(RtFireEmitterGpu emitter,
                             vec3 rayOrigin,
                             vec3 rayDirection,
                             float maximumDistance,
                             bool reflectionPath)
{
    float strength = emitter.colourIntensity.w;
    if (strength <= 0.0001)
        return vec4(0.0, 0.0, 0.0, 1.0);

    float radius = max(emitter.shape.x, 0.005);
    float height = max(emitter.shape.y, 0.02);
    float smokeHeight = height * (1.0 + emitter.smokeEmbers.x * 1.8);
    vec3 localCentre = vec3(0.0, smokeHeight * 0.48, 0.0);
    vec3 worldCentre = fireLocalToWorld(emitter, localCentre);
    float boundRadius = sqrt(smokeHeight * smokeHeight * 0.36 + radius * radius * 3.2);
    float nearDistance;
    float farDistance;
    if (!fireSphereInterval(rayOrigin, rayDirection, worldCentre, boundRadius,
                            maximumDistance, nearDistance, farDistance))
        return vec4(0.0, 0.0, 0.0, 1.0);

    int volumeSteps = int(clamp(emitter.identity.z, 1u, 10u));
    if (reflectionPath)
    {
        int reflectionSamples = int(clamp(emitter.identity.w, 0u, 2u));
        if (reflectionSamples == 0)
            return vec4(0.0, 0.0, 0.0, 1.0);
        volumeSteps = min(volumeSteps, reflectionSamples * 4);
    }
    vec3 localOrigin = fireWorldToLocalPoint(emitter, rayOrigin);
    vec3 localDirection = fireWorldToLocalDirection(emitter, rayDirection);
    float stepLength = (farDistance - nearDistance) / float(max(volumeSteps, 1));
    vec3 emission = vec3(0.0);
    float transmittance = 1.0;
    for (int stepIndex = 0; stepIndex < 10; ++stepIndex)
    {
        if (stepIndex >= volumeSteps) break;
        float distanceAlongRay = nearDistance + (float(stepIndex) + 0.5) * stepLength;
        vec3 local = localOrigin + localDirection * distanceAlongRay;
        // Periodic upper tongues curl and separate even while their attachment
        // is stationary. The base/core stays anchored to the shared Flame
        // socket; primary, glass and reflection paths integrate this same field.
        // This changes shape inside the existing bound, without more samples.
        float cycle = emitter.animation.x * 6.28318530718;
        float seedPhase = fireHash01(emitter.identity.y, 19u) * 6.28318530718;
        float pulse = sin(cycle * 2.0 + seedPhase);
        float liveHeight = height * (0.88 + 0.10 * pulse);
        float heightFraction = clamp((local.y + liveHeight * 0.16) /
                                     (liveHeight * 1.16), 0.0, 1.0);
        vec2 centreLine = emitter.animation.zw * heightFraction;
        float domainNoise = fireValueNoise(
            emitter.identity.y,
            local.y * 8.5 + emitter.animation.x * 7.0 +
                dot(local.xz, vec2(4.1, -3.7)));
        float turbulence = emitter.smokeEmbers.z;
        float tipBlend = smoothstep(0.18, 0.90, heightFraction);
        centreLine += vec2(domainNoise, -domainNoise * 0.73) *
            radius * (0.12 + 0.20 * turbulence) * heightFraction;
        centreLine += vec2(sin(cycle * 3.0 + heightFraction * 6.0 + seedPhase),
                          cos(cycle * 2.0 - heightFraction * 5.0 + seedPhase)) *
            radius * 0.34 * tipBlend;
        float taper = mix(1.0, 0.055, pow(heightFraction, 0.72));
        float localRadius = max(radius * taper,
                               emitter.shape.z * mix(0.55, 0.10, tipBlend));
        float radial = length(local.xz - centreLine) / max(localRadius, 0.001);
        float body = (1.0 - smoothstep(0.22, 1.0, radial)) *
                     smoothstep(-0.16, 0.05, local.y / liveHeight) *
                     (1.0 - smoothstep(0.72, 1.03, local.y / liveHeight));
        // A restrained second tongue breaks the single smooth spindle. Its
        // shoulder rises/falls against the main tongue instead of translating
        // the whole emitter or adding a disconnected floating flame.
        vec2 fork = centreLine + radius * tipBlend *
            vec2(0.44 * cos(cycle + seedPhase), 0.34 * sin(cycle + seedPhase));
        float forkRadial = length(local.xz - fork) / max(localRadius * 0.58, 0.001);
        float forkBody = (1.0 - smoothstep(0.20, 1.0, forkRadial)) *
            smoothstep(0.28, 0.50, heightFraction) *
            (1.0 - smoothstep(0.66 + 0.07 * pulse, 0.93, heightFraction));
        body = max(body, forkBody * 0.62);
        float breakup = clamp(0.76 + domainNoise * (0.18 + turbulence * 0.10), 0.22, 1.0);
        float density = body * breakup;
        float core = (1.0 - smoothstep(0.0, max(emitter.shape.z, 0.005),
                                       length(local.xz - centreLine))) *
                     (1.0 - smoothstep(0.30, 0.88, heightFraction));
        float opticalDepth = density * emitter.shape.w * stepLength;
        float sampleAlpha = 1.0 - exp(-max(opticalDepth, 0.0));
        vec3 hotCore = mix(emitter.colourIntensity.rgb,
                           vec3(1.0, 0.76, 0.30), core * 0.72);
        emission += transmittance * hotCore * strength *
            sampleAlpha * (2.2 + core * 2.8);
        transmittance *= 1.0 - sampleAlpha * 0.62;

        float smokeStart = height * 0.62;
        float smokeFraction = clamp((local.y - smokeStart) /
                                    max(smokeHeight - smokeStart, 0.01), 0.0, 1.0);
        float smokeRadius = radius * mix(0.72, 1.85, smokeFraction);
        float smokeRadial = length(local.xz - centreLine * 1.4) /
                            max(smokeRadius, 0.001);
        float smoke = emitter.smokeEmbers.x *
            (1.0 - smoothstep(0.28, 1.0, smokeRadial)) *
            smoothstep(0.0, 0.18, smokeFraction) * (1.0 - smokeFraction) *
            clamp(0.72 + 0.28 * domainNoise, 0.0, 1.0);
        transmittance *= exp(-smoke * stepLength * 1.7);
        emission += transmittance * vec3(0.030, 0.024, 0.020) * smoke * stepLength;
    }

    if (!reflectionPath && emitter.smokeEmbers.y > 0.001)
    {
        for (uint emberIndex = 0u; emberIndex < 4u; ++emberIndex)
        {
            float lateral = fireHash01(emitter.identity.y, 41u + emberIndex * 7u) * 2.0 - 1.0;
            float depth = fireHash01(emitter.identity.y, 59u + emberIndex * 11u) * 2.0 - 1.0;
            float speed = 0.48 + fireHash01(emitter.identity.y, 83u + emberIndex) * 0.44;
            float life = fract(emitter.animation.x * speed +
                               fireHash01(emitter.identity.y, 101u + emberIndex * 13u));
            vec3 emberLocal = vec3(lateral * radius * (0.45 + life * 0.8),
                                   height * (0.48 + life * 1.65),
                                   depth * radius * (0.45 + life * 0.8));
            emberLocal.xz += emitter.animation.zw * life;
            vec3 emberWorld = fireLocalToWorld(emitter, emberLocal);
            float emberDistance = dot(emberWorld - rayOrigin, rayDirection);
            vec3 nearest = rayOrigin + rayDirection * emberDistance;
            float emberRadius = mix(0.010, 0.003, life);
            float spark = 1.0 - smoothstep(emberRadius * 0.45, emberRadius,
                                           length(nearest - emberWorld));
            bool depthVisible = emberDistance > 0.0 && emberDistance < maximumDistance;
            emission += depthVisible
                ? emitter.colourIntensity.rgb * strength * emitter.smokeEmbers.y *
                    spark * (1.0 - life) * 2.4
                : vec3(0.0);
        }
    }
    return vec4(emission, clamp(transmittance, 0.0, 1.0));
}

vec4 integrateFireEmitters(vec3 rayOrigin, vec3 rayDirection,
                           float maximumDistance, bool reflectionPath)
{
    vec3 accumulated = vec3(0.0);
    float transmittance = 1.0;
    for (uint index = 0u; index < kRtActiveFireEmitterCapacity; ++index)
    {
        RtFireEmitterGpu emitter = rtFireEmitters.values[index];
        if (emitter.identity.x == 0u) continue;
        vec4 sampleValue = integrateOneFireEmitter(
            emitter, rayOrigin, rayDirection, maximumDistance, reflectionPath);
        accumulated += transmittance * sampleValue.rgb;
        transmittance *= sampleValue.a;
    }
    return vec4(accumulated, transmittance);
}
