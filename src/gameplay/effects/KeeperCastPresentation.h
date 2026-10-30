#pragma once
#include "gameplay/ShowcaseGameplay.h"

namespace horde::gameplay::effects
{
enum class KeeperCastPoseStyle { ExistingControl, WeightedCandidate };
// A paired diagnostic build may select ExistingControl here. That retains the
// original two-joint curve exactly; no saved setting or live gameplay timing is
// changed by selecting a presentation comparison.
// The heavier presentation is retained as an unadmitted paired-build study.
// It adds refresh work during settling; normal gameplay keeps the exact control.
inline constexpr KeeperCastPoseStyle kKeeperCastPoseStyle = KeeperCastPoseStyle::ExistingControl;
struct KeeperCastPose
{
    float staffLiftRadians = 0.0f;
    float staffCastRadians = 0.0f;
    float upperBodyPitchRadians = 0.0f;
};
inline float KeeperCastSmoothStep(float value)
{
    value = std::clamp(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
}
inline float KeeperCastUpperBodyPitch(float lift, float cast,
    KeeperCastPoseStyle style = kKeeperCastPoseStyle)
{
    if (style == KeeperCastPoseStyle::ExistingControl) return 0.0f;
    // Spine is above the independent Hips/leg branches in the audited 24-joint
    // rig. Brace back, then carry the magic release forward without translating
    // the root or changing its existing whole-instance transform.
    return -0.12f * std::clamp(lift / 0.72f, 0.0f, 1.0f) +
            0.20f * std::clamp(cast / 0.50f, 0.0f, 1.0f);
}
inline KeeperCastPose EvaluateKeeperCastPose(const LichSnapshot& keeper,
    KeeperCastPoseStyle style = kKeeperCastPoseStyle)
{
    KeeperCastPose pose;
    if (!keeper.readableCastPresentation || !std::isfinite(keeper.phaseTime)) return pose;
    if (keeper.phase == LichPhase::Charging)
    {
        const float charge = std::clamp(keeper.phaseTime / LichEncounter::kChargeDuration, 0.0f, 1.0f);
        if (style == KeeperCastPoseStyle::ExistingControl)
            pose.staffLiftRadians = 0.58f * KeeperCastSmoothStep(charge / 0.80f);
        else
            pose.staffLiftRadians = charge < 0.30f
                ? 0.22f * KeeperCastSmoothStep(charge / 0.30f)
                : 0.22f + 0.50f * KeeperCastSmoothStep((charge - 0.30f) / 0.50f);
    }
    else if (keeper.phase == LichPhase::Recovering)
    {
        if (style == KeeperCastPoseStyle::ExistingControl)
        {
            const float fade = 1.0f - KeeperCastSmoothStep(
                keeper.phaseTime / LichEncounter::kDischargeVisibleBurstDuration);
            pose.staffLiftRadians = 0.58f * fade;
            pose.staffCastRadians = 0.46f * fade;
        }
        else
        {
            // The authoritative light burst stays 0.18 s and damage is already
            // resolved at discharge. Only upper-body settling lasts 0.60 s.
            pose.staffLiftRadians = 0.72f * (1.0f - KeeperCastSmoothStep(keeper.phaseTime / 0.60f));
            pose.staffCastRadians = 0.50f * KeeperCastSmoothStep(keeper.phaseTime / 0.10f) *
                (1.0f - KeeperCastSmoothStep((keeper.phaseTime - 0.18f) / 0.42f));
        }
    }
    pose.upperBodyPitchRadians = KeeperCastUpperBodyPitch(pose.staffLiftRadians, pose.staffCastRadians, style);
    return pose;
}

// Existing float discriminator: 0 = skeleton; [0.75, 1] = live Keeper
// with room-mist density [0, 1]; 2 = discharge or preserved legacy capture.
// No field, packing, physical light or quality-buffer layout is reinterpreted.
// Every existing surface/light consumer still identifies the Keeper by > 0.5.
inline float KeeperShaderPresentationKind(bool selectedKeeper, const LichSnapshot& keeper)
{
    if (!selectedKeeper) return 0.0f;
    if (!keeper.readableCastPresentation) return 2.0f;
    const bool discharge = keeper.phase == LichPhase::Recovering &&
        keeper.phaseTime >= 0.0f &&
        keeper.phaseTime < LichEncounter::kDischargeVisibleBurstDuration &&
        keeper.staffLightStrength >= 0.05f;
    if (discharge) return 2.0f;
    const float density = std::isfinite(keeper.roomMistDensityScale) ?
        std::clamp(keeper.roomMistDensityScale, 0.0f, 1.0f) : 1.0f;
    return 0.75f + 0.25f * density;
}
} // namespace horde::gameplay::effects
