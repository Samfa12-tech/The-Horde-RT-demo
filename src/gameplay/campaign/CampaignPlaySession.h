#pragma once
#include "gameplay/campaign/CampaignSession.h"

namespace horde::gameplay::campaign {
enum class CatalogueStatus { Checking, Empty, Valid, Invalid, ReadError };
struct CatalogueEntry {
    CatalogueStatus status=CatalogueStatus::Checking;
    std::optional<Checkpoint> checkpoint;
    storage::Result result;
};
enum class PlayOperation { None, ManualSave, Autosave, Load };
enum class PlayOutcome { None, Pending, Succeeded, Failed, Cancelled };
struct PlayState {
    std::optional<unsigned> activeSlot;
    PlayOperation operation=PlayOperation::None;
    PlayOutcome outcome=PlayOutcome::None;
    unsigned operationSlot=0;
    SaveKind operationKind=SaveKind::Manual;
    std::uint64_t operationRevision=0,checkpointRevision=0,latestRevision=0,catalogueRevision=0;
    storage::Result result;
    SessionPhase phase=SessionPhase::Closed;
    bool hasLatestCheckpoint=false;
};
// Owner/render-thread facade shared by Android and Windows. No filesystem IO,
// directory creation, platform controls, GPU readiness claims or UI policy here.
// Six catalogue entries are slot*2 + (Manual=0, Autosave=1). A single existing
// Session worker services both gameplay IO and sequential read-only catalogue.
class PlaySession {
public:
    PlaySession(simulation::GameSimulation&,std::filesystem::path directory,storage::FaultInjector* faults=nullptr);
    ~PlaySession();
    void RefreshCatalogue();
    const std::array<CatalogueEntry,6>& Catalogue() const {return catalogue_;}
    PlayState State() const;
    // Owner-only single-consumption view of the latest terminal operation.
    // State retains its result; no receipt history or automatic retry exists.
    std::optional<PlayState> TakeCompletion();
    const std::optional<Checkpoint>& LatestCheckpoint() const {return latest_;}
    // Root resets the simulation separately. Never removes/overwrites records.
    bool BeginNewGame(unsigned slot);
    // Consume the simulation's fixed-tick authored-boundary latch. Strict
    // actual-position capture remains a separate simulation API. Movement and
    // crouching retain the last admitted immutable record.
    // Autosave only on a new boundary and only with an explicit active slot.
    bool ObserveSafeBoundary();
    // Manual choice does not change the current game's active autosave slot.
    bool SaveManual(unsigned slot);
    // Target becomes active only after successful CommitLoad.
    bool Load(unsigned slot,SaveKind kind);
    bool Poll();
    std::optional<LoadPreparation> PrepareLoad();
    bool CommitLoad(const LoadPreparation&,simulation::WorldZoneToken ready,
        std::shared_ptr<const simulation::TriangleSupportSurface> terrain,
        simulation::InputSnapshot& ownerInput,std::uint64_t publicationSequence);
    void Cancel();
private:
    struct PendingAuto {Checkpoint checkpoint;std::uint64_t revision;unsigned slot;};
    bool Busy() const;
    bool Select(unsigned slot);
    void SuspendCatalogue();
    bool PollCatalogue();
    bool StartSave(unsigned slot,SaveKind kind,const Checkpoint&,std::uint64_t revision);
    bool SetLatest(const Checkpoint&);
    void Finish(PlayOutcome);
    simulation::GameSimulation& simulation_;
    Session session_;
    async::Coordinator catalogueCoordinator_;
    std::optional<async::Request> catalogueRequest_;
    std::array<CatalogueEntry,6> catalogue_{};
    std::size_t catalogueIndex_=0;
    bool catalogueActive_=false;
    std::optional<Checkpoint> latest_;
    std::optional<CheckpointId> boundary_;
    std::uint64_t observedBoundaryRevision_=0;
    std::uint64_t consumedCompletionRevision_=0;
    std::optional<PendingAuto> pendingAuto_;
    PlayState state_;
};
} // namespace horde::gameplay::campaign
