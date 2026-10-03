// LOCAL INVESTIGATION ONLY. Never publish this worktree or its derived APK.
// 1: omit fire-emitter visibility traversal, keeping the direct BRDF calculation.
// 2: omit fire-volume/ember composition. Both intentionally change the image.
// Other geometry, masks, lighting, material transport and budgets stay unchanged.
// Timing deltas include compiler layout/occupancy effects, not isolated ray cost.
#define HORDE_LOCAL_LIGHTING_ISOLATION_MODE 2
#if HORDE_RT_VARIANT_INSTRUMENTATION == 0 && HORDE_RT_VARIANT_QUALITY == 0
#define HORDE_LOCAL_FIRE_SHADOW_ISOLATION (HORDE_LOCAL_LIGHTING_ISOLATION_MODE == 1)
#define HORDE_LOCAL_FIRE_VOLUME_ISOLATION (HORDE_LOCAL_LIGHTING_ISOLATION_MODE == 2)
#else
#define HORDE_LOCAL_FIRE_SHADOW_ISOLATION 0
#define HORDE_LOCAL_FIRE_VOLUME_ISOLATION 0
#endif
