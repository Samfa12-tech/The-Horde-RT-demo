#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

namespace horde::graphics
{
enum class ForegroundPauseScene : std::uint8_t
{
    Showcase,
    EntryMenu,
    GraphicsPreview,
};

struct ForegroundPauseRenderInput
{
    bool foreground = true;
    bool paused = false;
    ForegroundPauseScene scene = ForegroundPauseScene::Showcase;
    bool currentRtOutputValid = false;
    bool wakeRequested = false;
    std::uint64_t surfaceGeneration = 0u;
};

struct ForegroundPauseRenderDecision
{
    bool renderNow = true;
    bool usingPausedFallback = false;
    std::chrono::milliseconds wait{0};
};

struct ForegroundPauseRenderAggregate
{
    std::uint64_t wallNanoseconds = 0u;
    std::uint64_t renderAttempts = 0u;
    std::uint64_t skippedIterations = 0u;
    std::uint64_t enteredFallback = 0u;
    std::uint64_t exitedFallback = 0u;
};

// This policy only suppresses redundant RT frames in a foreground paused
// Showcase. Platforms continue to own their existing background behavior and
// their live Entry/Preview cap. It intentionally makes no FPS claim.
class ForegroundPauseRenderCadence
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr std::chrono::milliseconds kPausedFallbackInterval{500};
    static constexpr std::chrono::milliseconds kMaximumPollInterval{20};
    static constexpr int kEntryMaximumFrameRateHz = 30;
    static constexpr std::chrono::seconds kAggregateInterval{5};

    static constexpr int ResolveLiveProfileCapHz(const ForegroundPauseScene scene,
                                                 const int requestedHz) noexcept
    {
        return scene == ForegroundPauseScene::EntryMenu
            ? std::clamp(requestedHz, 1, kEntryMaximumFrameRateHz)
            : requestedHz;
    }

    ForegroundPauseRenderDecision Evaluate(const Clock::time_point now,
                                           const ForegroundPauseRenderInput& input) noexcept
    {
        const bool pausedFallback = input.foreground && input.paused &&
            input.scene == ForegroundPauseScene::Showcase;
        if (generation_ != input.surfaceGeneration || pausedFallback_ != pausedFallback)
        {
            if (pausedFallback_ && !pausedFallback) SaturatingIncrement(aggregate_.exitedFallback);
            if (!pausedFallback_ && pausedFallback)
            {
                SaturatingIncrement(aggregate_.enteredFallback);
                if (aggregateStart_ == Clock::time_point{}) aggregateStart_ = now;
            }
            generation_ = input.surfaceGeneration;
            pausedFallback_ = pausedFallback;
            nextRenderDue_ = now;
        }

        if (!pausedFallback)
        {
            return {true, false, std::chrono::milliseconds{0}};
        }

        if (input.wakeRequested || !input.currentRtOutputValid || now >= nextRenderDue_)
        {
            return {true, true, std::chrono::milliseconds{0}};
        }

        SaturatingIncrement(aggregate_.skippedIterations);
        const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(nextRenderDue_ - now);
        const auto bounded = std::clamp(remaining, std::chrono::milliseconds{1}, kMaximumPollInterval);
        return {false, true, bounded};
    }

    void RecordRenderAttempt(const Clock::time_point now) noexcept
    {
        if (!pausedFallback_) return;
        SaturatingIncrement(aggregate_.renderAttempts);
        nextRenderDue_ = now + kPausedFallbackInterval;
    }

    bool UsingPausedFallback() const noexcept { return pausedFallback_; }

    bool TakeAggregate(const Clock::time_point now, ForegroundPauseRenderAggregate& result) noexcept
    {
        if (aggregateStart_ == Clock::time_point{}) return false;
        const bool intervalComplete = now - aggregateStart_ >= kAggregateInterval;
        const bool fallbackExited = !pausedFallback_ && aggregate_.exitedFallback != 0u;
        if (!intervalComplete && !fallbackExited) return false;
        result = aggregate_;
        result.wallNanoseconds = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - aggregateStart_).count());
        aggregate_ = {};
        aggregateStart_ = pausedFallback_ ? now : Clock::time_point{};
        return true;
    }

private:
    static void SaturatingIncrement(std::uint64_t& value) noexcept
    {
        if (value != UINT64_MAX) ++value;
    }

    std::uint64_t generation_ = UINT64_MAX;
    bool pausedFallback_ = false;
    Clock::time_point nextRenderDue_{};
    Clock::time_point aggregateStart_{};
    ForegroundPauseRenderAggregate aggregate_{};
};
} // namespace horde::graphics
