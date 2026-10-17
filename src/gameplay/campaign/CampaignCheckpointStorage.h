#pragma once

#include "gameplay/campaign/CampaignCheckpointCodec.h"
#include <filesystem>
#include <optional>
#include <string_view>

namespace horde::gameplay::campaign::storage {
enum class Status {
    Committed, AlreadyCurrent, Read, NotFound, InvalidPath, InvalidCandidate, InvalidExisting,
    InvalidExpectation, Conflict, Busy, OpenFailed, ReadFailed, WriteFailed,
    FlushFailed, CloseFailed, PublishFailed, PublishedDurabilityUnknown, PublishOutcomeUnknown
};
enum class Publication { Unchanged, Published, Unknown };
enum class Durability { None, FileFlushed, FileAndDirectoryFlushed };
struct Result {
    Status status = Status::ReadFailed;
    Publication publication = Publication::Unchanged;
    Durability durability = Durability::None;
    codec::Error codecError = codec::Error::None;
    std::uint32_t systemError = 0;
};
struct StoredCheckpoint {
    Checkpoint checkpoint;
    std::vector<std::byte> encoded;
    std::filesystem::path target; // Read identity only; never serialized.
};
enum class Operation { Read, OpenTemporary, Write, FlushFile, CloseFile, Publish, AfterPublish };
// Optional fault seam; applications use nullptr. Called before each operation.
// Tests may terminate their own disposable child here to model interruption.
class FaultInjector {
public:
    virtual ~FaultInjector() = default;
    virtual bool Fail(Operation, std::size_t /*bytesWritten*/) { return false; }
    virtual std::size_t WriteLimit(std::size_t requested) { return requested; }
};

// Explicit existing directory and plain basename only; no default owner-save
// location, directory creation, orphan sweep, migration, slots or UI policy.
// Read is bounded and leaves output untouched on every unsuccessful result.
Result Read(const std::filesystem::path& directory, std::string_view name,
            StoredCheckpoint& output, FaultInjector* faults = nullptr);
// null expected means create-only. Replacement requires the exact valid bytes
// and target identity returned by Read, under a nonblocking cooperative lock.
// Ordinary prepublication failures preserve the target. A failed result with
// Published/Unknown outcome MUST be read back before retry or advancing a save
// coordinator. Committed means complete visible bytes: inspect Durability
// separately. AlreadyCurrent performs no write and claims no new flush.
Result Write(const std::filesystem::path& directory, std::string_view name,
             const Checkpoint& candidate, const std::optional<StoredCheckpoint>& expected = std::nullopt,
             FaultInjector* faults = nullptr);
} // namespace horde::gameplay::campaign::storage
