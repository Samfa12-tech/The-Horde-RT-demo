#include "vulkan/raytracing/DielectricMath.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace
{

using horde::vulkan::raytracing::BeerLambert;
using horde::vulkan::raytracing::ClassifyDielectricTerminal;
using horde::vulkan::raytracing::ConstrainClosedVolumeTransmission;
using horde::vulkan::raytracing::ConstrainDirectionToIdealHemisphere;
using horde::vulkan::raytracing::DielectricBudgetResolution;
using horde::vulkan::raytracing::DielectricStack;
using horde::vulkan::raytracing::DielectricTerminalKind;
using horde::vulkan::raytracing::DielectricTerminalResolution;
using horde::vulkan::raytracing::DielectricEnergyPartition;
using horde::vulkan::raytracing::DielectricRayEpsilon;
using horde::vulkan::raytracing::EffectiveDielectricFresnel;
using horde::vulkan::raytracing::EvaluateBoundedShadow;
using horde::vulkan::raytracing::InterfaceTransition;
using horde::vulkan::raytracing::IsDielectricNearSelfHit;
using horde::vulkan::raytracing::OrientInterface;
using horde::vulkan::raytracing::OffsetShadowRayOrigin;
using horde::vulkan::raytracing::RefractDirection;
using horde::vulkan::raytracing::ResolveDielectricInterfaceBudget;
using horde::vulkan::raytracing::ResolveDielectricTerminal;
using horde::vulkan::raytracing::SchlickFresnel;
using horde::vulkan::raytracing::ShadowInterfaceSample;
using horde::vulkan::raytracing::ThinWallTransition;
using horde::vulkan::raytracing::Vec3;

int failures = 0;

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool NearlyEqual(float actual, float expected, float epsilon = 0.00001f)
{
    return std::isfinite(actual) && std::abs(actual - expected) <= epsilon;
}

bool NearlyEqual(const Vec3& actual, const Vec3& expected, float epsilon = 0.00001f)
{
    return NearlyEqual(actual.x, expected.x, epsilon) &&
           NearlyEqual(actual.y, expected.y, epsilon) &&
           NearlyEqual(actual.z, expected.z, epsilon);
}

bool Finite(const Vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void TestSchlickEndpoints()
{
    Check(NearlyEqual(SchlickFresnel(1.0f, 1.0f, 1.5f), 0.04f),
          "air/glass normal incidence is four percent");
    Check(NearlyEqual(SchlickFresnel(0.0f, 1.0f, 1.5f), 1.0f),
          "Schlick reaches one at grazing incidence");
    Check(NearlyEqual(SchlickFresnel(1.0f, 1.5f, 1.0f), 0.04f),
          "normal-incidence Fresnel is symmetric across an interface");
}

void TestNormalOrientationAndSnellDirection()
{
    const auto entry = OrientInterface(Vec3{0.6f, -0.8f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f});
    Check(entry.entering && NearlyEqual(entry.normal, Vec3{0.0f, 1.0f, 0.0f}),
          "entry keeps the outward normal facing the incident ray");
    const auto exit = OrientInterface(Vec3{0.0f, 1.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f});
    Check(!exit.entering && NearlyEqual(exit.normal, Vec3{0.0f, -1.0f, 0.0f}),
          "exit flips the outward normal to face the incident ray");

    Vec3 transmitted{};
    Check(RefractDirection(Vec3{0.6f, -0.8f, 0.0f}, entry.normal,
                           1.0f, 1.5f, transmitted),
          "air/glass Snell refraction has a transmitted direction");
    Check(NearlyEqual(transmitted, Vec3{0.4f, -0.91651514f, 0.0f}),
          "Snell direction bends toward the glass normal");
}

void TestAirGlassAirStack()
{
    DielectricStack<4u> stack;
    Check(stack.Depth() == 0u && NearlyEqual(stack.CurrentIor(), 1.0f),
          "a dielectric stack starts in air");

    const InterfaceTransition entry = stack.Enter(17u, 1.5f);
    Check(entry.accepted && !entry.overflow && NearlyEqual(entry.incidentIor, 1.0f) &&
              NearlyEqual(entry.transmittedIor, 1.5f) && stack.Depth() == 1u &&
              NearlyEqual(stack.CurrentIor(), 1.5f),
          "closed-volume entry pushes glass after resolving air to glass");

    const InterfaceTransition exit = stack.Exit(17u);
    Check(exit.accepted && !exit.overflow && NearlyEqual(exit.incidentIor, 1.5f) &&
              NearlyEqual(exit.transmittedIor, 1.0f) && stack.Depth() == 0u,
          "closed-volume exit pops glass and restores air");
}

void TestStackPairsExactInstanceAndMaterial()
{
    DielectricStack<4u> stack;
    Check(stack.Enter(7u, 17u, 1.5f).accepted,
          "closed-volume entry records an exact instance/material pair");
    Check(!stack.Exit(8u, 17u).accepted && stack.Depth() == 1u,
          "same material on a different instance cannot pop a closed volume");
    Check(!stack.Exit(7u, 18u).accepted && stack.Depth() == 1u,
          "different material on the same instance cannot pop a closed volume");
    Check(stack.Exit(7u, 17u).accepted && stack.Depth() == 0u,
          "the exact paired exit restores the outside medium");

    Check(stack.Enter(3u, 41u, 1.33f).accepted &&
              stack.Enter(4u, 41u, 1.52f).accepted &&
              stack.Exit(4u, 41u).accepted &&
              stack.Exit(3u, 41u).accepted && stack.Depth() == 0u,
          "nested instances may share a material while retaining LIFO instance identity");
}

void TestTotalInternalReflection()
{
    const Vec3 incident{0.8660254f, -0.5f, 0.0f};
    Vec3 transmitted{4.0f, 5.0f, 6.0f};
    Check(!RefractDirection(incident, Vec3{0.0f, 1.0f, 0.0f},
                            1.5f, 1.0f, transmitted),
          "glass/air at sixty degrees reports total internal reflection");
    Check(Finite(transmitted) && NearlyEqual(transmitted, Vec3{}),
          "TIR returns a deterministic finite zero transmission direction");
}

void TestCriticalAngleIsTransmissionBoundary()
{
    Vec3 transmitted{};
    Check(RefractDirection(Vec3{0.5f, -0.8660254038f, 0.0f},
                           Vec3{0.0f, 1.0f, 0.0f}, 2.0f, 1.0f, transmitted),
          "an exact zero Snell discriminant is the critical transmission boundary, not TIR");
    Check(Finite(transmitted) && NearlyEqual(transmitted, Vec3{1.0f, 0.0f, 0.0f}, 0.0001f),
          "critical-angle transmission is finite and tangent to the interface");
}

void TestEffectiveFresnelPartitionsAllValidEnergy()
{
    for (int incidentIndex = 0; incidentIndex <= 6; ++incidentIndex)
    {
        const float incidentIor = 1.0f + static_cast<float>(incidentIndex) * 0.5f;
        for (int transmittedIndex = 0; transmittedIndex <= 6; ++transmittedIndex)
        {
            const float transmittedIor = 1.0f + static_cast<float>(transmittedIndex) * 0.5f;
            for (int cosineIndex = 0; cosineIndex <= 100; ++cosineIndex)
            {
                const float cosine = static_cast<float>(cosineIndex) / 100.0f;
                for (int roughnessIndex = 0; roughnessIndex <= 100; ++roughnessIndex)
                {
                    const float roughness = static_cast<float>(roughnessIndex) / 100.0f;
                    const float fresnel = EffectiveDielectricFresnel(
                        cosine, incidentIor, transmittedIor, roughness);
                    const DielectricEnergyPartition energy =
                        horde::vulkan::raytracing::PartitionDielectricEnergy(
                            fresnel, 1.0f);
                    Check(std::isfinite(fresnel) && fresnel >= 0.0f && fresnel <= 1.0f,
                          "effective Fresnel stays finite and normalized over valid inputs");
                    Check(energy.reflection >= 0.0f && energy.transmission >= 0.0f &&
                              energy.reflection + energy.transmission <= 1.000001f,
                          "rough dielectric reflection and transmission never create energy");
                }
            }
        }
    }
    const float roughGlass = EffectiveDielectricFresnel(1.0f, 1.0f, 1.5f, 1.0f);
    const auto roughGlassEnergy =
        horde::vulkan::raytracing::PartitionDielectricEnergy(roughGlass, 1.0f);
    Check(NearlyEqual(roughGlassEnergy.reflection + roughGlassEnergy.transmission, 1.0f),
          "IOR 1.5 roughness 1 retains one bounded energy budget");

    const float degenerate = EffectiveDielectricFresnel(
        std::numeric_limits<float>::quiet_NaN(), -4.0f,
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN());
    const auto clamped = horde::vulkan::raytracing::PartitionDielectricEnergy(
        degenerate, std::numeric_limits<float>::infinity());
    Check(std::isfinite(degenerate) && Finite(Vec3{clamped.reflection, clamped.transmission, 0.0f}) &&
              clamped.reflection + clamped.transmission <= 1.000001f,
          "degenerate Fresnel and transmission inputs clamp to a finite energy partition");
}

void TestNearestShadowTraversalIsCandidateOrderIndependent()
{
    const std::array<ShadowInterfaceSample, 5u> ordered{{
        {1.0f, 10u, 101u, true, false, 0.90f, 0.0f,
         Vec3{0.25f, 1.0f, 1.0f}, 2.0f},
        {1.5f, 20u, 202u, true, false, 0.80f, 0.0f,
         Vec3{1.0f, 1.0f, 0.25f}, 1.0f},
        {2.5f, 21u, 202u, false, false, 0.80f, 0.0f,
         Vec3{1.0f, 1.0f, 0.25f}, 1.0f},
        {3.0f, 11u, 101u, false, false, 0.90f, 0.0f,
         Vec3{0.25f, 1.0f, 1.0f}, 2.0f},
        {4.0f, 30u, 303u, true, true, 0.75f, 0.0f,
         Vec3{0.5f, 1.0f, 0.25f}, 0.0f},
    }};
    const Vec3 expected{0.1269f, 0.5400f, 0.12285f};
    std::array<int, ordered.size()> permutation{{0, 1, 2, 3, 4}};
    do
    {
        std::array<ShadowInterfaceSample, ordered.size()> shuffled{};
        for (std::size_t index = 0u; index < shuffled.size(); ++index)
            shuffled[index] = ordered[static_cast<std::size_t>(permutation[index])];
        const auto result = EvaluateBoundedShadow<8u, 4u>(
            std::span<const ShadowInterfaceSample>(shuffled), 5.0f);
        Check(!result.blocked && !result.overflow && !result.unclosedVolume &&
                  result.interfaceCount == 5u && NearlyEqual(result.transmittance, expected, 0.0001f),
              "nearest shadow traversal gives identical nested RGB attenuation for every candidate order");
    }
    while (std::next_permutation(permutation.begin(), permutation.end()));
}

void TestShadowBlockersAndUnclosedVolumesFailDeterministically()
{
    const std::array<ShadowInterfaceSample, 2u> opaqueBehindTint{{
        {1.0f, 1u, 11u, true, true, 0.8f, 0.0f,
         Vec3{0.25f, 0.5f, 1.0f}, 0.0f},
        {2.0f, 2u, 12u, true, false, 0.0f, 0.0f,
         Vec3{1.0f, 1.0f, 1.0f}, 0.0f},
    }};
    const auto opaque = EvaluateBoundedShadow<4u, 2u>(opaqueBehindTint, 3.0f);
    Check(opaque.blocked && NearlyEqual(opaque.transmittance, Vec3{}),
          "an opaque blocker terminates tinted shadow transmission");

    auto metallicSamples = opaqueBehindTint;
    metallicSamples[1].transmission = 0.9f;
    metallicSamples[1].metallic = 0.8f;
    const auto metallic = EvaluateBoundedShadow<4u, 2u>(metallicSamples, 3.0f);
    Check(metallic.blocked && NearlyEqual(metallic.transmittance, Vec3{}),
          "a transmissive metal remains a shadow blocker");

    const std::array<ShadowInterfaceSample, 1u> finiteEndpointInsideVolume{{
        {1.0f, 4u, 44u, true, false, 0.9f, 0.0f,
         Vec3{0.25f, 0.81f, 1.0f}, 2.0f},
    }};
    const auto finiteEndpoint = EvaluateBoundedShadow<4u, 2u>(
        finiteEndpointInsideVolume, 3.0f);
    Check(!finiteEndpoint.unclosedVolume && !finiteEndpoint.blocked &&
              NearlyEqual(finiteEndpoint.transmittance, Vec3{0.225f, 0.729f, 0.9f}),
          "a finite shadow endpoint inside a closed medium uses partial Beer-Lambert attenuation");

    const std::array<ShadowInterfaceSample, 2u> mismatchedInstance{{
        {0.5f, 4u, 44u, true, false, 0.9f, 0.0f,
         Vec3{0.5f, 0.8f, 1.0f}, 2.0f, 4u},
        {1.0f, 5u, 44u, false, false, 0.9f, 0.0f,
         Vec3{0.5f, 0.8f, 1.0f}, 2.0f, 5u},
    }};
    const auto mismatched = EvaluateBoundedShadow<4u, 2u>(mismatchedInstance, 3.0f);
    Check(mismatched.unclosedVolume && !mismatched.blocked &&
              NearlyEqual(mismatched.transmittance, Vec3{0.072f, 0.072f, 0.072f}),
          "an exit from a different instance still uses the bounded stack-failure fallback");
}

void TestShadowOriginInsideClosedVolumes()
{
    const std::array<ShadowInterfaceSample, 1u> singleExit{{
        {0.6f, 4u, 44u, false, false, 0.9f, 0.0f,
         Vec3{0.25f, 0.81f, 1.0f}, 1.2f},
    }};
    const auto single = EvaluateBoundedShadow<4u, 2u>(singleExit, 3.0f);
    Check(!single.unclosedVolume && !single.blocked && !single.overflow &&
              single.interfaceCount == 1u &&
              NearlyEqual(single.transmittance, Vec3{0.45f, 0.81f, 0.9f}),
          "a shadow born inside one closed volume attenuates from its origin through the first exit");

    const std::array<ShadowInterfaceSample, 2u> nestedExits{{
        {0.5f, 20u, 202u, false, false, 0.8f, 0.0f,
         Vec3{0.25f, 1.0f, 1.0f}, 1.0f},
        {1.0f, 10u, 101u, false, false, 0.9f, 0.0f,
         Vec3{1.0f, 0.25f, 1.0f}, 2.0f},
    }};
    const auto nested = EvaluateBoundedShadow<4u, 2u>(nestedExits, 3.0f);
    Check(!nested.unclosedVolume && !nested.blocked && !nested.overflow &&
              nested.interfaceCount == 2u &&
              NearlyEqual(nested.transmittance, Vec3{0.36f, 0.36f, 0.72f}),
          "nested closed volumes containing the shadow origin attenuate independently through ordered exits");

    const std::array<ShadowInterfaceSample, 3u> laterUnmatchedExit{{
        {0.5f, 4u, 44u, true, false, 0.9f, 0.0f,
         Vec3{0.5f, 0.8f, 1.0f}, 2.0f},
        {1.0f, 4u, 44u, false, false, 0.9f, 0.0f,
         Vec3{0.5f, 0.8f, 1.0f}, 2.0f},
        {1.5f, 5u, 55u, false, false, 0.9f, 0.0f,
         Vec3{0.5f, 0.8f, 1.0f}, 2.0f},
    }};
    const auto unmatched = EvaluateBoundedShadow<4u, 2u>(laterUnmatchedExit, 3.0f);
    Check(unmatched.unclosedVolume,
          "an unmatched exit after a complete entry-exit pair is not reclassified as an inside-origin segment");
}

void TestMobileAndHighShadowBounds()
{
    const std::array<ShadowInterfaceSample, 5u> thinInterfaces{{
        {0.2f, 1u, 10u, true, true, 0.9f},
        {0.4f, 2u, 11u, true, true, 0.9f},
        {0.6f, 3u, 12u, true, true, 0.9f},
        {0.8f, 4u, 13u, true, true, 0.9f},
        {1.0f, 5u, 14u, true, true, 0.9f},
    }};
    const auto mobileInterfaces = EvaluateBoundedShadow<4u, 2u>(thinInterfaces, 2.0f);
    const auto highInterfaces = EvaluateBoundedShadow<8u, 4u>(thinInterfaces, 2.0f);
    Check(mobileInterfaces.overflow && mobileInterfaces.interfaceCount == 4u &&
              !highInterfaces.overflow && highInterfaces.interfaceCount == 5u,
          "Mobile enforces four shadow interfaces while High accepts the same five-interface path");

    const std::array<ShadowInterfaceSample, 6u> threeNestedVolumes{{
        {0.2f, 1u, 21u, true, false, 0.95f, 0.0f, {1.0f, 1.0f, 1.0f}, 1.0f, 101u},
        {0.4f, 2u, 22u, true, false, 0.95f, 0.0f, {1.0f, 1.0f, 1.0f}, 1.0f, 102u},
        {0.6f, 3u, 23u, true, false, 0.95f, 0.0f, {1.0f, 1.0f, 1.0f}, 1.0f, 103u},
        {0.8f, 4u, 23u, false, false, 0.95f, 0.0f, {1.0f, 1.0f, 1.0f}, 1.0f, 103u},
        {1.0f, 5u, 22u, false, false, 0.95f, 0.0f, {1.0f, 1.0f, 1.0f}, 1.0f, 102u},
        {1.2f, 6u, 21u, false, false, 0.95f, 0.0f, {1.0f, 1.0f, 1.0f}, 1.0f, 101u},
    }};
    const auto mobileVolumes = EvaluateBoundedShadow<8u, 2u>(threeNestedVolumes, 2.0f);
    const auto highVolumes = EvaluateBoundedShadow<8u, 4u>(threeNestedVolumes, 2.0f);
    Check(mobileVolumes.overflow && mobileVolumes.interfaceCount == 3u &&
              !highVolumes.overflow && !highVolumes.unclosedVolume &&
              highVolumes.interfaceCount == 6u,
          "Mobile enforces two nested shadow volumes while High closes the same three-volume path");
}

void TestMillimetreScaleRayAdvance()
{
    const float epsilon = DielectricRayEpsilon(Vec3{-9.1f, -0.3f, -15.2f}, 0.001f);
    Check(epsilon >= 0.00002f && epsilon <= 0.00010f,
          "world-scale dielectric epsilon stays below one tenth millimetre in the authored corridor");
    Check(2.0f * epsilon < 0.001f,
          "entry advance and next-query minimum cannot skip a one millimetre closed volume");
    const float farEpsilon = DielectricRayEpsilon(Vec3{10000.0f, 0.0f, 0.0f}, 5000.0f);
    Check(std::isfinite(farEpsilon) && farEpsilon <= 0.00025f,
          "large finite coordinates retain a bounded sub-millimetre dielectric epsilon");
    const Vec3 grazingDirection{0.020f, 0.0f, 0.9998f};
    const Vec3 advanced = AdvanceDielectricRayOrigin(
        Vec3{0.0f, 0.0f, 0.0f}, Vec3{-1.0f, 0.0f, 0.0f},
        grazingDirection, epsilon);
    Check(advanced.x > epsilon && advanced.x < epsilon * 1.10f &&
              advanced.z > epsilon * 0.99f,
          "grazing entry uses the bounded normal-aware bias and remains far below a one-millimetre wall");
}

void TestGenericShadowOriginKeepsMillimetreClearance()
{
    const Vec3 position{-12.31f, -0.48f, -15.20f};
    const Vec3 normal{-1.0f, 0.0f, 0.0f};
    const Vec3 direction{-0.85f, 0.20f, 0.48f};
    const Vec3 generic = OffsetShadowRayOrigin(
        position, normal, direction, 1.6f, true);
    const Vec3 legacy = OffsetShadowRayOrigin(
        position, normal, direction, 1.6f, false);
    const float genericDistance = std::sqrt(
        (generic.x - position.x) * (generic.x - position.x) +
        (generic.y - position.y) * (generic.y - position.y) +
        (generic.z - position.z) * (generic.z - position.z));
    Check(genericDistance < 0.0015f,
          "generic shadow origin cannot jump a 1.5 mm cage-to-glass clearance");
    Check(NearlyEqual(legacy, Vec3{position.x - 0.004f, position.y, position.z}),
          "legacy-inactive shadow origin retains the reviewed four-millimetre normal offset");
}

void TestRoughClosedVolumeTransmissionReachesPairedBoundary()
{
    const Vec3 ideal = horde::vulkan::raytracing::dielectric_detail::Normalize(
        Vec3{0.20f, -0.95f, 0.24f}, Vec3{0.0f, -1.0f, 0.0f});
    const Vec3 rough = horde::vulkan::raytracing::dielectric_detail::Normalize(
        Vec3{0.34f, -0.88f, 0.33f}, Vec3{0.0f, -1.0f, 0.0f});
    Check(NearlyEqual(
              ConstrainClosedVolumeTransmission(ideal, rough, false, true), ideal),
          "rough thick-volume entry remains on the geometric Snell path to its paired boundary");
    Check(NearlyEqual(
              ConstrainClosedVolumeTransmission(ideal, rough, false, false), rough) &&
              NearlyEqual(
                  ConstrainClosedVolumeTransmission(ideal, rough, true, true), rough),
          "roughness remains directional at thick exits and thin-wall interfaces");
    Check(NearlyEqual(
              ConstrainClosedVolumeTransmission(ideal, ideal, false, true), ideal) &&
              NearlyEqual(
                  ConstrainClosedVolumeTransmission(ideal, ideal, false, false), ideal),
          "smooth thick entry and exit preserve the ideal Snell direction");

    const Vec3 grazingIdeal = horde::vulkan::raytracing::dielectric_detail::Normalize(
        Vec3{1.0f, 0.00002f, 0.0f}, Vec3{1.0f, 0.0f, 0.0f});
    const Vec3 crossedRough = horde::vulkan::raytracing::dielectric_detail::Normalize(
        Vec3{1.0f, -0.004f, 0.0f}, Vec3{1.0f, 0.0f, 0.0f});
    const Vec3 constrained = ConstrainDirectionToIdealHemisphere(
        grazingIdeal, crossedRough, Vec3{0.0f, 1.0f, 0.0f});
    Check(constrained.y > 0.0f && NearlyEqual(
              std::sqrt(constrained.x * constrained.x + constrained.y * constrained.y +
                        constrained.z * constrained.z), 1.0f),
          "rough reflection and exit lobes cannot cross the physical ideal interface hemisphere at grazing angles");
}

void TestBoundedTirAndWaterTerminationContracts()
{
    Check(ResolveDielectricInterfaceBudget(1u, 8u) ==
              DielectricBudgetResolution::AbsorbTrappedTir &&
              ResolveDielectricInterfaceBudget(4u, 1u) ==
              DielectricBudgetResolution::AbsorbTrappedTir,
          "Mobile and High interface limits absorb energy still trapped by TIR inside bounded media");
    Check(ResolveDielectricInterfaceBudget(0u, 8u) ==
              DielectricBudgetResolution::Overflow &&
              ResolveDielectricInterfaceBudget(1u, 0u) ==
              DielectricBudgetResolution::Overflow,
          "closed-stack and non-TIR budget exhaustion remain explicit transport overflows");
    Check(ClassifyDielectricTerminal(true, false, true) ==
              DielectricTerminalKind::Water &&
              ClassifyDielectricTerminal(true, true, false) ==
              DielectricTerminalKind::ContinueGeneric &&
              ClassifyDielectricTerminal(true, false, false) ==
              DielectricTerminalKind::Opaque &&
              ClassifyDielectricTerminal(false, false, false) ==
              DielectricTerminalKind::Miss,
          "water terminates the bounded generic path without being reclassified as opaque or recursively continued");
    Check(ResolveDielectricTerminal(0u, DielectricTerminalKind::Water) ==
              DielectricTerminalResolution::ShadeTerminal &&
              ResolveDielectricTerminal(0u, DielectricTerminalKind::Opaque) ==
              DielectricTerminalResolution::ShadeTerminal &&
              ResolveDielectricTerminal(0u, DielectricTerminalKind::Miss) ==
              DielectricTerminalResolution::ShadeTerminal,
          "water, opaque, and miss terminals use their ordinary terminal path after a paired stack exit");
    Check(ResolveDielectricTerminal(1u, DielectricTerminalKind::Water) ==
              DielectricTerminalResolution::AbsorbUnresolvedClosedVolume &&
              ResolveDielectricTerminal(2u, DielectricTerminalKind::Opaque) ==
              DielectricTerminalResolution::AbsorbUnresolvedClosedVolume &&
              ResolveDielectricTerminal(4u, DielectricTerminalKind::Miss) ==
              DielectricTerminalResolution::AbsorbUnresolvedClosedVolume,
          "an ordinary terminal reached before a validated closed-volume exit is conservatively absorbed at Mobile and High depths");
    Check(ResolveDielectricTerminal(4u, DielectricTerminalKind::ContinueGeneric) ==
              DielectricTerminalResolution::ContinueGeneric,
          "a generic dielectric terminal always continues to exact instance/material stack validation");
}

void TestSelfHitClassificationUsesBoundedEpsilon()
{
    const float epsilon = DielectricRayEpsilon(
        Vec3{-12.5f, 0.12f, -15.42f}, 1.55f);
    Check(IsDielectricNearSelfHit(epsilon * 7.99f, epsilon) &&
              !IsDielectricNearSelfHit(epsilon * 8.01f, epsilon),
          "secondary dielectric self-hit attribution has an exact eight-epsilon boundary");
    Check(!IsDielectricNearSelfHit(0.0015f, epsilon),
          "a distinct millimetre-clearance boundary is never attributed as an epsilon self-hit");
}

void TestBeerLambertAttenuation()
{
    Check(NearlyEqual(BeerLambert(Vec3{0.25f, 0.5f, 1.0f}, 2.0f, 2.0f),
                      Vec3{0.25f, 0.5f, 1.0f}),
          "one attenuation distance reproduces the authored attenuation colour");
    Check(NearlyEqual(BeerLambert(Vec3{0.25f, 0.5f, 1.0f}, 1.0f, 2.0f),
                      Vec3{0.5f, 0.70710678f, 1.0f}),
          "Beer-Lambert uses actual path length relative to attenuation distance");
    Check(NearlyEqual(BeerLambert(Vec3{0.25f, 0.5f, 1.0f}, 9.0f,
                                  std::numeric_limits<float>::infinity()),
                      Vec3{1.0f, 1.0f, 1.0f}),
          "infinite attenuation distance is lossless");
}

void TestThinWallDoesNotMutateStack()
{
    DielectricStack<4u> stack;
    Check(stack.Enter(9u, 1.33f).accepted, "nested medium setup succeeds");
    const ThinWallTransition wall = stack.ThinWall(1.52f);
    Check(NearlyEqual(wall.outsideIor, 1.33f) && NearlyEqual(wall.wallIor, 1.52f) &&
              stack.Depth() == 1u && NearlyEqual(stack.CurrentIor(), 1.33f),
          "thin-wall entry and exit leave the closed-volume stack unchanged");
}

void TestBoundedOverflowAndMismatchedExit()
{
    DielectricStack<2u> stack;
    Check(stack.Enter(1u, 1.2f).accepted && stack.Enter(2u, 1.4f).accepted,
          "stack fills to its exact closed-volume budget");
    const InterfaceTransition overflow = stack.Enter(3u, 1.6f);
    Check(!overflow.accepted && overflow.overflow && stack.Depth() == 2u &&
              NearlyEqual(stack.CurrentIor(), 1.4f),
          "closed-volume overflow is reported without corrupting the stack");
    const InterfaceTransition mismatch = stack.Exit(1u);
    Check(!mismatch.accepted && !mismatch.overflow && stack.Depth() == 2u &&
              NearlyEqual(stack.CurrentIor(), 1.4f),
          "out-of-order exit terminates deterministically without popping another volume");
}

void TestDegenerateInputsStayFinite()
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    Check(std::isfinite(SchlickFresnel(nan, 0.0f, nan)),
          "degenerate Fresnel inputs remain finite");
    Vec3 transmitted{};
    (void)RefractDirection(Vec3{nan, 0.0f, 0.0f}, Vec3{}, 0.0f, nan, transmitted);
    Check(Finite(transmitted), "degenerate Snell inputs remain finite");
    Check(Finite(BeerLambert(Vec3{nan, -1.0f, 4.0f}, nan, 0.0f)),
          "degenerate Beer-Lambert inputs remain finite");
    const auto orientation = OrientInterface(Vec3{}, Vec3{});
    Check(Finite(orientation.normal), "degenerate interface orientation remains finite");

    DielectricStack<1u> stack;
    const InterfaceTransition entry = stack.Enter(4u, nan);
    Check(entry.accepted && std::isfinite(entry.transmittedIor) &&
              std::isfinite(stack.CurrentIor()),
          "degenerate material IOR is sanitized before entering the stack");
}

} // namespace

