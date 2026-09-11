#include "vulkan/raytracing/RtSceneRecordObservation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <type_traits>

namespace
{

using horde::telemetry::RtSampleStatus;
using horde::telemetry::RtStage;
using horde::telemetry::RtStageAccumulator;
using horde::telemetry::RtStageFrameSample;
using horde::telemetry::RtStageIndex;
using horde::vulkan::raytracing::RtSceneRecordObservation;
using horde::vulkan::raytracing::RtSceneStageScope;

bool Require(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

struct TestClock
{
    std::array<std::uint64_t, 8u> values{};
    std::size_t valueCount = 0u;
    std::size_t readCount = 0u;
};

std::uint64_t ReadTestClock(void* user) noexcept
{
    auto& clock = *static_cast<TestClock*>(user);
    const std::size_t index = clock.readCount++;
    return index < clock.valueCount ? clock.values[index] : 0u;
}

} // namespace

int main()
{
    static_assert(std::is_trivially_copyable_v<RtSceneRecordObservation>);
    static_assert(std::is_standard_layout_v<RtSceneRecordObservation>);

    bool ok = true;
    TestClock clock{{10u, 17u, 20u, 31u, 40u, 45u}, 6u, 0u};

    {
        RtSceneStageScope omitted(nullptr, RtStage::PlayerSkin);
        omitted.Complete(1u);
    }
    RtStageAccumulator inactiveAccumulator;
    RtSceneRecordObservation inactive{&inactiveAccumulator, &clock, ReadTestClock};
    {
        RtSceneStageScope inactiveScope(&inactive, RtStage::PlayerSkin);
        inactiveScope.Complete(1u);
    }
    ok &= Require(clock.readCount == 0u,
                  "omitted or inactive observation must perform no clock reads");

    RtStageAccumulator accumulator;
    ok &= Require(accumulator.Begin(), "stage attempt did not begin");
    RtSceneRecordObservation observation{&accumulator, &clock, ReadTestClock};
    {
        RtSceneStageScope playerSkin(&observation, RtStage::PlayerSkin);
        playerSkin.Complete(1u);
    }
    {
        RtSceneStageScope characterSkin(&observation, RtStage::CharacterSkin);
        characterSkin.Complete(2u);
    }
    RtStageFrameSample committed{};
    ok &= Require(accumulator.Commit(committed), "observed stage attempt did not commit");
    const auto& player = committed.values[RtStageIndex(RtStage::PlayerSkin)];
    const auto& character = committed.values[RtStageIndex(RtStage::CharacterSkin)];
    const auto& skin = committed.values[RtStageIndex(RtStage::Skin)];
    ok &= Require(committed.status == RtSampleStatus::Valid &&
                      player.durationNanoseconds == 7u &&
                      player.workInvocationCount == 1u &&
                      character.durationNanoseconds == 11u &&
                      character.workInvocationCount == 2u &&
                      skin.durationNanoseconds == 18u &&
                      skin.workInvocationCount == 3u,
                  "skin observation must attribute components once and derive their total");

    const auto beforeAbort = accumulator.AggregatesByValue();
    ok &= Require(accumulator.Begin(), "discarded stage attempt did not begin");
    {
        RtSceneStageScope discarded(&observation, RtStage::TraceCopyRecord);
        discarded.Complete(1u);
    }
    ok &= Require(accumulator.Abort(), "discarded stage attempt did not abort");
    const auto aggregates = accumulator.AggregatesByValue();
    ok &= Require(
        aggregates.values[RtStageIndex(RtStage::TraceCopyRecord)].sampleCount ==
                beforeAbort.values[RtStageIndex(RtStage::TraceCopyRecord)].sampleCount &&
            aggregates.values[RtStageIndex(RtStage::TraceCopyRecord)].workInvocationCount ==
                beforeAbort.values[RtStageIndex(RtStage::TraceCopyRecord)].workInvocationCount,
                  "aborted renderer observation must not enter committed aggregates");

    ok &= Require(clock.readCount == 6u,
                  "only three active observed scopes should sample start and end clocks");

    RtStageAccumulator reversedAccumulator;
    TestClock reversedClock{{100u, 90u}, 2u, 0u};
    RtSceneRecordObservation reversedObservation{
        &reversedAccumulator, &reversedClock, ReadTestClock};
    ok &= Require(reversedAccumulator.Begin(), "reversed-clock attempt did not begin");
    {
        RtSceneStageScope reversed(
            &reversedObservation, RtStage::TraceCopyRecord);
        reversed.Complete(1u);
        reversed.Complete(1u);
    }
    RtStageFrameSample reversedCommitted{};
    ok &= Require(
        !reversedObservation.healthy && reversedClock.readCount == 2u &&
            reversedAccumulator.Commit(reversedCommitted) &&
            reversedCommitted.values[RtStageIndex(RtStage::TraceCopyRecord)]
                    .durationNanoseconds == 0u &&
            reversedCommitted.values[RtStageIndex(RtStage::TraceCopyRecord)]
                    .workInvocationCount == 0u,
        "reversed clocks must be unhealthy without wrapping or double-counting");

    TestClock abortClock{{50u, 40u}, 2u, 0u};
    RtSceneRecordObservation abortObservation{
        &reversedAccumulator, &abortClock, ReadTestClock};
    ok &= Require(reversedAccumulator.Begin(), "unhealthy abort attempt did not begin");
    {
        RtSceneStageScope reversed(
            &abortObservation, RtStage::TlasUpdateRecord);
        reversed.Complete(1u);
    }
    ok &= Require(!abortObservation.healthy && reversedAccumulator.Active() &&
                      reversedAccumulator.Abort(),
                  "unhealthy renderer observation scratch must remain abortable");
    return ok ? 0 : 1;
}
