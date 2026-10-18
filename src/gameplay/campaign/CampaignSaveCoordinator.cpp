#include "gameplay/campaign/CampaignSaveCoordinator.h"
#include <atomic>
#include <limits>
#include <mutex>

namespace horde::gameplay::campaign::async {
struct TicketState {
    std::mutex mutex;
    bool enabled=true,done=false,publicationAuthorized=false,malformed=false;
    std::optional<Completion> completion;
};
namespace {
std::uint64_t OwnerIdentity() {
    static std::atomic<std::uint64_t> sequence{0};
    auto previous=sequence.load(std::memory_order_relaxed);
    while(previous!=UINT64_MAX) {
        if(sequence.compare_exchange_weak(previous,previous+1,std::memory_order_relaxed)) return previous+1;
    }
    return 0; // Exhaustion is closed, never an identity wrap/reuse.
}
void Deliver(TicketState& state,Completion completion) {
    if(completion.observed && completion.observed->encoded.size()>codec::kMaxFileBytes) {
        completion.observed.reset();state.malformed=true;
    }
    state.completion=std::move(completion);state.done=true;
}
bool ValidRecord(const storage::StoredCheckpoint& record,const std::filesystem::path& target) {
    if(record.target!=target || target.empty() || record.encoded.size()>codec::kMaxFileBytes) return false;
    Checkpoint decoded;codec::Error error{};std::vector<std::byte> logical;
    return codec::Decode(record.encoded,decoded,error) && codec::Encode(record.checkpoint,logical,error) && logical==record.encoded;
}
bool ReadReceipt(const Completion& c,const Description& work) {
    if(c.target.empty() || (!work.boundTarget.empty() && c.target!=work.boundTarget) ||
       c.result.publication!=storage::Publication::Unchanged || c.result.durability!=storage::Durability::None ||
       c.result.codecError!=codec::Error::None) return false;
    if(c.result.status==storage::Status::NotFound) return !c.observed;
    return c.result.status==storage::Status::Read && c.result.systemError==0 && c.observed && ValidRecord(*c.observed,c.target);
}
bool WriteReceipt(const Completion& c,const Description& work,bool authorized) {
    if(!authorized || !work.candidate || c.target!=work.boundTarget || !c.observed ||
       !ValidRecord(*c.observed,c.target) || c.result.codecError!=codec::Error::None || c.result.systemError!=0) return false;
    std::vector<std::byte> expected;codec::Error error{};
    if(!codec::Encode(*work.candidate,expected,error) || c.observed->encoded!=expected) return false;
    if(c.result.status==storage::Status::AlreadyCurrent)
        return work.expected && ValidRecord(*work.expected,work.boundTarget) && work.expected->encoded==expected &&
            c.result.publication==storage::Publication::Unchanged && c.result.durability==storage::Durability::None;
    return c.result.status==storage::Status::Committed && c.result.publication==storage::Publication::Published &&
        (c.result.durability==storage::Durability::FileFlushed || c.result.durability==storage::Durability::FileAndDirectoryFlushed);
}
bool IsFailure(const storage::Result& result) {
    if(result.durability!=storage::Durability::None) return false;
    if(result.status==storage::Status::PublishedDurabilityUnknown) return result.publication==storage::Publication::Published;
    if(result.status==storage::Status::PublishOutcomeUnknown) return result.publication==storage::Publication::Unknown;
    if(result.publication!=storage::Publication::Unchanged) return false;
    switch(result.status) {
    case storage::Status::InvalidPath:case storage::Status::InvalidCandidate:case storage::Status::InvalidExisting:
    case storage::Status::InvalidExpectation:case storage::Status::Conflict:case storage::Status::Busy:
    case storage::Status::OpenFailed:case storage::Status::ReadFailed:case storage::Status::WriteFailed:
    case storage::Status::FlushFailed:case storage::Status::CloseFailed:case storage::Status::PublishFailed:return true;
    default:return false;
    }
}
bool FailureReceipt(const Completion& c,const Description& work) {
    if(!IsFailure(c.result) || (!work.boundTarget.empty() && c.target!=work.boundTarget)) return false;
    if(work.token.operation==Operation::Read) {
        if(c.observed || c.result.publication!=storage::Publication::Unchanged) return false;
        switch(c.result.status) {
        case storage::Status::InvalidPath:case storage::Status::InvalidExisting:case storage::Status::Busy:
        case storage::Status::OpenFailed:case storage::Status::ReadFailed:return true;
        default:return false;
        }
    }
    return !c.observed || ValidRecord(*c.observed,c.target);
}
} // namespace

bool Request::IsCurrent() const {
    const auto state=state_.lock();if(!state) return false;
    std::lock_guard lock(state->mutex);return state->enabled && !state->done;
}
bool Request::CompleteRead(Completion completion) const {
    if(description_->token.operation!=Operation::Read) return false;
    const auto state=state_.lock();if(!state) return false;
    std::lock_guard lock(state->mutex);if(!state->enabled || state->done) return false;
    Deliver(*state,std::move(completion));return true;
}
bool Request::CompleteWriteFailure(Completion completion) const {
    if(description_->token.operation!=Operation::Write) return false;
    const auto state=state_.lock();if(!state) return false;
    std::lock_guard lock(state->mutex);if(!state->enabled || state->done || state->publicationAuthorized) return false;
    Deliver(*state,std::move(completion));return true;
}
bool Request::TryPublishPrepared(const std::function<void()>& publish) const {
    if(description_->token.operation!=Operation::Write || !publish) return false;
    const auto state=state_.lock();if(!state) return false;
    std::lock_guard lock(state->mutex);if(!state->enabled || state->done || state->publicationAuthorized) return false;
    state->publicationAuthorized=true;
    try {publish();}
    catch(...) {
        Completion uncertain;uncertain.target=description_->boundTarget;
        uncertain.result={storage::Status::PublishOutcomeUnknown,storage::Publication::Unknown};
        Deliver(*state,std::move(uncertain));
    }
    return true;
}
bool Request::CompletePublishedWrite(Completion completion) const {
    if(description_->token.operation!=Operation::Write) return false;
    const auto state=state_.lock();if(!state) return false;
    std::lock_guard lock(state->mutex);
    if(!state->enabled || state->done || !state->publicationAuthorized) return false;
    Deliver(*state,std::move(completion));return true;
}
bool Request::PublishWrite(const std::function<Completion()>& publish) const {
    if(!publish) return false;
    Completion completion;
    if(!TryPublishPrepared([&]{completion=publish();})) return false;
    CompletePublishedWrite(std::move(completion));return true;
}

Coordinator::Coordinator():owner_(OwnerIdentity()){}
Coordinator::~Coordinator(){Revoke();}
void Coordinator::Revoke() {
    if(pending_) {std::lock_guard lock(pending_->mutex);pending_->enabled=false;pending_->completion.reset();}
    pending_.reset();pendingDescription_.reset();prepared_.reset();state_.preparedRead.reset();
}
bool Coordinator::OpenSession(TargetId target) {
    if(!owner_ || !target || sessionSequence_==UINT64_MAX) return false;
    Revoke();state_={};state_.session=++sessionSequence_;state_.target=target;
    state_.phase=Phase::NeedsRead;baselineReady_=false;boundTarget_.clear();return true;
}
void Coordinator::Close() {Revoke();state_={};baselineReady_=false;boundTarget_.clear();}
void Coordinator::Cancel() {
    Revoke();baselineReady_=false;
    if(state_.phase!=Phase::Closed) {state_.phase=Phase::NeedsRead;state_.error=Error::None;}
}
bool Coordinator::NoteCheckpoint(const Checkpoint& checkpoint) {
    if(state_.phase==Phase::Closed || !Validate(checkpoint) || state_.logicalRevision==UINT64_MAX) return false;
    if(state_.phase==Phase::Reading || state_.phase==Phase::AwaitingReadDecision) Cancel();
    state_.logicalCheckpoint=checkpoint;++state_.logicalRevision;return true;
}
std::optional<Request> Coordinator::Start(Operation operation) {
    if(state_.phase==Phase::Closed || requestSequence_==UINT64_MAX) return std::nullopt;
    auto work=std::make_shared<Description>();
    work->token={owner_,state_.session,++requestSequence_,state_.logicalRevision,state_.target,operation};
    work->boundTarget=boundTarget_;
    if(operation==Operation::Write) {work->candidate=state_.logicalCheckpoint;work->expected=state_.confirmed;}
    pendingDescription_=std::move(work);pending_=std::make_shared<TicketState>();
    state_.phase=operation==Operation::Read?Phase::Reading:Phase::Writing;state_.error=Error::None;
    return Request(pendingDescription_,pending_);
}
std::optional<Request> Coordinator::BeginRead() {
    if(state_.phase==Phase::Closed || requestSequence_==UINT64_MAX) return std::nullopt;
    Cancel();return Start(Operation::Read);
}
std::optional<Request> Coordinator::BeginWrite() {
    if(state_.phase!=Phase::Ready || !baselineReady_ || !state_.logicalCheckpoint) return std::nullopt;
    return Start(Operation::Write);
}
bool Coordinator::Poll() {
    if(!pending_ || !pendingDescription_) return false;
    Completion completion;bool authorized=false,malformed=false;
    {std::lock_guard lock(pending_->mutex);
        if(!pending_->done || !pending_->completion) return false;
        completion=std::move(*pending_->completion);authorized=pending_->publicationAuthorized;
        malformed=pending_->malformed;pending_->enabled=false;
    }
    const auto work=pendingDescription_;pending_.reset();pendingDescription_.reset();
    state_.lastResult=completion.result;
    if(work->token.operation==Operation::Read && !malformed && ReadReceipt(completion,*work)) {
        prepared_=std::move(completion);state_.preparedRead=work->token;
        state_.phase=Phase::AwaitingReadDecision;return true;
    }
    if(work->token.operation==Operation::Write && !malformed && WriteReceipt(completion,*work,authorized)) {
        state_.confirmed=std::move(completion.observed);state_.acknowledgedRevision=work->token.revision;
        state_.durability=completion.result.durability;baselineReady_=true;state_.phase=Phase::Ready;return true;
    }
    state_.error=!malformed && FailureReceipt(completion,*work)?Error::StorageFailure:Error::MalformedReceipt;
    baselineReady_=false;state_.phase=Phase::Failed;return true;
}
bool Coordinator::CurrentRead(Token token) const {
    return state_.phase==Phase::AwaitingReadDecision && prepared_ && state_.preparedRead &&
        token==*state_.preparedRead && token.owner==owner_ && token.session==state_.session &&
        token.target==state_.target && token.revision==state_.logicalRevision && token.operation==Operation::Read;
}
std::optional<storage::StoredCheckpoint> Coordinator::ReadCandidate(Token token) const {
    return CurrentRead(token)?prepared_->observed:std::nullopt;
}
bool Coordinator::AcceptRead(Token token,bool adopt) {
    if(!CurrentRead(token) || (adopt && (!prepared_->observed || state_.logicalRevision==UINT64_MAX))) return false;
    // Allocate/copy offside. A caller may couple adoption with a no-throw
    // simulation/camera swap; allocation failure must leave this decision live.
    auto next=state_;auto target=prepared_->target;next.confirmed=prepared_->observed;
    next.durability=storage::Durability::None;next.acknowledgedRevision=0;
    if(adopt) {next.logicalCheckpoint=prepared_->observed->checkpoint;++next.logicalRevision;}
    if(next.logicalCheckpoint && next.confirmed) {
        std::vector<std::byte> logical;codec::Error error{};
        if(codec::Encode(*next.logicalCheckpoint,logical,error) && logical==next.confirmed->encoded)
            next.acknowledgedRevision=next.logicalRevision;
    }
    next.preparedRead.reset();next.phase=Phase::Ready;next.error=Error::None;
    state_=std::move(next);boundTarget_=std::move(target);
    baselineReady_=true;prepared_.reset();return true;
}
bool Coordinator::UseReadAsWriteBaseline(Token token) {return AcceptRead(token,false);}
bool Coordinator::AdoptRead(Token token) {return AcceptRead(token,true);}
Snapshot Coordinator::State() const {
    auto snapshot=state_;
    snapshot.currentCheckpointStored=baselineReady_ && snapshot.logicalCheckpoint && snapshot.confirmed &&
        snapshot.logicalRevision==snapshot.acknowledgedRevision;
    return snapshot;
}
} // namespace horde::gameplay::campaign::async
