#pragma once
#include "gameplay/campaign/CampaignSaveWorker.h"
#include "gameplay/simulation/GameSimulation.h"

namespace horde::gameplay::campaign {
enum class SessionPhase { Closed, Idle, Saving, ReadingLoad, LoadReady, PreparingLoad, Failed };
struct SessionState {
    SessionPhase phase=SessionPhase::Closed;
    unsigned slot=0;
    SaveKind kind=SaveKind::Manual;
    storage::Result lastResult;
};
struct LoadPreparation {
    async::Token read;
    RestoreToken restore;
    simulation::WorldZoneToken destination;
    bool fromConfirmedRecord=false;
};
struct ConfirmedCampaignRecord {
    storage::StoredCheckpoint stored;
    storage::Result acknowledgement;
};
inline std::optional<ConfirmedCampaignRecord> ConfirmedCampaignRecordFrom(const async::Snapshot& c) {
    if(c.phase!=async::Phase::Ready || c.error!=async::Error::None ||
       !c.currentCheckpointStored || !c.confirmed)return {};
    return ConfirmedCampaignRecord{*c.confirmed,c.lastResult};
}
// Owner-thread controller with one bounded IO worker. The simulation must
// outlive this controller. No default save directory, menus or renderer access.
// Platform adapters must release/rebase their upstream camera/input owners
// after successful CommitLoad, before publishing fresh unpaused input.
class Session {
public:
    Session(simulation::GameSimulation& simulation,std::filesystem::path directory,
        storage::FaultInjector* faults=nullptr);
    ~Session();
    bool SelectSlot(unsigned slot); // zero-based, exactly three slots
    bool Save(SaveKind kind,CheckpointId checkpoint);
    // Explicit immutable simulation-authored boundary; does not recapture the
    // current player position. Platform adapters must not fabricate records.
    bool SaveCheckpoint(SaveKind kind,const Checkpoint& checkpoint);
    // Read-only catalogue work shares this session's bounded worker without
    // selecting a slot, adopting a record or changing gameplay/session state.
    bool SubmitCatalogueRead(const async::Request& request);
    bool Load(SaveKind kind);
    bool Poll();
    std::optional<LoadPreparation> PrepareLoad();
    bool CommitLoad(const LoadPreparation& preparation,simulation::WorldZoneToken ready,
        std::shared_ptr<const simulation::TriangleSupportSurface> terrain,
        simulation::InputSnapshot& ownerInput,std::uint64_t publicationSequence);
    void Cancel();
    void Close();
    SessionState State() const {return state_;}
    // Actual adopted/read-back coordinator record, never its logical candidate.
    // No IO or new durability claim; an uncertain/failed write is not confirmed.
    std::optional<ConfirmedCampaignRecord> ConfirmedRecord() const;
private:
    friend class PlaySession;
    bool BeginConfirmedRetry(const ConfirmedCampaignRecord&,SaveKind);
    async::Coordinator& Current();
    bool Submit(std::optional<async::Request> request);
    void CancelPreparation();
    simulation::GameSimulation& simulation_;
    std::array<async::Coordinator,2> coordinators_;
    async::Worker worker_;
    SessionState state_;
    std::optional<LoadPreparation> preparation_;
    std::optional<ConfirmedCampaignRecord> confirmedRetry_;
};
} // namespace horde::gameplay::campaign
