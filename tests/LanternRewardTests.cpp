#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "gameplay/interactions/ChestRewardSequence.h"
#include "gameplay/interactions/FinaleSequence.h"
#include "gameplay/interactions/InteractionState.h"
#include "gameplay/simulation/InputMailbox.h"

namespace
{

using namespace horde::gameplay::interactions;
using horde::gameplay::simulation::InputMailbox;
using horde::gameplay::simulation::InputSnapshot;

bool passed = true;

void Check(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "Lantern reward test failed: " << message << '\n';
        passed = false;
    }
}

bool NearlyEqual(const float left, const float right, const float tolerance = 0.0002f)
{
    return std::abs(left - right) <= tolerance;
}

InteractionQuery ValidChestQuery()
{
    return {
        kRewardChestInteractionPosition.x + 1.30f,
        kRewardChestInteractionPosition.z,
        -1.57079632679f};
}

} // namespace

int main()
{
    InputSnapshot publication;
    publication.commands.attack = 7u;
    publication.commands.interact = 11u;
    publication.commands.toggleHeldLightPose = 13u;
    publication.moveForward = 0.25f;
    InputMailbox mailbox;
    const std::uint64_t publicationSequence = mailbox.Publish(publication);
    const auto consumed = mailbox.ConsumeLatest();
    Check(consumed.publicationSequence == publicationSequence &&
              consumed.snapshot.commands.attack == 7u &&
              consumed.snapshot.commands.interact == 11u &&
              consumed.snapshot.commands.toggleHeldLightPose == 13u &&
              consumed.snapshot.moveForward == 0.25f,
          "the coherent mailbox publication must carry both appended command sequences");

    ChestRewardSequence chest;
    Check(chest.Snapshot().phase == ChestRewardPhase::Locked,
          "the route-local chest must begin locked");
    Check(chest.TryInteract(ValidChestQuery()) == ChestRewardAction::None,
          "an interact edge before unlock must be rejected without changing phase");
    Check(chest.Unlock() && !chest.Unlock() &&
              chest.Snapshot().phase == ChestRewardPhase::ClosedUnlocked,
          "lich defeat must unlock the chest exactly once");

    InteractionQuery tooFar = ValidChestQuery();
    tooFar.playerX += 0.051f;
    Check(chest.TryInteract(tooFar) == ChestRewardAction::None,
          "interaction beyond 1.35 metres must be rejected");
    InteractionQuery facingAway = ValidChestQuery();
    facingAway.playerYawRadians = 1.57079632679f;
    Check(chest.TryInteract(facingAway) == ChestRewardAction::None,
          "interaction outside the 55 degree facing cone must be rejected");

    Check(chest.TryInteract(ValidChestQuery()) == ChestRewardAction::OpeningStarted &&
              chest.Snapshot().phase == ChestRewardPhase::Opening,
          "the first valid post-unlock edge must begin the one opening transition");
    Check(chest.TryInteract(ValidChestQuery()) == ChestRewardAction::None,
          "an interact edge during opening must be consumed without buffering a claim");
    chest.Update(1.19f);
    Check(chest.Snapshot().phase == ChestRewardPhase::Opening &&
              chest.Snapshot().lidOpenProgress > 0.98f &&
              chest.Snapshot().lidOpenProgress < 1.0f,
          "the lid must remain in its deterministic 1.20 second opening phase");
    chest.Update(0.01f);
    Check(chest.Snapshot().phase == ChestRewardPhase::LanternAvailable &&
              NearlyEqual(chest.Snapshot().lidOpenProgress, 1.0f),
          "the reward must become available exactly when the lid reaches its target");
    Check(chest.TryInteract(ValidChestQuery()) == ChestRewardAction::LanternClaimed &&
              chest.Snapshot().phase == ChestRewardPhase::LanternClaimed &&
              chest.TryInteract(ValidChestQuery()) == ChestRewardAction::None,
          "the second post-open edge must claim the reward exactly once");

    InteractionState interaction;
    EquipRewardLantern(interaction);
    Check(interaction.heldLightKind == HeldLightKind::RewardLantern &&
              interaction.heldLightPose == HeldLightPose::TransitioningToHigh &&
              NearlyEqual(interaction.heldLightPoseProgress, 0.0f),
          "claim must begin the shared 0.65 second high carry transition");
    AdvanceHeldLightPose(interaction, 0.64f);
    Check(interaction.heldLightPose == HeldLightPose::TransitioningToHigh,
          "the pickup raise must not finish early");
    AdvanceHeldLightPose(interaction, 0.01f);
    Check(interaction.heldLightPose == HeldLightPose::High &&
              NearlyEqual(interaction.heldLightPoseProgress, 1.0f),
          "the pickup raise must settle high at 0.65 seconds");
    Check(RequestHeldLightPoseToggle(interaction),
          "a settled claimed lantern must accept one high-to-low edge");
    Check(!RequestHeldLightPoseToggle(interaction),
          "a transition must consume further pose edges without buffering");
    AdvanceHeldLightPose(interaction, kHeldLightPoseTransitionSeconds);
    Check(interaction.heldLightPose == HeldLightPose::Low,
          "the shared carry transition must settle low deterministically");

    FinaleSequence finale;
    Check(finale.NotifyLichDefeated() && !finale.NotifyLichDefeated() &&
              finale.Snapshot().endingPhase == FinaleEndingPhase::LichFalling,
          "the extracted finale must accept lich defeat exactly once");
    finale.Update(20.0f);
    Check(finale.Snapshot().endingPhase == FinaleEndingPhase::LichFalling &&
              NearlyEqual(finale.Snapshot().skylightOpenProgress, 0.0f),
          "defeat and platform polling must not advance the roof before reward claim");
    Check(finale.NotifyLanternClaimed(),
          "the first claim must start the extracted raise/reveal sequence");
    finale.Update(kFinaleLanternRaiseSeconds - 0.01f);
    Check(finale.Snapshot().phase == FinaleSequencePhase::RaisingLantern,
          "the finale reveal must wait for the complete pickup raise");
    finale.Update(0.01f + kFinaleLanternRevealSeconds - 0.01f);
    Check(finale.Snapshot().phase == FinaleSequencePhase::RevealingLantern &&
              NearlyEqual(finale.Snapshot().skylightOpenProgress, 0.0f),
          "the roof must remain closed through the 1.25 second reward reveal");
    finale.Update(0.01f + kFinaleSkylightOpenSeconds);
    Check(finale.Snapshot().endingPhase == FinaleEndingPhase::DawnRevealed &&
              NearlyEqual(finale.Snapshot().skylightOpenProgress, 1.0f),
          "the extracted sequence must own the existing 4.50 second roof clock");
    finale.Update(kFinaleDawnRevealSeconds);
    Check(finale.Snapshot().endingPhase == FinaleEndingPhase::Complete &&
              NearlyEqual(finale.Snapshot().dawnRevealProgress, 1.0f),
          "the extracted sequence must own the existing 1.75 second dawn clock");
    const FinaleSequenceSnapshot complete = finale.Snapshot();
    finale.Update(10.0f);
    Check(finale.Snapshot() == complete,
          "repeated ending polls after completion must not change shared finale state");

    chest.Reset();
    finale.Reset();
    ResetInteractionState(interaction, HeldLightKind::Torch);
    Check(chest.Snapshot().phase == ChestRewardPhase::Locked &&
              finale.Snapshot().phase == FinaleSequencePhase::Inactive &&
              interaction.heldLightKind == HeldLightKind::Torch,
          "route reset/retry/import foundations must restore reward authorities without stale state");

    if (!passed) return EXIT_FAILURE;
    std::cout << "Lantern reward interaction, command, and finale tests passed\n";
    return EXIT_SUCCESS;
}
