#pragma once

#include "graphics/GraphicsSettings.h"
#include "graphics/RtPresentationTransform.h"
#include "gameplay/BenchmarkWorkload.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>

namespace horde::gameplay
{
// Provisional contracts only: neither selection is admitted for ranking. The
// platform must apply/observe real surface and dispatch extents before sampling.
enum class BenchmarkProfileId : std::uint8_t { Custom, MobileProvisionalV1, WindowsProvisionalV1 };
enum class BenchmarkOutputPolicy : std::uint8_t { Native, FixedBufferPreserveAspect };

inline constexpr std::string_view BenchmarkProfileName(const BenchmarkProfileId id) noexcept
{
    switch (id) {
    case BenchmarkProfileId::Custom: return "custom";
    case BenchmarkProfileId::MobileProvisionalV1: return "mobile-rt-provisional-v1";
    case BenchmarkProfileId::WindowsProvisionalV1: return "windows-rt-provisional-v1";
    default: return "invalid";
    }
}
inline constexpr std::string_view BenchmarkProfileClassificationName(const BenchmarkProfileId id) noexcept
{
    switch (id) {
    case BenchmarkProfileId::Custom: return "custom-unranked";
    case BenchmarkProfileId::MobileProvisionalV1:
    case BenchmarkProfileId::WindowsProvisionalV1: return "provisional-unranked";
    default: return "invalid";
    }
}
inline std::optional<BenchmarkProfileId> ParseBenchmarkProfile(const std::string_view name) noexcept
{
    for (const auto id : {BenchmarkProfileId::Custom, BenchmarkProfileId::MobileProvisionalV1,
                         BenchmarkProfileId::WindowsProvisionalV1})
        if (BenchmarkProfileName(id) == name) return id;
    return std::nullopt;
}

struct BenchmarkProfileSpec
{
    graphics::GraphicsPlatform platform{};
    graphics::OpticalProfile optical{};
    graphics::GraphicsExtent internal{}, output{};
    graphics::GraphicsSettings graphics{};
    BenchmarkWorkload workload = BenchmarkWorkload::ShowcaseRoute;
};
inline std::optional<BenchmarkProfileSpec> BenchmarkProfileSpecification(const BenchmarkProfileId id) noexcept
{
    using namespace graphics;
    if (id == BenchmarkProfileId::MobileProvisionalV1)
        return BenchmarkProfileSpec{GraphicsPlatform::Android, OpticalProfile::Mobile, {640, 360}, {1280, 720},
            {50, WaterQuality::Mobile, FireDetail::Low, 30, false, ShadowQuality::Current, false, DustQuality::Off}};
    if (id == BenchmarkProfileId::WindowsProvisionalV1)
        return BenchmarkProfileSpec{GraphicsPlatform::Windows, OpticalProfile::High, {1920, 1080}, {1920, 1080},
            {100, WaterQuality::High, FireDetail::High, 30, true, ShadowQuality::Current, true, DustQuality::Low}};
    return std::nullopt;
}

// Application-owner working state, never the persisted confirmed/pending store.
// cameraAspect is observed from the actual camera input, not inferred to certify
// a stretched/cropped presentation. The host still has to implement fixed-buffer
// presentation without stretching and prove it on the affected platform.
struct BenchmarkProfileConfiguration
{
    graphics::GraphicsPlatform platform = graphics::GraphicsPlatform::Windows;
    graphics::OpticalProfile optical = graphics::OpticalProfile::High;
    graphics::GraphicsBackend backend = graphics::GraphicsBackend::Unsupported;
    graphics::GraphicsSettings graphics{};
    graphics::GraphicsExtent internal{}, output{};
    double cameraAspect = 0;
    std::uint32_t presentationTransform = 0;
    BenchmarkOutputPolicy outputPolicy = BenchmarkOutputPolicy::Native;
    bool dynamicResolutionEnabled = false;
    bool temporaryOverrideActive = false;
    BenchmarkWorkload workload = BenchmarkWorkload::ShowcaseRoute;
    bool operator==(const BenchmarkProfileConfiguration&) const = default;
};

inline bool ValidBenchmarkProfileConfiguration(const BenchmarkProfileConfiguration& c) noexcept
{
    using namespace graphics;
    return (c.platform == GraphicsPlatform::Android || c.platform == GraphicsPlatform::Windows) &&
        (c.optical == OpticalProfile::Mobile || c.optical == OpticalProfile::High) &&
        (c.backend == GraphicsBackend::RayTracingPipeline || c.backend == GraphicsBackend::RayQueryCompute) &&
        ValidGraphicsSettings(c.graphics) && BenchmarkWorkloadName(c.workload) != "invalid" &&
        c.internal.width && c.internal.height &&
        c.internal.width <= c.output.width && c.internal.height <= c.output.height &&
        c.output.width <= 16384 && c.output.height <= 16384 &&
        (c.outputPolicy == BenchmarkOutputPolicy::Native ||
         c.outputPolicy == BenchmarkOutputPolicy::FixedBufferPreserveAspect) &&
        c.presentationTransform <= 7 &&
        std::isfinite(c.cameraAspect) &&
        std::abs(c.cameraAspect - RtPresentationCameraAspect(c.internal.width, c.internal.height,
            RtPresentationTransformFromIndex(c.presentationTransform))) <= 1e-5;
}
inline bool MatchesBenchmarkProfile(const BenchmarkProfileId id, const BenchmarkProfileConfiguration& c) noexcept
{
    if (!ValidBenchmarkProfileConfiguration(c)) return false;
    if (id == BenchmarkProfileId::Custom) return true; // Explicit selection, never inferred from matching pixels.
    const auto spec = BenchmarkProfileSpecification(id);
    if (!spec || spec->platform != c.platform || spec->optical != c.optical || spec->workload != c.workload ||
        spec->internal != c.internal || spec->output != c.output || c.dynamicResolutionEnabled || c.presentationTransform != 0 ||
        c.outputPolicy != BenchmarkOutputPolicy::FixedBufferPreserveAspect) return false;
    auto actual = c.graphics;
    // This setting caps the ordinary preview. It is not measured benchmark
    // pacing and must not overwrite the user's value or certify scanout FPS.
    actual.previewFrameCap = spec->graphics.previewFrameCap;
    return actual == spec->graphics;
}

enum class BenchmarkProfileRunStatus : std::uint8_t { Idle, Active, Complete, Cancelled, Failed };

// A scoped temporary override only. No settings file, surface, GPU, report send
// or timing collector is touched here. The working-state owner must outlive the
// scope. Platform apply failure must call Fail; every exit restores the exact
// original tuple. Observations come from actual submitted state, not this plan.
class BenchmarkProfileRun
{
public:
    BenchmarkProfileRun() = default;
    BenchmarkProfileRun(const BenchmarkProfileRun&) = delete;
    BenchmarkProfileRun& operator=(const BenchmarkProfileRun&) = delete;
    ~BenchmarkProfileRun() { Cancel(); }

