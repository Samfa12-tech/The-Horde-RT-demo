#include "graphics/ForegroundPauseRenderCadence.h"

#include <chrono>
#include <cstdint>

int main()
{
    using namespace horde::graphics;
    using Clock = ForegroundPauseRenderCadence::Clock;
    using namespace std::chrono_literals;

    static_assert(ForegroundPauseRenderCadence::kPausedFallbackInterval == 500ms);
    static_assert(ForegroundPauseRenderCadence::kEntryMaximumFrameRateHz == 30);
    if (ForegroundPauseRenderCadence::ResolveLiveProfileCapHz(ForegroundPauseScene::EntryMenu, 60) != 30 ||
        ForegroundPauseRenderCadence::ResolveLiveProfileCapHz(ForegroundPauseScene::GraphicsPreview, 60) != 60 ||
        ForegroundPauseRenderCadence::ResolveLiveProfileCapHz(ForegroundPauseScene::GraphicsPreview, 15) != 15 ||
        ForegroundPauseRenderCadence::ResolveLiveProfileCapHz(ForegroundPauseScene::Showcase, 120) != 120) return 14;

    ForegroundPauseRenderCadence cadence;
    ForegroundPauseRenderInput input{};
    input.foreground = true;
    input.paused = true;
    input.currentRtOutputValid = false;
    input.surfaceGeneration = 41;
    const auto start = Clock::time_point{} + 10s;

    auto decision = cadence.Evaluate(start, input);
    if (!decision.renderNow || !decision.usingPausedFallback) return 1;
    cadence.RecordRenderAttempt(start);

    input.currentRtOutputValid = true;
    decision = cadence.Evaluate(start + 1ms, input);
    if (decision.renderNow || !decision.usingPausedFallback || decision.wait <= 0ms ||
        decision.wait > ForegroundPauseRenderCadence::kMaximumPollInterval) return 2;
    decision = cadence.Evaluate(start + 499ms, input);
    if (decision.renderNow) return 3;
    decision = cadence.Evaluate(start + 500ms, input);
    if (!decision.renderNow) return 4;
    cadence.RecordRenderAttempt(start + 500ms);

    // A control or lifecycle wake, and loss of current RT output, both bypass
    // the interval so changes cannot wait for the next fallback frame.
    input.wakeRequested = true;
    decision = cadence.Evaluate(start + 510ms, input);
    if (!decision.renderNow) return 5;
    input.wakeRequested = false;
    input.currentRtOutputValid = false;
    decision = cadence.Evaluate(start + 511ms, input);
    if (!decision.renderNow) return 6;

    // The bounded aggregate reports work and wall time, not FPS.
    ForegroundPauseRenderAggregate aggregate{};
    if (cadence.TakeAggregate(start + 5s - 1ms, aggregate)) return 7;
    cadence.Evaluate(start + 5s, input);
    if (!cadence.TakeAggregate(start + 5s, aggregate) || aggregate.wallNanoseconds != 5'000'000'000ull ||
        aggregate.renderAttempts != 2u || aggregate.skippedIterations < 2u || aggregate.enteredFallback != 1u)
        return 8;

    // Resume, Entry, Preview and surface-generation changes are immediate and
    // leave platform background behavior to the platform loops.
    input.currentRtOutputValid = true;
    input.paused = false;
    decision = cadence.Evaluate(start + 5s + 1ms, input);
    if (!decision.renderNow || decision.usingPausedFallback || cadence.UsingPausedFallback()) return 9;
    input.paused = true;
    input.scene = ForegroundPauseScene::EntryMenu;
    decision = cadence.Evaluate(start + 6s, input);
    if (!decision.renderNow || decision.usingPausedFallback) return 10;
    input.scene = ForegroundPauseScene::GraphicsPreview;
    decision = cadence.Evaluate(start + 7s, input);
    if (!decision.renderNow || decision.usingPausedFallback) return 11;
    input.scene = ForegroundPauseScene::Showcase;
    input.surfaceGeneration = 42;
    decision = cadence.Evaluate(start + 8s, input);
    if (!decision.renderNow || !decision.usingPausedFallback) return 12;
    input.foreground = false;
    decision = cadence.Evaluate(start + 9s, input);
    if (!decision.renderNow || decision.usingPausedFallback) return 13;
    // Even if the platform has not flushed a short exit before re-entering,
    // preserve both exits and the full aggregate wall interval.
    if (!cadence.TakeAggregate(start + 9s, aggregate) || aggregate.enteredFallback != 1u ||
        aggregate.exitedFallback != 2u || aggregate.wallNanoseconds != 4'000'000'000ull) return 15;
    return 0;
}
