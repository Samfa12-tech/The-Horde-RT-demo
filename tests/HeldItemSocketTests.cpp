#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/HeldItemState.h"
#include "vulkan/raytracing/HeldItemRenderSlot.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using horde::gameplay::items::HeldHand;
using horde::gameplay::items::HeldItemId;
using horde::gameplay::items::HeldItemParentMode;
using horde::gameplay::items::HeldItemState;
using horde::gameplay::items::HeldItemTransform;

int failures = 0;

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool Near(const float actual, const float expected, const float tolerance = 0.0001f)
{
    return std::abs(actual - expected) <= tolerance;
}

HeldItemTransform Translation(const float x, const float y, const float z)
{
    HeldItemTransform result = horde::gameplay::items::IdentityHeldItemTransform();
    result[12] = x;
    result[13] = y;
    result[14] = z;
    return result;
}

void TestSocketLookupIsNamedAndOrderIndependent()
{
    const std::vector<horde::scene::assets::StaticSocket> sockets{
        {"Flame", 3u, Translation(0.0f, 0.8f, 0.0f)},
        {"Grip", 1u, Translation(0.0f, -0.2f, 0.0f)},
        {"Light", 4u, Translation(0.0f, 0.9f, 0.0f)},
    };
    const auto* grip = horde::gameplay::items::FindHeldItemSocket(sockets, "Grip");
    Check(grip != nullptr && grip->nodeTransformIndex == 1u && Near(grip->world[13], -0.2f),
          "Grip lookup must use the exact socket name rather than vector order");
    Check(horde::gameplay::items::FindHeldItemSocket(sockets, "grip") == nullptr,
          "socket names must retain the authored case-sensitive contract");
}

void TestWorldFromItemUsesRequiredCompositionOrder()
{
    HeldItemTransform worldFromRightHand{{
        0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        4.0f, 5.0f, 6.0f, 1.0f}};
    const HeldItemTransform itemFromGrip = Translation(0.0f, -2.0f, 0.0f);

    HeldItemTransform worldFromItem{};
    std::string diagnostic;
    Check(horde::gameplay::items::ComposeWorldFromItem(
              worldFromRightHand, itemFromGrip, worldFromItem, diagnostic),
          "a rigid hand and grip transform must compose");
    Check(Near(worldFromItem[12], 2.0f) && Near(worldFromItem[13], 5.0f) &&
              Near(worldFromItem[14], 6.0f),
          "worldFromItem must equal worldFromHandSocket * inverse(itemFromGrip)");
}

void TestScaledGripSocketIsRejected()
{
    HeldItemTransform scaledGrip = Translation(0.0f, -0.2f, 0.0f);
    scaledGrip[0] = 1.25f;
    std::string diagnostic;
    Check(!horde::gameplay::items::ValidateHeldItemSocketTransform(scaledGrip, diagnostic) &&
              diagnostic == "Held-item socket transform must be rigid and unit scale.",
          "scaled Grip sockets must be rejected before attachment");
}

void TestLeftAndRightHandsCannotBeSwapped()
{
    const HeldItemTransform left = Translation(-3.0f, 1.0f, 2.0f);
    const HeldItemTransform right = Translation(7.0f, 1.0f, 2.0f);
    const auto selectedLeft = horde::gameplay::items::SelectHandSocketTransform(
        HeldHand::LeftHand, left, right);
    const auto selectedRight = horde::gameplay::items::SelectHandSocketTransform(
        HeldHand::RightHand, left, right);
    Check(Near(selectedLeft[12], -3.0f) && Near(selectedRight[12], 7.0f),
          "LeftHand and RightHand must retain distinct transforms");
}

