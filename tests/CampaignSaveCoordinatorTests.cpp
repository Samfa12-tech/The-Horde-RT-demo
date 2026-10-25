#include "gameplay/campaign/CampaignSaveCoordinator.h"
#include "gameplay/campaign/CampaignSession.h"
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>

namespace cp=horde::gameplay::campaign;
namespace io=cp::storage;
namespace save=cp::async;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message) {++checks;if(!value) {++failures;std::cerr<<message<<'\n';}}
cp::Checkpoint Checkpoint(int health=3) {cp::Checkpoint c;c.vitality=health;return c;}
std::vector<std::byte> Bytes(const cp::Checkpoint& c) {
    std::vector<std::byte> bytes;cp::codec::Error error;
    Check(cp::codec::Encode(c,bytes,error),"test logical checkpoint encodes");return bytes;
}
io::StoredCheckpoint Stored(const cp::Checkpoint& c,const std::filesystem::path& path) {return {c,Bytes(c),path};}
save::Completion Read(const io::StoredCheckpoint& stored) {
    return {{io::Status::Read,io::Publication::Unchanged,io::Durability::None},stored.target,stored};
}
save::Completion Missing(const std::filesystem::path& path) {
    return {{io::Status::NotFound,io::Publication::Unchanged,io::Durability::None},path,{}};
}
save::Completion Failure(const std::filesystem::path& path,io::Status status=io::Status::WriteFailed) {
    return {{status,io::Publication::Unchanged,io::Durability::None},path,{}};
}
save::Completion Written(const save::Request& request,io::Durability durability=io::Durability::FileFlushed) {
    const auto& work=request.Work();
    return {{io::Status::Committed,io::Publication::Published,durability},work.boundTarget,
        Stored(*work.candidate,work.boundTarget)};
}
struct FakeBackend {
    std::filesystem::path path="synthetic-save-root/checkpoint.bin";
    std::optional<io::StoredCheckpoint> stored;
    unsigned publications=0;
    save::Completion ReadNow() const {return stored?Read(*stored):Missing(path);}
    save::Completion Publish(const save::Request& request,io::Durability durability=io::Durability::FileFlushed) {
        const auto& work=request.Work();
        const bool matches=work.boundTarget==path && bool(work.expected)==bool(stored) &&
            (!stored || (work.expected->encoded==stored->encoded && work.expected->target==stored->target));
        if(!matches) return Failure(path,io::Status::Conflict);
        stored=Stored(*work.candidate,path);++publications;
        return {{io::Status::Committed,io::Publication::Published,durability},path,stored};
    }
};
std::optional<save::Request> Prepare(save::Coordinator& owner,const save::Completion& receipt,bool adopt=false) {
    const auto read=owner.BeginRead();Check(bool(read),"owner creates immutable read work");
    if(!read)return {};
    const auto before=owner.State();
    Check(read->CompleteRead(receipt),"worker deposits one read completion");
    Check(owner.State().phase==before.phase && !owner.State().preparedRead,
        "worker completion alone cannot apply owner state");
    Check(owner.Poll() && owner.State().phase==save::Phase::AwaitingReadDecision,
        "owner polling prepares a valid read for an explicit decision");
    const auto candidate=owner.ReadCandidate(read->Work().token);
    Check(receipt.observed ? candidate && candidate->encoded==receipt.observed->encoded &&
        candidate->target==receipt.target : !candidate,
        "current found read exposes validated offside candidate; NotFound exposes none");
    Check(owner.State().logicalRevision==before.logicalRevision,
        "inspecting the prepared candidate does not adopt or advance logical gameplay");
    Check(adopt?owner.AdoptRead(read->Work().token):owner.UseReadAsWriteBaseline(read->Work().token),
        "matching owner read decision accepts the prepared data");
    Check(!owner.ReadCandidate(read->Work().token),"consumed read decision no longer exposes a load candidate");
    return read;
}
void TestReadDecisions() {
    FakeBackend backend;save::Coordinator owner;
    Check(!owner.OpenSession(0),"zero logical target cannot open a session");
    Check(owner.OpenSession(11) && owner.State().phase==save::Phase::NeedsRead,"logical target opens without filesystem policy");
    Check(!owner.BeginWrite(),"write requires both a logical checkpoint and an accepted read baseline");
    Check(owner.NoteCheckpoint(Checkpoint(2)),"owner notes a valid logical checkpoint");
    const auto revision=owner.State().logicalRevision;
    auto invalid=Checkpoint();invalid.schema=99;
    Check(!owner.NoteCheckpoint(invalid),"unknown checkpoint schema cannot become logical gameplay");
    invalid=Checkpoint();invalid.yaw=std::numeric_limits<float>::infinity();
    Check(!owner.NoteCheckpoint(invalid) && owner.State().logicalRevision==revision &&
        owner.State().logicalCheckpoint->vitality==2,"invalid logical values preserve the previous checkpoint revision");
    Check(!owner.BeginWrite(),"logical checkpoint alone cannot assume create or overwrite permission");
    const auto read=owner.BeginRead();Check(bool(read),"missing target read starts");
    if(!read)return;
    Check(read->CompleteRead(backend.ReadNow()) && owner.Poll(),"missing target receipt reaches the owner");
    auto wrong=read->Work().token;++wrong.request;
    Check(!owner.UseReadAsWriteBaseline(wrong) && !owner.AdoptRead(wrong),"stale read decision cannot select a prepared receipt");
    Check(!owner.AdoptRead(read->Work().token),"NotFound cannot manufacture loaded gameplay");
    Check(owner.UseReadAsWriteBaseline(read->Work().token),"NotFound explicitly authorizes a create-only baseline");
    Check(owner.State().logicalCheckpoint->vitality==2 && !owner.State().confirmed && !owner.State().currentCheckpointStored,
        "baseline decision preserves current gameplay and records no existing checkpoint");
    auto write=owner.BeginWrite();Check(write && !write->Work().expected && write->Work().boundTarget==backend.path,
        "create work binds the backend-resolved missing target without inventing slot paths");
    owner.Cancel();
    backend.stored=Stored(Checkpoint(1),backend.path);
    const auto adopted=Prepare(owner,backend.ReadNow(),true);
    Check(adopted && owner.State().logicalCheckpoint->vitality==1 && owner.State().confirmed &&
        owner.State().currentCheckpointStored && owner.State().logicalRevision==owner.State().acknowledgedRevision,
        "explicit adoption exposes loaded logical data and its acknowledged revision");
    Check(owner.NoteCheckpoint(Checkpoint(3)),"new gameplay checkpoint can follow adoption");
    Prepare(owner,backend.ReadNow());
    Check(owner.State().logicalCheckpoint->vitality==3 && owner.State().confirmed->checkpoint.vitality==1 &&
        !owner.State().currentCheckpointStored,"write baseline never silently adopts a different saved checkpoint");
}
void TestWriteRevisions() {
    FakeBackend backend;save::Coordinator owner;owner.OpenSession(12);owner.NoteCheckpoint(Checkpoint(1));
    Prepare(owner,backend.ReadNow());const auto a=owner.BeginWrite();Check(bool(a),"candidate A starts against create baseline");
    if(!a)return;
    const auto aRevision=a->Work().token.revision;
    Check(!owner.BeginWrite() && a->IsCurrent(),"second write cannot supersede an in-flight publication");
    owner.NoteCheckpoint(Checkpoint(2));
    Check(a->Work().candidate->vitality==1 && owner.State().logicalRevision>aRevision,
        "immutable work A retains its value when gameplay advances to B");
    Check(a->PublishWrite([&]{return backend.Publish(*a);}),"current write publishes inside its single-use gate");
    Check(backend.publications==1 && owner.State().phase==save::Phase::Writing && !owner.State().confirmed,
        "authorized fake storage mutation does not acknowledge gameplay before owner Poll");
    Check(!a->PublishWrite([&]{return backend.Publish(*a);}) && backend.publications==1,"duplicate write callback cannot publish twice");
    Check(owner.Poll() && owner.State().phase==save::Phase::Ready && owner.State().confirmed->checkpoint.vitality==1 &&
        owner.State().acknowledgedRevision==aRevision && !owner.State().currentCheckpointStored &&
        owner.State().logicalCheckpoint->vitality==2 && owner.State().durability==io::Durability::FileFlushed,
        "A success confirms A while newer gameplay B remains unsaved");
    Check(!owner.Poll(),"receipt is consumed once");
    const auto b=owner.BeginWrite();Check(b && b->Work().expected && b->Work().expected->checkpoint.vitality==1 &&
        b->Work().candidate->vitality==2,"next B write expects precisely the confirmed A bytes");
    if(!b)return;
    Check(b->PublishWrite([&]{return backend.Publish(*b,io::Durability::FileAndDirectoryFlushed);}) && owner.Poll() &&
        owner.State().currentCheckpointStored && owner.State().durability==io::Durability::FileAndDirectoryFlushed,
        "B completion acknowledges its own revision and retains stronger durability evidence");
    const auto unchanged=owner.BeginWrite();Check(bool(unchanged),"already-current checkpoint may request an idempotent write");
    if(unchanged) {
        Check(unchanged->PublishWrite([&]{return save::Completion{
            {io::Status::AlreadyCurrent,io::Publication::Unchanged,io::Durability::None},backend.path,backend.stored};}) && owner.Poll(),
            "AlreadyCurrent can acknowledge canonical current bytes");
        Check(owner.State().currentCheckpointStored && owner.State().durability==io::Durability::None && backend.publications==2,
            "AlreadyCurrent claims no new file or directory flush");
    }
}
void TestReadRevocation() {
    FakeBackend backend;backend.stored=Stored(Checkpoint(1),backend.path);
    save::Coordinator owner,other;owner.OpenSession(21);other.OpenSession(21);
    const auto first=owner.BeginRead(),foreign=other.BeginRead(),second=owner.BeginRead();
    Check(first && foreign && second && first->Work().token.owner!=foreign->Work().token.owner,
        "coordinator instances use distinct owner identities even for the same logical target");
    if(!first||!foreign||!second)return;
    Check(!first->IsCurrent() && !first->CompleteRead(backend.ReadNow()),"superseded delayed read cannot deposit a receipt");
    Check(second->CompleteRead(backend.ReadNow()) && owner.Poll(),"newest read prepares successfully");
    Check(owner.ReadCandidate(second->Work().token) && !owner.ReadCandidate(first->Work().token) &&
        !owner.ReadCandidate(foreign->Work().token),"prepared candidate requires the current owner/session/request token");
    Check(!owner.AdoptRead(first->Work().token) && !owner.AdoptRead(foreign->Work().token),"stale and foreign owner read tokens cannot adopt current data");
    Check(owner.AdoptRead(second->Work().token),"current owner decision remains usable after stale attempts");
    const auto pending=owner.BeginRead();owner.NoteCheckpoint(Checkpoint(2));
    Check(pending && !pending->CompleteRead(backend.ReadNow()) && !owner.Poll() && owner.State().logicalCheckpoint->vitality==2,
        "new logical checkpoint revokes a pending read so delayed load cannot replace gameplay");
    const auto prepared=owner.BeginRead();
    Check(prepared && prepared->CompleteRead(backend.ReadNow()) && owner.Poll(),"read decision race setup prepares data");
    owner.NoteCheckpoint(Checkpoint(3));
    Check(prepared && !owner.AdoptRead(prepared->Work().token) && owner.State().phase==save::Phase::NeedsRead,
        "new logical checkpoint also revokes an already prepared load decision");
    const auto session=owner.BeginRead();const auto oldSession=owner.State().session;owner.OpenSession(22);
    Check(session && !session->CompleteRead(backend.ReadNow()) && !owner.Poll() && owner.State().target==22 &&
        owner.State().session!=oldSession && !owner.State().logicalCheckpoint && !owner.State().confirmed,
        "opening another target starts a fresh session and revokes old load callbacks");
    const auto closed=owner.BeginRead();owner.Close();
    Check(closed && !closed->CompleteRead(backend.ReadNow()) && !owner.Poll() && owner.State().phase==save::Phase::Closed,
        "closing revokes delayed reads");
    std::optional<save::Request> destroyed;
    {save::Coordinator temporary;temporary.OpenSession(23);destroyed=temporary.BeginRead();}
    Check(destroyed && !destroyed->IsCurrent() && !destroyed->CompleteRead(backend.ReadNow()),
        "weak worker read tickets cannot capture a destroyed coordinator");
}
void TestFailures() {
    FakeBackend backend;backend.stored=Stored(Checkpoint(3),backend.path);
    save::Coordinator owner;owner.OpenSession(31);owner.NoteCheckpoint(Checkpoint(1));
    const auto failedRead=owner.BeginRead();
    Check(failedRead && failedRead->CompleteRead(Failure(backend.path,io::Status::ReadFailed)) && owner.Poll() &&
        owner.State().phase==save::Phase::Failed && owner.State().error==save::Error::StorageFailure &&
        owner.State().logicalCheckpoint->vitality==1 && !owner.BeginWrite(),
        "ordinary read failure retains logical gameplay and cannot authorize writing");
    Check(failedRead && !failedRead->CompleteRead(backend.ReadNow()),"failed request cannot later deliver a replacement success");
    Prepare(owner,backend.ReadNow());
    auto write=owner.BeginWrite();Check(bool(write),"failure test write starts");if(!write)return;
    const auto before=owner.State();
    Check(write->CompleteWriteFailure(Failure(backend.path)) && owner.Poll(),"queue/preparation failure is consumed on owner thread");
    Check(owner.State().phase==save::Phase::Failed && owner.State().error==save::Error::StorageFailure &&
        owner.State().logicalCheckpoint->vitality==1 && owner.State().confirmed->encoded==before.confirmed->encoded &&
        owner.State().acknowledgedRevision==before.acknowledgedRevision && !owner.BeginWrite(),
        "failed write preserves logical and confirmed data but requires a fresh read before retry");
    Prepare(owner,backend.ReadNow());write=owner.BeginWrite();
    Check(write && write->PublishWrite([&]{return backend.Publish(*write);}) && owner.Poll() && owner.State().currentCheckpointStored,
        "fresh accepted read enables a successful explicit retry");
    for(auto status:{io::Status::PublishedDurabilityUnknown,io::Status::PublishOutcomeUnknown}) {
        owner.NoteCheckpoint(Checkpoint(2));write=owner.BeginWrite();Check(bool(write),"uncertain publication setup starts");if(!write)continue;
        const auto ack=owner.State().acknowledgedRevision;
        Check(write->PublishWrite([&]{auto c=Written(*write);c.result.status=status;
            c.result.publication=status==io::Status::PublishedDurabilityUnknown?io::Publication::Published:io::Publication::Unknown;
            c.result.durability=io::Durability::None;return c;}) && owner.Poll(),"uncertain publication receipt is delivered");
        Check(owner.State().phase==save::Phase::Failed && !owner.State().currentCheckpointStored &&
            owner.State().acknowledgedRevision==ack && !owner.BeginWrite(),"uncertain publication never acknowledges or automatically retries");
        Prepare(owner,backend.ReadNow());
    }
    write=owner.BeginWrite();Check(bool(write),"forged success setup starts");
    if(write) {
        const auto ack=owner.State().acknowledgedRevision;
        write->CompleteWriteFailure(Written(*write));
        Check(owner.Poll() && owner.State().error==save::Error::MalformedReceipt && owner.State().acknowledgedRevision==ack,
            "positive receipt forged through failure-only channel cannot acknowledge unpublished data");
    }
    Prepare(owner,backend.ReadNow());write=owner.BeginWrite();
    if(write) {
        Check(write->PublishWrite([]()->save::Completion {throw 7;}) && owner.Poll() &&
            owner.State().phase==save::Phase::Failed && !owner.BeginWrite(),
            "publication callback exception reports uncertain failure and requires reread");
    }
}
void TestMalformedReceipts() {
    FakeBackend backend;backend.stored=Stored(Checkpoint(3),backend.path);
    const auto rejectRead=[&](save::Completion receipt) {
        save::Coordinator owner;owner.OpenSession(41);owner.NoteCheckpoint(Checkpoint(2));const auto original=owner.State();
        const auto request=owner.BeginRead();if(!request){Check(false,"malformed read setup starts");return;}
        request->CompleteRead(std::move(receipt));
        Check(owner.Poll() && owner.State().phase==save::Phase::Failed && owner.State().error==save::Error::MalformedReceipt &&
            owner.State().logicalCheckpoint->vitality==2 && !owner.State().confirmed &&
            owner.State().acknowledgedRevision==original.acknowledgedRevision && !owner.AdoptRead(request->Work().token),
            "inconsistent read status/path/data cannot become a baseline or loaded gameplay");
    };
    auto bad=Read(*backend.stored);bad.result.publication=io::Publication::Published;rejectRead(bad);
    bad=Read(*backend.stored);bad.result.durability=io::Durability::FileFlushed;rejectRead(bad);
    bad=Read(*backend.stored);bad.observed.reset();rejectRead(bad);
    bad=Missing(backend.path);bad.observed=backend.stored;rejectRead(bad);
    bad=Read(*backend.stored);bad.target.clear();rejectRead(bad);
    bad=Read(*backend.stored);bad.observed->target="different-synthetic-target";rejectRead(bad);
    bad=Read(*backend.stored);bad.observed->encoded[20]^=std::byte{1};rejectRead(bad);
    bad=Read(*backend.stored);bad.observed->checkpoint.vitality=1;rejectRead(bad);
    bad=Read(*backend.stored);bad.observed->checkpoint.schema=99;rejectRead(bad);
    bad=Read(*backend.stored);bad.observed->checkpoint.yaw=std::numeric_limits<float>::quiet_NaN();rejectRead(bad);
    bad=Read(*backend.stored);bad.result.status=static_cast<io::Status>(999);rejectRead(bad);
    rejectRead(Failure(backend.path,io::Status::WriteFailed));
    {
        save::Coordinator owner;owner.OpenSession(43);owner.NoteCheckpoint(Checkpoint(1));Prepare(owner,backend.ReadNow());
        const auto request=owner.BeginWrite();Check(bool(request),"known-target failure identity setup starts");
        if(request) {
            const auto ack=owner.State().acknowledgedRevision;
            request->CompleteWriteFailure(Failure("wrong-synthetic-target"));
            Check(owner.Poll() && owner.State().error==save::Error::MalformedReceipt &&
                owner.State().acknowledgedRevision==ack && owner.State().confirmed->encoded==backend.stored->encoded &&
                !owner.BeginWrite(),"failure for a different bound target is malformed and cannot authorize retry");
        }
    }
    for(int mutation=0;mutation<5;++mutation) {
        save::Coordinator owner;owner.OpenSession(42);owner.NoteCheckpoint(Checkpoint(1));Prepare(owner,backend.ReadNow());
        const auto request=owner.BeginWrite();if(!request){Check(false,"malformed write setup starts");continue;}
        const auto ack=owner.State().acknowledgedRevision;
        request->PublishWrite([&]{auto receipt=Written(*request);
            if(mutation==0)receipt.target="wrong-synthetic-target";
            if(mutation==1)receipt.observed=Stored(Checkpoint(2),backend.path);
            if(mutation==2)receipt.result.publication=io::Publication::Unchanged;
            if(mutation==3)receipt.observed.reset();
            if(mutation==4)receipt.result.durability=static_cast<io::Durability>(999);
            return receipt;});
        Check(owner.Poll() && owner.State().error==save::Error::MalformedReceipt &&
            owner.State().acknowledgedRevision==ack && !owner.State().currentCheckpointStored && !owner.BeginWrite(),
            "wrong candidate/path or inconsistent publication receipt cannot acknowledge a write");
    }
    for(bool existing:{false,true}) {
        FakeBackend noOpBackend;
        if(existing)noOpBackend.stored=Stored(Checkpoint(3),noOpBackend.path);
        save::Coordinator owner;owner.OpenSession(45);owner.NoteCheckpoint(Checkpoint(1));Prepare(owner,noOpBackend.ReadNow());
        const auto request=owner.BeginWrite();Check(bool(request),"invalid no-op setup starts with create-only or different existing baseline");
        if(!request)continue;
        const auto before=owner.State();
        request->PublishWrite([&]{auto receipt=Written(*request);receipt.result={io::Status::AlreadyCurrent,
            io::Publication::Unchanged,io::Durability::None};return receipt;});
        Check(owner.Poll() && owner.State().error==save::Error::MalformedReceipt &&
            owner.State().acknowledgedRevision==before.acknowledgedRevision && !owner.State().currentCheckpointStored &&
            bool(owner.State().confirmed)==existing && (!existing || owner.State().confirmed->encoded==before.confirmed->encoded) &&
            noOpBackend.publications==0 && !owner.BeginWrite(),
            "AlreadyCurrent cannot acknowledge create-only work or candidate bytes that differ from the expected existing version");
    }
}
void TestExternalConflict() {
    FakeBackend backend;backend.stored=Stored(Checkpoint(3),backend.path);
    save::Coordinator owner;owner.OpenSession(44);owner.NoteCheckpoint(Checkpoint(1));Prepare(owner,backend.ReadNow());
    const auto request=owner.BeginWrite();Check(bool(request),"external conflict setup binds the observed existing version");
    if(!request)return;
    const auto before=owner.State();
    backend.stored=Stored(Checkpoint(2),backend.path);const auto externalBytes=backend.stored->encoded;
    Check(request->PublishWrite([&]{return backend.Publish(*request);}) && owner.Poll(),"delayed write discovers an externally newer fake record");
    Check(owner.State().lastResult.status==io::Status::Conflict && owner.State().error==save::Error::StorageFailure &&
        owner.State().phase==save::Phase::Failed && owner.State().acknowledgedRevision==before.acknowledgedRevision &&
        owner.State().confirmed->encoded==before.confirmed->encoded && owner.State().logicalCheckpoint->vitality==1 &&
        backend.publications==0 && backend.stored->encoded==externalBytes && !owner.BeginWrite(),
        "conflict preserves the external version, logical candidate and confirmed baseline without acknowledgement or retry");
    Prepare(owner,backend.ReadNow());
    const auto retry=owner.BeginWrite();
    Check(retry && retry->Work().expected && retry->Work().expected->encoded==externalBytes &&
        retry->Work().candidate->vitality==1,"only a fresh explicitly accepted read can bind retry to the externally newer record");
}
void TestWorkerConcurrency() {
    FakeBackend backend;save::Coordinator owner;owner.OpenSession(51);owner.NoteCheckpoint(Checkpoint(1));Prepare(owner,backend.ReadNow());
    const auto write=owner.BeginWrite();Check(bool(write),"concurrent publication setup starts");if(!write)return;
    std::promise<void> start;auto go=start.get_future().share();std::atomic<unsigned> accepted=0,mutations=0;
    std::vector<std::thread> workers;
    const auto receipt=Written(*write);
    for(int i=0;i<4;++i)workers.emplace_back([&,request=*write]{go.wait();
        if(request.PublishWrite([&]{++mutations;return receipt;}))++accepted;});
    start.set_value();for(auto& worker:workers)worker.join();
    Check(accepted==1 && mutations==1 && owner.State().phase==save::Phase::Writing && !owner.State().confirmed,
        "competing workers publish at most once and never apply owner state");
    Check(owner.Poll() && owner.State().currentCheckpointStored && !owner.Poll(),"owner consumes concurrent completion exactly once");
    const auto read=owner.BeginRead();Check(bool(read),"concurrent read completion starts");if(!read)return;
    accepted=0;workers.clear();const auto saved=Stored(Checkpoint(3),backend.path);const auto readReceipt=Read(saved);
    for(int i=0;i<4;++i)workers.emplace_back([&,request=*read]{if(request.CompleteRead(readReceipt))++accepted;});
    for(auto& worker:workers)worker.join();
    Check(accepted==1 && owner.State().phase==save::Phase::Reading && owner.Poll(),"competing read callbacks deposit one owner-consumed receipt");
    Check(owner.UseReadAsWriteBaseline(read->Work().token),"concurrent read result still needs explicit owner acceptance");
}
void TestPublicationRevocation() {
    FakeBackend backend;save::Coordinator owner;owner.OpenSession(61);owner.NoteCheckpoint(Checkpoint(1));Prepare(owner,backend.ReadNow());
    auto request=owner.BeginWrite();Check(bool(request),"revoked publication setup starts");if(!request)return;
    owner.Cancel();unsigned mutations=0;
    Check(!request->PublishWrite([&]{++mutations;return Written(*request);}) && mutations==0 && !owner.Poll(),
        "revocation before publication gate prevents every fake backend mutation");
    Prepare(owner,backend.ReadNow());request=owner.BeginWrite();Check(bool(request),"crossed publication setup starts");if(!request)return;
    const auto receipt=Written(*request);
    std::promise<void> entered,release,closeRequested;auto enteredFuture=entered.get_future();
    auto releaseFuture=release.get_future().share();auto closeFuture=closeRequested.get_future();
    std::atomic<unsigned> published=0;std::atomic<bool> accepted=false;
    std::thread worker([&,ticket=*request]{accepted=ticket.PublishWrite([&]{entered.set_value();releaseFuture.wait();++published;return receipt;});});
    const bool crossed=enteredFuture.wait_for(std::chrono::seconds(5))==std::future_status::ready;
    Check(crossed,"fake publisher reaches the authorized gate before close");
    // Only this thread calls owner lifecycle APIs. A helper releases the
    // bounded fake publication after the owner announces its Close attempt.
    std::thread releaser([&]{closeFuture.wait();release.set_value();});
    closeRequested.set_value();owner.Close();worker.join();releaser.join();
    Check(accepted && published==1 && owner.State().phase==save::Phase::Closed && !owner.Poll(),
        "publication already inside gate may finish once; Close discards its stale receipt");
    Check(!request->PublishWrite([&]{++mutations;return receipt;}),"closed crossed-gate ticket cannot publish again");
    std::optional<save::Request> destroyed;
    {save::Coordinator temporary;temporary.OpenSession(62);temporary.NoteCheckpoint(Checkpoint(2));
        Prepare(temporary,backend.ReadNow());destroyed=temporary.BeginWrite();}
    Check(destroyed && !destroyed->PublishWrite([&]{++mutations;return Written(*destroyed);}) && mutations==0,
        "delayed weak write ticket cannot publish after owner destruction");
    owner.OpenSession(63);owner.NoteCheckpoint(Checkpoint(3));Prepare(owner,backend.ReadNow());
    request=owner.BeginWrite();owner.OpenSession(64);
    Check(request && !request->PublishWrite([&]{++mutations;return Written(*request);}) && mutations==0 && !owner.Poll(),
        "loading another logical target revokes old pending write publication");
}
}
int main() {
    {
        FakeBackend backend;save::Coordinator owner;
        auto old=Checkpoint(1);old.schema=3;old.halfHeart=true;old.flask={true,1};
        backend.stored=Stored(old,backend.path);
        Check(owner.OpenSession(2),"confirmed recovery opens an independent actual coordinator");
        Prepare(owner,backend.ReadNow(),true);
        const auto acknowledged=cp::ConfirmedCampaignRecordFrom(owner.State());
        Check(acknowledged && acknowledged->stored.encoded==backend.stored->encoded &&
            acknowledged->stored.checkpoint.halfHeart && acknowledged->stored.checkpoint.flask.charges==1,
            "adopted exact half health/resources form a confirmed retry record");
        auto live=old;live.vitality=3;live.halfHeart=false;live.flask.charges=3;
        Check(owner.NoteCheckpoint(live) && !cp::ConfirmedCampaignRecordFrom(owner.State()),
            "new logical health/resources are not acknowledged recovery data");
        Prepare(owner,backend.ReadNow());
        const auto write=owner.BeginWrite();
        Check(write && !cp::ConfirmedCampaignRecordFrom(owner.State()),
            "in-flight save cannot promote its logical candidate to recovery");
        Check(write && write->CompleteWriteFailure(Failure(backend.path)) && owner.Poll() &&
            !cp::ConfirmedCampaignRecordFrom(owner.State()) && acknowledged &&
            acknowledged->stored.encoded==backend.stored->encoded,
            "write failure cannot fabricate saved resources or damage the prior confirmed record");
        Prepare(owner,backend.ReadNow());
        const auto next=owner.BeginWrite();
        Check(next && next->PublishWrite([&]{return backend.Publish(*next);}) &&
            !cp::ConfirmedCampaignRecordFrom(owner.State()),
            "worker publication alone still cannot update owner recovery state");
        Check(owner.Poll(),"owner consumes the successful publication receipt");
        const auto updated=cp::ConfirmedCampaignRecordFrom(owner.State());
        Check(updated && updated->stored.checkpoint.flask.charges==3 &&
            !updated->stored.checkpoint.halfHeart && updated->stored.encoded==backend.stored->encoded,
            "only owner-acknowledged publication advances exact recovery state");
        owner.Close();
        Check(!cp::ConfirmedCampaignRecordFrom(owner.State()),"closed ownership cannot supply another game's recovery");
    }
    TestReadDecisions();TestWriteRevisions();TestReadRevocation();TestFailures();
    TestMalformedReceipts();TestExternalConflict();TestWorkerConcurrency();TestPublicationRevocation();
    std::cout<<checks<<" campaign coordinator checks, "<<failures<<" failures\n";
    return failures?1:0;
}
