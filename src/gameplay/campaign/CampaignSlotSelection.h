#pragma once
#include "gameplay/campaign/CampaignPlaySession.h"

namespace horde::gameplay::campaign {
// One selection for display and Load. No mtime, file-copy time or current-time
// fallback. Wait for both reads; otherwise catalogue polling could change the
// chosen record underneath the player's click.
inline std::optional<SaveKind> SelectSlotRecord(const std::array<CatalogueEntry,6>& entries,unsigned slot) {
    if(slot>=3)return std::nullopt;
    const auto& manual=entries[slot*2];const auto& automatic=entries[slot*2+1];
    if(manual.status==CatalogueStatus::Checking || automatic.status==CatalogueStatus::Checking)return std::nullopt;
    const auto valid=[](const CatalogueEntry& e){return e.status==CatalogueStatus::Valid && e.checkpoint && Validate(*e.checkpoint);};
    const bool m=valid(manual),a=valid(automatic);
    if(!m && !a)return std::nullopt;
    if(!m)return SaveKind::Autosave;
    if(!a)return SaveKind::Manual;
    const auto mt=manual.checkpoint->savedAtUnixSeconds,at=automatic.checkpoint->savedAtUnixSeconds;
    // Dated ties favour an explicit manual save. Undated local fixtures favour
    // the latest checkpoint record, with an honest unavailable-date label.
    return mt>at || (mt && mt==at)?SaveKind::Manual:SaveKind::Autosave;
}
}
