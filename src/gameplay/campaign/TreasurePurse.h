#pragma once

#include <cstdint>

namespace horde::gameplay::campaign {

enum class OrdinarySilverProgress : std::uint8_t { Unfound, Carried, Sold };
enum class OrdinarySilverAction : std::uint8_t { None, Take, Sell, Sold };

struct TreasurePurse {
    static constexpr std::uint32_t kMaximumCoins = 9999;
    static constexpr std::uint32_t kRegisterRewardCoins = 3;
    static constexpr std::uint32_t kLanternFittingPrice = 2;
    static constexpr std::uint32_t kOrdinarySilverSaleCoins = 2;

    std::uint32_t coins = 0;
    bool registerRewardClaimed = false;
    bool lanternFittingPurchased = false;
    OrdinarySilverProgress ordinarySilver = OrdinarySilverProgress::Unfound;

    constexpr bool HasEarnedCoins() const {
        return registerRewardClaimed || ordinarySilver == OrdinarySilverProgress::Sold;
    }

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
        return HasEarnedCoins() && !lanternFittingPurchased && coins >= kLanternFittingPrice;
    }

    constexpr bool BuyLanternFitting() {
        if (!CanBuyLanternFitting()) return false;
        coins -= kLanternFittingPrice;
        lanternFittingPurchased = true;
        return true;
    }

    constexpr bool CanCollectOrdinarySilver() const {
        return ordinarySilver == OrdinarySilverProgress::Unfound;
    }

    constexpr bool CollectOrdinarySilver() {
        if (!CanCollectOrdinarySilver()) return false;
        ordinarySilver = OrdinarySilverProgress::Carried;
        return true;
    }

    constexpr bool CanSellOrdinarySilver() const {
        return ordinarySilver == OrdinarySilverProgress::Carried &&
            coins <= kMaximumCoins - kOrdinarySilverSaleCoins;
    }

    constexpr bool SellOrdinarySilver() {
        if (!CanSellOrdinarySilver()) return false;
        coins += kOrdinarySilverSaleCoins;
        ordinarySilver = OrdinarySilverProgress::Sold;
        return true;
    }

    bool operator==(const TreasurePurse&) const = default;
};

} // namespace horde::gameplay::campaign
