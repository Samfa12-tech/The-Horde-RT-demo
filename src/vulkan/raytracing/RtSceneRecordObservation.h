#pragma once

#include "telemetry/RtPerformanceEvidence.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace horde::vulkan::raytracing
{

using RtSceneObservationClock = std::uint64_t (*)(void*) noexcept;

enum class RtSceneCommandEvent : std::uint8_t
{
    HostWriteBarrier,
    BlasUpdate,
    BlasToTlasBarrier,
    TlasUpdate,
    TlasToTraceBarrier,
    Trace,
    CopyOrBlit,
    Count,
};

// Fixed, one-way event sink used by the live scene command path. It proves
// command cardinality/order without wrapping or replacing Vulkan entrypoints.
class RtSceneCommandObservation final
{
public:
    [[nodiscard]] bool Note(const RtSceneCommandEvent event) noexcept
    {
        if (event >= RtSceneCommandEvent::Count || size_ == events_.size())
        {
            overflowed_ = true;
            return false;
        }
        events_[size_++] = event;
        return true;
    }

    [[nodiscard]] bool ValidCompleted() const noexcept
    {
        if (overflowed_ || size_ < 5u ||
            events_[0] != RtSceneCommandEvent::HostWriteBarrier)
        {
            return false;
        }
        std::size_t index = 1u;
        while (index < size_ && events_[index] == RtSceneCommandEvent::BlasUpdate)
        {
            ++index;
        }
        const bool hasBlasUpdates = index > 1u;
        if (hasBlasUpdates)
        {
            if (index >= size_ ||
                events_[index++] != RtSceneCommandEvent::BlasToTlasBarrier)
            {
                return false;
            }
        }
        return index + 4u == size_ &&
               events_[index] == RtSceneCommandEvent::TlasUpdate &&
               events_[index + 1u] == RtSceneCommandEvent::TlasToTraceBarrier &&
               events_[index + 2u] == RtSceneCommandEvent::Trace &&
               events_[index + 3u] == RtSceneCommandEvent::CopyOrBlit;
    }

    [[nodiscard]] std::uint64_t BlasUpdateCount() const noexcept
    {
        return Count(RtSceneCommandEvent::BlasUpdate);
    }
    [[nodiscard]] std::uint64_t TlasUpdateCount() const noexcept
    {
        return Count(RtSceneCommandEvent::TlasUpdate);
    }
    [[nodiscard]] std::uint64_t TraceCount() const noexcept
    {
        return Count(RtSceneCommandEvent::Trace);
    }
    [[nodiscard]] std::uint64_t CopyCount() const noexcept
    {
        return Count(RtSceneCommandEvent::CopyOrBlit);
    }

private:
    [[nodiscard]] std::uint64_t Count(const RtSceneCommandEvent event) const noexcept
    {
        std::uint64_t count = 0u;
        for (std::size_t index = 0u; index < size_; ++index)
        {
            count += events_[index] == event ? 1u : 0u;
        }
        return count;
    }

    std::array<RtSceneCommandEvent, 10u> events_{};
    std::size_t size_ = 0u;
    bool overflowed_ = false;
};

enum class RtSceneRecordFailure : std::uint8_t
{
    None,
    DiagnosticReset,
};

// Stack-bound, non-owning observation context for one renderer record attempt.
// The record caller owns Begin/Commit/Abort on the accumulator; renderer
// helpers only append to its active scratch values.
struct RtSceneRecordObservation
{
    horde::telemetry::RtStageAccumulator* stages = nullptr;
    void* clockUser = nullptr;
    RtSceneObservationClock readClock = nullptr;
    RtSceneCommandObservation* commands = nullptr;
    horde::telemetry::RtRecordedSceneEvidence* recordedScene = nullptr;
    RtSceneRecordFailure failure = RtSceneRecordFailure::None;
    bool diagnosticResetCompleted = false;
    bool healthy = true;
};

inline void ObserveRtSceneCommand(RtSceneRecordObservation* const observation,
                                  const RtSceneCommandEvent event) noexcept
{
    if (observation != nullptr && observation->commands != nullptr &&
        !observation->commands->Note(event))
    {
        observation->healthy = false;
    }
}

// Executes the real command first, then records its semantic placement. The
// observation remains optional and can never suppress renderer work.
template <typename Command>
inline void ExecuteObservedRtSceneCommand(
    RtSceneRecordObservation* const observation,
    const RtSceneCommandEvent event,
    Command&& command) noexcept(noexcept(command()))
{
    command();
    ObserveRtSceneCommand(observation, event);
}

// Owns the production conditional BLAS routing so tests can exercise the same
// zero-work/per-work/barrier behavior while substituting only Vulkan commands.
template <std::size_t WorkCount, typename Command, typename Barrier>
inline std::uint64_t ExecuteObservedDynamicBlasCommands(
    RtSceneRecordObservation* const observation,
    const std::array<bool, WorkCount>& requestedWork,
    Command&& command,
    Barrier&& barrier) noexcept(
        noexcept(command(std::size_t{})) && noexcept(barrier()))
{
    std::uint64_t executed = 0u;
    for (std::size_t index = 0u; index < requestedWork.size(); ++index)
    {
        if (!requestedWork[index])
        {
            continue;
        }
        command(index);
        ObserveRtSceneCommand(observation, RtSceneCommandEvent::BlasUpdate);
        ++executed;
    }
    if (executed != 0u)
    {
        barrier();
        ObserveRtSceneCommand(observation, RtSceneCommandEvent::BlasToTlasBarrier);
    }
    return executed;
}

// Keeps the required TLAS-update/dependency pair under one production-used
// sequence owner while callers supply the concrete Vulkan commands.
template <typename TlasUpdateCommand, typename DependencyCommand>
inline void ExecuteObservedTlasUpdateCommands(
    RtSceneRecordObservation* const observation,
    TlasUpdateCommand&& tlasUpdate,
    DependencyCommand&& dependency) noexcept(
        noexcept(tlasUpdate()) && noexcept(dependency()))
{
    ExecuteObservedRtSceneCommand(
        observation, RtSceneCommandEvent::TlasUpdate, tlasUpdate);
    ExecuteObservedRtSceneCommand(
        observation, RtSceneCommandEvent::TlasToTraceBarrier, dependency);
}

// Keeps trace and its eventual swapchain copy/blit under one production-used
// sequence owner. The second callback retains the intervening image barriers.
template <typename TraceCommand, typename CopyCommand>
inline void ExecuteObservedTraceCopyCommands(
    RtSceneRecordObservation* const observation,
    TraceCommand&& trace,
    CopyCommand&& copy) noexcept(noexcept(trace()) && noexcept(copy()))
{
    ExecuteObservedRtSceneCommand(
        observation, RtSceneCommandEvent::Trace, trace);
    ExecuteObservedRtSceneCommand(
        observation, RtSceneCommandEvent::CopyOrBlit, copy);
}

inline std::uint64_t ReadRtSceneSteadyClock(void*) noexcept
{
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return elapsed < 0 ? 0u : static_cast<std::uint64_t>(elapsed);
}

class RtSceneStageScope final
{
public:
    RtSceneStageScope(RtSceneRecordObservation* observation,
                      const horde::telemetry::RtStage stage) noexcept
        : observation_(observation), stage_(stage)
    {
        if (observation_ == nullptr || observation_->stages == nullptr ||
            !observation_->stages->Active())
        {
            return;
        }
        clock_ = observation_->readClock != nullptr
            ? observation_->readClock
            : ReadRtSceneSteadyClock;
        startNanoseconds_ = clock_(observation_->clockUser);
        active_ = true;
    }

    RtSceneStageScope(RtSceneRecordObservation* observation,
                      const horde::telemetry::RtStage stage,
                      const std::uint64_t startNanoseconds) noexcept
        : observation_(observation), stage_(stage)
    {
        if (observation_ == nullptr || observation_->stages == nullptr ||
            !observation_->stages->Active())
        {
            return;
        }
        clock_ = observation_->readClock != nullptr
            ? observation_->readClock
            : ReadRtSceneSteadyClock;
        startNanoseconds_ = startNanoseconds;
        active_ = true;
    }

    RtSceneStageScope(const RtSceneStageScope&) = delete;
    RtSceneStageScope& operator=(const RtSceneStageScope&) = delete;

    void Complete(const std::uint64_t workInvocationCount = 0u,
                  const std::uint64_t byteCount = 0u,
                  const std::uint64_t operationCount = 0u) noexcept
    {
        if (!active_)
        {
            return;
        }
        const std::uint64_t endNanoseconds = clock_(observation_->clockUser);
        active_ = false;
        if (endNanoseconds < startNanoseconds_ ||
            !observation_->stages->Accumulate(
                stage_, endNanoseconds - startNanoseconds_,
                workInvocationCount, byteCount, operationCount))
        {
            observation_->healthy = false;
        }
    }

    void Cancel() noexcept { active_ = false; }
    [[nodiscard]] bool Active() const noexcept { return active_; }

private:
    RtSceneRecordObservation* observation_ = nullptr;
    RtSceneObservationClock clock_ = nullptr;
    horde::telemetry::RtStage stage_ = horde::telemetry::RtStage::Count;
    std::uint64_t startNanoseconds_ = 0u;
    bool active_ = false;
};

} // namespace horde::vulkan::raytracing
