#include "gameplay/BenchmarkProfile.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

using namespace horde::gameplay;
using namespace horde::graphics;
namespace {
unsigned checks = 0, failures = 0;
void Check(bool result, const char* message) { ++checks; if (!result) { ++failures; std::cerr << message << '\n'; } }
BenchmarkProfileConfiguration Personal(GraphicsPlatform platform)
{
    BenchmarkProfileConfiguration c;
    c.platform = platform; c.optical = platform == GraphicsPlatform::Android ? OpticalProfile::Mobile : OpticalProfile::High;
    c.backend = GraphicsBackend::RayQueryCompute;
    c.graphics = {75, WaterQuality::Mobile, FireDetail::Low, 60, true, ShadowQuality::Higher, true, DustQuality::Standard};
    c.output = {1280, 800}; c.internal = {960, 600}; c.cameraAspect = 1.6;
    c.dynamicResolutionEnabled = true; c.workload = BenchmarkWorkload::LanternHeldLow;
    return c;
}
void TestLifecycle(BenchmarkProfileId id, GraphicsPlatform platform)
{
    auto working = Personal(platform); const auto saved = working;
    {
        BenchmarkProfileRun run;
        Check(run.Begin(id, working), "provisional selection applies only a scoped working tuple");
        Check(working.temporaryOverrideActive && MatchesBenchmarkProfile(id, working) &&
            !working.dynamicResolutionEnabled && working.graphics.previewFrameCap == saved.graphics.previewFrameCap,
            "fixed actual pixels, compiled profile, effects, aspect, workload and DRS-off are selected without changing personal preview cap");
        const auto spec = BenchmarkProfileSpecification(id);
        Check(spec && working.internal == spec->internal && working.output == spec->output,
            "standard contract pins both actual extents, not just a display-relative percentage");
        Check(!run.Begin(id, working), "active scope cannot replace its original restoration snapshot");
        BenchmarkProfileRun nested;
        Check(!nested.Begin(BenchmarkProfileId::Custom, working), "second scope cannot acquire an already overridden owner");
        auto observed = working; observed.cameraAspect = static_cast<float>(working.cameraAspect);
        Check(run.Observe(observed), "truthful float camera ratio is accepted within the declared aspect tolerance");
        Check(run.Complete() && run.Status() == BenchmarkProfileRunStatus::Complete,
            "success requires an observed matching tuple before exact restoration");
        Check(working == saved, "complete restores every personal field exactly");
        run.Cancel(); run.Fail(); Check(working == saved, "late terminal calls cannot alter restored personal state");
    }
    for (unsigned exit = 0; exit < 4; ++exit) {
        try {
            BenchmarkProfileRun run; Check(run.Begin(id, working), "fresh bounded scope begins");
            if (exit == 0) run.Cancel();
            if (exit == 1) run.Fail();
            if (exit == 2) Check(!run.Complete() && run.Status() == BenchmarkProfileRunStatus::Failed,
                "unobserved requested pixels cannot certify successful application");
            if (exit == 3) throw std::runtime_error("simulated platform apply error");
        } catch (const std::runtime_error&) {}
        Check(working == saved, "cancel, apply failure, incomplete application and exception unwind restore exact settings");
    }
    {
        BenchmarkProfileRun run; Check(run.Begin(id, working), "scope begins before destructor cancellation");
    }
    Check(working == saved, "abandoned active scope restores on destruction");

    // Every deviation is observed between otherwise matching endpoints. Failure
    // must latch and restore, never recover by accepting a later matching sample.
    for (unsigned mutation = 0; mutation < 16; ++mutation) {
        BenchmarkProfileRun run; Check(run.Begin(id, working), "fresh mismatch case begins");
        const auto expected = working; auto actual = working;
        switch (mutation) {
        case 0: ++actual.internal.width; break;
        case 1: --actual.internal.height; break;
        case 2: ++actual.output.width; actual.cameraAspect = double(actual.output.width) / actual.output.height; break;
        case 3: ++actual.output.height; actual.cameraAspect = double(actual.output.width) / actual.output.height; break;
        case 4: actual.cameraAspect += 2e-5; break;
        case 5: actual.outputPolicy = BenchmarkOutputPolicy::Native; break;
        case 6: actual.dynamicResolutionEnabled = true; break;
        case 7: actual.graphics.fireDetail = actual.graphics.fireDetail == FireDetail::Low ? FireDetail::High : FireDetail::Low; break;
        case 8: actual.graphics.waterQuality = WaterQuality::Off; break;
        case 9: actual.graphics.shadowQuality = ShadowQuality::Higher; break;
        case 10: actual.graphics.glassEnabled = !actual.graphics.glassEnabled; break;
        case 11: actual.graphics.mistEnabled = !actual.graphics.mistEnabled; break;
        case 12: actual.graphics.dustQuality = actual.graphics.dustQuality == DustQuality::Off ? DustQuality::Low : DustQuality::Off; break;
        case 13: actual.workload = BenchmarkWorkload::LanternMotionExtreme; break;
        case 14: actual.backend = GraphicsBackend::RayTracingPipeline; break;
        case 15: actual.optical = actual.optical == OpticalProfile::High ? OpticalProfile::Mobile : OpticalProfile::High; break;
        }
        Check(!run.Observe(actual) && run.Status() == BenchmarkProfileRunStatus::Failed && working == saved,
            "mid-run dimension, aspect, policy, DRS, effect, workload or immutable backend drift fails and restores");
        Check(!run.Observe(expected) && !run.Complete() && working == saved,
            "matching closing state cannot erase a retained mid-run failure");
    }
}
}
int main()
{
    static_assert(!std::is_copy_constructible_v<BenchmarkProfileRun> && !std::is_move_constructible_v<BenchmarkProfileRun>);
    Check(ParseBenchmarkProfile("custom") == BenchmarkProfileId::Custom &&
        ParseBenchmarkProfile("mobile-rt-provisional-v1") == BenchmarkProfileId::MobileProvisionalV1 &&
        ParseBenchmarkProfile("windows-rt-provisional-v1") == BenchmarkProfileId::WindowsProvisionalV1,
        "closed versioned names distinguish platform profiles and explicit custom runs");
    for (const auto bad : {"", "Mobile-rt-provisional-v1", "mobile", "windows-rt-provisional-v2", " custom"})
        Check(!ParseBenchmarkProfile(bad), "unknown/aliased profile cannot become an admitted standard");
    Check(BenchmarkProfileClassificationName(BenchmarkProfileId::Custom) == "custom-unranked" &&
        BenchmarkProfileClassificationName(BenchmarkProfileId::MobileProvisionalV1) == "provisional-unranked" &&
        BenchmarkProfileClassificationName(BenchmarkProfileId::WindowsProvisionalV1) == "provisional-unranked",
        "no current profile can claim ranking or validated fairness");
    TestLifecycle(BenchmarkProfileId::MobileProvisionalV1, GraphicsPlatform::Android);
    TestLifecycle(BenchmarkProfileId::WindowsProvisionalV1, GraphicsPlatform::Windows);
    auto c = Personal(GraphicsPlatform::Android); const auto saved = c;
    BenchmarkProfileRun wrong;
    Check(!wrong.Begin(BenchmarkProfileId::WindowsProvisionalV1, c) && c == saved,
        "cross-platform selection rejects without touching settings");
    c.optical = OpticalProfile::High; const auto immutable = c;
    Check(!wrong.Begin(BenchmarkProfileId::MobileProvisionalV1, c) && c == immutable,
        "compiled optics mismatch cannot silently change immutable quality");
    c = saved;
    BenchmarkProfileRun custom;
    Check(custom.Begin(BenchmarkProfileId::Custom, c) && c.dynamicResolutionEnabled && custom.Observe(c) && custom.Complete() && c == saved,
        "custom can retain declared DRS and personal settings but remains explicitly unranked");
    c = saved;
    { BenchmarkProfileRun standard; Check(standard.Begin(BenchmarkProfileId::MobileProvisionalV1, c), "prepare matching custom pixel fixture");
      auto matched = c; matched.temporaryOverrideActive = false;
      BenchmarkProfileRun explicitCustom;
      Check(explicitCustom.Begin(BenchmarkProfileId::Custom, matched) && explicitCustom.Profile() == BenchmarkProfileId::Custom,
          "matching standard pixels/effects do not auto-promote an explicitly custom run"); }
    for (unsigned invalid = 0; invalid < 7; ++invalid) {
        c = saved;
        switch (invalid) {
        case 0: c.internal.width = 0; break;
        case 1: c.output.height = 0; break;
        case 2: c.output.width = 16385; break;
        case 3: c.cameraAspect = std::numeric_limits<double>::quiet_NaN(); break;
        case 4: c.cameraAspect = std::numeric_limits<double>::infinity(); break;
        case 5: c.backend = GraphicsBackend::Unsupported; break;
        case 6: c.outputPolicy = static_cast<BenchmarkOutputPolicy>(255); break;
        }
        const auto before = c;
        BenchmarkProfileRun run;
        Check(!run.Begin(BenchmarkProfileId::Custom, c) && !ValidBenchmarkProfileConfiguration(c),
            "invalid actual configuration cannot enter even an unranked run");
        Check(c.graphics == before.graphics && c.internal == before.internal && c.output == before.output &&
            c.temporaryOverrideActive == before.temporaryOverrideActive, "rejected begin never applies a partial override");
    }
    std::cout << "benchmark profile checks=" << checks << " failures=" << failures << '\n';
    return failures ? 1 : 0;
}
