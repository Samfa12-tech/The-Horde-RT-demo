#pragma once

#include "gameplay/campaign/CampaignLoadBarrier.h"

namespace horde::platform::android
{
// Preserve the Android adapter API while sharing the owner-thread policy.
using AndroidCampaignLoadBarrier = horde::gameplay::campaign::CampaignLoadBarrier;
}
