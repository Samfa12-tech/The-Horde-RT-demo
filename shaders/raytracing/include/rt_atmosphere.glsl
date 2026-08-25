float raySegmentGlow(vec3 rayOrigin, vec3 rayDirection, vec3 segmentStart, vec3 segmentEnd,
                     float radius, float sceneDepth)
{
    vec3 segment = segmentEnd - segmentStart;
    float segmentLengthSquared = max(dot(segment, segment), 0.00001);
    vec3 fromStart = rayOrigin - segmentStart;
    float raySegmentDot = dot(rayDirection, segment);
    float rayOriginDot = dot(rayDirection, fromStart);
    float segmentOriginDot = dot(segment, fromStart);
    float denominator = max(segmentLengthSquared - raySegmentDot * raySegmentDot, 0.00001);
    float segmentT = clamp((segmentOriginDot - raySegmentDot * rayOriginDot) / denominator, 0.0, 1.0);
    float rayT = max(dot(segmentStart + segment * segmentT - rayOrigin, rayDirection), 0.0);
    segmentT = clamp(dot(rayOrigin + rayDirection * rayT - segmentStart, segment) / segmentLengthSquared, 0.0, 1.0);
    rayT = max(dot(segmentStart + segment * segmentT - rayOrigin, rayDirection), 0.0);
    float distanceToSegment = length((rayOrigin + rayDirection * rayT) - (segmentStart + segment * segmentT));
    float visible = step(0.001, rayT) * step(rayT, sceneDepth + 0.02);
    return (1.0 - smoothstep(radius * 0.18, radius, distanceToSegment)) * visible;
}

vec3 staffElectricity(vec3 rayOrigin, vec3 rayDirection, float sceneDepth)
{
    if (controls.enemyKind < 0.5 || controls.staffLightStrength < 0.05)
    {
        return vec3(0.0);
    }

    vec3 staff = vec3(controls.staffX, controls.staffY, controls.staffZ);
    vec3 target = vec3(controls.cameraX, 0.28, controls.cameraZ);
    vec3 attackDirection = normalize(target - staff);
    vec3 side = normalize(cross(attackDirection, vec3(0.0, 1.0, 0.0)));
    vec3 arcUp = normalize(cross(side, attackDirection));
    float phase = controls.time * 19.0;
    vec3 firstKink = mix(staff, target, 0.34)
        + side * sin(phase) * 0.11 + arcUp * cos(phase * 1.31) * 0.055;
    vec3 secondKink = mix(staff, target, 0.68)
        - side * cos(phase * 1.17) * 0.10 + arcUp * sin(phase * 1.53) * 0.050;
    vec3 forkEnd = staff + attackDirection * 0.34
        + side * (0.16 + sin(phase * 1.7) * 0.035) + arcUp * 0.10;

    float bolt = raySegmentGlow(rayOrigin, rayDirection, staff, firstKink, 0.036, sceneDepth)
        + raySegmentGlow(rayOrigin, rayDirection, firstKink, secondKink, 0.034, sceneDepth)
        + raySegmentGlow(rayOrigin, rayDirection, secondKink, target, 0.032, sceneDepth)
        + raySegmentGlow(rayOrigin, rayDirection, staff, forkEnd, 0.026, sceneDepth);

    vec3 toStaff = staff - rayOrigin;
    float staffDepth = max(dot(toStaff, rayDirection), 0.0);
    float staffDistance = length(rayOrigin + rayDirection * staffDepth - staff);
    float coronaVisible = step(0.001, staffDepth) * step(staffDepth, sceneDepth + 0.18);
    float corona = (1.0 - smoothstep(0.035, 0.24, staffDistance)) * coronaVisible;
    float charge = clamp(controls.staffLightStrength / 2.2, 0.0, 1.0);
    float flicker = 0.88 + 0.12 * sin(phase * 1.91);
    return tunedLightColor(vec3(0.72, 0.10, 1.45), kLightStaff)
        * (bolt + corona * 0.72) * (0.42 + charge * 2.65) * flicker;
}

bool rayBoxInterval(vec3 origin, vec3 direction, vec3 boundsMin, vec3 boundsMax,
                    out float intervalStart, out float intervalEnd)
{
    // Avoid infinities for rays parallel to one of the room planes without
    // changing the sign of any non-zero direction component.
    vec3 safeDirection = sign(direction + vec3(0.0000001))
        * max(abs(direction), vec3(0.0001));
    vec3 first = (boundsMin - origin) / safeDirection;
    vec3 second = (boundsMax - origin) / safeDirection;
    vec3 nearPlane = min(first, second);
    vec3 farPlane = max(first, second);
    intervalStart = max(max(nearPlane.x, nearPlane.y), nearPlane.z);
    intervalEnd = min(min(farPlane.x, farPlane.y), farPlane.z);
    return intervalEnd > max(intervalStart, 0.0);
}

