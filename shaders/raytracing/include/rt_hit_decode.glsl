vec3 normalForSurfaceCode(uint code)
{
    uint normalCode = (code >> 8u) & 0xffu;
    if (normalCode == 0u) return vec3(0.0, 1.0, 0.0);
    if (normalCode == 1u) return vec3(0.0, -1.0, 0.0);
    if (normalCode == 2u) return vec3(1.0, 0.0, 0.0);
    if (normalCode == 3u) return vec3(-1.0, 0.0, 0.0);
    if (normalCode == 4u) return vec3(0.0, 0.0, 1.0);
    if (normalCode == 5u) return vec3(0.0, 0.0, -1.0);
    return normalize(vec3(0.46, 0.42, 0.0));
}

const float kWaterStreamBottomY = -0.91;
const float kWaterStreamTopY = 2.16;
const float kWaterStreamBottomScale = 0.70;
const float kWaterStreamTaperSlope = 0.30 / (kWaterStreamTopY - kWaterStreamBottomY);

float waterStreamScale(float y)
{
    return kWaterStreamBottomScale
        + clamp(y - kWaterStreamBottomY, 0.0,
                kWaterStreamTopY - kWaterStreamBottomY)
        * kWaterStreamTaperSlope;
}

void waterStreamProfile(vec3 p, out float centreZ, out float radiusX,
                        out float radiusZ)
{
    float scale = waterStreamScale(p.y);
    float widthScale = controls.waterfallWidthScale;
    centreZ = -15.26;
    radiusX = 0.006;
    radiusZ = 0.065 * widthScale;
    float nearest = abs(p.z - centreZ) / (radiusZ * scale);
    float sideCentreZ = -15.26 + (-15.44 + 15.26) * widthScale;
    float candidate = abs(p.z - sideCentreZ) / (0.014 * widthScale * scale);
    if (candidate < nearest)
    {
        centreZ = sideCentreZ;
        radiusX = 0.003;
        radiusZ = 0.014 * widthScale;
        nearest = candidate;
    }
    sideCentreZ = -15.26 + (-15.06 + 15.26) * widthScale;
    candidate = abs(p.z - sideCentreZ) / (0.012 * widthScale * scale);
    if (candidate < nearest)
    {
        centreZ = sideCentreZ;
        radiusX = 0.003;
        radiusZ = 0.012 * widthScale;
    }
}

vec3 waterBaseNormal(vec3 p, vec3 geometricNormal)
{
    vec3 authoredNormal = normalize(geometricNormal);
    if (abs(authoredNormal.y) > 0.65)
    {
        return authoredNormal;
    }

    // Geometry and shading share a fixed centreline and common linear taper.
    // This implicit-gradient normal makes the eight-sided phone-safe mesh read
    // as a smooth falling sheet, including its slight acceleration taper.
    float centreZ = 0.0;
    float radiusX = 0.0;
    float radiusZ = 0.0;
    waterStreamProfile(p, centreZ, radiusX, radiusZ);
    float scale = waterStreamScale(p.y);
    vec3 radialGradient = vec3((p.x + 2.32) / (radiusX * radiusX),
                               -scale * kWaterStreamTaperSlope,
                               (p.z - centreZ) / (radiusZ * radiusZ));
    return normalize(radialGradient);
}

