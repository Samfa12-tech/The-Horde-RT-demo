#include "gameplay/campaign/CampaignPlaySession.h"

namespace horde::gameplay::campaign {
PlaySession::PlaySession(simulation::GameSimulation& simulation,std::filesystem::path directory,storage::FaultInjector* faults)
    :simulation_(simulation),session_(simulation,std::move(directory),faults) {
    observedRetryGeneration_=simulation_.Snapshot().campaignRetryRequestGeneration;
    observedRestGeneration_=simulation_.Snapshot().completedRestGeneration;
}
PlaySession::~PlaySession(){catalogueCoordinator_.Close();session_.Close();}
bool PlaySession::Busy() const {return state_.outcome==PlayOutcome::Pending;}
PlayState PlaySession::State() const {
    auto result=state_;result.phase=session_.State().phase;
    result.hasLatestCheckpoint=latest_.has_value();return result;
}
std::optional<PlayState> PlaySession::TakeCompletion() {
    if((state_.outcome!=PlayOutcome::Succeeded && state_.outcome!=PlayOutcome::Failed) ||
       state_.operationRevision<=consumedCompletionRevision_)return std::nullopt;
    consumedCompletionRevision_=state_.operationRevision;return State();
}
void PlaySession::SuspendCatalogue() {
    catalogueCoordinator_.Close();catalogueRequest_.reset();catalogueActive_=false;
}
void PlaySession::RefreshCatalogue() {
    SuspendCatalogue();catalogue_={};catalogueIndex_=0;
    if(state_.catalogueRevision!=UINT64_MAX)++state_.catalogueRevision;
}
bool PlaySession::Select(unsigned slot) {
    if(slot>=3)return false;
    const auto s=session_.State();
    return (s.phase!=SessionPhase::Closed && s.slot==slot) || session_.SelectSlot(slot);
}
bool PlaySession::BeginNewGame(unsigned slot) {
    if(slot>=3)return false;
    Cancel();
    if(!Select(slot))return false;
    latest_.reset();boundary_.reset();pendingAuto_.reset();retryRecord_.reset();pendingRetry_=false;
    observedRetryGeneration_=simulation_.Snapshot().campaignRetryRequestGeneration;
    observedRestGeneration_=simulation_.Snapshot().completedRestGeneration;
    observedBoundaryRevision_=simulation_.CampaignBoundaryRevision();
    state_.activeSlot=slot;state_.latestRevision=0;
    state_.operation=PlayOperation::None;state_.outcome=PlayOutcome::None;
    return true;
}
bool PlaySession::SetLatest(const Checkpoint& checkpoint) {
    std::vector<std::byte> bytes,old;codec::Error error{};
    if(!codec::Encode(checkpoint,bytes,error))return false;
    if(latest_ && codec::Encode(*latest_,old,error) && old==bytes)return false;
    if(state_.latestRevision==UINT64_MAX)return false;
    latest_=checkpoint;++state_.latestRevision;return true;
}
bool PlaySession::ObserveSafeBoundary() {
    const auto& snapshot=simulation_.Snapshot();
    const bool restCompleted=snapshot.completedRestGeneration!=observedRestGeneration_;
    observedRestGeneration_=snapshot.completedRestGeneration;
    const bool retryRequested=snapshot.campaignRetryRequestGeneration!=observedRetryGeneration_;
    observedRetryGeneration_=snapshot.campaignRetryRequestGeneration;
    if(retryRequested)pendingRetry_=true;
    const auto revision=simulation_.CampaignBoundaryRevision();
    if(revision==observedBoundaryRevision_)return retryRequested;
    observedBoundaryRevision_=revision;
    const auto& captured=simulation_.LatestCampaignBoundary();
    if(!captured) {
        // Explicit reset/replacement clears the earned latch. Cancel old-game
        // tickets before they can acknowledge resources for the new game.
        if(snapshot.healingFlaskEnabled)Cancel();
        retryRecord_.reset();
        const bool changed=latest_.has_value();latest_.reset();boundary_.reset();pendingAuto_.reset();
        return changed;
    }
    const bool changed=SetLatest(*captured);
    if(!boundary_ || *boundary_!=captured->id || (restCompleted && changed)) {
        boundary_=captured->id;
        if(state_.activeSlot && latest_ && latest_->id==captured->id)
            pendingAuto_=PendingAuto{*latest_,state_.latestRevision,*state_.activeSlot};
    }
    return changed;
}
bool PlaySession::StartSave(unsigned slot,SaveKind kind,const Checkpoint& checkpoint,std::uint64_t revision) {
    if(Busy() || slot>=3 || state_.operationRevision==UINT64_MAX)return false;
    SuspendCatalogue();
    if(!Select(slot) || !session_.SaveCheckpoint(kind,checkpoint))return false;
    state_.fromConfirmedRecord=false;
    state_.operation=kind==SaveKind::Manual?PlayOperation::ManualSave:PlayOperation::Autosave;
    state_.outcome=PlayOutcome::Pending;state_.operationSlot=slot;state_.operationKind=kind;
    ++state_.operationRevision;state_.checkpointRevision=revision;state_.result={};return true;
}
bool PlaySession::SaveManual(unsigned slot) {
    return latest_ && StartSave(slot,SaveKind::Manual,*latest_,state_.latestRevision);
}
bool PlaySession::Load(unsigned slot,SaveKind kind) {
    if(Busy() || !SaveTarget(slot,kind) || state_.operationRevision==UINT64_MAX)return false;
    SuspendCatalogue();
    if(!Select(slot) || !session_.Load(kind))return false;
    state_.fromConfirmedRecord=false;
    state_.operation=PlayOperation::Load;state_.outcome=PlayOutcome::Pending;
    state_.operationSlot=slot;state_.operationKind=kind;++state_.operationRevision;state_.result={};
    return true;
}
bool PlaySession::RetryConfirmedCheckpoint() {
    if(Busy() || state_.operationRevision==UINT64_MAX)return false;
    SuspendCatalogue();
    if(!state_.activeSlot || !retryRecord_ || retryRecord_->slot!=*state_.activeSlot) {
        state_.operation=PlayOperation::Load;state_.outcome=PlayOutcome::Failed;
        state_.fromConfirmedRecord=true;state_.operationSlot=state_.activeSlot.value_or(0);
        state_.result={storage::Status::NotFound};++state_.operationRevision;
        return false; // No free retry heal and no unconfirmed-state substitution.
    }
    const auto record=*retryRecord_;
    if(!Select(record.slot) || !session_.BeginConfirmedRetry(record.confirmed,record.kind))return false;
    state_.operation=PlayOperation::Load;state_.outcome=PlayOutcome::Pending;
    state_.fromConfirmedRecord=true;state_.operationSlot=record.slot;state_.operationKind=record.kind;
    ++state_.operationRevision;state_.checkpointRevision=0;state_.result={};return true;
}
void PlaySession::Finish(PlayOutcome outcome) {
    state_.outcome=outcome;state_.result=session_.State().lastResult;
    if(outcome==PlayOutcome::Succeeded && state_.operation!=PlayOperation::Load &&
       state_.activeSlot && state_.operationSlot==*state_.activeSlot) {
        if(const auto confirmed=session_.ConfirmedRecord())
            retryRecord_=RetryRecord{*confirmed,state_.operationSlot,state_.operationKind};
    }
    // A refreshed catalogue always reads real records, including uncertain
    // publication outcomes. It never retries or acknowledges a failed save.
    RefreshCatalogue();
}
bool PlaySession::PollCatalogue() {
    if(catalogueIndex_>=catalogue_.size())return false;
    if(!catalogueActive_) {
        if(!catalogueCoordinator_.OpenSession(catalogueIndex_+1))return false;
        catalogueRequest_=catalogueCoordinator_.BeginRead();catalogueActive_=true;
    }
    if(catalogueRequest_) {
        if(!session_.SubmitCatalogueRead(*catalogueRequest_))return false;
        catalogueRequest_.reset();
    }
    if(!catalogueCoordinator_.Poll())return false;
    const auto observed=catalogueCoordinator_.State();auto& entry=catalogue_[catalogueIndex_];
    entry.result=observed.lastResult;entry.checkpoint.reset();
    if(observed.phase==async::Phase::AwaitingReadDecision && observed.preparedRead) {
        const auto record=catalogueCoordinator_.ReadCandidate(*observed.preparedRead);
        if(record){entry.status=CatalogueStatus::Valid;entry.checkpoint=record->checkpoint;}
        else entry.status=CatalogueStatus::Empty;
    } else entry.status=observed.lastResult.status==storage::Status::InvalidExisting
        ?CatalogueStatus::Invalid:CatalogueStatus::ReadError;
    SuspendCatalogue();++catalogueIndex_;
    if(state_.catalogueRevision!=UINT64_MAX)++state_.catalogueRevision;
    return true;
}
bool PlaySession::Poll() {
    bool changed=false;
    if(Busy()) {
        changed=session_.Poll();const auto phase=session_.State().phase;
        if(phase==SessionPhase::Failed){Finish(PlayOutcome::Failed);return true;}
        if(state_.operation!=PlayOperation::Load && phase==SessionPhase::Idle){Finish(PlayOutcome::Succeeded);return true;}
        return changed;
    }
    if(pendingAuto_) {
        const auto pending=*pendingAuto_;
        // An abandoned/newly loaded game may never retarget an older auto.
        pendingAuto_.reset();
        if(state_.activeSlot && *state_.activeSlot==pending.slot &&
           StartSave(pending.slot,SaveKind::Autosave,pending.checkpoint,pending.revision))return true;
    }
    if(pendingRetry_) {
        pendingRetry_=false;
        RetryConfirmedCheckpoint();return true;
    }
    return PollCatalogue();
}
std::optional<LoadPreparation> PlaySession::PrepareLoad() {
    return Busy() && state_.operation==PlayOperation::Load ? session_.PrepareLoad():std::nullopt;
}
bool PlaySession::CommitLoad(const LoadPreparation& preparation,simulation::WorldZoneToken ready,
    std::shared_ptr<const simulation::TriangleSupportSurface> terrain,simulation::InputSnapshot& input,
    std::uint64_t publication) {
    if(!Busy() || state_.operation!=PlayOperation::Load ||
       !session_.CommitLoad(preparation,ready,std::move(terrain),input,publication))return false;
    if(!preparation.fromConfirmedRecord) {
        const auto confirmed=session_.ConfirmedRecord();
        retryRecord_=confirmed?std::optional<RetryRecord>(RetryRecord{*confirmed,state_.operationSlot,state_.operationKind}):std::nullopt;
    }
    state_.activeSlot=state_.operationSlot;pendingAuto_.reset();latest_.reset();boundary_.reset();
    pendingRetry_=false;
    observedRetryGeneration_=simulation_.Snapshot().campaignRetryRequestGeneration;
    observedRestGeneration_=simulation_.Snapshot().completedRestGeneration;
    const auto& captured=simulation_.LatestCampaignBoundary();
    observedBoundaryRevision_=simulation_.CampaignBoundaryRevision();
    if(captured){SetLatest(*captured);boundary_=captured->id;}
    Finish(PlayOutcome::Succeeded);return true;
}
void PlaySession::Cancel() {
    SuspendCatalogue();session_.Cancel();pendingAuto_.reset();pendingRetry_=false;
    if(state_.activeSlot)Select(*state_.activeSlot);else session_.Close();
    if(Busy()){state_.outcome=PlayOutcome::Cancelled;state_.result={};}
}
} // namespace horde::gameplay::campaign
