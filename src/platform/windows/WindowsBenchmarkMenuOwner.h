#pragma once

namespace horde::platform::windows
{
enum class OpeningBenchmarkChoice { None, WindowsProvisional, Custom };
enum class OpeningBenchmarkOwnerPhase { Entry, Queued, Benchmark, ReturningEntry, Blocked };

struct OpeningBenchmarkEligibility
{
    bool entryVisible = false;
    bool entryScene = false;
    bool activeCampaign = false;
    bool pendingCampaign = false;
    bool graphicsTransition = false;
    bool playHandoff = false;
    bool benchmarkBusy = false;

    [[nodiscard]] constexpr bool Eligible() const noexcept
    {
        return entryVisible && entryScene && !activeCampaign && !pendingCampaign &&
            !graphicsTransition && !playHandoff && !benchmarkBusy;
    }
};

// Owner-thread policy only. The native caller supplies actual retirement and
// presentation proofs; this helper neither retires nor allocates a renderer.
class OpeningBenchmarkOwnerTransfer
{
public:
    [[nodiscard]] bool Queue(const OpeningBenchmarkChoice choice,
                             const OpeningBenchmarkEligibility& eligibility) noexcept
    {
        if (phase_ != OpeningBenchmarkOwnerPhase::Entry || !eligibility.Eligible() ||
            (choice != OpeningBenchmarkChoice::WindowsProvisional &&
             choice != OpeningBenchmarkChoice::Custom)) return false;
        choice_ = choice;
        phase_ = OpeningBenchmarkOwnerPhase::Queued;
        return true;
    }

    // Recheck eligibility BEFORE attempting native retirement. If eligibility
    // changed, the caller must leave the Entry owner alive and cancel the queue.
    // Once retirement has been attempted, false must never permit a successor.
    [[nodiscard]] bool RecheckAndRetireEntry(const OpeningBenchmarkEligibility& currentEligibility,
                                           const bool retiredSuccess) noexcept
    {
        if (phase_ != OpeningBenchmarkOwnerPhase::Queued) return false;
        if (!currentEligibility.Eligible()) {
            CancelQueued();
            return false;
        }
        phase_ = retiredSuccess ? OpeningBenchmarkOwnerPhase::Benchmark : OpeningBenchmarkOwnerPhase::Blocked;
        return retiredSuccess;
    }

    [[nodiscard]] bool BenchmarkRetired(const bool restoredSuccess,
                                        const bool retiredSuccess) noexcept
    {
        const bool admitted = phase_ == OpeningBenchmarkOwnerPhase::Benchmark && restoredSuccess && retiredSuccess;
        phase_ = admitted ? OpeningBenchmarkOwnerPhase::ReturningEntry : OpeningBenchmarkOwnerPhase::Blocked;
        return admitted;
    }

    [[nodiscard]] bool EntryPresented(const bool success) noexcept
    {
        const bool admitted = phase_ == OpeningBenchmarkOwnerPhase::ReturningEntry && success;
        phase_ = admitted ? OpeningBenchmarkOwnerPhase::Entry : OpeningBenchmarkOwnerPhase::Blocked;
        if (admitted) choice_ = OpeningBenchmarkChoice::None;
        return admitted;
    }

    bool CancelQueued() noexcept
    {
        if (phase_ != OpeningBenchmarkOwnerPhase::Queued) return false;
        choice_ = OpeningBenchmarkChoice::None;
        phase_ = OpeningBenchmarkOwnerPhase::Entry;
        return true;
    }

    [[nodiscard]] OpeningBenchmarkChoice Choice() const noexcept { return choice_; }
    [[nodiscard]] OpeningBenchmarkOwnerPhase Phase() const noexcept { return phase_; }

private:
    OpeningBenchmarkOwnerPhase phase_ = OpeningBenchmarkOwnerPhase::Entry;
    OpeningBenchmarkChoice choice_ = OpeningBenchmarkChoice::None;
};
} // namespace horde::platform::windows
