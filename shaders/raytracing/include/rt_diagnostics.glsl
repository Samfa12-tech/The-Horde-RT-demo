#ifndef HORDE_RT_DIAGNOSTICS_GLSL
#define HORDE_RT_DIAGNOSTICS_GLSL

// This boundary deliberately preserves the existing diagnostics behavior. A
// later reviewed variant-specialization task may make these writes no-ops for
// Shipping artifacts without changing their diagnostic call sites.
#define RT_DIAG_ADD(fieldName, deltaValue) atomicAdd(rtDielectricDiagnostics.value.fieldName, deltaValue)
#define RT_DIAG_OR(fieldName, maskValue) atomicOr(rtDielectricDiagnostics.value.fieldName, maskValue)

#endif
