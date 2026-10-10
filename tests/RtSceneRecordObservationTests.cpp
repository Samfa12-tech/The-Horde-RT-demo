#include "vulkan/raytracing/RtSceneRecordObservation.h"
#include "scene/RescueJourneyGeometry.h"
#include <cmath>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <vector>

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

    RtStageAccumulator entryAccumulator;
    TestClock entryClock{{250u}, 1u, 0u};
    RtSceneRecordObservation entryObservation{
        &entryAccumulator, &entryClock, ReadTestClock};
    ok &= Require(entryAccumulator.Begin(), "entry-timestamp attempt did not begin");
    {
        RtSceneStageScope wholeFrame(
            &entryObservation, RtStage::WholeFrameCycle, 100u);
        wholeFrame.Complete(1u, 0u, 1u);
    }
    RtStageFrameSample entryCommitted{};
    ok &= Require(
        entryAccumulator.Commit(entryCommitted) && entryClock.readCount == 1u &&
            entryCommitted.values[RtStageIndex(RtStage::WholeFrameCycle)]
                    .durationNanoseconds == 150u &&
            entryCommitted.values[RtStageIndex(RtStage::WholeFrameCycle)]
                    .workInvocationCount == 1u,
        "an explicit function-entry timestamp must include setup before scope construction without a second start-clock read");

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

    using namespace horde::vulkan::raytracing;
    RtSceneCommandObservation sixBlasCommands{};
    RtSceneRecordObservation sixBlasObservation{};
    sixBlasObservation.commands = &sixBlasCommands;
    std::vector<RtSceneCommandEvent> actualCommands;
    ExecuteObservedRtSceneCommand(&sixBlasObservation, RtSceneCommandEvent::HostWriteBarrier,
        [&]() { actualCommands.push_back(RtSceneCommandEvent::HostWriteBarrier); });
    const std::array<bool, 6u> sixRequested{{true, true, true, true, true, true}};
    const std::uint64_t sixUpdates = ExecuteObservedDynamicBlasCommands(
        &sixBlasObservation, sixRequested,
        [&](const std::size_t index) {
            // The command callback precedes its observation event.
            if (index == 0u)
                ok &= Require(sixBlasCommands.BlasUpdateCount() == 0u,
                    "six-producer batch records the first Vulkan command before observing it");
            actualCommands.push_back(RtSceneCommandEvent::BlasUpdate);
        },
        [&]() {
            ok &= Require(sixBlasCommands.BlasUpdateCount() == 6u &&
                              sixBlasCommands.ValidCompleted() == false,
                          "one dependency callback follows all six actual BLAS updates");
            actualCommands.push_back(RtSceneCommandEvent::BlasToTlasBarrier);
        });
    ExecuteObservedTlasUpdateCommands(
        &sixBlasObservation,
        [&]() { actualCommands.push_back(RtSceneCommandEvent::TlasUpdate); },
        [&]() { actualCommands.push_back(RtSceneCommandEvent::TlasToTraceBarrier); });
    ExecuteObservedTraceCopyCommands(
        &sixBlasObservation,
        [&]() { actualCommands.push_back(RtSceneCommandEvent::Trace); },
        [&]() { actualCommands.push_back(RtSceneCommandEvent::CopyOrBlit); });
    constexpr std::array<RtSceneCommandEvent, 12u> expectedSixCommands{{
        RtSceneCommandEvent::HostWriteBarrier,
        RtSceneCommandEvent::BlasUpdate, RtSceneCommandEvent::BlasUpdate,
        RtSceneCommandEvent::BlasUpdate, RtSceneCommandEvent::BlasUpdate,
        RtSceneCommandEvent::BlasUpdate, RtSceneCommandEvent::BlasUpdate,
        RtSceneCommandEvent::BlasToTlasBarrier,
        RtSceneCommandEvent::TlasUpdate, RtSceneCommandEvent::TlasToTraceBarrier,
        RtSceneCommandEvent::Trace, RtSceneCommandEvent::CopyOrBlit,
    }};
    ok &= Require(sixUpdates == 6u && actualCommands.size() == expectedSixCommands.size() &&
                      std::equal(actualCommands.begin(), actualCommands.end(),
                                 expectedSixCommands.begin()) &&
                      sixBlasObservation.healthy && sixBlasCommands.ValidCompleted() &&
                      sixBlasCommands.BlasUpdateCount() == 6u &&
                      sixBlasCommands.TlasUpdateCount() == 1u &&
                      sixBlasCommands.TraceCount() == 1u &&
                      sixBlasCommands.CopyCount() == 1u,
                  "six real BLAS callbacks share one ordered BLAS-to-TLAS barrier and fit observation capacity");

    RtSceneCommandObservation missingBarrierCommands{};
    for (const auto event : {RtSceneCommandEvent::HostWriteBarrier,
                             RtSceneCommandEvent::BlasUpdate,
                             RtSceneCommandEvent::BlasUpdate,
                             RtSceneCommandEvent::BlasUpdate,
                             RtSceneCommandEvent::BlasUpdate,
                             RtSceneCommandEvent::BlasUpdate,
                             RtSceneCommandEvent::BlasUpdate,
                             RtSceneCommandEvent::TlasUpdate,
                             RtSceneCommandEvent::TlasToTraceBarrier,
                             RtSceneCommandEvent::Trace,
                             RtSceneCommandEvent::CopyOrBlit})
        (void)missingBarrierCommands.Note(event);
    ok &= Require(!missingBarrierCommands.ValidCompleted(),
                  "six producer observations reject a missing BLAS-to-TLAS dependency");

    RtSceneCommandObservation reorderedCommands{};
    for (const auto event : {RtSceneCommandEvent::HostWriteBarrier,
                             RtSceneCommandEvent::BlasUpdate,
                             RtSceneCommandEvent::BlasToTlasBarrier,
                             RtSceneCommandEvent::BlasUpdate,
                             RtSceneCommandEvent::TlasUpdate,
                             RtSceneCommandEvent::TlasToTraceBarrier,
                             RtSceneCommandEvent::Trace,
                             RtSceneCommandEvent::CopyOrBlit})
        (void)reorderedCommands.Note(event);
    ok &= Require(!reorderedCommands.ValidCompleted(),
                  "six producer observations reject a BLAS update recorded after its dependency");

    RtSceneCommandObservation overflowCommands{};
    RtSceneRecordObservation overflowObservation{};
    overflowObservation.commands = &overflowCommands;
    for (const auto event : expectedSixCommands)
        ObserveRtSceneCommand(&overflowObservation, event);
    ObserveRtSceneCommand(&overflowObservation, RtSceneCommandEvent::BlasUpdate);
    ok &= Require(!overflowObservation.healthy && !overflowCommands.ValidCompleted(),
                  "a thirteenth observed command overflows the fixed twelve-event capacity");
    // Test the geometry and normal transport actually consumed by the world
    // BLAS and its shader metadata, for both vertical and thrown/bent ropes.
    using namespace horde::scene;
    const auto anchor=horde::gameplay::traversal::kAnchor;
    ok &= Require(anchor.x>=kRescueAnchorCollar.minimum[0] && anchor.x<=kRescueAnchorCollar.maximum[0] &&
                  anchor.y>=kRescueAnchorCollar.minimum[1] && anchor.y<=kRescueAnchorCollar.maximum[1] &&
                  anchor.z>=kRescueAnchorCollar.minimum[2] && anchor.z<=kRescueAnchorCollar.maximum[2] &&
                  kRescueAnchorCollar.maximum[1]>=kRescueBlockoutBoxes[11].minimum[1],
                  "actual clear solver anchor remains inside a retained fitting joined to the cantilever");
    horde::gameplay::traversal::RescueTraversalSnapshot rope{};
    constexpr std::array<RescueRopePoint,6> cardinal{{{0,1,0},{0,-1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}}};
    for(unsigned shape=0;shape<3;++shape) {
        for(std::size_t i=0;i<rope.ropeNodes.size();++i) {
            const float t=static_cast<float>(i);
            rope.ropeNodes[i]={shape==0?0.0f:t*.18f,4.0f-t*.35f,shape==2?std::sin(t*.4f)*.25f:0.0f};
        }
        const auto vertices=RescueRopeTriangleVertices(rope);
        ok &= Require(vertices.size()==176u*3u,"rope transport retains exact fixed 176-triangle topology");
        bool finiteOutward=true,multipleDirections=false;unsigned first=99;
        for(std::size_t i=0;i<vertices.size();i+=3) {
            const auto n=RopeCross(RopeSubtract(vertices[i+1],vertices[i]),RopeSubtract(vertices[i+2],vertices[i]));
            const auto code=RescueRopeTriangleNormalCode(vertices[i],vertices[i+1],vertices[i+2]);
            const float area=std::sqrt(RopeDot(n,n));
            finiteOutward &= std::isfinite(area)&&area>1e-7f&&code<cardinal.size()&&RopeDot(n,cardinal[code])>=area*.57f;
            if(first==99) first=code; else multipleDirections |= code!=first;
            if(shape==0) {
                const RescueRopePoint radial{(vertices[i][0]+vertices[i+1][0]+vertices[i+2][0])/3,0,
                    (vertices[i][2]+vertices[i+1][2]+vertices[i+2][2])/3};
                finiteOutward &= RopeDot(n,radial)>0;
            }
        }
        ok &= Require(finiteOutward&&multipleDirections,"finite outward rope facets publish actual nearest-cardinal normals, never one forward normal");
    }
    return ok ? 0 : 1;
}
