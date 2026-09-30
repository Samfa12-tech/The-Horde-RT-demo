#ifndef HORDE_RT_PRIMARY_HIT_REFERENCE_GLSL
#define HORDE_RT_PRIMARY_HIT_REFERENCE_GLSL

#if !defined(HORDE_RT_VARIANT_INSTRUMENTATION) || \
    !defined(HORDE_RT_VARIANT_QUALITY) || \
    HORDE_RT_VARIANT_INSTRUMENTATION != HORDE_RT_INSTRUMENTATION_SHIPPING || \
    HORDE_RT_VARIANT_QUALITY != HORDE_RT_QUALITY_MOBILE
#error "Primary-hit reference is restricted to Shipping/Mobile variants."
#endif

vec3 primaryHitReferenceColor(HitInfo primary, vec3 rayDirection)
{
    return primary.hit
        ? primary.base * (vec3(0.5) + 0.5 * abs(primary.normal))
        : skyColor(rayDirection);
}

#endif
