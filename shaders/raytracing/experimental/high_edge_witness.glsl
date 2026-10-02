// Isolated High/Diagnostic/Generic witness. Reuses the existing RGBA8 bit
// writer; observes actual calls at (456,304). The separately labelled contact
// probe adds ONE non-confirming hardware query at visit4, never transport work.
vec3 investigationPrimaryOrigin;
vec3 investigationReflectionDirection;
vec3 investigationReflectionOrigin;
HitInfo investigationReflectionHit;
vec3 investigationReflected;
vec3 investigationTransmitted;
vec3 investigationThroughput;
float investigationFirstFresnel;
vec4 investigationTermination;
vec4 investigationInterfaces[54]; // Nine existing interface-loop visits, 24 floats each.
int investigationInterfaceCount = 0;
bool investigationContactObserved = false;
vec4 investigationContact[13]; // Header/ray + nearest exit and opaque candidate.

int backendWitnessRow()
{
    return all(equal(HORDE_RT_PIXEL_ID.xy, uvec2(456u, 304u))) ? 0 : -1;
}

void backendWitnessFloat(int row, int field, float value)
{
    uint bits = floatBitsToUint(value);
    vec3 first = vec3(bits & 255u, (bits >> 8u) & 255u,
                      (bits >> 16u) & 255u) / 255.0;
    vec3 second = vec3((bits >> 24u) & 255u, 0u, 0u) / 255.0;
    if (controls.outputRedBlueSwap > 0.5)
    {
        first = first.bgr;
        second = second.bgr;
    }
    imageStore(outputImage, ivec2(field * 2, row), vec4(first, 1.0));
    imageStore(outputImage, ivec2(field * 2 + 1, row), vec4(second, 1.0));
}

void backendWitnessVector(int field, vec3 value)
{
    for (int axis = 0; axis < 3; ++axis)
        backendWitnessFloat(0, field + axis, value[axis]);
}

void backendWitnessInterface(HitInfo hit, bool volumeOpen, vec3 direction,
                           vec3 queryOrigin, float queryMinimum, float spawnEpsilon)
{
    if (backendWitnessRow() < 0) return;
    int index = investigationInterfaceCount++ * 6;
    investigationInterfaces[index] = vec4(hit.hit ? 1.0 : 0.0,
        float(hit.instance), float(hit.primitive), float(hit.material));
    investigationInterfaces[index + 1] = vec4(hit.t, volumeOpen ? 1.0 : 0.0,
        hit.roughness, hit.ior);
    investigationInterfaces[index + 2] = vec4(hit.position, hit.transmission);
    investigationInterfaces[index + 3] = vec4(hit.geometricNormal,
        dot(direction, hit.geometricNormal));
    investigationInterfaces[index + 4] = vec4(direction, spawnEpsilon);
    investigationInterfaces[index + 5] = vec4(queryOrigin, queryMinimum);
}

void backendWitnessContactCandidates(vec3 origin, vec3 direction, float minimum,
                                     uint volumeInstance, uint volumeMaterial)
{
    if (backendWitnessRow() < 0) return;
    investigationContactObserved = true;
    for (int index = 0; index < 13; ++index)
        investigationContact[index] = vec4(0.0);
    investigationContact[0].x = 1.0;
    investigationContact[1] = vec4(origin, minimum);
    investigationContact[2] = vec4(direction, 10000.0);
    float nearestExit = 10000.0;
    float nearestOpaque = 10000.0;
    rayQueryEXT query;
    rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsNoOpaqueEXT, 0x23u,
        origin, max(minimum, 0.0), direction, 10000.0);
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query, false) !=
            gl_RayQueryCandidateIntersectionTriangleEXT) continue;
        investigationContact[0].y += 1.0;
        int instance = int(rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));
        int primitive = rayQueryGetIntersectionPrimitiveIndexEXT(query, false);
        uint geometry = rayQueryGetIntersectionGeometryIndexEXT(query, false);
        float distance = rayQueryGetIntersectionTEXT(query, false);
        vec2 bary = rayQueryGetIntersectionBarycentricsEXT(query, false);
        bool front = rayQueryGetIntersectionFrontFaceEXT(query, false);
        vec3 position = origin + direction * distance;
        vec3 normal = vec3(0.0, 1.0, 0.0);
        int material = -1;
        uint flags = 0u;
        float transmission = 0.0;
        RtInstanceMetadata metadata = rtInstances.values[instance];
        if ((metadata.flags & kRtInstanceFlagStaticPbr) != 0u &&
            geometry < metadata.primitiveCount)
        {
            RtPrimitiveMetadata part = rtPrimitives.values[metadata.primitiveBase + geometry];
            uint triangle = part.indexOffset + uint(primitive) * 3u;
            StaticRtVertex v0, v1, v2;
            loadPbrTriangle(metadata,
                part.vertexOffset + rtStaticIndices.values[triangle],
                part.vertexOffset + rtStaticIndices.values[triangle + 1u],
                part.vertexOffset + rtStaticIndices.values[triangle + 2u], v0, v1, v2);
            mat4x3 transform = rayQueryGetIntersectionObjectToWorldEXT(query, false);
            mat3 linear = mat3(transform);
            vec3 p0 = linear * v0.position.xyz;
            vec3 p1 = linear * v1.position.xyz;
            vec3 p2 = linear * v2.position.xyz;
            normal = normalize(cross(p1 - p0, p2 - p0));
            precise vec3 localPosition = v0.position.xyz +
                (bary.x * (v1.position.xyz - v0.position.xyz) +
                 bary.y * (v2.position.xyz - v0.position.xyz));
            precise vec3 surfacePosition = linear * localPosition + transform[3];
            position = surfacePosition;
            RtMaterialGpu authored = rtMaterials.values[part.materialIndex];
            material = 100 + int(part.materialIndex);
            flags = authored.materialFlags.x;
            transmission = authored.metallicRoughnessOcclusionTransmission.w;
        }
        else
        {
            vec3 shadingNormal, base;
            float metallic, reflectivity, emissive;
            materialForPrimitive(primitive, instance, position, material,
                shadingNormal, normal, base, metallic, reflectivity, emissive);
        }
        bool matchedExit = uint(instance) == volumeInstance &&
            uint(material) == volumeMaterial && transmission > 0.0 &&
            (flags & kRtMaterialFlagThinWall) == 0u && dot(direction, normal) > 0.0;
        bool opaque = instance != kWaterfallInstance &&
            material != kMaterialWater && material != kMaterialClearGlass &&
            !((flags & kRtMaterialFlagTransmission) != 0u && transmission > 0.0);
        int slot = -1;
        if (matchedExit)
        {
            investigationContact[0].z += 1.0;
            if (distance < nearestExit) { nearestExit = distance; slot = 3; }
        }
        if (opaque)
        {
            investigationContact[0].w += 1.0;
            if (distance < nearestOpaque) { nearestOpaque = distance; slot = 8; }
        }
        if (slot >= 0)
        {
            investigationContact[slot] = vec4(1.0, float(instance), float(primitive), float(material));
            investigationContact[slot + 1] = vec4(distance, float(geometry), front ? 1.0 : 0.0, float(flags));
            investigationContact[slot + 2] = vec4(bary, transmission, dot(direction, normal));
            investigationContact[slot + 3] = vec4(position, 0.0);
            investigationContact[slot + 4] = vec4(normal, 0.0);
        }
        // Never confirm: commitment could shrink tmax and discard a tie.
        // This probe observes availability only; it admits no runtime contact.
    }
}

