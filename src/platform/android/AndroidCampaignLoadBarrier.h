#pragma once

#include "gameplay/campaign/CampaignLoadBarrier.h"
#include "gameplay/simulation/InputSnapshot.h"

namespace horde::platform::android
{
// Preserve the Android adapter API while sharing the owner-thread policy.
using AndroidCampaignLoadBarrier = horde::gameplay::campaign::CampaignLoadBarrier;
// Transport-owner policy: token admission cannot stand in for a neutral sample.
inline bool IsNeutralCampaignRebaseInput(const horde::gameplay::simulation::InputSnapshot& input,
    const AndroidCampaignLoadBarrier::Status& barrier) noexcept
{
    return input.paused && input.moveForward==0 && input.moveStrafe==0 &&
        !input.runHeld && !input.crouchHeld && !input.guardHeld && !input.analogueMovement &&
        !input.hasAuthoritativePlayerPose && barrier.MatchesRebasePose(input.yawRadians,input.pitchRadians);
}
}