vec3 waterSurfaceNormal(vec3 p, vec3 geometricNormal)
{
    vec3 n = waterBaseNormal(p, geometricNormal);
    bool runoff = abs(n.y) > 0.65;
    bool catchment = runoff && p.x > -2.88;
    if (catchment)
    {
        // The roof stream transfers momentum into concentric capillary waves in
        // the real catchment geometry. These normals drive the same bounded RT
        // reflection/refraction paths; no screen-space ring is composited later.
        vec2 impactOffset = p.xz - vec2(-2.32, -15.26);
        float impactDistance = length(impactOffset);
        vec2 radial = impactOffset / max(impactDistance, 0.001);
        float angularWarp = sin(atan(impactOffset.y, impactOffset.x) * 3.0
                              + controls.time * 0.37) * 0.28;
        float ripple = sin(impactDistance * 31.0 - controls.time * 6.2
                           + angularWarp);
        float rippleEnvelope = 1.0 - smoothstep(0.07, 0.72, impactDistance);
        vec3 radialDirection = vec3(radial.x, 0.0, radial.y);
        return normalize(n + radialDirection * ripple * rippleEnvelope * 0.020
                           + vec3(0.006 * sin(p.z * 23.0 - controls.time * 3.7),
                                  0.0,
                                  0.004 * sin(p.x * 29.0 + controls.time * 4.1)));
    }
    vec3 flow = runoff ? vec3(-1.0, 0.0, 0.0) : vec3(0.0, -1.0, 0.0);
    vec3 across = normalize(cross(flow, n));
    float alongPosition = dot(p, flow);
    float acrossPosition = dot(p, across);
    float slopeA = sin(alongPosition * 3.6 - controls.time * 4.0
                     + sin(acrossPosition * 1.7) * 0.24);
    float slopeB = sin(alongPosition * 7.1 - controls.time * 6.8
                     + acrossPosition * 2.9);
    float amplitude = runoff ? 0.008 : 0.018;
    return normalize(n + flow * slopeA * amplitude
                       + across * slopeB * amplitude * 0.65);
}

bool waterStreamExit(vec3 entryPoint, vec3 insideDirection,
                     out vec3 exitPoint, out vec3 exitOutwardNormal,
                     out float waterDistance)
{
    float centreZ = 0.0;
    float radiusX = 0.0;
    float radiusZ = 0.0;
    waterStreamProfile(entryPoint, centreZ, radiusX, radiusZ);

    // The stream is the exact implicit surface
    //   dx^2/rx^2 + dz^2/rz^2 - scale(y)^2 = 0.
    // Since scale(y) is linear, substituting the ray produces a quadratic even
    // when the camera ray has a vertical component. The farther positive root
    // is the real back interface of the closed stream.
    vec3 insideStart = entryPoint + insideDirection * 0.0004;
    float localX = insideStart.x + 2.32;
    float localZ = insideStart.z - centreZ;
    float inverseRadiusXSquared = 1.0 / (radiusX * radiusX);
    float inverseRadiusZSquared = 1.0 / (radiusZ * radiusZ);
    float scale = waterStreamScale(insideStart.y);
    float scaleRate = kWaterStreamTaperSlope * insideDirection.y;
    float a = insideDirection.x * insideDirection.x * inverseRadiusXSquared
            + insideDirection.z * insideDirection.z * inverseRadiusZSquared
            - scaleRate * scaleRate;
    float b = 2.0 * (localX * insideDirection.x * inverseRadiusXSquared
                   + localZ * insideDirection.z * inverseRadiusZSquared
                   - scale * scaleRate);
    float c = localX * localX * inverseRadiusXSquared
            + localZ * localZ * inverseRadiusZSquared - scale * scale;
    float discriminant = b * b - 4.0 * a * c;
    if (a <= 0.000001 || discriminant < 0.0)
    {
        exitPoint = entryPoint;
        exitOutwardNormal = waterBaseNormal(entryPoint, vec3(1.0, 0.0, 0.0));
        waterDistance = 0.0;
        return false;
    }

    float root = sqrt(discriminant);
    float firstDistance = (-b - root) / (2.0 * a);
    float secondDistance = (-b + root) / (2.0 * a);
    float exitDistance = max(firstDistance, secondDistance);
    if (exitDistance <= 0.0002 || exitDistance > 0.20)
    {
        exitPoint = entryPoint;
        exitOutwardNormal = waterBaseNormal(entryPoint, vec3(1.0, 0.0, 0.0));
        waterDistance = 0.0;
        return false;
    }

    exitPoint = insideStart + insideDirection * exitDistance;
    float exitScale = waterStreamScale(exitPoint.y);
    exitOutwardNormal = normalize(vec3(
        (exitPoint.x + 2.32) * inverseRadiusXSquared,
        -exitScale * kWaterStreamTaperSlope,
        (exitPoint.z - centreZ) * inverseRadiusZSquared));
    waterDistance = exitDistance + 0.0004;
    return true;
}

