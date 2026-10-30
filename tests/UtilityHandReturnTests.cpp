#include "gameplay/items/UtilityHandReturn.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

namespace
{
using namespace horde::gameplay::items;
int checks = 0;
int failures = 0;

void Check(const bool value, const char* message)
{
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}

bool Near(const double actual, const double expected)
{
    return std::abs(actual - expected) <= 0.000001;
}

void TestAuthoritativeUsingClock()
{
    UtilityHandReturn hand;
    const auto idle = hand.Snapshot();
    Check(idle == UtilityHandReturnSnapshot{} && !hand.FlaskHeld() &&
              hand.GripWeight() == 0.0f && !hand.EquipmentReleaseReady(),
          "default owner is idle without a release entitlement");
    Check(hand.Step(.05) && hand.Snapshot() == idle && !hand.Cancel() &&
              !hand.SynchronizeUsing(0.0) && hand.Snapshot() == idle,
          "idle cannot acquire an action through stepping, cancellation or synchronization");
    Check(hand.Begin() && hand.Snapshot().generation == 1u &&
              hand.Snapshot().phase == UtilityHandPhase::Using,
          "fresh admission allocates its first generation");
    const auto beginning = hand.Snapshot();
    Check(!hand.Begin() && hand.Snapshot() == beginning,
          "duplicate begin cannot restart or allocate another generation");
    for (unsigned tick = 0u; tick < 120u; ++tick)
        Check(hand.Step(1.0 / 60.0) && hand.Snapshot() == beginning,
              "simulation Step never becomes a second Using clock");
    Check(hand.SynchronizeUsing(.40) && hand.Snapshot().profileSeconds == .40 &&
              hand.Snapshot().usingElapsedSeconds == .40,
          "only the authoritative elapsed sample moves Using");
    const auto synchronized = hand.Snapshot();
    Check(hand.SynchronizeUsing(.40) && hand.Snapshot() == synchronized &&
              hand.Step(.05) && hand.Snapshot() == synchronized,
          "equal synchronization and subsequent stepping are idempotent");
    for (double invalid : {-1.0, std::nextafter(.40, 0.0),
                           std::nextafter(1.25, std::numeric_limits<double>::infinity()),
                           std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity()})
        Check(!hand.SynchronizeUsing(invalid) && hand.Snapshot() == synchronized,
              "regressing, nonfinite or out-of-range Using samples reject without mutation");
    Check(hand.SynchronizeUsing(1.25) && hand.EquipmentReleaseReady() &&
              hand.Snapshot().phase == UtilityHandPhase::Using && !hand.Snapshot().cancelled,
          "real completion sample holds Using at 1.25 until its owner resets");
    const auto completed = hand.Snapshot();
    Check(hand.Step(.05) && hand.Snapshot() == completed && !hand.Begin(),
          "completion cannot wrap, restart, or advance through another timer");
    hand.Reset();
    Check(hand.Snapshot() == UtilityHandReturnSnapshot{UtilityHandPhase::Idle, 0, 0, false, 1u} &&
              !hand.EquipmentReleaseReady() && hand.Begin() && hand.Snapshot().generation == 2u,
          "reset drops transient data and retains monotonic admission identity");
}

void TestGripAndEquipmentBoundaries()
{
    UtilityHandReturn hand;
    Check(hand.Begin(), "grip boundary fixture begins");
    const double cue = UtilityHandReturn::kEquipmentReleaseSeconds;
    Check(hand.SynchronizeUsing(std::nextafter(.30, 0.0)) && !hand.FlaskHeld() &&
              hand.GripWeight() == 0 && !hand.EquipmentReleaseReady(),
          "zero pre-lift weight does not release equipment");
    Check(hand.SynchronizeUsing(.30) && hand.FlaskHeld() && hand.GripWeight() == 0 &&
              !hand.EquipmentReleaseReady(),
          "exact acquisition boundary starts held state with zero blend weight");
    Check(hand.SynchronizeUsing(.325) && Near(hand.GripWeight(), .5),
          "acquisition midpoint uses the smooth fifty-millisecond blend");
    Check(hand.SynchronizeUsing(.35) && hand.GripWeight() == 1.0f,
          "acquisition blend reaches full grip");
    Check(hand.SynchronizeUsing(cue - .05) && hand.GripWeight() == 1.0f,
          "full grip lasts until the release blend starts");
    Check(hand.SynchronizeUsing(cue - .025) && Near(hand.GripWeight(), .5),
          "release midpoint uses the matching smooth blend");
    Check(hand.SynchronizeUsing(std::nextafter(cue, 0.0)) && hand.FlaskHeld() &&
              !hand.EquipmentReleaseReady(),
          "even a nearly zero release weight cannot grant an early equipment cue");
    Check(hand.SynchronizeUsing(cue) && !hand.FlaskHeld() && hand.GripWeight() == 0 &&
              hand.EquipmentReleaseReady(),
          "the exact authoritative 53/60 cue releases the utility");
}

void TestPhaseCorrectCancellation()
{
    const double cue = UtilityHandReturn::kEquipmentReleaseSeconds;
    const std::array<double, 18u> samples{{0, .15, .20, std::nextafter(.30, 0.0), .30,
        std::nextafter(.30, 1.0), .40, std::nextafter(.55, 0.0), .55, .60,
        std::nextafter(.70, 0.0), .70, .80, std::nextafter(cue, 0.0), cue,
        std::nextafter(cue, 1.0), 1.0, 1.25}};
    for (const double sample : samples)
    {
        UtilityHandReturn hand;
        Check(hand.Begin() && hand.SynchronizeUsing(sample) && hand.Cancel(),
              "each early/middle/late cancellation retains a real owner sample");
        const auto expectedPhase = sample <= .30 || sample >= cue ? UtilityHandPhase::Released :
            sample < .55 ? UtilityHandPhase::ReverseLift : UtilityHandPhase::ForwardReturn;
        const double expectedProfile = sample >= .55 && sample < .70 ? .70 : sample;
        Check(hand.Snapshot().phase == expectedPhase && hand.Snapshot().cancelled &&
                  hand.Snapshot().usingElapsedSeconds == sample &&
                  hand.Snapshot().profileSeconds == expectedProfile && hand.Snapshot().generation == 1u,
              "cancellation selects its actual phase without resetting use time to the fixed .60 seed");
        const auto cancelled = hand.Snapshot();
        Check(hand.Cancel() && hand.Step(0.0) && hand.Snapshot() == cancelled &&
                  !hand.Begin() && !hand.SynchronizeUsing(sample) && hand.Snapshot() == cancelled,
              "repeated cancellation, pause, begin and stale sync cannot restart return");
        Check(hand.EquipmentReleaseReady() == (expectedPhase == UtilityHandPhase::Released),
              "only already-released cancellations permit the equipment return cue");
        for (unsigned tick = 0u; tick < 64u && hand.Snapshot().phase != UtilityHandPhase::Released; ++tick)
        {
            const auto before = hand.Snapshot();
            Check(hand.Step(1.0 / 60.0), "cancelled return accepts the simulation delta");
            const auto after = hand.Snapshot();
            Check(after.generation == before.generation && after.cancelled &&
                      after.usingElapsedSeconds == sample && hand.Cancel() &&
                      hand.Snapshot() == after,
                  "return preserves its captured use sample and repeated cancel is idempotent");
            Check(expectedPhase == UtilityHandPhase::ReverseLift
                      ? after.profileSeconds <= before.profileSeconds
                      : after.profileSeconds >= before.profileSeconds,
                  "profile follows only the selected reverse or forward return direction");
            Check(std::isfinite(hand.GripWeight()) && hand.GripWeight() >= 0 && hand.GripWeight() <= 1,
                  "every return weight remains finite and bounded");
        }
        Check(hand.Snapshot().phase == UtilityHandPhase::Released && hand.EquipmentReleaseReady() &&
                  !hand.FlaskHeld() && hand.GripWeight() == 0,
              "every admitted cancellation reaches a released terminal state");
        if (expectedPhase == UtilityHandPhase::ReverseLift)
            Check(hand.Snapshot().profileSeconds == .30, "reverse return clamps exactly at the belt");
        if (expectedPhase == UtilityHandPhase::ForwardReturn)
            Check(hand.Snapshot().profileSeconds == cue, "forward return clamps exactly at the release cue");
        const auto released = hand.Snapshot();
        Check(hand.Step(.05) && hand.Cancel() && hand.Snapshot() == released,
              "released state holds until the owning caller explicitly resets it");
    }
}

void TestStepGuardsAndCopy()
{
    for (double sample : {0.0, .40, .60, 1.0})
    {
        UtilityHandReturn hand;
        Check(hand.Begin() && hand.SynchronizeUsing(sample), "delta fixture begins");
        if (sample != 0.0) Check(hand.Cancel(), "delta fixture enters its selected cancellation phase");
        const auto before = hand.Snapshot();
        for (double invalid : {-1.0, -std::numeric_limits<double>::denorm_min(),
                               std::nextafter(.05, std::numeric_limits<double>::infinity()),
                               std::numeric_limits<double>::max(), std::numeric_limits<double>::infinity(),
                               -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            Check(!hand.Step(invalid) && hand.Snapshot() == before,
                  "invalid deltas reject without clamping, advancing or mutating ownership");
        Check(hand.Step(-0.0) && hand.Snapshot() == before,
              "zero gameplay delta is a bit-preserving pause");
        auto copy = hand;
        for (unsigned tick = 0u; tick < 80u; ++tick)
        {
            const double dt = tick % 3u == 0u ? 0.0 : tick % 3u == 1u ? .01 : .05;
            Check(hand.Step(dt) && copy.Step(dt) && hand.Snapshot() == copy.Snapshot(),
                  "copied transient state deterministically continues under the same simulation deltas");
            if (sample != 0.0)
                Check(hand.Cancel() && copy.Cancel() && hand.Snapshot() == copy.Snapshot(),
                      "copied repeated cancellation preserves identical return ownership");
        }
    }
    UtilityHandReturn boundary;
    Check(boundary.Begin() && boundary.SynchronizeUsing(.40) && boundary.Cancel() &&
              boundary.Step(.05) && boundary.Step(.05) &&
              boundary.Snapshot().phase == UtilityHandPhase::Released &&
              boundary.Snapshot().profileSeconds == .30,
          "endpoint arithmetic cannot add a spurious fixed tick to an exact .10-second return");
}

void TestGenerationExhaustion()
{
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    UtilityHandReturn hand(maximum - 1u);
    Check(hand.Begin() && hand.Snapshot().generation == maximum,
          "last available generation is admitted without wraparound");
    Check(hand.SynchronizeUsing(.60) && hand.Cancel(), "last generation owns an ordinary return");
    hand.Reset();
    const auto exhausted = hand.Snapshot();
    Check(exhausted.generation == maximum && !hand.Begin() && hand.Snapshot() == exhausted,
          "reset retains an exhausted counter and cannot reuse generation zero or a prior identity");
    UtilityHandReturn alreadyExhausted(maximum);
    Check(!alreadyExhausted.Begin() && alreadyExhausted.Snapshot().generation == maximum,
          "an owner seeded at maximum fails admission without mutation");
}
} // namespace

int main()
{
    static_assert(std::is_trivially_copyable_v<UtilityHandReturn>);
    static_assert(std::is_trivially_copyable_v<UtilityHandReturnSnapshot>);
    TestAuthoritativeUsingClock();
    TestGripAndEquipmentBoundaries();
    TestPhaseCorrectCancellation();
    TestStepGuardsAndCopy();
    TestGenerationExhaustion();
    std::cout << "utility_hand_return_checks=" << checks << " failures=" << failures << '\n';
    return failures ? 1 : 0;
}
