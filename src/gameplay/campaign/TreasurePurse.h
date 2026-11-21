#pragma once

#include <cstdint>

namespace horde::gameplay::campaign {

struct TreasurePurse {
    static constexpr std::uint32_t kMaximumCoins = 9999;
    static constexpr std::uint32_t kRegisterRewardCoins = 3;
    static constexpr std::uint32_t kLanternFittingPrice = 2;

    std::uint32_t coins = 0;
    bool registerRewardClaimed = false;
    bool lanternFittingPurchased = false;

    constexpr bool CanClaimRegisterReward() const {
        return !registerRewardClaimed && coins <= kMaximumCoins - kRegisterRewardCoins;
    }

    constexpr bool ClaimRegisterReward() {
        if (!CanClaimRegisterReward()) return false;
        coins += kRegisterRewardCoins;
        registerRewardClaimed = true;
        return true;
    }

    constexpr bool CanBuyLanternFitting() const {
        return registerRewardClaimed && !lanternFittingPurchased && coins >= kLanternFittingPrice;
    }

    constexpr bool BuyLanternFitting() {
        if (!CanBuyLanternFitting()) return false;
        coins -= kLanternFittingPrice;
        lanternFittingPurchased = true;
        return true;
    }

    bool operator==(const TreasurePurse&) const = default;
};

} // namespace horde::gameplay::campaign
