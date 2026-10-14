#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace horde::world::streaming {

using RegionId = std::uint32_t;
struct Bounds {
    std::array<float,3> min{}, max{};
    bool Valid() const noexcept;
    double DistanceSquared(const std::array<float,3>& position) const noexcept;
};
struct RegionDefinition {
    RegionId id = 0;
    Bounds bounds;
    bool alwaysResident = false;
    // Manifest/provider reservation, replaced with actual allocation costs.
    // Zero means unknown, not free. Shared costs belong to the dependency owner.
    std::uint64_t preparationPeakDeviceBytes = 0;
    std::vector<RegionId> dependencies;
};
struct StreamingRules {
    float prepareDistance = 12.f;
    float retainDistance = 24.f;
    float predictionSeconds = 2.f;
    float retireDelaySeconds = 3.f;
    // Zero explicitly means no measured target has been supplied yet.
    // Sum of actual/reserved region owners, including shared catalog owners.
    // Other app allocations are deducted by the caller before setting this.
    std::uint64_t workingSetDeviceBytes = 0;
    std::uint32_t maxConcurrentPreparations = 1;
    std::uint32_t maxResidentDetailOwners = 4;
};
enum class Phase { Absent, Preparing, Ready, Resident, Retiring, Failed, Quarantined };
struct Token {
    std::uint64_t worldGeneration = 0, requestGeneration = 0;
    RegionId region = 0;
    bool operator==(const Token&) const = default;
};
struct Contribution {
    RegionId region = 0;
    // The renderer supplies a current all-query certificate or an admitted
    // coarse fallback. Unknown defaults pinned, even far outside the camera.
    bool mayBeAbsent = false;
};
struct Frame {
    std::uint64_t worldGeneration = 0;
    double monotonicSeconds = 0;
    std::array<float,3> position{}, velocity{};
    std::span<const RegionId> requiredForTraversal;
    std::span<const Contribution> contributions;
};
enum class ActionKind { Prepare, AdvancePreparation, Adopt, Retire, CancelPreparation, CancelRetirement };
struct Action { ActionKind kind; Token token; };
struct Plan {
    // One resource step per turn; bounded upload size is the backend's contract.
    std::array<Action,1> actions{};
    std::uint32_t count = 0;
    std::uint32_t requiredUnavailable = 0;
    std::uint64_t residentDeviceBytes = 0, preparationReservedDeviceBytes = 0;
    bool requiredExceedsTarget = false, costUnknown = false;
    bool requiredBlockedByOwnerCapacity = false;
};
struct ResidentState {
    Token token;
    Phase phase = Phase::Absent;
    std::uint64_t deviceBytes = 0;
    double undesiredSince = -1;
    bool cancelRequested = false;
    bool allocationKnown = false;
    bool retirementForRequired = false;
};

// Logical residency decisions only. GPU/IO ownership remains with the backend;
// acknowledgements must follow its real completion/reference checks. This class
// never frees memory, mutates simulation, or claims hardware readiness itself.
class RegionStreaming {
public:
    bool Configure(std::span<const RegionDefinition>, StreamingRules,
                   std::uint64_t worldGeneration, std::string& error);
    bool Evaluate(const Frame&, Plan&, std::string& error);
    bool MarkPrepared(Token, std::uint64_t actualDeviceBytes) noexcept;
    bool MarkResident(Token) noexcept;
    bool MarkRetired(Token) noexcept;
    bool MarkRetirementCancelled(Token) noexcept;
    bool MarkPreparationCancelled(Token) noexcept;
    bool ObservePreparationAllocation(Token, std::uint64_t actualDeviceBytes) noexcept;
    bool MarkFailed(Token, bool gpuCompletionUnproved) noexcept;
    // Backend calls only after real quarantined work/reference cleanup succeeds.
    bool MarkQuarantineReleased(Token) noexcept;
    bool RetryFailedRegion(RegionId) noexcept;
    bool SeedResident(RegionId, std::uint64_t actualDeviceBytes) noexcept;
    bool Invalidate(std::uint64_t nextWorldGeneration) noexcept;
    std::span<const ResidentState> States() const noexcept { return states_; }
    std::span<const RegionDefinition> Definitions() const noexcept { return definitions_; }
private:
    std::size_t Find(RegionId) const noexcept;
    ResidentState* Match(Token) noexcept;
    std::vector<RegionDefinition> definitions_;
    std::vector<ResidentState> states_;
    std::unordered_map<RegionId,std::size_t> index_;
    std::vector<std::vector<std::size_t>> dependencies_;
    std::vector<std::size_t> dependencyOrder_;
    std::vector<std::uint8_t> required_, wanted_, nearPrepare_, absentAllowed_, seen_, dependencyInUse_;
    StreamingRules rules_;
    std::uint64_t generation_ = 0, requestSequence_ = 0;
    double lastSeconds_ = -1;
};
}
