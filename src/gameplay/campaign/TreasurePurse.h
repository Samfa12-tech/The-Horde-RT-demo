#pragma once

#include <cstdint>

namespace horde::gameplay::campaign {

struct TreasurePurse {
    static constexpr std::uint32_t kMaximumCoins = 9999;
    static constexpr std::uint32_t kRegisterRewardCoins = 3;

    std::uint32_t coins = 0;
    bool registerRewardClaimed = false;

    constexpr bool CanClaimRegisterReward() const {
        return !registerRewardClaimed && coins <= kMaximumCoins - kRegisterRewardCoins;
    }

    constexpr bool ClaimRegisterReward() {
        if (!CanClaimRegisterReward()) return false;
        coins += kRegisterRewardCoins;
        registerRewardClaimed = true;
        return true;
    }

    bool operator==(const TreasurePurse&) const = default;
};

} // namespace horde::gameplay::campaign
