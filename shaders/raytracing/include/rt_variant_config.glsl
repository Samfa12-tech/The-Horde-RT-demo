// Compile-time raygen variant contract. This file is injected only by the
// temporary matrix compiler until the shader-specialization task wires it into
// the normal source include graph.
#define HORDE_RT_INSTRUMENTATION_SHIPPING 0
#define HORDE_RT_INSTRUMENTATION_DIAGNOSTIC 1
#define HORDE_RT_QUALITY_MOBILE 0
#define HORDE_RT_QUALITY_HIGH 1
#define HORDE_RT_MATERIAL_OPAQUE_FAST 0
#define HORDE_RT_MATERIAL_GENERIC_DIELECTRIC 1

#ifndef HORDE_RT_VARIANT_INSTRUMENTATION
#error "raygen variant instrumentation was not selected"
#endif
#ifndef HORDE_RT_VARIANT_QUALITY
#error "raygen variant quality was not selected"
#endif
#ifndef HORDE_RT_VARIANT_MATERIAL
#error "raygen variant material route was not selected"
#endif

#if HORDE_RT_VARIANT_INSTRUMENTATION != HORDE_RT_INSTRUMENTATION_SHIPPING && \
    HORDE_RT_VARIANT_INSTRUMENTATION != HORDE_RT_INSTRUMENTATION_DIAGNOSTIC
#error "raygen variant instrumentation selection is invalid"
#endif

#if HORDE_RT_VARIANT_QUALITY != HORDE_RT_QUALITY_MOBILE && \
    HORDE_RT_VARIANT_QUALITY != HORDE_RT_QUALITY_HIGH
#error "raygen variant quality selection is invalid"
#endif

#if HORDE_RT_VARIANT_MATERIAL != HORDE_RT_MATERIAL_OPAQUE_FAST && \
    HORDE_RT_VARIANT_MATERIAL != HORDE_RT_MATERIAL_GENERIC_DIELECTRIC
#error "raygen variant material selection is invalid"
#endif

// Matrix shaders compile these bounded transport ceilings directly into the
// module. The compatibility shader remains macro-absent and retains its
// runtime WaterQuality policy below the variant boundary.
#if HORDE_RT_VARIANT_QUALITY == HORDE_RT_QUALITY_MOBILE
const int kRtVariantDielectricInterfaceBudget = 4;
const int kRtVariantDielectricVolumeBudget = 2;
const int kRtVariantShadowInterfaceBudget = 4;
const int kRtVariantShadowVolumeBudget = 2;
#else
const int kRtVariantDielectricInterfaceBudget = 8;
const int kRtVariantDielectricVolumeBudget = 4;
const int kRtVariantShadowInterfaceBudget = 8;
const int kRtVariantShadowVolumeBudget = 4;
#endif
