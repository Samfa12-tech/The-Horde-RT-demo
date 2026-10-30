#include "gameplay/campaign/CampaignSession.h"
#include <type_traits>

namespace horde::gameplay::campaign {
static_assert(std::is_nothrow_move_assignable_v<simulation::GameSimulation>);
static_assert(std::is_nothrow_move_assignable_v<async::Snapshot>);
Session::Session(simulation::GameSimulation& simulation,std::filesystem::path directory,storage::FaultInjector* faults)
    :simulation_(simulation),worker_(std::move(directory),faults){}
Session::~Session(){Close();worker_.Stop();}
async::Coordinator& Session::Current(){return coordinators_[state_.kind==SaveKind::Manual?0:1];}
void Session::CancelPreparation(){if(preparation_) simulation_.CancelCampaignRestore(preparation_->restore);preparation_.reset();confirmedRetry_.reset();}
void Session::Close(){CancelPreparation();for(auto& c:coordinators_)c.Close();state_.phase=SessionPhase::Closed;}
void Session::Cancel(){CancelPreparation();for(auto& c:coordinators_)c.Cancel();if(state_.phase!=SessionPhase::Closed)state_.phase=SessionPhase::Idle;}
bool Session::SelectSlot(unsigned slot) {
    if(slot>=3) return false;
    Close();state_={};state_.slot=slot;
    if(!coordinators_[0].OpenSession(*SaveTarget(slot,SaveKind::Manual)) ||
       !coordinators_[1].OpenSession(*SaveTarget(slot,SaveKind::Autosave))) return false;
    state_.phase=SessionPhase::Idle;return true;
}
bool Session::Submit(std::optional<async::Request> request) {
    if(!request) return false;
    if(worker_.Submit(*request)) return true;
    async::Completion failure;failure.target=request->Work().boundTarget;failure.result={storage::Status::Busy};
    if(request->Work().token.operation==async::Operation::Read)request->CompleteRead(std::move(failure));
    else request->CompleteWriteFailure(std::move(failure));
    return true; // Typed failure is consumed by owner Poll, never silently lost.
}
bool Session::Save(SaveKind kind,CheckpointId checkpoint) {
    const auto captured=simulation_.CaptureCampaignCheckpoint(checkpoint);if(!captured)return false;
    return SaveCheckpoint(kind,*captured);
}
bool Session::SubmitCatalogueRead(const async::Request& request) {
    return request.Work().token.operation==async::Operation::Read && worker_.Submit(request);
}
bool Session::SaveCheckpoint(SaveKind kind,const Checkpoint& captured) {
    if(!Validate(captured) || !SaveTarget(state_.slot,kind) ||
       (state_.phase!=SessionPhase::Idle && state_.phase!=SessionPhase::Failed))return false;
    state_.kind=kind;
    if(!Current().NoteCheckpoint(captured) || !Submit(Current().BeginRead()))return false;
    state_.phase=SessionPhase::Saving;return true;
}
bool Session::Load(SaveKind kind) {
    if(!SaveTarget(state_.slot,kind) || (state_.phase!=SessionPhase::Idle && state_.phase!=SessionPhase::Failed))return false;
    state_.kind=kind;if(!Submit(Current().BeginRead()))return false;
    state_.phase=SessionPhase::ReadingLoad;return true;
}
std::optional<ConfirmedCampaignRecord> Session::ConfirmedRecord() const {
    if(state_.phase!=SessionPhase::Idle)return {};
    const auto c=coordinators_[state_.kind==SaveKind::Manual?0:1].State();
    return ConfirmedCampaignRecordFrom(c);
}
bool Session::BeginConfirmedRetry(const ConfirmedCampaignRecord& record,SaveKind kind) {
    const auto target=SaveTarget(state_.slot,kind);
    if(!target || (state_.phase!=SessionPhase::Idle && state_.phase!=SessionPhase::Failed) ||
       !Validate(record.stored.checkpoint) ||
       record.stored.target.filename().string()!=SaveBasename(*target))return false;
    std::vector<std::byte> canonical;codec::Error error{};
    if(!codec::Encode(record.stored.checkpoint,canonical,error) || canonical!=record.stored.encoded)return false;
    // PlaySession retains this immutable record only after a successful actual
    // publication or adopted load for this game/slot. Retry restores those exact
    // bytes through the normal resource/terrain gate; it performs no write/read.
    confirmedRetry_=record;state_.kind=kind;state_.lastResult=record.acknowledgement;
    state_.phase=SessionPhase::LoadReady;return true;
}
bool Session::Poll() {
    if(!Current().Poll())return false;
    const auto c=Current().State();state_.lastResult=c.lastResult;
    if(c.phase==async::Phase::Failed) {state_.phase=SessionPhase::Failed;return true;}
    if(c.phase==async::Phase::AwaitingReadDecision) {
        if(state_.phase==SessionPhase::Saving) {
            if(!Current().UseReadAsWriteBaseline(*c.preparedRead) || !Submit(Current().BeginWrite()))state_.phase=SessionPhase::Failed;
        } else if(state_.phase==SessionPhase::ReadingLoad) {
            state_.phase=SessionPhase::LoadReady;
            // ReadCandidate is the prepared receipt; confirmed is still old.
            if(!Current().ReadCandidate(*c.preparedRead))state_.phase=SessionPhase::Failed;
        }
    } else if(c.phase==async::Phase::Ready && state_.phase==SessionPhase::Saving)state_.phase=SessionPhase::Idle;
    return true;
}
std::optional<LoadPreparation> Session::PrepareLoad() {
    if(state_.phase!=SessionPhase::LoadReady)return {};
    if(confirmedRetry_) {
        const auto& checkpoint=confirmedRetry_->stored.checkpoint;
        const auto restore=simulation_.BeginCampaignRestore(checkpoint);if(!restore)return {};
        preparation_=LoadPreparation{{},*restore,{restore->worldGeneration,checkpoint.zone},true};
        state_.phase=SessionPhase::PreparingLoad;return preparation_;
    }
    const auto read=Current().State().preparedRead;if(!read)return {};
    const auto candidate=Current().ReadCandidate(*read);if(!candidate)return {};
    const auto restore=simulation_.BeginCampaignRestore(candidate->checkpoint);if(!restore)return {};
    preparation_=LoadPreparation{*read,*restore,{restore->worldGeneration,candidate->checkpoint.zone}};
    state_.phase=SessionPhase::PreparingLoad;return preparation_;
}
bool Session::CommitLoad(const LoadPreparation& preparation,simulation::WorldZoneToken ready,
    std::shared_ptr<const simulation::TriangleSupportSurface> terrain,simulation::InputSnapshot& ownerInput,
    std::uint64_t publicationSequence) {
    if(state_.phase!=SessionPhase::PreparingLoad || !preparation_ || preparation.read!=preparation_->read ||
        preparation.restore!=preparation_->restore || ready.generation!=preparation.destination.generation ||
        ready.zone!=preparation.destination.zone ||
        preparation.fromConfirmedRecord!=preparation_->fromConfirmedRecord ||
        (preparation.fromConfirmedRecord ? !confirmedRetry_.has_value() : !Current().ReadCandidate(preparation.read)))return false;
    // Copy the CURRENT live session at final apply, never a snapshot retained
    // across resource/worker delay. Failure leaves simulation/camera/input live.
    auto candidate=std::make_unique<simulation::GameSimulation>(simulation_);
    if(!candidate->CommitCampaignRestore(preparation.restore,ready,std::move(terrain),ownerInput,publicationSequence))return false;
    auto rebased=ownerInput;const auto& loaded=candidate->Snapshot();
    rebased.yawRadians=loaded.playerYawRadians;rebased.pitchRadians=loaded.playerPitchRadians;
    rebased.moveForward=rebased.moveStrafe=0;rebased.runHeld=false;rebased.crouchHeld=false;rebased.hasAuthoritativePlayerPose=false;
    rebased.authoritativePlayerX=rebased.authoritativePlayerZ=0;
    rebased.combatEdgeHistory={};rebased.dialogueCompletionLine=0;rebased.dialogueCompletionGeneration=0;
    // Adoption allocates offside and has a no-throw commit. No owner methods can
    // interleave here; worker callbacks only deposit immutable ticket receipts.
    if(!preparation.fromConfirmedRecord && !Current().AdoptRead(preparation.read))return false;
    simulation_=std::move(*candidate);ownerInput=rebased;
    preparation_.reset();confirmedRetry_.reset();state_.phase=SessionPhase::Idle;return true;
}
} // namespace horde::gameplay::campaign
