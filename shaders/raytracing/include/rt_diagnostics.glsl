#ifndef HORDE_RT_DIAGNOSTICS_GLSL
#define HORDE_RT_DIAGNOSTICS_GLSL

// The macro-absent compatibility shaders remain Diagnostic so the existing
// embedded runtime modules retain their byte-verified behavior. Shipping
// matrix modules deliberately erase both the SSBO access and their arguments.
#if !defined(HORDE_RT_VARIANT_INSTRUMENTATION) || \
    HORDE_RT_VARIANT_INSTRUMENTATION == HORDE_RT_INSTRUMENTATION_DIAGNOSTIC
#define RT_DIAG_ADD(fieldName, deltaValue) atomicAdd(rtDielectricDiagnostics.value.fieldName, deltaValue)
#define RT_DIAG_OR(fieldName, maskValue) atomicOr(rtDielectricDiagnostics.value.fieldName, maskValue)
#else
#define RT_DIAG_ADD(fieldName, deltaValue)
#define RT_DIAG_OR(fieldName, maskValue)
#endif

#endif