    bool Begin(const BenchmarkProfileId id, BenchmarkProfileConfiguration& working) noexcept
    {
        if (status_ != BenchmarkProfileRunStatus::Idle || working.temporaryOverrideActive ||
            !ValidBenchmarkProfileConfiguration(working) || BenchmarkProfileName(id) == "invalid") return false;
        auto requested = working;
        if (id != BenchmarkProfileId::Custom) {
            const auto spec = BenchmarkProfileSpecification(id);
            // Compiled optical quality/backend are immutable; never fake a
            // Mobile/High switch by changing a setting or silently falling back.
            if (!spec || spec->platform != working.platform || spec->optical != working.optical) return false;
            const auto previewCap = working.graphics.previewFrameCap;
            requested.graphics = spec->graphics;
            requested.graphics.previewFrameCap = previewCap;
            requested.internal = spec->internal; requested.output = spec->output;
            requested.cameraAspect = double(spec->output.width) / spec->output.height;
            requested.presentationTransform = 0;
            requested.outputPolicy = BenchmarkOutputPolicy::FixedBufferPreserveAspect;
            requested.dynamicResolutionEnabled = false;
            requested.workload = spec->workload;
        }
        if (!MatchesBenchmarkProfile(id, requested)) return false;
        original_ = working; expected_ = requested; owner_ = &working; id_ = id;
        requested.temporaryOverrideActive = true; working = requested;
        status_ = BenchmarkProfileRunStatus::Active; return true;
    }
    bool Observe(BenchmarkProfileConfiguration actual) noexcept
    {
        if (status_ != BenchmarkProfileRunStatus::Active) return false;
        actual.temporaryOverrideActive = expected_.temporaryOverrideActive;
        if (!MatchesBenchmarkProfile(id_, actual)) { Fail(); return false; }
        // Camera inputs are floats on the real render path. Validation above
        // enforces the aspect tolerance against the observed output; keep every
        // other field exact without rejecting a truthful float-rounded ratio.
        actual.cameraAspect = expected_.cameraAspect;
        if (actual != expected_) { Fail(); return false; }
        observed_ = true; return true;
    }
    bool Complete() noexcept
    {
        if (status_ != BenchmarkProfileRunStatus::Active) return false;
        if (!observed_) { Fail(); return false; }
        Restore(); status_ = BenchmarkProfileRunStatus::Complete; return true;
    }
    void Cancel() noexcept
    { if (status_ == BenchmarkProfileRunStatus::Active) { Restore(); status_ = BenchmarkProfileRunStatus::Cancelled; } }
    void Fail() noexcept
    { if (status_ == BenchmarkProfileRunStatus::Active) { Restore(); status_ = BenchmarkProfileRunStatus::Failed; } }
    BenchmarkProfileRunStatus Status() const noexcept { return status_; }
    BenchmarkProfileId Profile() const noexcept { return id_; }
private:
    void Restore() noexcept { if (owner_) { *owner_ = original_; owner_ = nullptr; } }
    BenchmarkProfileConfiguration* owner_ = nullptr;
    BenchmarkProfileConfiguration original_{}, expected_{};
    BenchmarkProfileId id_ = BenchmarkProfileId::Custom;
    BenchmarkProfileRunStatus status_ = BenchmarkProfileRunStatus::Idle;
    bool observed_ = false;
};
} // namespace horde::gameplay
