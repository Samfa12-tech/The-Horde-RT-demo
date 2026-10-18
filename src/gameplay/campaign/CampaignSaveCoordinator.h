#pragma once

#include "gameplay/campaign/CampaignCheckpointStorage.h"
#include <functional>
#include <memory>

namespace horde::gameplay::campaign::async {
using TargetId = std::uint64_t; // Caller-defined logical identity, not a slot/path policy.
enum class Operation { Read, Write };
struct Token {
    std::uint64_t owner=0,session=0,request=0,revision=0;
    TargetId target=0;
    Operation operation=Operation::Read;
    bool operator==(const Token&) const = default;
};
struct Completion {
    storage::Result result;
    std::filesystem::path target; // Backend-resolved identity, even for NotFound.
    std::optional<storage::StoredCheckpoint> observed;
};
struct Description {
    Token token;
    std::filesystem::path boundTarget;
    std::optional<Checkpoint> candidate;
    std::optional<storage::StoredCheckpoint> expected;
};
struct TicketState;
// Immutable work description + weak, single-use return/publication channel.
// Only these methods are worker-thread safe. No callback captures Coordinator.
class Request {
public:
    const Description& Work() const { return *description_; }
    bool IsCurrent() const;
    bool CompleteRead(Completion completion) const;
    // Queue rejection/preparation failure: no publication permission granted.
    bool CompleteWriteFailure(Completion completion) const;
    // Execute at most once, only while the request/session is current. A future
    // real adapter must prepare IO offside and put ONLY final name publication
    // inside this gate, not an entire blocking storage::Write call. Lifecycle
    // revocation serializes with this bounded callback; it must not reenter
    // Coordinator/Request methods. No real IO is wired here.
    bool PublishWrite(const std::function<Completion()>& publish) const;
private:
    friend class Coordinator;
    Request(std::shared_ptr<const Description> description,std::weak_ptr<TicketState> state)
        :description_(std::move(description)),state_(std::move(state)){}
    std::shared_ptr<const Description> description_;
    std::weak_ptr<TicketState> state_;
};
enum class Phase { Closed, NeedsRead, Reading, AwaitingReadDecision, Ready, Writing, Failed };
enum class Error { None, StorageFailure, MalformedReceipt };
struct Snapshot {
    TargetId target=0;
    std::uint64_t session=0,logicalRevision=0,acknowledgedRevision=0;
    Phase phase=Phase::Closed;
    Error error=Error::None;
    storage::Result lastResult;
    std::optional<Checkpoint> logicalCheckpoint;
    std::optional<storage::StoredCheckpoint> confirmed;
    std::optional<Token> preparedRead;
    storage::Durability durability=storage::Durability::None;
    bool currentCheckpointStored=false;
};
// Owner-thread state only; no threads, filesystem calls, game-state mutation,
// automatic retry or menu/overwrite policy. Poll is the only completion consumer.
class Coordinator {
public:
    Coordinator();
    ~Coordinator();
    Coordinator(const Coordinator&)=delete;
    Coordinator& operator=(const Coordinator&)=delete;
    bool OpenSession(TargetId target);
    void Close();
    void Cancel();
    bool NoteCheckpoint(const Checkpoint& checkpoint);
    std::optional<Request> BeginRead(); // Supersedes/revokes an older request.
    std::optional<Request> BeginWrite(); // Requires an explicitly accepted read baseline.
    bool Poll(); // No completion or duplicate/stale callback => false.
    // Copy for offside simulation/resource preparation; no adoption or IO.
    std::optional<storage::StoredCheckpoint> ReadCandidate(Token token) const;
    bool UseReadAsWriteBaseline(Token token); // Keeps current logical checkpoint.
    bool AdoptRead(Token token); // Returns logical loaded data only; no GameSimulation call.
    Snapshot State() const;
private:
    void Revoke();
    std::optional<Request> Start(Operation operation);
    bool AcceptRead(Token token,bool adopt);
    bool CurrentRead(Token token) const;
    const std::uint64_t owner_;
    std::uint64_t sessionSequence_=0,requestSequence_=0;
    Snapshot state_;
    bool baselineReady_=false;
    std::filesystem::path boundTarget_;
    std::shared_ptr<TicketState> pending_;
    std::shared_ptr<const Description> pendingDescription_;
    std::optional<Completion> prepared_;
};
} // namespace horde::gameplay::campaign::async