void TestWallRetractionMovesHandAndAttachedItemTogether()
{
    const HeldItemTransform grip = Translation(0.0f, -0.25f, 0.0f);
    const HeldItemTransform nominalHand = Translation(-0.34f, 0.18f, -1.05f);
    const HeldItemTransform retractedHand = Translation(-0.34f, 0.18f, -0.62f);
    HeldItemTransform nominalItem{};
    HeldItemTransform retractedItem{};
    std::string diagnostic;
    Check(horde::gameplay::items::ComposeWorldFromItem(
              nominalHand, grip, nominalItem, diagnostic) &&
              horde::gameplay::items::ComposeWorldFromItem(
                  retractedHand, grip, retractedItem, diagnostic),
          "wall retraction fixtures must compose");
    Check(Near(retractedHand[14] - nominalHand[14], 0.43f) &&
              Near(retractedItem[14] - nominalItem[14], 0.43f) &&
              Near(retractedItem[13] - retractedHand[13], 0.25f),
          "wall retraction must move the shared hand target, preserving Grip attachment");
}

void TestTorchDetachesOnceWithTransformContinuity()
{
    HeldItemState torch = horde::gameplay::items::MakeHeldItemState(
        HeldItemId::OriginalTorch, HeldHand::LeftHand);
    const HeldItemTransform release = Translation(-1.8f, -0.22f, -15.2f);
    horde::gameplay::items::UpdateHeldItemParent(
        torch, HeldItemParentMode::HandSocket, 41u, release);
    const HeldItemTransform before = torch.worldFromItem;
    horde::gameplay::items::UpdateHeldItemParent(
        torch, HeldItemParentMode::AuthoredWorldTrajectory, 42u, release);
    const std::uint64_t detachTick = torch.detachTick;
    horde::gameplay::items::UpdateHeldItemParent(
        torch, HeldItemParentMode::AuthoredWorldTrajectory, 43u,
        Translation(-1.8f, -0.5f, -15.2f));

    Check(torch.detached && detachTick == 42u && torch.detachTick == detachTick,
          "original torch must detach exactly once at the first authored trajectory tick");
    Check(Near(before[12], release[12]) && Near(before[13], release[13]) &&
              Near(before[14], release[14]),
          "the detach boundary must preserve the last hand-authored world transform");
}

void TestResetAndCheckpointImportRestoreParentContracts()
{
    auto items = horde::gameplay::items::MakeDefaultHeldItemStates();
    horde::gameplay::items::ImportHeldItemCheckpoint(items, false, 900u);
    Check(items[0].parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
              items[0].detached && items[0].detachTick == 900u &&
              items[1].parentMode == HeldItemParentMode::HandSocket,
          "post-drop checkpoint import must detach only the original torch");
    horde::gameplay::items::ResetHeldItemStates(items);
    Check(items[0].id == HeldItemId::OriginalTorch &&
              items[0].parentMode == HeldItemParentMode::HandSocket && !items[0].detached &&
              items[1].id == HeldItemId::Sword &&
              items[1].parentMode == HeldItemParentMode::HandSocket,
          "route reset must restore both canonical held-item attachments");
}

void TestRenderSlotConvertsGenericTransformWithoutItemBranches()
{
    HeldItemState sword = horde::gameplay::items::MakeHeldItemState(
        HeldItemId::Sword, HeldHand::RightHand);
    sword.worldFromItem = Translation(2.0f, 3.0f, 4.0f);
    const auto instance = horde::vulkan::raytracing::HeldItemRenderSlot::BuildInstanceTransform(sword);
    Check(Near(instance[3], 2.0f) && Near(instance[7], 3.0f) && Near(instance[11], 4.0f),
          "generic held-item render slot must preserve matrix translation in Vulkan 3x4 order");
}

} // namespace

int main()
{
    TestSocketLookupIsNamedAndOrderIndependent();
    TestWorldFromItemUsesRequiredCompositionOrder();
    TestScaledGripSocketIsRejected();
    TestLeftAndRightHandsCannotBeSwapped();
    TestWallRetractionMovesHandAndAttachedItemTogether();
    TestTorchDetachesOnceWithTransformContinuity();
    TestResetAndCheckpointImportRestoreParentContracts();
    TestRenderSlotConvertsGenericTransformWithoutItemBranches();
    if (failures == 0)
    {
        std::cout << "Held-item socket contracts passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
