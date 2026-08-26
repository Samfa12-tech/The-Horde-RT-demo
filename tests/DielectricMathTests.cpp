#include "vulkan/raytracing/DielectricMath.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{

using horde::vulkan::raytracing::BeerLambert;
using horde::vulkan::raytracing::DielectricStack;
using horde::vulkan::raytracing::InterfaceTransition;
using horde::vulkan::raytracing::OrientInterface;
using horde::vulkan::raytracing::RefractDirection;
using horde::vulkan::raytracing::SchlickFresnel;
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
    TestTotalInternalReflection();
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