int main()
{
    TestSchlickEndpoints();
    TestNormalOrientationAndSnellDirection();
    TestAirGlassAirStack();
    TestStackPairsExactInstanceAndMaterial();
    TestTotalInternalReflection();
    TestCriticalAngleIsTransmissionBoundary();
    TestEffectiveFresnelPartitionsAllValidEnergy();
    TestNearestShadowTraversalIsCandidateOrderIndependent();
    TestShadowBlockersAndUnclosedVolumesFailDeterministically();
    TestShadowOriginInsideClosedVolumes();
    TestMobileAndHighShadowBounds();
    TestMillimetreScaleRayAdvance();
    TestGenericShadowOriginKeepsMillimetreClearance();
    TestRoughClosedVolumeTransmissionReachesPairedBoundary();
    TestBoundedTirAndWaterTerminationContracts();
    TestSelfHitClassificationUsesBoundedEpsilon();
    TestBeerLambertAttenuation();
    TestThinWallDoesNotMutateStack();
    TestBoundedOverflowAndMismatchedExit();
    TestDegenerateInputsStayFinite();
    if (failures == 0)
    {
        std::cout << "Dielectric math tests passed.\n";
        return 0;
    }
    std::cerr << failures << " dielectric math test(s) failed.\n";
    return 1;
}
