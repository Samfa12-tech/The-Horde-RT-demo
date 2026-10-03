// Temporary native-output marker only. Never admit this file to production.
// The transport branches, fixed budgets and all existing counters remain intact.
#if defined(HORDE_RT_VARIANT_INSTRUMENTATION) && \
    HORDE_RT_VARIANT_INSTRUMENTATION == HORDE_RT_INSTRUMENTATION_DIAGNOSTIC && \
    HORDE_RT_VARIANT_QUALITY == HORDE_RT_QUALITY_MOBILE && \
    HORDE_RT_VARIANT_MATERIAL == HORDE_RT_MATERIAL_GENERIC_DIELECTRIC
#define HORDE_LOCAL_ISOLATED_GLASS_MARKER 1
vec3 isolatedGlassMarker = vec3(0.0);
#endif
