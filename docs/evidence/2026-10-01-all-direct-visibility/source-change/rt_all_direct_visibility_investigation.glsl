// Investigation-only control: remove all direct-light visibility work from
// the Shipping/Mobile shader pair without changing primary or secondary rays.
// Variant generators define both selectors; macro-absent shaders, Diagnostic,
// and High-quality variants remain on their original physical shadow paths.
#if defined(HORDE_RT_VARIANT_INSTRUMENTATION) && \
    defined(HORDE_RT_VARIANT_QUALITY) && \
    defined(HORDE_RT_INSTRUMENTATION_SHIPPING) && \
    defined(HORDE_RT_QUALITY_MOBILE) && \
    HORDE_RT_VARIANT_INSTRUMENTATION == HORDE_RT_INSTRUMENTATION_SHIPPING && \
    HORDE_RT_VARIANT_QUALITY == HORDE_RT_QUALITY_MOBILE
#define HORDE_INVESTIGATION_ALL_DIRECT_VISIBILITY_ISOLATE 1
#else
#define HORDE_INVESTIGATION_ALL_DIRECT_VISIBILITY_ISOLATE 0
#endif
