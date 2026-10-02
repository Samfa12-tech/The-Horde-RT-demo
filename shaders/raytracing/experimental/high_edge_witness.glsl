// Isolated High/Diagnostic/Generic witness. Reuses the existing RGBA8 bit
// writer; observes only actual calls at (456,304), with no extra rays/samples.
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
}
