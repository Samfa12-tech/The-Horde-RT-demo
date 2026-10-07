#pragma once

namespace horde::gameplay
{

// Shared authored combat phase timings. Keep these as the single source for
// both fixed-step combat edges and the corresponding renderer clip mapping.
struct CombatTimeline
{
    // Stroke-relative contact, calibrated against the final imported player
    // Grip and sword approaching the frontal 1.2m skeleton idle mesh. These
    // are semantic contact samples, not a claim of mesh collision for every
    // target pose or of exact visible latency at every presentation rate.
    static constexpr float kPlayerDownwardContactSeconds = 0.10f;
    static constexpr float kPlayerUpwardContactSeconds = 0.05f;
    static constexpr float kSkeletonAttackWindupSeconds = 1.12f;
    static constexpr float kSkeletonAttackActiveSeconds = 0.18f;
    static constexpr float kSkeletonAttackRecoverySeconds = 1.50f;

    // Renderer-only clip sample used to start the stagger recovery pose after
    // a successful parry. It does not define the normal gameplay contact edge.
    static constexpr float kSkeletonStaggerRecoverySampleSeconds = 1.20f;
    static constexpr float kSkeletonAttackRecoverySampleEndSeconds = 2.80f;

    static constexpr float kParryPresentationSeconds = 0.12f;
};

} // namespace horde::gameplay
