#pragma once

#include <cstdint>

namespace horde::gameplay::campaign {

enum class OrdinarySilverProgress : std::uint8_t { Unfound, Carried, Sold };
enum class OrdinarySilverAction : std::uint8_t { None, Take, Sell, Sold };

enum class MillTallyProgress : std::uint8_t {
    Unfound, Carried, Sold, ClaimRecorded, Recovered, Returned, ReturnedAfterSale
};
enum class MillTallyAction : std::uint8_t { None, Take, Sell, RecordClaim, Recover, Return };

struct TreasurePurse {
    static constexpr std::uint32_t kMaximumCoins = 9999;
    static constexpr std::uint32_t kRegisterRewardCoins = 3;
    static constexpr std::uint32_t kLanternFittingPrice = 2;
    static constexpr std::uint32_t kOrdinarySilverSaleCoins = 2;
    static constexpr std::uint32_t kMillTallySaleCoins = 2;

    std::uint32_t coins = 0;
    bool registerRewardClaimed = false;
    bool lanternFittingPurchased = false;
    OrdinarySilverProgress ordinarySilver = OrdinarySilverProgress::Unfound;

    MillTallyProgress millTally = MillTallyProgress::Unfound;

    constexpr std::uint32_t CoinsIssuedFromSales() const {
        const bool soldTally=millTally==MillTallyProgress::Sold || millTally==MillTallyProgress::ClaimRecorded ||
            millTally==MillTallyProgress::Recovered || millTally==MillTallyProgress::ReturnedAfterSale;
        return (ordinarySilver==OrdinarySilverProgress::Sold ? kOrdinarySilverSaleCoins : 0u) +
            (soldTally ? kMillTallySaleCoins : 0u);
    }

    constexpr bool HasEarnedCoins() const {
        return registerRewardClaimed || CoinsIssuedFromSales()!=0;
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

    constexpr bool CanSellMillTally() const {
        return millTally==MillTallyProgress::Carried && coins<=kMaximumCoins-kMillTallySaleCoins;
    }

    constexpr bool ApplyMillTallyAction(MillTallyAction action) {
        switch(action) {
        case MillTallyAction::Take:
            if(millTally!=MillTallyProgress::Unfound)return false;
            millTally=MillTallyProgress::Carried;
            return true;
        case MillTallyAction::Sell:
            if(!CanSellMillTally())return false;
            coins+=kMillTallySaleCoins;
            millTally=MillTallyProgress::Sold;
            return true;
        case MillTallyAction::RecordClaim:
            if(millTally!=MillTallyProgress::Sold)return false;
            millTally=MillTallyProgress::ClaimRecorded;
            return true;
        case MillTallyAction::Recover:
            if(millTally!=MillTallyProgress::ClaimRecorded)return false;
            millTally=MillTallyProgress::Recovered;
            return true;
        case MillTallyAction::Return:
            if(millTally!=MillTallyProgress::Carried && millTally!=MillTallyProgress::Recovered)return false;
            millTally=millTally==MillTallyProgress::Recovered ? MillTallyProgress::ReturnedAfterSale : MillTallyProgress::Returned;
            return true;
        case MillTallyAction::None:
            return false;
        }
        return false;
    }

    bool operator==(const TreasurePurse&) const = default;
};

} // namespace horde::gameplay::campaign
