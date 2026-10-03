// Investigation-only policy for the opaque-primary secondary-bounce profile.
// This intentionally removes physical reflected radiance and must never ship.
// The selector is deliberately exact: all Diagnostic and High variants
// retain the production secondary-bounce path unchanged. Both material
// strategies are included so opaque receivers share this nonphysical probe.
#if defined(HORDE_RT_VARIANT_INSTRUMENTATION) && \
    defined(HORDE_RT_VARIANT_QUALITY) && \
    HORDE_RT_VARIANT_INSTRUMENTATION == 0 && \
    HORDE_RT_VARIANT_QUALITY == 0
#define HORDE_RT_OMIT_OPAQUE_PRIMARY_SECONDARY_INVESTIGATION 1
#else
#define HORDE_RT_OMIT_OPAQUE_PRIMARY_SECONDARY_INVESTIGATION 0
#endif
