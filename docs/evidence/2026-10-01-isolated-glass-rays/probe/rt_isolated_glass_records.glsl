#include "rt_isolated_glass_targets.generated.glsl"
#if HORDE_LOCAL_ISOLATED_GLASS_MARKER
int isolatedTargetRow()
{
    for (int i = 0; i < kIsolatedTargetCount; ++i)
        if (HORDE_RT_PIXEL_ID.xy == kIsolatedTargets[i]) return i;
    return -1;
}

void isolatedRecordInterface(int slot, HitInfo hit, vec3 direction,
    bool open, bool certified, int tirCount, float totalDistance, vec3 throughput)
{
    int row = isolatedTargetRow();
    if (row < 0) return;
    const int fieldsPerSlot = 59;
    float fields[59] = float[59](
        hit.position.x, hit.position.y, hit.position.z,
        hit.geometricNormal.x, hit.geometricNormal.y, hit.geometricNormal.z,
        direction.x, direction.y, direction.z, hit.t, float(hit.instance),
        float(hit.material), float(hit.primitive), open ? 1.0 : 0.0,
        hit.hit ? 1.0 : 0.0, float(tirCount), totalDistance, hit.roughness,
        hit.ior, hit.attenuationDistance, throughput.x, throughput.y, throughput.z,
        float(hit.materialFlags), hit.transmission,
        hit.dielectricSpawnPosition.x, hit.dielectricSpawnPosition.y, hit.dielectricSpawnPosition.z,
        hit.dielectricSpawnGuarded ? 1.0 : 0.0, hit.dielectricSpawnMinimumNormalBias,
        hit.investigationQueryOrigin.x, hit.investigationQueryOrigin.y, hit.investigationQueryOrigin.z,
        hit.investigationQueryDirection.x, hit.investigationQueryDirection.y, hit.investigationQueryDirection.z,
        hit.investigationObjectOrigin.x, hit.investigationObjectOrigin.y, hit.investigationObjectOrigin.z,
        hit.investigationObjectDirection.x, hit.investigationObjectDirection.y, hit.investigationObjectDirection.z,
        hit.investigationBarycentrics.x, hit.investigationBarycentrics.y,
        hit.investigationQueryMinimum, hit.investigationRawDistance, float(hit.investigationGeometryIndex),
        hit.investigationVertex0.x, hit.investigationVertex0.y, hit.investigationVertex0.z,
        hit.investigationVertex1.x, hit.investigationVertex1.y, hit.investigationVertex1.z,
        hit.investigationVertex2.x, hit.investigationVertex2.y, hit.investigationVertex2.z,
        certified ? 1.0 : 0.0, dot(direction, hit.geometricNormal), float(slot));
    for (int i = 0; i < fieldsPerSlot; ++i)
    {
        uint bits = floatBitsToUint(fields[i]);
        vec3 low = vec3(uvec3(bits & 255u, (bits >> 8) & 255u, (bits >> 16) & 255u)) / 255.0;
        vec3 high = vec3(float(bits >> 24) / 255.0, 0.0, 0.0);
        if (controls.outputRedBlueSwap > 0.5) { low = low.bgr; high = high.bgr; }
        imageStore(outputImage, ivec2(slot * fieldsPerSlot * 2 + i * 2, row), vec4(low, 1.0));
        imageStore(outputImage, ivec2(slot * fieldsPerSlot * 2 + i * 2 + 1, row), vec4(high, 1.0));
    }
}
#endif
