#pragma once
#include "gameplay/campaign/CampaignSaveCoordinator.h"
#include <array>
#include <optional>
#include <string_view>

namespace horde::gameplay::campaign {
// Confirmed retention policy: three slots, one manual and one latest autosave
// each. These six stable identities never alias; no indefinite history.
enum class SaveKind { Manual, Autosave };
inline std::optional<async::TargetId> SaveTarget(unsigned slot,SaveKind kind) {
    if(slot>=3 || (kind!=SaveKind::Manual && kind!=SaveKind::Autosave)) return {};
    return 1+slot*2+(kind==SaveKind::Autosave?1:0);
}
inline std::string_view SaveBasename(async::TargetId target) {
    constexpr std::array<std::string_view,6> names{"slot-1-manual.hrd","slot-1-auto.hrd",
        "slot-2-manual.hrd","slot-2-auto.hrd","slot-3-manual.hrd","slot-3-auto.hrd"};
    return target>=1 && target<=names.size()?names[target-1]:std::string_view{};
}
} // namespace horde::gameplay::campaign