void backendWitness(HitInfo primary, vec3 direction, vec3 origin,
                    vec3 surfaceColor, vec3 displayColor)
{
    if (backendWitnessRow() < 0) return;
    backendWitnessFloat(0, 0, 12345.0);
    backendWitnessFloat(0, 1, 456.0);
    backendWitnessFloat(0, 2, 304.0);
    backendWitnessFloat(0, 3, primary.hit ? 1.0 : 0.0);
    backendWitnessFloat(0, 4, float(primary.instance));
    backendWitnessFloat(0, 5, float(primary.primitive));
    backendWitnessFloat(0, 6, float(primary.material));
    backendWitnessFloat(0, 7, primary.t);
    backendWitnessVector(8, primary.geometricNormal);
    backendWitnessVector(11, primary.normal);
    backendWitnessVector(14, primary.base);
    backendWitnessFloat(0, 17, primary.roughness);
    backendWitnessFloat(0, 18, primary.transmission);
    backendWitnessFloat(0, 19, float(primary.materialFlags));
    backendWitnessVector(20, direction);
    backendWitnessVector(23, origin);
    backendWitnessVector(26, primary.position);
    backendWitnessVector(29, surfaceColor);
    backendWitnessVector(32, displayColor);
    backendWitnessFloat(0, 35, investigationFirstFresnel);
    backendWitnessVector(36, investigationReflectionDirection);
    backendWitnessVector(39, investigationReflectionOrigin);
    backendWitnessFloat(0, 42, investigationReflectionHit.hit ? 1.0 : 0.0);
    backendWitnessFloat(0, 43, float(investigationReflectionHit.instance));
    backendWitnessFloat(0, 44, float(investigationReflectionHit.primitive));
    backendWitnessFloat(0, 45, float(investigationReflectionHit.material));
    backendWitnessFloat(0, 46, investigationReflectionHit.t);
    backendWitnessVector(47, investigationReflectionHit.position);
    backendWitnessVector(50, investigationReflected);
    backendWitnessFloat(0, 53, float(investigationInterfaceCount));
    backendWitnessVector(54, investigationTransmitted);
    backendWitnessVector(57, investigationThroughput);
    for (int axis = 0; axis < 4; ++axis)
        backendWitnessFloat(0, 60 + axis, investigationTermination[axis]);
    for (int visit = 0; visit < investigationInterfaceCount * 6; ++visit)
        for (int axis = 0; axis < 4; ++axis)
            backendWitnessFloat(0, 64 + visit * 4 + axis,
                investigationInterfaces[visit][axis]);
    backendWitnessFloat(0, 280, investigationContactObserved ? 1.0 : 0.0);
    if (investigationContactObserved)
        for (int index = 0; index < 13; ++index)
            for (int axis = 0; axis < 4; ++axis)
                backendWitnessFloat(0, 280 + index * 4 + axis,
                    investigationContact[index][axis]);
}
