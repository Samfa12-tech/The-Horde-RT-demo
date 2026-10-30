#include "gameplay/campaign/CampaignSaveWorker.h"

namespace horde::gameplay::campaign::async {
Worker::Worker(std::filesystem::path directory,storage::FaultInjector* faults)
    :directory_(std::move(directory)),faults_(faults),thread_([this]{Run();}){}
Worker::~Worker(){Stop();}
bool Worker::Submit(Request request) {
    std::lock_guard lock(mutex_);
    if(stopping_ || queued_ || !request.IsCurrent() || SaveBasename(request.Work().token.target).empty()) return false;
    queued_=std::move(request);wake_.notify_one();return true;
}
void Worker::Stop() {
    {std::lock_guard lock(mutex_);stopping_=true;queued_.reset();}
    wake_.notify_one();if(thread_.joinable()) thread_.join();
}
bool Worker::Idle() const {std::lock_guard lock(mutex_);return !active_ && !queued_;}
void Worker::Run() {
    for(;;) {
        std::optional<Request> request;
        {std::unique_lock lock(mutex_);wake_.wait(lock,[&]{return stopping_ || queued_.has_value();});
            if(stopping_) return;
            request=std::move(queued_);queued_.reset();active_=true;
        }
        try {Execute(*request);}
        catch(...) {
            // If publication already began, absence of a final receipt is
            // uncertain. Never acknowledge or blindly retry that operation.
            Completion c;c.target=request->Work().boundTarget;
            c.result={storage::Status::PublishOutcomeUnknown,storage::Publication::Unknown};
            if(!request->CompletePublishedWrite(c)) {
                c.result={storage::Status::ReadFailed};
                if(request->Work().token.operation==Operation::Read) request->CompleteRead(std::move(c));
                else request->CompleteWriteFailure(std::move(c));
            }
        }
        {std::lock_guard lock(mutex_);active_=false;}
    }
}
void Worker::Execute(const Request& request) {
    if(!request.IsCurrent()) return;
    const auto& work=request.Work();const auto name=SaveBasename(work.token.target);
    std::error_code ec;const auto directory=std::filesystem::absolute(directory_,ec).lexically_normal();
    Completion c;c.target=ec?std::filesystem::path{}:directory/std::string(name);
    if(ec || directory_.empty() || name.empty() || (!work.boundTarget.empty() && c.target!=work.boundTarget)) {
        c.result={storage::Status::InvalidPath};
        if(work.token.operation==Operation::Read) request.CompleteRead(std::move(c));
        else request.CompleteWriteFailure(std::move(c));
        return;
    }
    if(work.token.operation==Operation::Read) {
        storage::StoredCheckpoint found;c.result=storage::Read(directory,name,found,faults_);
        if(c.result.status==storage::Status::Read) c.observed=std::move(found);
        request.CompleteRead(std::move(c));return;
    }
    if(!work.candidate) {c.result={storage::Status::InvalidCandidate};request.CompleteWriteFailure(std::move(c));return;}
    storage::PreparedWrite prepared;
    c.result=storage::Prepare(directory,name,*work.candidate,work.expected,prepared,faults_);
    const bool unchanged=c.result.status==storage::Status::AlreadyCurrent;
    if(c.result.status!=storage::Status::Prepared && !unchanged) {request.CompleteWriteFailure(std::move(c));return;}
    // The gate contains the single OS name cutover only. File writes/flushes,
    // handle opening, readback, directory fsync and cleanup occur off-gate.
    if(!request.TryPublishPrepared([&]{if(!unchanged) prepared.Publish();})) return;
    if(!unchanged) c.result=prepared.Finish();
    if(c.result.status==storage::Status::Committed || unchanged) {
        std::vector<std::byte> bytes;codec::Error error{};
        if(codec::Encode(*work.candidate,bytes,error)) c.observed=storage::StoredCheckpoint{*work.candidate,std::move(bytes),c.target};
    }
    request.CompletePublishedWrite(std::move(c));
}
} // namespace horde::gameplay::campaign::async
