#include "gameplay/effects/FireEmitterState.h"
#include "gameplay/simulation/GameSimulation.h"
#include "vulkan/raytracing/FireEmitterBuffer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>

namespace
{

using horde::gameplay::ShowcaseZone;
using horde::gameplay::effects::FireEmitterCheckpoint;
using horde::gameplay::effects::FireEmitterFixedStepInput;
using horde::gameplay::effects::FireEmitterState;
using horde::gameplay::effects::MakeOpeningTorchFireEmitter;
using horde::gameplay::effects::ResetFireEmitter;
using horde::gameplay::effects::StepFireEmitterFixed;
using horde::gameplay::items::HeldItemTransform;
using horde::gameplay::items::IdentityHeldItemTransform;
using horde::vulkan::raytracing::BuildFireEmitterUpload;
using horde::vulkan::raytracing::FireEmitterQuality;
using horde::vulkan::raytracing::FireEmitterSelectionContext;
using horde::vulkan::raytracing::FireEmitterTuning;
using horde::vulkan::raytracing::FireEmitterUpload;

constexpr float kFixedDelta = 1.0f / 60.0f;

bool Require(const bool condition, const char* message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool Approx(const float left, const float right, const float tolerance = 0.00001f)
{
    return std::abs(left - right) <= tolerance;
}

HeldItemTransform Translation(const float x, const float y, const float z)
{
    HeldItemTransform result = IdentityHeldItemTransform();
    result[12] = x;
    result[13] = y;
    result[14] = z;
    return result;
}

FireEmitterFixedStepInput InputAt(const float flameX,
                                  const float flameY,
                                  const float flameZ,
                                  const float lightX,
                                  const float lightY,
                                  const float lightZ,
                                  const float strength = 1.0f,
                                  const float fuel = 1.0f,
                                  const ShowcaseZone zone = ShowcaseZone::Opening)
{
    FireEmitterFixedStepInput input;
    input.worldFromFlame = Translation(flameX, flameY, flameZ);
    input.worldFromLight = Translation(lightX, lightY, lightZ);
    input.strength = strength;
    input.fuel = fuel;
    input.zone = zone;
    return input;
}

bool SameDynamicState(const FireEmitterState& left, const FireEmitterState& right)
{
    return left.stableId == right.stableId &&
           left.seed == right.seed &&
           Approx(left.phase, right.phase) &&
           Approx(left.lowPassNoise, right.lowPassNoise) &&
           Approx(left.leanX, right.leanX) &&
           Approx(left.leanZ, right.leanZ) &&
           Approx(left.motionTurbulence, right.motionTurbulence) &&
           Approx(left.previousPivotVelocity[0], right.previousPivotVelocity[0]) &&
           Approx(left.previousPivotVelocity[1], right.previousPivotVelocity[1]) &&
           Approx(left.previousPivotVelocity[2], right.previousPivotVelocity[2]) &&
           left.motionInitialized == right.motionInitialized;
}

bool TestFixedTickPhaseIsSeededAndDeterministic()
{
    FireEmitterState first = MakeOpeningTorchFireEmitter();
    FireEmitterState second = MakeOpeningTorchFireEmitter();
    const FireEmitterFixedStepInput input = InputAt(1.0f, 2.0f, 3.0f, 1.0f, 1.96f, 3.03f);
    for (int tick = 0; tick < 60; ++tick)
    {
        StepFireEmitterFixed(first, input, kFixedDelta);
        StepFireEmitterFixed(second, input, kFixedDelta);
    }
    return Require(Approx(first.phase, 0.37f, 0.00002f),
                   "sixty fixed ticks must advance the authored phase by 0.37 cycles") &&
           Require(SameDynamicState(first, second),
                   "equal seed and fixed inputs must produce identical emitter state") &&
           Require(first.lowPassNoise >= -1.0f && first.lowPassNoise <= 1.0f,
                   "deterministic low-pass emitter noise must stay normalized");
}

bool TestResetAndCheckpointImportRestoreExactState()
{
    FireEmitterState source = MakeOpeningTorchFireEmitter();
    for (int tick = 0; tick < 24; ++tick)
    {
        const float x = 0.0025f * static_cast<float>(tick * tick);
        StepFireEmitterFixed(source, InputAt(x, 1.25f, -2.0f, x, 1.22f, -1.98f), kFixedDelta);
    }
    const FireEmitterCheckpoint checkpoint = horde::gameplay::effects::ExportFireEmitterCheckpoint(source);

    FireEmitterState imported = MakeOpeningTorchFireEmitter();
    ResetFireEmitter(imported);
    const bool reset = Approx(imported.phase, 0.0f) &&
                       Approx(imported.leanX, 0.0f) &&
                       Approx(imported.leanZ, 0.0f) &&
                       !imported.motionInitialized;
    horde::gameplay::effects::ImportFireEmitterCheckpoint(imported, checkpoint);
    return Require(reset, "route reset must clear dynamic fire phase and motion history") &&
           Require(SameDynamicState(source, imported),
                   "checkpoint import must restore exact deterministic phase and motion state") &&
           Require(imported.worldFromFlame == source.worldFromFlame &&
                       imported.worldFromLight == source.worldFromLight,
                   "checkpoint import must restore the exact Flame and Light socket transforms");
}

FireEmitterState Emitter(const std::uint32_t id,
                         const float x,
                         const ShowcaseZone zone,
                         const float strength = 1.0f)
{
    FireEmitterState emitter = MakeOpeningTorchFireEmitter();
    emitter.stableId = id;
    StepFireEmitterFixed(emitter, InputAt(x, 1.0f, 0.0f, x, 0.96f, 0.03f,
                                          strength, 1.0f, zone), kFixedDelta);
    return emitter;
}

bool TestStableCameraAndZoneSelection()
{
    std::array<FireEmitterState, 4u> emitters{{
        Emitter(40u, 0.25f, ShowcaseZone::Opening),
        Emitter(10u, 7.0f, ShowcaseZone::Opening),
        Emitter(30u, 0.10f, ShowcaseZone::Finale),
        Emitter(20u, 0.50f, ShowcaseZone::Opening)}};
    FireEmitterUpload upload;
    std::string diagnostic;
    const bool built = BuildFireEmitterUpload(
        emitters, {{0.0f, 1.0f, 0.0f}, ShowcaseZone::Opening, 10.0f},
        {}, FireEmitterQuality::Mobile, upload, diagnostic);
    return Require(built, "bounded emitter upload must accept four configured emitters") &&
           Require(upload.activeCount == 2u,
                   "CPU selection must expose no more than two active emitters") &&
           Require(upload.selectedStableIds[0] == 20u && upload.selectedStableIds[1] == 40u,
                   "selection must choose the two nearest matching-zone emitters then publish stable-ID order") &&
           Require(upload.emitters[0].identity[0] == 20u &&
                       upload.emitters[1].identity[0] == 40u,
                   "GPU records must retain the selected emitters' stable identities");
}

bool TestDistanceTieBreaksByStableEmitterId()
{
    std::array<FireEmitterState, 3u> emitters{{
        Emitter(9u, -1.0f, ShowcaseZone::Opening),
        Emitter(7u, 1.0f, ShowcaseZone::Opening),
        Emitter(11u, 1.0f, ShowcaseZone::Opening)}};
    FireEmitterUpload upload;
    std::string diagnostic;
    const bool built = BuildFireEmitterUpload(
        emitters, {{0.0f, 1.0f, 0.0f}, ShowcaseZone::Opening, 4.0f},
        {}, FireEmitterQuality::High, upload, diagnostic);
    return Require(built, "equal-distance selection must build") &&
           Require(upload.activeCount == 2u,
                   "equal-distance selection must retain the bounded two-emitter budget") &&
           Require(upload.selectedStableIds[0] == 7u && upload.selectedStableIds[1] == 9u,
                   "distance ties must be resolved by stable emitter ID independent of input order");
}

bool TestTorchExtinguishZerosStrengthWithoutChangingTransforms()
{
    FireEmitterState emitter = MakeOpeningTorchFireEmitter();
    const FireEmitterFixedStepInput lit = InputAt(-0.4f, 0.82f, -2.1f, -0.4f, 0.79f, -2.075f);
    StepFireEmitterFixed(emitter, lit, kFixedDelta);
    const HeldItemTransform flameBefore = emitter.worldFromFlame;
    const HeldItemTransform lightBefore = emitter.worldFromLight;
    FireEmitterFixedStepInput extinguished = lit;
    extinguished.strength = 0.0f;
    StepFireEmitterFixed(emitter, extinguished, kFixedDelta);

    const auto gpu = horde::vulkan::raytracing::PackFireEmitterGpu(
        emitter, {}, horde::vulkan::raytracing::ResolveFireEmitterQualityBudget(
                         FireEmitterQuality::Mobile));
    return Require(Approx(emitter.strength, 0.0f) && Approx(gpu.lightPositionStrength[3], 0.0f) &&
                       Approx(gpu.colourIntensity[3], 0.0f),
                   "extinguish must zero visible-core and direct-light strength in the shared record") &&
           Require(emitter.worldFromFlame == flameBefore && emitter.worldFromLight == lightBefore,
                   "extinguish must not perturb Flame/Light socket transforms or require geometry motion");
}

bool TestActualPivotAccelerationDrivesBoundedMotionResponse()
{
    FireEmitterState stationary = MakeOpeningTorchFireEmitter();
    FireEmitterState moving = MakeOpeningTorchFireEmitter();
    StepFireEmitterFixed(stationary, InputAt(0.0f, 1.0f, 0.0f, 0.0f, 0.97f, 0.025f), kFixedDelta);
    StepFireEmitterFixed(moving, InputAt(0.0f, 1.0f, 0.0f, 0.0f, 0.97f, 0.025f), kFixedDelta);
    for (int tick = 1; tick <= 8; ++tick)
    {
        StepFireEmitterFixed(stationary, InputAt(0.0f, 1.0f, 0.0f, 0.0f, 0.97f, 0.025f), kFixedDelta);
        const float x = 0.006f * static_cast<float>(tick * tick);
        StepFireEmitterFixed(moving, InputAt(x, 1.0f, 0.0f, x, 0.97f, 0.025f), kFixedDelta);
    }
    return Require(std::abs(moving.leanX) > std::abs(stationary.leanX) + 0.01f,
                   "actual socket acceleration must visibly influence flame lean") &&
           Require(moving.motionTurbulence > stationary.motionTurbulence + 0.01f,
                   "actual socket acceleration must influence bounded turbulence") &&
           Require(std::abs(moving.leanX) <= 0.32f && std::abs(moving.leanZ) <= 0.32f &&
                       moving.motionTurbulence <= 1.0f,
                   "motion response must remain bounded under hand acceleration");
}

horde::gameplay::effects::FireEmitterState RunSimulationAtRenderRate(const int framesPerSecond)
{
    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot input;
    input.moveForward = 0.64f;
    input.moveStrafe = 0.22f;
    input.yawRadians = 0.31f;
    input.pitchRadians = -0.08f;
    for (int frame = 0; frame < framesPerSecond * 2; ++frame)
    {
        simulation.AdvanceFrame(input, 1.0 / static_cast<double>(framesPerSecond),
                                static_cast<std::uint64_t>(frame + 1));
    }
    return simulation.Snapshot().fireEmitters[0];
}

bool TestRenderDeliveryEquivalenceAt30_60_120Hz()
{
    const FireEmitterState at30 = RunSimulationAtRenderRate(30);
    const FireEmitterState at60 = RunSimulationAtRenderRate(60);
    const FireEmitterState at120 = RunSimulationAtRenderRate(120);
    return Require(SameDynamicState(at30, at60) && SameDynamicState(at60, at120),
                   "30/60/120 Hz render delivery must produce identical fixed-step fire phase, flicker, and motion") &&
           Require(at30.worldFromFlame == at60.worldFromFlame &&
                       at60.worldFromFlame == at120.worldFromFlame &&
                       at30.worldFromLight == at120.worldFromLight,
                   "render delivery rate must not change exact Flame/Light socket transforms");
}

bool TestQualityChangesOnlyBoundedRayBudgets()
{
    const FireEmitterState emitter = Emitter(1u, 0.0f, ShowcaseZone::Opening);
    const auto mobileBudget = horde::vulkan::raytracing::ResolveFireEmitterQualityBudget(
        FireEmitterQuality::Mobile);
    const auto highBudget = horde::vulkan::raytracing::ResolveFireEmitterQualityBudget(
        FireEmitterQuality::High);
    auto mobile = horde::vulkan::raytracing::PackFireEmitterGpu(emitter, {}, mobileBudget);
    auto high = horde::vulkan::raytracing::PackFireEmitterGpu(emitter, {}, highBudget);
    const auto mobileIdentity = mobile.identity;
    const auto highIdentity = high.identity;
    mobile.identity[2] = high.identity[2];
    mobile.identity[3] = high.identity[3];
    return Require(mobileBudget.volumeSteps == 6u && mobileBudget.reflectionSamples == 1u &&
                       highBudget.volumeSteps == 10u && highBudget.reflectionSamples == 2u,
                   "Mobile/High must use the authored bounded integration/reflection budgets") &&
           Require(mobile.worldFromLocal0 == high.worldFromLocal0 &&
                       mobile.worldFromLocal1 == high.worldFromLocal1 &&
                       mobile.worldFromLocal2 == high.worldFromLocal2 &&
                       mobile.worldFromLocal3 == high.worldFromLocal3 &&
                       mobile.lightPositionStrength == high.lightPositionStrength &&
                       mobile.colourIntensity == high.colourIntensity &&
                       mobile.shape == high.shape && mobile.animation == high.animation &&
                       mobile.smokeEmbers == high.smokeEmbers && mobile.identity == high.identity,
                   "quality level must not change emitter phase, noise, transform, colour, shape, or art state") &&
           Require(mobileIdentity[2] != highIdentity[2] && mobileIdentity[3] != highIdentity[3],
                   "quality-specific GPU differences must be limited to explicit bounded budgets");
}

bool TestHardCapacityRejectsFifthEmitter()
{
    std::array<FireEmitterState, 5u> emitters{{
        Emitter(1u, 0.0f, ShowcaseZone::Opening), Emitter(2u, 1.0f, ShowcaseZone::Opening),
        Emitter(3u, 2.0f, ShowcaseZone::Opening), Emitter(4u, 3.0f, ShowcaseZone::Opening),
        Emitter(5u, 4.0f, ShowcaseZone::Opening)}};
    FireEmitterUpload upload;
    std::string diagnostic;
    const bool built = BuildFireEmitterUpload(
        std::span<const FireEmitterState>(emitters),
        {{0.0f, 1.0f, 0.0f}, ShowcaseZone::Opening, 10.0f},
        {}, FireEmitterQuality::Mobile, upload, diagnostic);
    return Require(!built, "a fifth configured fire emitter must be rejected") &&
           Require(diagnostic == "FireEmitterBuffer capacity exceeded: 5 configured, maximum 4.",
                   "capacity rejection must be precise and actionable");
}

} // namespace

int main()
{
    bool ok = true;
    ok &= TestFixedTickPhaseIsSeededAndDeterministic();
    ok &= TestResetAndCheckpointImportRestoreExactState();
    ok &= TestStableCameraAndZoneSelection();
    ok &= TestDistanceTieBreaksByStableEmitterId();
    ok &= TestTorchExtinguishZerosStrengthWithoutChangingTransforms();
    ok &= TestActualPivotAccelerationDrivesBoundedMotionResponse();
    ok &= TestRenderDeliveryEquivalenceAt30_60_120Hz();
    ok &= TestQualityChangesOnlyBoundedRayBudgets();
    ok &= TestHardCapacityRejectsFifthEmitter();
    if (!ok) return 1;
    std::cout << "Fire emitter deterministic state, motion, selection, and budget contracts passed\n";
    return 0;
}
