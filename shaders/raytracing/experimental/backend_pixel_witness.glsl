// Investigation-only output witness. No extra rays, counters or CPU/GPU ABI.
// Five coordinate rows cover the six retained outliers (blue/red share a pixel).
vec3 investigationDirect;
vec4 investigationLightTerms;
vec3 investigationBounceDirection;
HitInfo investigationBounceHit;
vec3 investigationBounce;
vec3 investigationAfterBounce;
float investigationFog;
int backendWitnessRow()
{
    uvec2 pixel = HORDE_RT_PIXEL_ID.xy;
    if (pixel == uvec2(556u, 378u)) return 0;
    if (pixel == uvec2(396u, 262u)) return 1;
    if (pixel == uvec2(552u, 395u)) return 2;
    if (pixel == uvec2(770u, 526u)) return 3;
    if (pixel == uvec2(564u, 393u)) return 4;
    return -1;
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

void backendWitnessVector(int row, int field, vec3 value)
{
    for (int axis = 0; axis < 3; ++axis)
        backendWitnessFloat(row, field + axis, value[axis]);
}

void backendWitness(HitInfo primary, vec3 direction, vec3 origin,
    vec3 surfaceColor, vec4 fire, vec3 afterFire, vec3 afterMist,
    vec3 displayColor)
{
    int row = backendWitnessRow();
    if (row < 0) return;
    backendWitnessFloat(row, 0, 12345.0);
    backendWitnessFloat(row, 1, float(HORDE_RT_PIXEL_ID.x));
    backendWitnessFloat(row, 2, float(HORDE_RT_PIXEL_ID.y));
    backendWitnessFloat(row, 3, primary.hit ? 1.0 : 0.0);
    backendWitnessFloat(row, 4, float(primary.instance));
    backendWitnessFloat(row, 5, float(primary.primitive));
    backendWitnessFloat(row, 6, float(primary.material));
    backendWitnessFloat(row, 7, primary.t);
    backendWitnessVector(row, 8, primary.geometricNormal);
    backendWitnessVector(row, 11, primary.normal);
    backendWitnessVector(row, 14, primary.base);
    backendWitnessFloat(row, 17, primary.roughness);
    backendWitnessFloat(row, 18, primary.reflectivity);
    backendWitnessFloat(row, 19, primary.metallic);
    backendWitnessFloat(row, 20, primary.emissive);
    backendWitnessFloat(row, 21, primary.transmission);
    backendWitnessFloat(row, 22, float(primary.materialFlags));
    backendWitnessVector(row, 23, direction);
    backendWitnessVector(row, 26, primary.position);
    backendWitnessVector(row, 29, surfaceColor);
    backendWitnessVector(row, 32, fire.rgb);
    backendWitnessFloat(row, 35, fire.a);
    backendWitnessVector(row, 36, afterFire);
    backendWitnessVector(row, 39, afterMist);
    backendWitnessVector(row, 42, displayColor);
    backendWitnessVector(row, 45, origin);
    backendWitnessVector(row, 48, investigationDirect);
    backendWitnessFloat(row, 51, investigationLightTerms.x);
    backendWitnessFloat(row, 52, investigationLightTerms.y);
    backendWitnessFloat(row, 53, investigationLightTerms.z);
    backendWitnessFloat(row, 54, investigationLightTerms.w);
    backendWitnessVector(row, 55, investigationBounceDirection);
    backendWitnessFloat(row, 58, investigationBounceHit.hit ? 1.0 : 0.0);
    backendWitnessFloat(row, 59, float(investigationBounceHit.instance));
    backendWitnessFloat(row, 60, float(investigationBounceHit.primitive));
    backendWitnessFloat(row, 61, float(investigationBounceHit.material));
    backendWitnessFloat(row, 62, investigationBounceHit.t);
    backendWitnessVector(row, 63, investigationBounceHit.position);
    backendWitnessVector(row, 66, investigationBounceHit.normal);
    backendWitnessVector(row, 69, investigationBounceHit.base);
    backendWitnessFloat(row, 72, investigationBounceHit.roughness);
    backendWitnessFloat(row, 73, investigationBounceHit.reflectivity);
    backendWitnessFloat(row, 74, investigationBounceHit.metallic);
    backendWitnessFloat(row, 75, investigationBounceHit.emissive);
    backendWitnessFloat(row, 76, investigationBounceHit.transmission);
    backendWitnessFloat(row, 77, float(investigationBounceHit.materialFlags));
    backendWitnessVector(row, 78, investigationBounce);
    backendWitnessVector(row, 81, investigationAfterBounce);
    backendWitnessFloat(row, 84, investigationFog);
}
