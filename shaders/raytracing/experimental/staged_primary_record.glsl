// Investigation only. The generator inserts this after the normal HitInfo ABI.
// Lossless FP32 fields: no new transform inverse, normal or material decode.
struct StagedPrimaryRecord { uvec4 lanes[8]; };
#if HORDE_RT_STAGED_PRIMARY_PASS == 1
#define STAGED_ACCESS writeonly
#else
#define STAGED_ACCESS readonly
#endif
layout(std430, set = 1, binding = 0) STAGED_ACCESS buffer StagedPage0 { StagedPrimaryRecord records[]; } staged0;
layout(std430, set = 1, binding = 1) STAGED_ACCESS buffer StagedPage1 { StagedPrimaryRecord records[]; } staged1;
layout(std430, set = 1, binding = 2) STAGED_ACCESS buffer StagedPage2 { StagedPrimaryRecord records[]; } staged2;

uint stagedPageCapacity()
{
    uint pixels = HORDE_RT_PIXEL_EXTENT.x * HORDE_RT_PIXEL_EXTENT.y;
    return pixels / 3u + (pixels % 3u != 0u ? 1u : 0u);
}
uint stagedPixelIndex()
{
    return HORDE_RT_PIXEL_ID.y * HORDE_RT_PIXEL_EXTENT.x + HORDE_RT_PIXEL_ID.x;
}

#if HORDE_RT_STAGED_PRIMARY_PASS == 1
void storeStagedPrimary(HitInfo h)
{
    uint instanceAndGuard = uint(h.instance);
    vec3 spawn = vec3(0.0);
    float bias = 0.0;
#if HORDE_GENERIC_TRANSMISSION_VARIANT
    spawn = h.dielectricSpawnPosition;
    bias = h.dielectricSpawnMinimumNormalBias;
    if (h.dielectricSpawnGuarded) instanceAndGuard |= 0x80000000u;
#endif
    StagedPrimaryRecord r;
    r.lanes[0] = uvec4(floatBitsToUint(h.t), uint(h.primitive), instanceAndGuard, uint(h.material));
    r.lanes[1] = uvec4(floatBitsToUint(h.position), floatBitsToUint(h.normal.x));
    r.lanes[2] = uvec4(floatBitsToUint(h.normal.yz), floatBitsToUint(h.geometricNormal.xy));
    r.lanes[3] = uvec4(floatBitsToUint(h.geometricNormal.z), floatBitsToUint(h.base));
    r.lanes[4] = floatBitsToUint(vec4(h.metallic, h.reflectivity, h.roughness, h.emissive));
    r.lanes[5] = floatBitsToUint(vec4(h.transmission, h.ior, h.thickness, h.attenuationDistance));
    r.lanes[6] = uvec4(floatBitsToUint(h.attenuationColor), h.materialFlags);
    r.lanes[7] = uvec4(floatBitsToUint(spawn), floatBitsToUint(bias));
    uint capacity = stagedPageCapacity();
    uint pixel = stagedPixelIndex();
    uint page = pixel / capacity;
    uint index = pixel % capacity;
    if (page == 0u) staged0.records[index] = r;
    else if (page == 1u) staged1.records[index] = r;
    else staged2.records[index] = r;
}
#else
HitInfo loadStagedPrimary()
{
    uint capacity = stagedPageCapacity();
    uint pixel = stagedPixelIndex();
    uint page = pixel / capacity;
    uint index = pixel % capacity;
    StagedPrimaryRecord r;
    if (page == 0u) r = staged0.records[index];
    else if (page == 1u) r = staged1.records[index];
    else r = staged2.records[index];
    HitInfo h;
    h.primitive = int(r.lanes[0].y);
    h.hit = h.primitive >= 0;
    h.t = uintBitsToFloat(r.lanes[0].x);
    h.instance = int(r.lanes[0].z & 0x00ffffffu);
    h.material = int(r.lanes[0].w);
    h.position = uintBitsToFloat(r.lanes[1].xyz);
    h.normal = uintBitsToFloat(uvec3(r.lanes[1].w, r.lanes[2].xy));
    h.geometricNormal = uintBitsToFloat(uvec3(r.lanes[2].zw, r.lanes[3].x));
    h.base = uintBitsToFloat(r.lanes[3].yzw);
    h.metallic = uintBitsToFloat(r.lanes[4].x);
    h.reflectivity = uintBitsToFloat(r.lanes[4].y);
    h.roughness = uintBitsToFloat(r.lanes[4].z);
    h.emissive = uintBitsToFloat(r.lanes[4].w);
    h.transmission = uintBitsToFloat(r.lanes[5].x);
    h.ior = uintBitsToFloat(r.lanes[5].y);
    h.thickness = uintBitsToFloat(r.lanes[5].z);
    h.attenuationDistance = uintBitsToFloat(r.lanes[5].w);
    h.attenuationColor = uintBitsToFloat(r.lanes[6].xyz);
    h.materialFlags = r.lanes[6].w;
#if HORDE_GENERIC_TRANSMISSION_VARIANT
    h.dielectricSpawnPosition = uintBitsToFloat(r.lanes[7].xyz);
    h.dielectricSpawnMinimumNormalBias = uintBitsToFloat(r.lanes[7].w);
    h.dielectricSpawnGuarded = (r.lanes[0].z & 0x80000000u) != 0u;
#endif
    return h;
}
#endif