vec4 lichMistSample(vec3 p)
{
    // Broad, low-frequency world-space curls move slowly enough to read as a
    // coherent ground layer rather than screen-space violet noise.
    float curlA = sin(p.x * 1.32 + p.z * 0.86 + controls.time * 0.19
                    + sin(p.z * 0.71 - controls.time * 0.11) * 0.72);
    float curlB = sin(p.x * -0.63 + p.z * 1.57 - controls.time * 0.14
                    + sin(p.x * 0.84 + controls.time * 0.09) * 0.58);
    float flowNoise = clamp(0.54 + curlA * 0.23 + curlB * 0.19, 0.12, 1.0);

    float floorHeight = clamp(p.y + 0.95, 0.0, 1.15);
    float heightFalloff = exp(-floorHeight * 3.25)
        * (1.0 - smoothstep(0.82, 1.12, floorHeight));
    vec2 fromLich = p.xz - vec2(-32.20, -13.10);
    float ritualFocus = 1.0 - smoothstep(1.25, 3.05, length(fromLich));
    float dawnFade = 1.0 - 0.78 * smoothstep(0.12, 1.0, controls.finaleDawnReveal);
    float density = (0.080 + ritualFocus * 0.27) * heightFalloff
        * flowNoise * dawnFade;

    vec3 staff = vec3(controls.staffX, controls.staffY, controls.staffZ);
    float staffGlow = controls.staffLightStrength /
        (1.0 + dot(p - staff, p - staff) * 1.15);
    vec3 ambientScattering = tunedLightColor(vec3(0.45, 0.60, 0.80), kLightSkylight);
    vec3 ritualScattering = tunedLightColor(vec3(0.75, 0.16, 1.00), kLightStaff);
    vec3 incidentLight = ambientScattering
        + ritualScattering * clamp(staffGlow * 1.35, 0.0, 1.8);
    // RGB stores in-scattering per metre and A stores extinction per metre.
    return vec4(incidentLight * density * 0.82, density);
}

void integrateLichMistSample(vec4 sampleValue, float stepLength,
                             inout vec3 scattered, inout float transmittance)
{
    scattered += transmittance * sampleValue.rgb * stepLength;
    transmittance *= exp(-sampleValue.a * stepLength);
}

vec4 lichGroundMist(vec3 rayOrigin, vec3 rayDirection, float sceneDepth)
{
    if (controls.enemyKind < 0.5)
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    float marchStart = 0.0;
    float marchEnd = 0.0;
    // The volume is confined to the final room and to the lowest 1.15 m above
    // its floor. Intersecting this box first concentrates every deterministic
    // sample in actual mist instead of stepping sparsely through empty air.
    if (!rayBoxInterval(rayOrigin, rayDirection,
                        vec3(-36.65, -0.95, -18.15),
                        vec3(-30.65,  0.20, -12.25),
                        marchStart, marchEnd))
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }
    marchStart = max(marchStart, 0.0);
    marchEnd = min(marchEnd, sceneDepth);
    if (marchEnd <= marchStart + 0.01)
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    vec3 scattered = vec3(0.0);
    float transmittance = 1.0;
    if (controls.workloadPreset < 0.5)
    {
        const float sampleCount = 2.0;
        float stepLength = (marchEnd - marchStart) / sampleCount;
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 0.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 1.5)), stepLength, scattered, transmittance);
    }
    else if (controls.workloadPreset < 1.5)
    {
        const float sampleCount = 6.0;
        float stepLength = (marchEnd - marchStart) / sampleCount;
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 0.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 1.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 2.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 3.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 4.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 5.5)), stepLength, scattered, transmittance);
    }
    else
    {
        const float sampleCount = 8.0;
        float stepLength = (marchEnd - marchStart) / sampleCount;
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 0.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 1.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 2.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 3.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 4.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 5.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 6.5)), stepLength, scattered, transmittance);
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * 7.5)), stepLength, scattered, transmittance);
    }
    return vec4(scattered, transmittance);
}

vec3 toneMapAces(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 linearToSrgb(vec3 color)
{
    vec3 low = color * 12.92;
    vec3 high = 1.055 * pow(max(color, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
    return mix(low, high, step(vec3(0.0031308), color));
}