void materialForPrimitive(int primitive,
                          int instance,
                          vec3 p,
                          out int material,
                          out vec3 normal,
                          out vec3 geometricNormal,
                          out vec3 base,
                          out float metallic,
                          out float reflectivity,
                          out float emissive)
{
    material = -1;
    if (instance == kWaterfallInstance)
    {
        material = kMaterialWater;
        geometricNormal = waterBaseNormal(p, vec3(1.0, 0.0, 0.0));
        normal = waterSurfaceNormal(p, geometricNormal);
        base = vec3(1.0);
        metallic = 0.0;
        reflectivity = 0.0204;
        emissive = 0.0;
        return;
    }
    if (instance == 1)
    {
        vec3 cameraForward = normalize(vec3(sin(controls.yaw), -0.05 + controls.pitch, -cos(controls.yaw)));
        vec3 cameraRight = normalize(cross(cameraForward, vec3(0.0, 1.0, 0.0)));
        vec3 cameraUp = normalize(cross(cameraRight, cameraForward));
        int boxFace = (primitive % 12) / 2;
        normal = boxFace == 0 ? -cameraForward :
                 (boxFace == 1 ? cameraForward :
                 (boxFace == 2 ? -cameraRight :
                 (boxFace == 3 ? cameraRight :
                 (boxFace == 4 ? cameraUp : -cameraUp))));
        geometricNormal = normal;
        metallic = 0.0;
        reflectivity = primitive < 12 ? 0.14 : 0.48;
        if (primitive < 12)
        {
            base = vec3(0.18, 0.075, 0.025);
            emissive = 0.0;
        }
        else if (primitive < 84)
        {
            base = vec3(0.12, 0.095, 0.07);
            metallic = 0.82;
            emissive = 0.0;
        }
        else
        {
            bool innerFlame = primitive >= 92;
            float flicker = sin(controls.time * 17.0 + float(primitive) * 1.7);
            base = tunedLightColor(
                innerFlame ? vec3(1.0, 0.56, 0.055) : vec3(1.0, 0.105, 0.008),
                kLightTorch);
            emissive = controls.torchLight * ((innerFlame ? 3.6 : 2.75) + 0.35 * flicker);
            if (controls.torchLight <= 0.001)
            {
                base = vec3(0.045, 0.025, 0.018);
            }
            normal = cameraForward;
            geometricNormal = normal;
            metallic = 0.0;
            reflectivity = 0.0;
        }
        return;
    }

    if (instance == 3)
    {
        vec3 cameraForward = normalize(vec3(sin(controls.yaw), -0.05 + controls.pitch, -cos(controls.yaw)));
        normal = cameraForward;
        geometricNormal = normal;
        bool leather = primitive < 2 || (primitive >= 7 && primitive < 9);
        base = leather ? vec3(0.16, 0.075, 0.028) : vec3(0.48, 0.52, 0.56);
        metallic = leather ? 0.12 : 0.92;
        reflectivity = leather ? 0.22 : 0.86;
        emissive = 0.0;
        return;
    }

    if (instance >= 4 && instance <= 16)
    {
        normal = normalize(vec3(controls.cameraX, 0.75, controls.cameraZ) - p);
        geometricNormal = normal;
        if (instance == 4)
        {
            bool brass = primitive >= 60 && primitive < 72;
            bool leather = (primitive >= 48 && primitive < 60) ||
                           (primitive >= 72 && primitive < 96) || primitive >= 120;
            base = brass ? vec3(0.52, 0.29, 0.065) :
                   (leather ? vec3(0.24, 0.090, 0.030) : vec3(0.18, 0.028, 0.034));
            metallic = brass ? 0.82 : (leather ? 0.08 : 0.0);
            reflectivity = brass ? 0.72 : (leather ? 0.20 : 0.08);
        }
        else
        {
            bool boots = instance == 14 || instance == 15;
            bool leather = instance == 6 || instance == 8 || instance == 9 ||
                           instance == 11 || instance == 13;
            bool head = instance == 16;
            base = boots ? vec3(0.028, 0.014, 0.011) :
                   (head ? vec3(0.16, 0.085, 0.055) :
                   (leather ? vec3(0.19, 0.064, 0.023) : vec3(0.14, 0.024, 0.030)));
            metallic = boots ? 0.02 : (leather ? 0.05 : 0.0);
            reflectivity = boots ? 0.13 : (leather ? 0.18 : 0.08);
        }
        emissive = 0.0;
        return;
    }

    if (instance == 17)
    {
        int face = (primitive % 12) / 2;
        normal = face == 0 ? vec3(0.0, 0.0, -1.0) :
                 (face == 1 ? vec3(0.0, 0.0, 1.0) :
                 (face == 2 ? vec3(-1.0, 0.0, 0.0) :
                 (face == 3 ? vec3(1.0, 0.0, 0.0) :
                 (face == 4 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, -1.0, 0.0)))));
        geometricNormal = normal;
        base = vec3(0.115, 0.105, 0.092);
        metallic = 0.0;
        reflectivity = 0.06;
        emissive = 0.0;
        return;
    }

    if (instance == 2 || instance == 18)
    {
        normal = vec3(0.0, 1.0, 0.0);
        geometricNormal = normal;
        // Keep the untextured proof readable under a moving torch. The previous
        // floor/hash lookup changed colour in hard 10 cm cells and looked like
        // checkerboard lighting across the animated mesh.
        float age = 0.5 + 0.5 * sin(p.y * 3.1 + p.x * 1.7 + p.z * 2.3);
        bool lichInstance = instance == 2 && controls.enemyKind > 0.5;
        base = lichInstance ? vec3(0.16, 0.13, 0.19)
                            : mix(vec3(0.34, 0.29, 0.21), vec3(0.48, 0.42, 0.30), age * 0.42);
        metallic = lichInstance ? 0.08 : 0.04;
        reflectivity = lichInstance ? 0.18 : 0.24;
        emissive = 0.0;
        return;
    }

    uint code = worldSurfaces.codes[primitive];
    uint normalCode = (code >> 8u) & 0xffu;
    material = int(code & 0xffu);
    normal = normalForSurfaceCode(code);
    geometricNormal = normal;
    base = vec3(0.16, 0.15, 0.135);
    metallic = 0.0;
    reflectivity = 0.07;
    emissive = 0.0;

    if (material >= kMaterialDryStone && material <= kMaterialAgedMetal)
    {
        float layer = float(material);
        float puddle = material == kMaterialWetCobble && normalCode == 0u ? puddleMask(p) : 0.0;
        vec2 uv;
        vec3 galleryBitangent = vec3(0.0);
        if (normalCode == 6u)
        {
            galleryBitangent = normalize(cross(geometricNormal, vec3(0.0, 0.0, 1.0)));
            uv = vec2(p.z, dot(p, galleryBitangent)) * 0.42;
        }
        else
        {
            uv = abs(normal.y) > 0.5 ? p.xz * 0.42 : (abs(normal.x) > 0.5 ? p.zy * 0.42 : p.xy * 0.42);
        }
        vec4 albedo = texture(materialDiffuse, vec3(uv, layer));
        vec3 arm = texture(materialArm, vec3(uv, layer)).rgb;
        if (puddle > 0.18)
        {
            albedo = mix(albedo, texture(materialDiffuse, vec3(uv * 0.72, float(kMaterialDampGround))), puddle * 0.65);
            arm.g = mix(arm.g, 0.08, puddle);
        }
        base = albedo.rgb * mix(0.62, 1.0, arm.r);
        metallic = arm.b;
        reflectivity = clamp(1.0 - arm.g, 0.05, 0.96);
        vec3 tangentNormal = texture(materialNormal, vec3(uv, layer)).xyz * 2.0 - 1.0;
        vec3 mapped;
        if (normalCode == 6u)
        {
            mapped = normalize(vec3(0.0, 0.0, 1.0) * tangentNormal.x
                + galleryBitangent * tangentNormal.y + geometricNormal * tangentNormal.z);
        }
        else
        {
            mapped = abs(normal.y) > 0.5 ? vec3(tangentNormal.x, tangentNormal.z * sign(normal.y), tangentNormal.y)
                : (abs(normal.x) > 0.5 ? vec3(tangentNormal.z * sign(normal.x), tangentNormal.y, tangentNormal.x)
                                           : vec3(tangentNormal.x, tangentNormal.y, tangentNormal.z * sign(normal.z)));
        }
        normal = normalize(mix(normal, mapped, 0.34));
    }
    else if (material == kMaterialFlame)
    {
        vec3 authoredFlame = p.x < -23.5 ? vec3(0.19, 0.95, 0.26) :
                              (p.x < -18.5 ? vec3(1.0, 0.055, 0.018) :
                              (p.x < -13.5 ? vec3(0.055, 0.24, 1.0) : vec3(1.0, 0.44, 0.055)));
        base = tunedLightColor(authoredFlame, kLightPassage);
        emissive = 2.2 + 0.45 * sin((p.y + p.x) * 35.0 + controls.time * 18.0);
    }
    else if (material == kMaterialMirror)
    {
        base = vec3(0.045, 0.032, 0.022);
        metallic = 0.95;
        reflectivity = 0.95;
    }
    else if (material == kMaterialDarkFigure)
    {
        base = vec3(0.012, 0.016, 0.014);
        reflectivity = 0.12;
        float eyeBand = 1.0 - smoothstep(0.0, 0.055, abs(p.y + 0.13));
        float eyePair = 1.0 - smoothstep(0.0, 0.11, abs(abs(fract(p.x * 3.7) - 0.5) - 0.19));
        base += vec3(0.08, 0.82, 0.21) * eyeBand * eyePair;
    }
    else if (material == kMaterialHiddenShell)
    {
        base = vec3(0.15, 0.14, 0.125);
        reflectivity = 0.06;
    }
    else if (material == kMaterialClearGlass)
    {
        base = vec3(0.32, 0.48, 0.52);
        reflectivity = 0.22;
    }
    else if (material == kMaterialWater)
    {
        base = vec3(1.0);
        geometricNormal = waterBaseNormal(p, geometricNormal);
        normal = waterSurfaceNormal(p, geometricNormal);
        metallic = 0.0;
        reflectivity = 0.0204;
    }
}

