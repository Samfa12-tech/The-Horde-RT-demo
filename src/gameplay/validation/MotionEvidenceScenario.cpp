#include "gameplay/validation/MotionEvidenceScenario.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace horde::gameplay::validation
{
namespace
{
using namespace simulation;
constexpr float kPi = 3.14159265359f;
float Distance(const SimulationSnapshot& state, float x, float z) noexcept
{
    return std::hypot(state.playerX - x, state.playerZ - z);
}
bool KeeperScenario(MotionScenario scenario) noexcept
{
    return scenario == MotionScenario::KeeperFirstEntry || scenario == MotionScenario::KeeperRetryReward;
}
}

bool ParseMotionScenario(std::string_view text, MotionScenario& scenario) noexcept
{
    for (auto candidate : {MotionScenario::TorchLowOpening, MotionScenario::ShaftUp,
                           MotionScenario::KeeperFirstEntry, MotionScenario::KeeperRetryReward})
        if (text == MotionScenarioName(candidate)) { scenario = candidate; return true; }
    return false;
}
const char* MotionScenarioName(MotionScenario scenario) noexcept
{
    switch (scenario)
    {
    case MotionScenario::TorchLowOpening: return "torch-low-opening";
    case MotionScenario::ShaftUp: return "shaft-up";
    case MotionScenario::KeeperFirstEntry: return "keeper-first-entry";
    case MotionScenario::KeeperRetryReward: return "keeper-retry-reward";
    default: return "invalid";
    }
}
const char* MotionStageName(MotionStage stage) noexcept
{
    switch (stage)
    {
    case MotionStage::NotStarted: return "not-started";
    case MotionStage::Approach: return "approach";
    case MotionStage::TorchMotion: return "torch-motion";
    case MotionStage::ShaftMotion: return "shaft-motion";
    case MotionStage::Reveal: return "reveal";
    case MotionStage::FirstTelegraph: return "first-telegraph";
    case MotionStage::AwaitDeath: return "await-death";
    case MotionStage::RetryRequested: return "retry-requested";
    case MotionStage::Recognition: return "recognition";
    case MotionStage::RetryTelegraph: return "retry-telegraph";
    case MotionStage::Fight: return "fight";
    case MotionStage::Reward: return "reward";
    case MotionStage::TerminalReentry: return "terminal-reentry";
    case MotionStage::Complete: return "complete";
    case MotionStage::Failed: return "failed";
    default: return "invalid";
    }
}
bool MotionEvidenceScenario::Begin(MotionScenario scenario, GameSimulation& simulation,
                                   std::uint64_t now) noexcept
{
    *this = {};
    scenario_ = scenario;
    if (std::string_view(MotionScenarioName(scenario)) == "invalid" || now == 0u ||
        simulation.Snapshot().playerMountProfile != items::PlayerMountProfile::AnatomicalBody)
    { Fail("Motion evidence requires an admitted scenario, clock and production simulation profile."); return false; }
    const int checkpoint = scenario == MotionScenario::TorchLowOpening ? 0 : scenario == MotionScenario::ShaftUp ? 4 : 8;
    if (!simulation.ApplyShowcaseCheckpoint(checkpoint))
    { Fail("The accepted scenario checkpoint seed could not be applied."); return false; }
    startWall_ = latestWall_ = now;
    previousTick_ = simulation.Snapshot().tickIndex;
    initialVitality_ = simulation.Snapshot().playerVitals.vitality;
    initialRetryGeneration_ = simulation.Snapshot().retryGeneration;
    Enter(scenario == MotionScenario::ShaftUp ? MotionStage::ShaftMotion : MotionStage::Approach);
    return true;
}
std::string_view MotionEvidenceScenario::Failure() const noexcept { return failure_.data(); }
void MotionEvidenceScenario::Fail(std::string_view reason) noexcept
{
    if (Failed()) return;
    const auto size = std::min(reason.size(), failure_.size() - 1u);
    std::memcpy(failure_.data(), reason.data(), size);
    failure_[size] = '\0';
    stage_ = MotionStage::Failed;
}
void MotionEvidenceScenario::Enter(MotionStage stage) noexcept
{
    stage_ = stage;
    stageStartSeconds_ = simulationSeconds_;
    telegraphInitialSeconds_ = 0.0f;
}
void MotionEvidenceScenario::PublishEdge(std::uint64_t& sequence) noexcept
{
    if (sequence == std::numeric_limits<std::uint64_t>::max()) Fail("Input command sequence exhausted.");
    else ++sequence;
}
void MotionEvidenceScenario::AimAt(InputSnapshot& input, const SimulationSnapshot& state,
                                  float x, float z) const noexcept
{
    input.yawRadians = std::atan2(x - state.playerX, state.playerZ - z);
}
void MotionEvidenceScenario::MoveTowards(InputSnapshot& input, const SimulationSnapshot& state,
                                        float x, float z) const noexcept
{
    const float dx = x - state.playerX, dz = z - state.playerZ;
    const float distance = std::hypot(dx, dz);
    if (distance < 0.025f) return;
    const float inverse = 1.0f / std::max(distance, 0.25f);
    input.moveForward = (dx * std::sin(input.yawRadians) - dz * std::cos(input.yawRadians)) * inverse;
    input.moveStrafe = (dx * std::cos(input.yawRadians) + dz * std::sin(input.yawRadians)) * inverse;
}
InputSnapshot MotionEvidenceScenario::BuildInput(const SimulationSnapshot& state,
    const InputSnapshot& published, std::uint64_t now, bool ready) noexcept
{
    InputSnapshot input = published;
    input.moveForward = input.moveStrafe = 0.0f;
    input.hasAuthoritativePlayerPose = false;
    input.authoritativePlayerX = input.authoritativePlayerZ = 0.0f;
    input.damageEnabled = KeeperScenario(scenario_);
    input.torchLightStrength = 1.8f;
    input.yawRadians = state.playerYawRadians;
    input.pitchRadians = state.playerPitchRadians;
    const auto merge = [](std::uint64_t& target, std::uint64_t incoming, std::uint64_t consumed)
    { target = std::max({target, incoming, consumed}); };
    merge(commands_.attack, published.commands.attack, state.lastConsumedAttackSequence);
    merge(commands_.parry, published.commands.parry, state.lastConsumedParrySequence);
    merge(commands_.dodge, published.commands.dodge, state.lastConsumedDodgeSequence);
    merge(commands_.routeReset, published.commands.routeReset, state.lastConsumedRouteResetSequence);
    merge(commands_.retry, published.commands.retry, state.lastConsumedRetrySequence);
    merge(commands_.interact, published.commands.interact, state.lastConsumedInteractSequence);
    merge(commands_.toggleHeldLightPose, published.commands.toggleHeldLightPose, state.lastConsumedToggleHeldLightPoseSequence);
    if (now < latestWall_ || now < startWall_) Fail("Monotonic motion clock regressed.");
    else if (startWall_ != 0u && now - startWall_ > kMaximumWallNanoseconds) Fail("Motion evidence wall deadline exceeded.");
    latestWall_ = std::max(latestWall_, now);
    input.paused = published.paused || !ready || Failed() || Complete() || stage_ == MotionStage::NotStarted;
    if (!input.paused)
    {
        const float elapsed = static_cast<float>(simulationSeconds_ - stageStartSeconds_);
        if (elapsed > 45.0f) Fail("Motion stage failed to progress through the actual gameplay contract.");
        switch (stage_)
        {
        case MotionStage::Approach:
            if (KeeperScenario(scenario_)) { input.yawRadians = -kPi * 0.5f; MoveTowards(input, state, kKeeperRetryPosition.x, kKeeperRetryPosition.z); }
            else { input.yawRadians = 0.0f; MoveTowards(input, state, 0.0f, -2.5f); }
            break;
        case MotionStage::TorchMotion:
            input.yawRadians = std::sin(elapsed * kPi / 8.0f) * 0.45f;
            input.pitchRadians = std::clamp(-0.02f + 0.30f * std::sin(elapsed * kPi / 4.0f), -0.32f, 0.28f);
            if (elapsed < 14.0f) MoveTowards(input, state, 0.0f, (static_cast<int>(elapsed / 2.0f) & 1) ? -2.5f : -3.8f);
            if (elapsed > 3.0f && !firstAttackSent_) { PublishEdge(commands_.attack); firstAttackSent_ = true; }
            if (elapsed > 7.0f && !parrySent_) { PublishEdge(commands_.parry); parrySent_ = true; }
            break;
        case MotionStage::ShaftMotion:
            input.yawRadians = elapsed * kPi / 6.0f;
            input.pitchRadians = elapsed < 8.0f ? 0.28f : 0.12f + 0.16f * std::cos(elapsed * kPi / 2.0f);
            MoveTowards(input, state, -5.5f + 0.6f * std::sin(elapsed * kPi / 3.0f),
                        -15.2f + 0.6f * std::cos(elapsed * kPi / 3.0f));
            break;
        case MotionStage::Reveal:
        {
            const float reveal = state.lich.revealElapsedSeconds;
            input.yawRadians = -kPi * 0.5f + ((reveal > 1.8f && reveal < 2.4f) ? kPi : 0.0f);
            input.pitchRadians = 0.0f;
            MoveTowards(input, state, (reveal > 2.1f && reveal < 2.9f) ? -30.2f : kKeeperRetryPosition.x, kKeeperRetryPosition.z);
            if (reveal > 1.0f && !firstAttackSent_) { PublishEdge(commands_.attack); firstAttackSent_ = true; }
            if (reveal > 5.8f && !secondAttackSent_) { PublishEdge(commands_.attack); secondAttackSent_ = true; }
            break;
        }
        case MotionStage::FirstTelegraph:
        case MotionStage::AwaitDeath:
        case MotionStage::Recognition:
        case MotionStage::RetryTelegraph:
            AimAt(input, state, state.lich.x, state.lich.z);
            input.pitchRadians = 0.0f;
            MoveTowards(input, state, kKeeperRetryPosition.x, kKeeperRetryPosition.z);
            break;
        case MotionStage::RetryRequested:
            if (!retrySent_) { PublishEdge(commands_.retry); retrySent_ = true; }
            break;
        case MotionStage::Fight:
        {
            if (state.lich.phase == LichPhase::Dead) break;
            AimAt(input, state, state.lich.x, state.lich.z);
            input.pitchRadians = 0.0f;
            const float distance = Distance(state, state.lich.x, state.lich.z);
            if (distance > 1.3f) MoveTowards(input, state, state.lich.x, state.lich.z);
            if (distance < 1.8f && state.lich.hitCooldownRemaining <= 0.0f &&
                state.playerCombat.action == PlayerCombatAction::Idle && commands_.attack == state.lastConsumedAttackSequence)
                PublishEdge(commands_.attack);
            break;
        }
        case MotionStage::Reward:
            AimAt(input, state, kRewardChestRoutePosition.x, kRewardChestRoutePosition.z);
            input.pitchRadians = -0.10f;
            MoveTowards(input, state, -34.0f, -17.55f);
            if (state.chestPrompt == interactions::ChestRewardPrompt::OpenChest && !openSent_)
            { PublishEdge(commands_.interact); openSent_ = true; }
            if (state.chestPrompt == interactions::ChestRewardPrompt::ClaimLantern && !claimSent_)
            { PublishEdge(commands_.interact); claimSent_ = true; }
            break;
        case MotionStage::TerminalReentry:
            input.yawRadians = -kPi * 0.5f;
            input.pitchRadians = 0.0f;
            MoveTowards(input, state, elapsed < 3.0f ? -30.2f : kKeeperRetryPosition.x, kKeeperRetryPosition.z);
            if (elapsed > 0.5f && !lowSent_) { PublishEdge(commands_.toggleHeldLightPose); lowSent_ = true; }
            if (elapsed > 2.0f && !highSent_) { PublishEdge(commands_.toggleHeldLightPose); highSent_ = true; }
            break;
        default: break;
        }
    }
    input.commands = commands_;
    if (Failed()) { input.paused = true; input.moveForward = input.moveStrafe = 0.0f; }
    return input;
}
void MotionEvidenceScenario::ObserveAdvance(const SimulationSnapshot& state,
                                            std::span<const GameplayEvent> events) noexcept
{
    if (Failed() || Complete() || stage_ == MotionStage::NotStarted) return;
    if (state.tickIndex < previousTick_) { Fail("Shared simulation tick regressed during measured motion."); return; }
    simulationSeconds_ += static_cast<double>(state.tickIndex - previousTick_) * FixedStepRunner::kFixedDeltaSeconds;
    previousTick_ = state.tickIndex;
    if (state.eventQueueOverflowCount != 0u) { Fail("Semantic gameplay event queue overflowed."); return; }
    bool impact = false;
    for (const auto& event : events)
    {
        if (event.sequence <= previousEventSequence_ || static_cast<std::size_t>(event.type) >= eventCounts_.size())
        { Fail("Semantic event ordering or type admission failed."); return; }
        previousEventSequence_ = event.sequence;
        ++eventCounts_[static_cast<std::size_t>(event.type)];
        impact = impact || event.type == GameplayEventType::LichImpact;
    }
    if (KeeperScenario(scenario_) && IsKeeperRevealing(state.lich.revealPhase) &&
        (state.lich.health != 3 || state.lich.damagePulse || state.lich.staffLightStrength != 0.0f ||
         state.playerVitals.vitality != initialVitality_ || state.chestReward.phase != interactions::ChestRewardPhase::Locked))
    { Fail("Real reveal safety or locked reward invariant failed."); return; }
    if (state.paused && state.lich.revealElapsedSeconds != previousRevealElapsed_)
    { Fail("Keeper reveal advanced across paused input."); return; }
    previousRevealElapsed_ = state.lich.revealElapsedSeconds;
    if (!state.playerAlive && stage_ != MotionStage::AwaitDeath && stage_ != MotionStage::RetryRequested)
    { Fail("Player died outside the intended actual death/retry stage."); return; }
    const double elapsed = simulationSeconds_ - stageStartSeconds_;
    switch (stage_)
    {
    case MotionStage::Approach:
        if (KeeperScenario(scenario_)) { if (state.lich.revealStarted) Enter(MotionStage::Reveal); }
        else if (Distance(state, 0.0f, -2.5f) < 0.04f) Enter(MotionStage::TorchMotion);
        break;
    case MotionStage::TorchMotion:
        nearPortalSeen_ = nearPortalSeen_ || state.playerZ > -2.6f;
        farPortalSeen_ = farPortalSeen_ || state.playerZ < -3.7f;
        attackPoseSeen_ = attackPoseSeen_ || state.playerCombat.action == PlayerCombatAction::SwingActive;
        parryPoseSeen_ = parryPoseSeen_ || state.playerCombat.action == PlayerCombatAction::ParryActive;
        upLookSeen_ = upLookSeen_ || state.playerPitchRadians > 0.275f;
        downLookSeen_ = downLookSeen_ || state.playerPitchRadians < -0.315f;
        if (!state.torchFailure.heldByPlayer) Fail("Effects scenario unexpectedly lost its actual torch.");
        else if (elapsed >= 16.0)
        {
            if (!nearPortalSeen_ || !farPortalSeen_ || !attackPoseSeen_ || !parryPoseSeen_ || !upLookSeen_ || !downLookSeen_)
                Fail("Real portal traversal, attack/parry or supported look extremes were not observed.");
            else Enter(MotionStage::Complete);
        }
        break;
    case MotionStage::ShaftMotion:
        if (elapsed >= 12.0) Enter(MotionStage::Complete);
        break;
    case MotionStage::Reveal:
        revealRetreatSeen_ = revealRetreatSeen_ || !HasReachedKeeperArrivalThreshold(state.playerX, state.playerZ);
        revealReturnSeen_ = revealReturnSeen_ || (revealRetreatSeen_ && HasReachedKeeperArrivalThreshold(state.playerX, state.playerZ));
        if (state.lich.revealComplete)
        {
            if (state.lich.revealElapsedSeconds < LichEncounter::kRevealDuration || state.lich.health != 3 ||
                state.lich.phase != LichPhase::MaintainingRange || eventCounts_[17] != 1u || eventCounts_[18] != 1u || eventCounts_[19] != 1u ||
                !revealRetreatSeen_ || !revealReturnSeen_ || eventCounts_[6] != 0u)
                Fail("First reveal did not complete exactly once with a fresh full telegraph.");
            else { Enter(MotionStage::FirstTelegraph); telegraphInitialSeconds_ = state.lich.phaseTime; }
        }
        break;
    case MotionStage::FirstTelegraph:
    case MotionStage::RetryTelegraph:
        if (impact)
        {
            // At low FPS the snapshot can span several fixed ticks. Validate
            // the unchanged whole post-ready telegraph, not a last-sample guess.
            const double observationSpan = static_cast<double>(state.simulationTicksThisFrame) * FixedStepRunner::kFixedDeltaSeconds;
            if (elapsed + telegraphInitialSeconds_ + observationSpan + 0.0001 <
                LichEncounter::kMinimumRepositionDuration + LichEncounter::kChargeDuration)
                Fail("The actual first attack pulse spent less than its full telegraph.");
            else if (stage_ == MotionStage::RetryTelegraph) Enter(MotionStage::Fight);
            else Enter(scenario_ == MotionScenario::KeeperFirstEntry ? MotionStage::Complete : MotionStage::AwaitDeath);
        }
        break;
    case MotionStage::AwaitDeath:
        if (!state.playerAlive) Enter(MotionStage::RetryRequested);
        break;
    case MotionStage::RetryRequested:
        if (state.playerAlive)
        {
            if (state.retryGeneration != initialRetryGeneration_ + 1u ||
                state.lich.revealPhase != KeeperRevealPhase::RetryRecognition ||
                Distance(state, kKeeperRetryPosition.x, kKeeperRetryPosition.z) > 0.0001f)
                Fail("Actual retry did not restore the single safe arrival recognition attempt.");
            else { initialVitality_ = state.playerVitals.vitality; Enter(MotionStage::Recognition); }
        }
        break;
    case MotionStage::Recognition:
        if (state.lich.revealComplete)
        {
            if (state.lich.revealElapsedSeconds < LichEncounter::kRetryRecognitionDuration ||
                state.lich.phase != LichPhase::MaintainingRange || eventCounts_[17] != 1u || eventCounts_[18] != 1u || eventCounts_[19] != 2u)
                Fail("Actual one-second retry replayed awakening or skipped its fresh telegraph.");
            else { Enter(MotionStage::RetryTelegraph); telegraphInitialSeconds_ = state.lich.phaseTime; }
        }
        break;
    case MotionStage::Fight:
        if (state.lich.health > previousKeeperHealth_) Fail("Keeper health increased during actual combat.");
        if (state.lich.phase == LichPhase::Dead && state.lich.deathAnimationComplete)
        {
            if (eventCounts_[10] != 1u || eventCounts_[6] != 3u) Fail("Real keeper defeat or its three accepted hits were absent or duplicated.");
            else Enter(MotionStage::Reward);
        }
        break;
    case MotionStage::Reward:
        if (state.finaleComplete)
        {
            if (eventCounts_[13] != 1u || eventCounts_[14] != 1u || eventCounts_[15] != 1u || eventCounts_[11] != 1u)
                Fail("Actual chest, claim or finale progression was absent or duplicated.");
            else Enter(MotionStage::TerminalReentry);
        }
        break;
    case MotionStage::TerminalReentry:
        terminalExitSeen_ = terminalExitSeen_ || state.zone != ShowcaseZone::Finale;
        terminalReturnSeen_ = terminalReturnSeen_ || (terminalExitSeen_ && state.zone == ShowcaseZone::Finale);
        if (eventCounts_[10] != 1u || eventCounts_[13] != 1u || eventCounts_[14] != 1u || eventCounts_[15] != 1u || eventCounts_[11] != 1u)
            Fail("Terminal return duplicated encounter or reward semantics.");
        else if (elapsed >= 6.0)
        {
            if (!terminalReturnSeen_) Fail("Terminal encounter boundary retreat and reentry were not observed.");
            else Enter(MotionStage::Complete);
        }
        break;
    default: break;
    }
    previousKeeperHealth_ = state.lich.health;
}

} // namespace horde::gameplay::validation
