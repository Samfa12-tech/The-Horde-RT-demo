// Investigation-only, bounded primary-hit sampler. The generator inserts this
// after RtDielectricDiagnosticsBuffer without changing the production ABI.
const uint kPrimaryHitProbeMagic = 0x48505231u;
const uint kPrimaryHitProbeRecordWords = 96u;
const uint kPrimaryHitProbeRecordCount = 75u;
const uint kPrimaryHitProbeExtentX = 1080u;
const uint kPrimaryHitProbeExtentY = 2235u;

bool primaryHitProbeSlot(out uint slot)
{
    slot = 0u;
    if (any(notEqual(HORDE_RT_PIXEL_EXTENT.xy, uvec2(kPrimaryHitProbeExtentX, kPrimaryHitProbeExtentY))))
        return false;

    uvec2 pixel = HORDE_RT_PIXEL_ID.xy;
    const uvec2 centers[3] = uvec2[3](uvec2(875u, 1857u), uvec2(216u, 1955u), uvec2(194u, 1085u));
    for (uint patchIndex = 0u; patchIndex < 3u; ++patchIndex)
    {
        uvec2 first = centers[patchIndex] - uvec2(2u);
        uvec2 last = centers[patchIndex] + uvec2(2u);
        if (all(greaterThanEqual(pixel, first)) && all(lessThanEqual(pixel, last)))
        {
            slot = patchIndex * 25u + (pixel.y - first.y) * 5u + (pixel.x - first.x);
            return slot < kPrimaryHitProbeRecordCount;
        }
    }
    return false;
}

void primaryHitProbeStore(uint phase, HitInfo h, vec3 origin, vec3 direction, uint mask)
{
    uint slot;
    if (phase > 1u || !primaryHitProbeSlot(slot)) return;

    uint base = (phase * kPrimaryHitProbeRecordCount + slot) * kPrimaryHitProbeRecordWords;
    for (uint word = 0u; word < kPrimaryHitProbeRecordWords; ++word)
        rtDielectricDiagnostics.probeWords[base + word] = 0u;

    uvec2 pixel = HORDE_RT_PIXEL_ID.xy;
    rtDielectricDiagnostics.probeWords[base + 0u] = kPrimaryHitProbeMagic;
    rtDielectricDiagnostics.probeWords[base + 1u] = phase;
    rtDielectricDiagnostics.probeWords[base + 2u] = pixel.x;
    rtDielectricDiagnostics.probeWords[base + 3u] = pixel.y;
    rtDielectricDiagnostics.probeWords[base + 4u] = kPrimaryHitProbeExtentX;
    rtDielectricDiagnostics.probeWords[base + 5u] = kPrimaryHitProbeExtentY;
    rtDielectricDiagnostics.probeWords[base + 6u] = mask;
    rtDielectricDiagnostics.probeWords[base + 7u] = h.hit ? 1u : 0u;
    rtDielectricDiagnostics.probeWords[base + 8u] = floatBitsToUint(origin.x);
    rtDielectricDiagnostics.probeWords[base + 9u] = floatBitsToUint(origin.y);
    rtDielectricDiagnostics.probeWords[base + 10u] = floatBitsToUint(origin.z);
    rtDielectricDiagnostics.probeWords[base + 12u] = floatBitsToUint(direction.x);
    rtDielectricDiagnostics.probeWords[base + 13u] = floatBitsToUint(direction.y);
    rtDielectricDiagnostics.probeWords[base + 14u] = floatBitsToUint(direction.z);

    // Keep the exact eight-lane/32-word staged record encoding.
    uvec4 lanes[8];
    vec3 spawn = vec3(0.0);
    float bias = 0.0;
    uint instanceAndGuard = uint(h.instance);
#if HORDE_GENERIC_TRANSMISSION_VARIANT
    spawn = h.dielectricSpawnPosition;
    bias = h.dielectricSpawnMinimumNormalBias;
    if (h.dielectricSpawnGuarded) instanceAndGuard |= 0x80000000u;
#endif
    lanes[0] = uvec4(floatBitsToUint(h.t), uint(h.primitive), instanceAndGuard, uint(h.material));
    lanes[1] = uvec4(floatBitsToUint(h.position), floatBitsToUint(h.normal.x));
    lanes[2] = uvec4(floatBitsToUint(h.normal.yz), floatBitsToUint(h.geometricNormal.xy));
    lanes[3] = uvec4(floatBitsToUint(h.geometricNormal.z), floatBitsToUint(h.base));
    lanes[4] = floatBitsToUint(vec4(h.metallic, h.reflectivity, h.roughness, h.emissive));
    lanes[5] = floatBitsToUint(vec4(h.transmission, h.ior, h.thickness, h.attenuationDistance));
    lanes[6] = uvec4(floatBitsToUint(h.attenuationColor), h.materialFlags);
    lanes[7] = uvec4(floatBitsToUint(spawn), floatBitsToUint(bias));
    for (uint lane = 0u; lane < 8u; ++lane)
    {
        rtDielectricDiagnostics.probeWords[base + 16u + lane * 4u + 0u] = lanes[lane].x;
        rtDielectricDiagnostics.probeWords[base + 16u + lane * 4u + 1u] = lanes[lane].y;
        rtDielectricDiagnostics.probeWords[base + 16u + lane * 4u + 2u] = lanes[lane].z;
        rtDielectricDiagnostics.probeWords[base + 16u + lane * 4u + 3u] = lanes[lane].w;
    }

    float pushValues[32] = float[32](
        controls.yaw, controls.pitch, controls.torchLight, controls.time,
        controls.cameraX, controls.cameraZ, controls.walkAmount, controls.outputRedBlueSwap,
        controls.outputExposure, controls.damageFlash, controls.enemyKind, controls.staffLightStrength,
        controls.staffX, controls.staffY, controls.staffZ, controls.finaleSkylightOpen,
        controls.finaleDawnReveal, controls.heldPropDepth, controls.waterQuality, controls.waterfallWidthScale,
        controls.fogDensityScale, controls.torchHueDegrees, controls.torchIntensityScale, controls.skylightHueDegrees,
        controls.skylightIntensityScale, controls.passageHueDegrees, controls.passageIntensityScale,
        controls.staffHueDegrees, controls.staffIntensityScale, controls.workloadPreset,
        controls.genericTransmissionActive, controls.guidanceLightStrength);
    for (uint field = 0u; field < 32u; ++field)
        rtDielectricDiagnostics.probeWords[base + 48u + field] = floatBitsToUint(pushValues[field]);
}

void primaryHitProbeOutput(uint phase, vec4 color)
{
    uint slot;
    if (phase > 1u || !primaryHitProbeSlot(slot)) return;
    uint base = (phase * kPrimaryHitProbeRecordCount + slot) * kPrimaryHitProbeRecordWords;
    rtDielectricDiagnostics.probeWords[base + 80u] = floatBitsToUint(color.r);
    rtDielectricDiagnostics.probeWords[base + 81u] = floatBitsToUint(color.g);
    rtDielectricDiagnostics.probeWords[base + 82u] = floatBitsToUint(color.b);
    rtDielectricDiagnostics.probeWords[base + 83u] = floatBitsToUint(color.a);
}