HitInfo traceScene(vec3 origin, vec3 direction, float maxDistance, uint mask,
                   bool ignoreWater)
{
    HitInfo h;
    h.hit = false;
    h.t = maxDistance;
    h.primitive = -1;
    h.instance = 0;
    h.material = -1;
    h.position = origin + direction * maxDistance;
    h.normal = vec3(0.0, 1.0, 0.0);
    h.geometricNormal = h.normal;
    h.base = vec3(0.0);
    h.metallic = 0.0;
    h.reflectivity = 0.0;
    h.emissive = 0.0;

    rayQueryEXT query;
    uint rayFlags = ignoreWater ? gl_RayFlagsNoOpaqueEXT : gl_RayFlagsOpaqueEXT;
    rayQueryInitializeEXT(query, topLevelAS, rayFlags, mask, origin, 0.002, direction, maxDistance);
    while (rayQueryProceedEXT(query))
    {
        if (!ignoreWater ||
            rayQueryGetIntersectionTypeEXT(query, false) != gl_RayQueryCandidateIntersectionTriangleEXT)
        {
            continue;
        }
        int candidateInstance = int(rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));
        int candidatePrimitive = rayQueryGetIntersectionPrimitiveIndexEXT(query, false);
        bool candidateIsWater = candidateInstance == kWaterfallInstance ||
            (candidateInstance == 0 &&
             int(worldSurfaces.codes[candidatePrimitive] & 0xffu) == kMaterialWater);
        if (!candidateIsWater)
        {
            rayQueryConfirmIntersectionEXT(query);
        }
    }

    if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        h.hit = true;
        h.t = rayQueryGetIntersectionTEXT(query, true);
        h.primitive = rayQueryGetIntersectionPrimitiveIndexEXT(query, true);
        h.instance = int(rayQueryGetIntersectionInstanceCustomIndexEXT(query, true));
        h.position = origin + direction * h.t;
        materialForPrimitive(h.primitive, h.instance, h.position, h.material, h.normal, h.geometricNormal, h.base, h.metallic, h.reflectivity, h.emissive);
        RtInstanceMetadata instanceMetadata = rtInstances.values[h.instance];
        if ((instanceMetadata.geometry.w & kRtInstanceFlagStaticPbr) != 0u)
        {
            uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(query, true);
            if (geometryIndex < instanceMetadata.geometry.y)
            {
                RtPrimitiveMetadata primitiveMetadata =
                    rtPrimitives.values[instanceMetadata.geometry.x + geometryIndex];
                uint triangleIndex = primitiveMetadata.geometry.y + uint(h.primitive) * 3u;
                uint i0 = primitiveMetadata.geometry.x + rtStaticIndices.values[triangleIndex];
                uint i1 = primitiveMetadata.geometry.x + rtStaticIndices.values[triangleIndex + 1u];
                uint i2 = primitiveMetadata.geometry.x + rtStaticIndices.values[triangleIndex + 2u];
                vec2 bary = rayQueryGetIntersectionBarycentricsEXT(query, true);
                vec3 weights = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);
                StaticRtVertex v0 = rtStaticVertices.values[i0];
                StaticRtVertex v1 = rtStaticVertices.values[i1];
                StaticRtVertex v2 = rtStaticVertices.values[i2];
                vec2 uv = v0.uv0.xy * weights.x + v1.uv0.xy * weights.y +
                          v2.uv0.xy * weights.z;
                mat3 objectToWorld = mat3(rayQueryGetIntersectionObjectToWorldEXT(query, true));
                mat3 normalToWorld = transpose(inverse(objectToWorld));
                vec3 localNormal = normalize(v0.normal.xyz * weights.x +
                                             v1.normal.xyz * weights.y +
                                             v2.normal.xyz * weights.z);
                vec4 localTangent = v0.tangent * weights.x +
                                    v1.tangent * weights.y +
                                    v2.tangent * weights.z;
                vec3 worldNormal = normalize(normalToWorld * localNormal);
                vec3 worldTangent = normalize(objectToWorld * localTangent.xyz);
                vec3 worldBitangent = normalize(cross(worldNormal, worldTangent)) *
                                      (localTangent.w < 0.0 ? -1.0 : 1.0);
                RtMaterialGpu staticMaterial =
                    rtMaterials.values[primitiveMetadata.geometry.w];
                vec3 sampledNormal = (staticMaterial.materialFlags.x &
                    kRtMaterialFlagNormalTexture) != 0u
                    ? texture(rtNormalTextures,
                        vec3(uv, float(staticMaterial.textureLayers.y))).xyz * 2.0 - 1.0
                    : vec3(0.0, 0.0, 1.0);
                h.normal = normalize(worldTangent * sampledNormal.x +
                                     worldBitangent * sampledNormal.y +
                                     worldNormal * sampledNormal.z);
                vec3 p0 = objectToWorld * v0.position.xyz;
                vec3 p1 = objectToWorld * v1.position.xyz;
                vec3 p2 = objectToWorld * v2.position.xyz;
                h.geometricNormal = normalize(cross(p1 - p0, p2 - p0));
                vec4 baseSample = (staticMaterial.materialFlags.x &
                    kRtMaterialFlagBaseColorTexture) != 0u
                    ? texture(rtBaseColorTextures,
                        vec3(uv, float(staticMaterial.textureLayers.x)))
                    : vec4(1.0);
                vec3 orm = (staticMaterial.materialFlags.x &
                    kRtMaterialFlagOrmTexture) != 0u
                    ? texture(rtOrmTextures,
                        vec3(uv, float(staticMaterial.textureLayers.z))).rgb
                    : vec3(1.0);
                vec3 emissiveSample = (staticMaterial.materialFlags.x &
                    kRtMaterialFlagEmissiveTexture) != 0u
                    ? texture(rtEmissiveTextures,
                        vec3(uv, float(staticMaterial.textureLayers.w))).rgb
                    : vec3(1.0);
                h.material = 100 + int(primitiveMetadata.geometry.w);
                h.base = baseSample.rgb * staticMaterial.baseColorFactor.rgb *
                         mix(1.0, orm.r, staticMaterial.metallicRoughnessOcclusionTransmission.z);
                h.metallic = clamp(orm.b *
                    staticMaterial.metallicRoughnessOcclusionTransmission.x, 0.0, 1.0);
                float roughness = clamp(orm.g *
                    staticMaterial.metallicRoughnessOcclusionTransmission.y, 0.04, 1.0);
                h.reflectivity = mix(0.04, 0.92, h.metallic) * (1.0 - roughness * 0.45);
                vec3 emission = emissiveSample * staticMaterial.emissiveFactorStrength.rgb *
                                staticMaterial.emissiveFactorStrength.w;
                h.emissive = max(emission.r, max(emission.g, emission.b));
                if (h.emissive > 0.0001) h.base = emission / h.emissive;
            }
        }
        if (h.instance == 2 || h.instance == 18)
        {
            const int firstVertex = h.primitive * 3;
            const vec2 bary = rayQueryGetIntersectionBarycentricsEXT(query, true);
            const float w0 = 1.0 - bary.x - bary.y;
            bool lichInstance = h.instance == 2 && controls.enemyKind > 0.5;
            bool secondSkeletonPose = h.instance == 18;
            vec3 localNormal = lichInstance
                ? normalize(lich.vertices[firstVertex].normal.xyz * w0
                    + lich.vertices[firstVertex + 1].normal.xyz * bary.x
                    + lich.vertices[firstVertex + 2].normal.xyz * bary.y)
                : (secondSkeletonPose
                    ? normalize(secondSkeleton.vertices[firstVertex].normal.xyz * w0
                        + secondSkeleton.vertices[firstVertex + 1].normal.xyz * bary.x
                        + secondSkeleton.vertices[firstVertex + 2].normal.xyz * bary.y)
                    : normalize(skeleton.vertices[firstVertex].normal.xyz * w0
                        + skeleton.vertices[firstVertex + 1].normal.xyz * bary.x
                        + skeleton.vertices[firstVertex + 2].normal.xyz * bary.y));
            const mat3 objectToWorld = mat3(rayQueryGetIntersectionObjectToWorldEXT(query, true));
            h.normal = normalize(objectToWorld * localNormal);
            vec3 edgeA = lichInstance
                ? lich.vertices[firstVertex + 1].position.xyz - lich.vertices[firstVertex].position.xyz
                : (secondSkeletonPose
                    ? secondSkeleton.vertices[firstVertex + 1].position.xyz - secondSkeleton.vertices[firstVertex].position.xyz
                    : skeleton.vertices[firstVertex + 1].position.xyz - skeleton.vertices[firstVertex].position.xyz);
            vec3 edgeB = lichInstance
                ? lich.vertices[firstVertex + 2].position.xyz - lich.vertices[firstVertex].position.xyz
                : (secondSkeletonPose
                    ? secondSkeleton.vertices[firstVertex + 2].position.xyz - secondSkeleton.vertices[firstVertex].position.xyz
                    : skeleton.vertices[firstVertex + 2].position.xyz - skeleton.vertices[firstVertex].position.xyz);
            vec3 faceNormal = cross(edgeA, edgeB);
            h.geometricNormal = length(faceNormal) > 0.00001
                ? normalize(objectToWorld * normalize(faceNormal))
                : h.normal;
            if (dot(h.geometricNormal, -direction) < 0.0)
            {
                h.geometricNormal = -h.geometricNormal;
            }
            if (lichInstance)
            {
                vec2 uv = lich.vertices[firstVertex].texcoord.xy * w0
                    + lich.vertices[firstVertex + 1].texcoord.xy * bary.x
                    + lich.vertices[firstVertex + 2].texcoord.xy * bary.y;
                vec3 albedo = texture(lichBaseColor, vec3(uv, 0.0)).rgb;
                vec3 glow = texture(lichEmissive, vec3(uv, 0.0)).rgb;
                float glowMask = smoothstep(0.025, 0.50, max(glow.r, max(glow.g, glow.b)));
                vec3 authoredStaffEmission = mix(albedo * 0.72, vec3(0.72, 0.12, 1.0), glowMask);
                h.base = glowMask > 0.0 ? tunedLightColor(authoredStaffEmission, kLightStaff)
                                        : albedo * 0.72;
                h.emissive = glowMask * (3.8 + controls.staffLightStrength * 2.3);
            }
        }
    }
    return h;
}
