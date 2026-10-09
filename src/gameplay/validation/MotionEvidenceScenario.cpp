#include "gameplay/validation/MotionEvidenceScenario.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <cstdio>

namespace horde::gameplay::validation
{
namespace
{
using namespace simulation;
constexpr float kPi = 3.14159265359f;
// Retained phone projection places the nearer inspection below the right
// hand/body. Use one additional metre of ordinary movement to view the floor.
constexpr float kTorchDrenchInspectionX = -5.5f;
constexpr float kTorchDrenchInspectionZ = -15.2f;
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
                           MotionScenario::KeeperFirstEntry, MotionScenario::KeeperRetryReward,
                           MotionScenario::WaterfallEquipment, MotionScenario::TorchDrench})
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
    case MotionScenario::WaterfallEquipment: return "waterfall-equipment";
    case MotionScenario::TorchDrench: return "torch-drench";
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
    case MotionStage::EquipmentDraw: return "equipment-draw";
    case MotionStage::EquipmentAttack: return "equipment-attack";
    case MotionStage::EquipmentParry: return "equipment-parry";
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
    case MotionStage::TorchDrenchGuttering: return "torch-drench-guttering";
    case MotionStage::TorchDrenchFalling: return "torch-drench-falling";
    case MotionStage::TorchDrenchSettled: return "torch-drench-settled";
    default: return "invalid";
    }
}
bool MotionEvidenceScenario::Begin(MotionScenario scenario, GameSimulation& simulation,
                                   std::uint64_t now) noexcept
{
    if (equipmentSeedOwned_) return false;
    *this = {};
    scenario_ = scenario;
    if (std::string_view(MotionScenarioName(scenario)) == "invalid" || now == 0u ||
        simulation.Snapshot().playerMountProfile != items::PlayerMountProfile::AnatomicalBody)
    { Fail("Motion evidence requires an admitted scenario, clock and production simulation profile."); return false; }
    const bool seeded = scenario == MotionScenario::WaterfallEquipment
        ? simulation.BeginMotionEvidenceEquipmentSeed()
        : simulation.ApplyShowcaseCheckpoint(
            scenario == MotionScenario::TorchLowOpening ? 0 :
            (scenario == MotionScenario::ShaftUp || scenario == MotionScenario::TorchDrench) ? 2 : 8);
    if (!seeded)
    { Fail("The accepted scenario checkpoint seed could not be applied."); return false; }
    equipmentSeedOwned_ = scenario == MotionScenario::WaterfallEquipment;
    startWall_ = latestWall_ = now;
    previousTick_ = simulation.Snapshot().tickIndex;
    previousRevealElapsed_ = simulation.Snapshot().lich.revealElapsedSeconds;
    initialVitality_ = simulation.Snapshot().playerVitals.vitality;
    initialRetryGeneration_ = simulation.Snapshot().retryGeneration;
    Enter(MotionStage::Approach);
    return true;
}
void MotionEvidenceScenario::End(GameSimulation& simulation) noexcept
{
    if (!equipmentSeedOwned_) return;
    simulation.EndMotionEvidenceEquipmentSeed();
    equipmentSeedOwned_ = false;
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
    input.runHeld = false;
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
    merge(commands_.runToggle, published.commands.runToggle, state.lastConsumedRunToggleSequence);
    merge(commands_.clearRunIntent, published.commands.clearRunIntent,
          state.lastConsumedClearRunIntentSequence);
    if (!runClearSent_)
    {
        const std::uint64_t runClearBase = std::max(
            commands_.clearRunIntent, state.lastConsumedClearRunIntentSequence);
        commands_.clearRunIntent = runClearBase == UINT64_MAX ? runClearBase : runClearBase + 1u;
        runClearSent_ = true;
    }
    if (now < latestWall_ || now < startWall_) Fail("Monotonic motion clock regressed.");
    else if (startWall_ != 0u && now - startWall_ > kMaximumWallNanoseconds) Fail("Motion evidence wall deadline exceeded.");
    latestWall_ = std::max(latestWall_, now);
    input.paused = published.paused || !ready || Failed() || Complete() || stage_ == MotionStage::NotStarted;
    if (!input.paused)
    {
        const float elapsed = static_cast<float>(simulationSeconds_ - stageStartSeconds_);
        if (elapsed > 45.0f)
        {
            std::array<char, 128u> reason{};
            std::snprintf(reason.data(), reason.size(), "%s stage %s timed out at (%.2f, %.2f).",
                MotionScenarioName(scenario_), MotionStageName(stage_), state.playerX, state.playerZ);
            Fail(reason.data());
        }
        switch (stage_)
        {
        case MotionStage::Approach:
            if (KeeperScenario(scenario_)) { input.yawRadians = -kPi * 0.5f; MoveTowards(input, state, kKeeperRetryPosition.x, kKeeperRetryPosition.z); }
            else if (scenario_==MotionScenario::ShaftUp)
            {
                const bool atWestLeg=state.playerZ< -15.05f;
                input.yawRadians=atWestLeg ? -kPi*0.5f : 0.0f;
                input.pitchRadians=atWestLeg ? 0.28f : -0.04f;
                MoveTowards(input,state,atWestLeg ? -1.66f : 4.2f,atWestLeg ? -15.35f : -15.2f);
            }
            else if (scenario_ == MotionScenario::WaterfallEquipment)
            {
                input.pitchRadians = -0.04f;
                // Follow the authored corridor's existing two-leg path. A
                // diagonal from checkpoint 2 cuts across the solid corner.
                const bool atWestLeg = state.playerZ < -15.05f;
                input.yawRadians = atWestLeg ? -kPi * 0.5f : 0.0f;
                MoveTowards(input, state, atWestLeg ? -2.0f : 4.2f, -15.2f);
            }
            else if (scenario_ == MotionScenario::TorchDrench)
            {
                input.pitchRadians = -0.04f;
                // Use the same two ordinary corridor legs as the waterfall
                // encounter. The second target lies inside the production
                // automatic drench trigger; the scenario never imports a phase.
                const bool atWestLeg = state.playerZ < -15.05f;
                input.yawRadians = atWestLeg ? -kPi * 0.5f : 0.0f;
                MoveTowards(input, state, atWestLeg ? -2.0f : 4.2f, -15.2f);
            }
            else if (elapsed<6.0f || !rearReturnSeen_)
            {
                const auto smooth=[](float value) {value=std::clamp(value,0.0f,1.0f);return value*value*(3.0f-2.0f*value);};
                const float rearBlend=elapsed<5.2f ? smooth(elapsed/0.8f) : 1.0f-smooth((elapsed-5.2f)/0.8f);
                input.yawRadians=kPi*rearBlend;
                input.pitchRadians=0.10f+0.08f*std::sin(elapsed*kPi/3.0f);
                if(elapsed<4.2f)
                    MoveTowards(input,state,0.35f*std::sin(elapsed*kPi/4.2f),2.65f);
                else MoveTowards(input,state,kPlayerSpawn.x,kPlayerSpawn.z);
            }
            else { input.yawRadians = 0.0f; input.pitchRadians=-0.05f; MoveTowards(input, state, 0.0f, -2.5f); }
            break;
        case MotionStage::TorchMotion:
            input.yawRadians = std::sin(elapsed * kPi / 8.0f) * 0.45f;
            input.pitchRadians = std::clamp(-0.02f + 0.30f * std::sin(elapsed * kPi / 4.0f), -0.32f, 0.28f);
            if (elapsed < 14.0f) MoveTowards(input, state, 0.0f, (static_cast<int>(elapsed / 2.0f) & 1) ? -2.5f : -3.8f);
            if (elapsed > 3.0f && !firstAttackSent_) { PublishEdge(commands_.attack); firstAttackSent_ = true; }
            if (elapsed > 7.0f && !parrySent_) { PublishEdge(commands_.parry); parrySent_ = true; }
            break;
        case MotionStage::ShaftMotion:
            // The report's shaft is the waterfall aperture, not the later
            // skylight chamber or finale roof. Stay on the east side of the
            // unchanged automatic drench trigger while looking into its rim.
            input.yawRadians=-kPi*0.5f+0.06f*std::sin(elapsed*kPi/3.0f);
            input.pitchRadians = 0.275f + 0.005f * std::cos(elapsed * kPi / 3.0f);
            MoveTowards(input,state,-1.66f,-15.35f+0.26f*std::sin(elapsed*kPi/3.0f));
            break;
        case MotionStage::EquipmentDraw:
        case MotionStage::EquipmentAttack:
        case MotionStage::EquipmentParry:
            AimAt(input, state, kWaterfallSkeletonPairCenter.x, kWaterfallSkeletonPairCenter.z);
            if (stage_ == MotionStage::EquipmentAttack && !firstAttackSent_)
            { PublishEdge(commands_.attack); firstAttackSent_ = true; }
            if (stage_ == MotionStage::EquipmentParry && !parrySent_)
            { PublishEdge(commands_.parry); parrySent_ = true; }
            break;
        case MotionStage::TorchDrenchFalling:
        case MotionStage::TorchDrenchSettled:
            // Continue with normal axes after release so capture can prove the
            // torch owns a fixed world transform while the player steps back
            // and changes view to inspect it.
            input.yawRadians = kPi * 0.5f;
            input.pitchRadians = -0.32f;
            MoveTowards(input, state, kTorchDrenchInspectionX, kTorchDrenchInspectionZ);
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
    const bool advanced = state.tickIndex > previousTick_;
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
        if (scenario_ == MotionScenario::WaterfallEquipment)
        {
            if (event.type == GameplayEventType::SkeletonEncounterWarning && waterfallWarningSequence_ == 0u)
                waterfallWarningSequence_ = event.sequence;
            else if (event.type == GameplayEventType::PlayerSwordDrawStarted && swordDrawSequence_ == 0u)
                swordDrawSequence_ = event.sequence;
            else if (event.type == GameplayEventType::PlayerSwordAttachmentChanged && swordAttachmentSequence_ == 0u)
                swordAttachmentSequence_ = event.sequence;
            else if (event.type == GameplayEventType::PlayerSwing && swordSwingSequence_ == 0u)
                swordSwingSequence_ = event.sequence;
        }
        if (scenario_ == MotionScenario::TorchDrench &&
            event.type == GameplayEventType::TorchExtinguished)
        {
            if (torchExtinguishedSequence_ != 0u)
            { Fail("Automatic torch-drench emitted a duplicate extinguish event."); return; }
            torchExtinguishedSequence_ = event.sequence;
        }
        impact = impact || event.type == GameplayEventType::LichImpact;
    }
    if (scenario_ == MotionScenario::TorchDrench)
    {
        const auto& torch = state.torchFailure;
        const auto& eventCount = eventCounts_[static_cast<std::size_t>(GameplayEventType::TorchExtinguished)];
        if (eventCount > 1u)
        { Fail("Automatic torch-drench emitted more than one extinguish event."); return; }
        if (!torch.triggered)
        {
            if (stage_ != MotionStage::Approach || !torch.heldByPlayer ||
                state.interaction.heldLightKind != interactions::HeldLightKind::Torch ||
                eventCount != 0u || torch.phase != TorchFailurePhase::Held)
                Fail("Torch-drench route changed the held torch before its authored trigger.");
            return;
        }
        if (torchExtinguishedSequence_ == 0u || eventCount != 1u)
        { Fail("Automatic torch-drench trigger lacked its single semantic event."); return; }
        if (stage_ == MotionStage::Approach)
            Enter(MotionStage::TorchDrenchGuttering);

        if (stage_ == MotionStage::TorchDrenchGuttering)
        {
            if (torch.phase == TorchFailurePhase::Guttering)
            {
                if (!torch.heldByPlayer || state.interaction.heldLightKind != interactions::HeldLightKind::Torch ||
                    torch.flameStrength <= 0.0f || torch.fallProgress != 0.0f)
                    Fail("Torch guttering did not retain the held light and its live flame.");
                return;
            }
            if (torch.phase != TorchFailurePhase::Falling)
            { Fail("Torch drench skipped its observable guttering or falling phase."); return; }
            torchDrenchReleaseX_ = torch.droppedX;
            torchDrenchReleaseZ_ = torch.droppedZ;
            torchDrenchReleaseYaw_ = torch.droppedYawRadians;
            torchDrenchReleaseViewPitch_ = torch.droppedViewPitchRadians;
            torchDrenchPreviousFallProgress_ = torch.fallProgress;
            torchDrenchPreviousDroppedY_ = torch.droppedY;
            torchDrenchFallingSeen_ = true;
            Enter(MotionStage::TorchDrenchFalling);
        }
        if (stage_ == MotionStage::TorchDrenchFalling)
        {
            if (torch.phase == TorchFailurePhase::Settled)
            {
                torchDrenchSettledSeen_ = true;
                Enter(MotionStage::TorchDrenchSettled);
            }
        }
        if (stage_ == MotionStage::TorchDrenchFalling)
        {
            if (!torchDrenchFallingSeen_ || torch.phase != TorchFailurePhase::Falling)
            {
                std::array<char, 128u> reason{};
                std::snprintf(reason.data(), reason.size(),
                    "Released Rag torch skipped its observable falling phase (phase=%u, sequence=%.3f, fall=%.3f).",
                    static_cast<unsigned>(torch.phase), torch.sequenceTime, torch.fallProgress);
                Fail(reason.data()); return;
            }
            if (torch.heldByPlayer || state.interaction.heldLightKind != interactions::HeldLightKind::None)
            { Fail("Released Rag torch remained owned by the player or held-light slot."); return; }
            if (torch.flameStrength != 0.0f)
            { Fail("Released Rag torch flame strength was not zero."); return; }
            if (torch.fallProgress < torchDrenchPreviousFallProgress_)
            { Fail("Released Rag torch fall progress regressed."); return; }
            if (torch.droppedY > torchDrenchPreviousDroppedY_ + 0.0001f)
            { Fail("World-owned torch drop moved upward during its fall."); return; }
            if (std::abs(torch.droppedX - torchDrenchReleaseX_) > 0.0001f ||
                std::abs(torch.droppedZ - torchDrenchReleaseZ_) > 0.0001f ||
                std::abs(torch.droppedYawRadians - torchDrenchReleaseYaw_) > 0.0001f ||
                std::abs(torch.droppedViewPitchRadians - torchDrenchReleaseViewPitch_) > 0.0001f)
            { Fail("Released Rag torch did not retain a fixed world-owned falling transform."); return; }
            torchDrenchPreviousFallProgress_ = torch.fallProgress;
            torchDrenchPreviousDroppedY_ = torch.droppedY;
            torchDrenchWalkingAfterRelease_ = torchDrenchWalkingAfterRelease_ ||
                std::hypot(state.playerX - torchDrenchReleaseX_, state.playerZ - torchDrenchReleaseZ_) > 0.40f;
            torchDrenchStepBackLookSeen_ = torchDrenchStepBackLookSeen_ ||
                (torchDrenchWalkingAfterRelease_ &&
                 (std::abs(state.playerYawRadians - torchDrenchReleaseYaw_) > 0.5f ||
                  std::abs(state.playerPitchRadians - torchDrenchReleaseViewPitch_) > 0.10f));
        }
        if (stage_ == MotionStage::TorchDrenchSettled)
        {
            if (!torchDrenchSettledSeen_ || torch.phase != TorchFailurePhase::Settled ||
                torch.heldByPlayer || state.interaction.heldLightKind != interactions::HeldLightKind::None ||
                torch.flameStrength != 0.0f || torch.fallProgress != 1.0f ||
                std::abs(torch.droppedY - kRouteFloorWorldY) > 0.0001f ||
                std::abs(torch.droppedX - torchDrenchReleaseX_) > 0.0001f ||
                std::abs(torch.droppedZ - torchDrenchReleaseZ_) > 0.0001f ||
                std::abs(torch.droppedYawRadians - torchDrenchReleaseYaw_) > 0.0001f ||
                std::abs(torch.droppedViewPitchRadians - torchDrenchReleaseViewPitch_) > 0.0001f)
            { Fail("Settled Rag torch lost its world-owned floor transform or held-light loss."); return; }
            torchDrenchWalkingAfterRelease_ = torchDrenchWalkingAfterRelease_ ||
                std::hypot(state.playerX - torchDrenchReleaseX_, state.playerZ - torchDrenchReleaseZ_) > 0.40f;
            torchDrenchStepBackLookSeen_ = torchDrenchStepBackLookSeen_ ||
                (torchDrenchWalkingAfterRelease_ &&
                 (std::abs(state.playerYawRadians - torchDrenchReleaseYaw_) > 0.5f ||
                  std::abs(state.playerPitchRadians - torchDrenchReleaseViewPitch_) > 0.10f));
            const bool atSettledInspectionPoint =
                Distance(state, kTorchDrenchInspectionX, kTorchDrenchInspectionZ) <= 0.06f &&
                state.walkAmount <= 0.05f;
            if (atSettledInspectionPoint)
            {
                if (torchDrenchInspectionStartSeconds_ < 0.0)
                    torchDrenchInspectionStartSeconds_ = simulationSeconds_;
                else if (simulationSeconds_ - torchDrenchInspectionStartSeconds_ >= 0.35)
                {
                    if (!torchDrenchWalkingAfterRelease_ || !torchDrenchStepBackLookSeen_ ||
                        eventCount != 1u || torchExtinguishedSequence_ == 0u)
                        Fail("Torch-drench inspection missed the step-back/look or once-only event evidence.");
                    else Enter(MotionStage::Complete);
                }
            }
            else torchDrenchInspectionStartSeconds_ = -1.0;
        }
        return;
    }
    if (scenario_ == MotionScenario::WaterfallEquipment)
    {
        const auto& snapshot = state;
        const auto& sword = snapshot.heldItems[1];
        if (snapshot.skeletonEnemyCount != 2u || snapshot.activeSkeletonCount != 2u ||
            snapshot.skeletonEnemies[0].id != EntityId::SkeletonA ||
            snapshot.skeletonEnemies[1].id != EntityId::SkeletonB ||
            snapshot.skeletonEnemies[0].health != 1 || snapshot.skeletonEnemies[1].health != 1 ||
            !snapshot.torchFailure.heldByPlayer || snapshot.torchFailure.triggered)
        { Fail("Equipment motion lost the pre-drench two-guard waterfall encounter."); return; }
        if (sword.parentMode == items::HeldItemParentMode::BodyStow &&
            sword.transition.kind == items::HeldItemTransitionKind::Draw && sword.transition.active)
            equipmentDrawTransitionSeen_ = true;
        equipmentAttackPoseSeen_ = equipmentAttackPoseSeen_ ||
            snapshot.playerCombat.action == PlayerCombatAction::SwingActive;
        equipmentParryPoseSeen_ = equipmentParryPoseSeen_ ||
            snapshot.playerCombat.action == PlayerCombatAction::ParryActive;
        if (stage_ == MotionStage::Approach)
        {
            if (Distance(snapshot, kWaterfallSkeletonPairCenter.x, kWaterfallSkeletonPairCenter.z) >
                    horde::gameplay::kWaterfallSwordCueRadius &&
                (waterfallWarningSequence_ != 0u || swordDrawSequence_ != 0u))
            { Fail("Waterfall draw began outside the authored warning range."); return; }
            if (swordDrawSequence_ != 0u) Enter(MotionStage::EquipmentDraw);
        }
        else if (stage_ == MotionStage::EquipmentDraw)
        {
            if (swordDrawSequence_ == 0u || !equipmentDrawTransitionSeen_)
            { Fail("Waterfall LOS did not start the actual stowed-sword draw."); return; }
            if (swordAttachmentSequence_ != 0u && swordAttachmentSequence_ > swordDrawSequence_ &&
                sword.parentMode == items::HeldItemParentMode::HandSocket && !sword.transition.active)
                Enter(MotionStage::EquipmentAttack);
        }
        else if (stage_ == MotionStage::EquipmentAttack)
        {
            if (equipmentAttackPoseSeen_ && swordSwingSequence_ != 0u &&
                snapshot.playerCombat.action == PlayerCombatAction::Idle)
                Enter(MotionStage::EquipmentParry);
        }
        else if (stage_ == MotionStage::EquipmentParry && equipmentParryPoseSeen_ &&
                 snapshot.playerCombat.action == PlayerCombatAction::Idle)
        {
            if (waterfallWarningSequence_ == 0u || swordDrawSequence_ <= waterfallWarningSequence_ ||
                swordAttachmentSequence_ <= swordDrawSequence_ || swordSwingSequence_ <= swordAttachmentSequence_ ||
                !equipmentAttackPoseSeen_ || !equipmentParryPoseSeen_ ||
                eventCounts_[static_cast<std::size_t>(GameplayEventType::PlayerSwing)] != 1u)
                Fail("Waterfall draw, attachment and swing semantics were absent or out of order.");
            else Enter(MotionStage::Complete);
        }
        if (snapshot.playerCombat.action != PlayerCombatAction::Idle &&
            stage_ == MotionStage::EquipmentDraw &&
            snapshot.playerCombat.action != PlayerCombatAction::SwingWindup &&
            snapshot.playerCombat.action != PlayerCombatAction::SwingActive)
        { Fail("Equipment motion produced an unrelated combat action during draw."); return; }
        return;
    }
    if (KeeperScenario(scenario_) && IsKeeperRevealing(state.lich.revealPhase) &&
        (state.lich.health != 3 || state.lich.damagePulse || state.lich.staffLightStrength != 0.0f ||
         state.playerVitals.vitality != initialVitality_ || state.chestReward.phase != interactions::ChestRewardPhase::Locked))
    { Fail("Real reveal safety or locked reward invariant failed."); return; }
    // Zero-delta ordinary retry resets the reveal clock while its fresh RT
    // output is still paused. Admit only that one consumed, safe reset; normal
    // pause must continue to freeze the clock, including the recognition phase.
    const bool admittedRetryReset = stage_ == MotionStage::RetryRequested && retrySent_ &&
        commands_.retry != 0u && state.lastConsumedRetrySequence >= commands_.retry &&
        initialRetryGeneration_ != UINT32_MAX && state.retryGeneration == initialRetryGeneration_ + 1u &&
        state.playerAlive && state.lich.revealPhase == KeeperRevealPhase::RetryRecognition &&
        state.lich.revealElapsedSeconds == 0.0f &&
        Distance(state, kKeeperRetryPosition.x, kKeeperRetryPosition.z) <= 0.0001f;
    if (state.paused && !admittedRetryReset && state.lich.revealElapsedSeconds != previousRevealElapsed_)
    { Fail("Keeper reveal advanced across paused input."); return; }
    previousRevealElapsed_ = state.lich.revealElapsedSeconds;
    if (scenario_==MotionScenario::ShaftUp && (!state.torchFailure.heldByPlayer || state.torchFailure.triggered))
    { Fail("Water shaft route must retain the active pre-drench torch."); return; }
    if (!state.playerAlive && stage_ != MotionStage::AwaitDeath && stage_ != MotionStage::RetryRequested)
    { Fail("Player died outside the intended actual death/retry stage."); return; }
    const double elapsed = simulationSeconds_ - stageStartSeconds_;
    switch (stage_)
    {
    case MotionStage::Approach:
        if (KeeperScenario(scenario_))
        {
            if (state.lich.revealStarted)
            {
                revealHeldX_ = state.playerX;
                revealHeldZ_ = state.playerZ;
                Enter(MotionStage::Reveal);
            }
        }
        else if (scenario_==MotionScenario::ShaftUp)
        { if (Distance(state,-1.66f,-15.35f)<0.04f) Enter(MotionStage::ShaftMotion); }
        else
        {
            rearLookSeen_=rearLookSeen_ || (std::cos(state.playerYawRadians)< -0.95f && state.playerZ>2.50f);
            rearParallaxSeen_=rearParallaxSeen_ || (state.playerZ>2.50f && state.playerX>0.15f && state.walkAmount>0.1f);
            rearReturnSeen_=rearReturnSeen_ || (elapsed>=5.2 && rearLookSeen_ && rearParallaxSeen_ &&
                Distance(state,kPlayerSpawn.x,kPlayerSpawn.z)<0.06f);
            if (elapsed>=6.0 && Distance(state,0.0f,-2.5f)<0.04f)
            {
                if(!rearLookSeen_ || !rearParallaxSeen_ || !rearReturnSeen_)
                    Fail("Actual rear-collapse look, positive-Z parallax and spawn return were not observed.");
                else Enter(MotionStage::TorchMotion);
            }
        }
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
    {
        if (!state.torchFailure.heldByPlayer || state.torchFailure.triggered ||
            state.interaction.heldLightKind!=interactions::HeldLightKind::Torch || state.finale.skylightOpenProgress!=0.0f)
        { Fail("Water shaft motion lost the actual active torch or admitted pre-drench route state."); break; }
        {
            // Actual upper-screen ray (u=.5,v=.1), with unchanged projection:
            // forward*1.22 + up*.592. It enters the physical waterfall rim;
            // it is not a claim that the enclosed5.6m sky or sprigs are visible.
            const float pitch=state.playerPitchRadians-0.05f;
            const float slope=(1.22f*pitch+0.592f)/(1.22f-0.592f*pitch);
            const float distance=(1.35f-kShowcaseEyeWorldY)/slope;
            const float x=state.playerX+std::sin(state.playerYawRadians)*distance;
            const float z=state.playerZ-std::cos(state.playerYawRadians)*distance;
            shaftOpeningSeen_=shaftOpeningSeen_ || (x> -2.9f && x< -1.58f && z> -16.1f && z< -14.72f);
        }
        shaftParallaxSeen_=shaftParallaxSeen_ || (std::abs(state.playerZ+15.35f)>0.12f && state.walkAmount>0.1f);
        if (elapsed >= 12.0)
        {
            if (!shaftOpeningSeen_ || !shaftParallaxSeen_)
                Fail("Actual upward waterfall rim view and walking parallax were not observed.");
            else Enter(MotionStage::Complete);
        }
        break;
    }
    case MotionStage::Reveal:
        if (IsKeeperRevealing(state.lich.revealPhase) &&
            (Distance(state, revealHeldX_, revealHeldZ_) > 0.0001f ||
             (advanced && state.playerTravelledThisTick != 0.0f) ||
             state.playerCombat.action != PlayerCombatAction::Idle))
        { Fail("Keeper presentation did not hold translation and player actions."); break; }
        // The schedule still tries to retreat and swing during the intro.
        // Validate the new owner contract: reject movement, permit look, and
        // discard the scheduled action edges rather than replaying them later.
        revealHeldMoveSeen_ = revealHeldMoveSeen_ ||
            (state.lich.revealElapsedSeconds > 2.2f && state.lich.revealElapsedSeconds < 2.8f &&
             Distance(state, revealHeldX_, revealHeldZ_) <= 0.0001f);
        revealLookAwaySeen_ = revealLookAwaySeen_ || std::sin(state.playerYawRadians) > 0.95f;
        revealLookReturnSeen_ = revealLookReturnSeen_ ||
            (revealLookAwaySeen_ && std::sin(state.playerYawRadians) < -0.95f);
        if (state.lich.revealComplete)
        {
            if (state.lich.revealElapsedSeconds < LichEncounter::kRevealDuration || state.lich.health != 3 ||
                state.lich.phase != LichPhase::MaintainingRange || eventCounts_[17] != 1u || eventCounts_[18] != 1u || eventCounts_[19] != 1u ||
                !revealHeldMoveSeen_ || !revealLookAwaySeen_ || !revealLookReturnSeen_ ||
                eventCounts_[static_cast<std::size_t>(GameplayEventType::PlayerSwing)] != 0u || eventCounts_[6] != 0u)
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
