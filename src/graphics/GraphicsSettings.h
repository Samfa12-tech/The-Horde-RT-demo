#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <string_view>
#include "graphics/DustQuality.h"

namespace horde::graphics
{
inline constexpr std::uint32_t kGraphicsSettingsSchema = 5u;
inline constexpr double kGraphicsConfirmationSeconds = 15.0;

#ifndef HORDE_RT_MIN_RENDER_SCALE_PERCENT
#define HORDE_RT_MIN_RENDER_SCALE_PERCENT 33
#endif
#if HORDE_RT_MIN_RENDER_SCALE_PERCENT != 33 && HORDE_RT_MIN_RENDER_SCALE_PERCENT != 50
#error "Render scale minimum must be 33 or 50."
#endif
inline constexpr int kMinimumGraphicsRenderScalePercent = HORDE_RT_MIN_RENDER_SCALE_PERCENT;
inline constexpr bool ValidGraphicsRenderScalePercent(const int percent) noexcept
{
    return (percent >= 50 && percent <= 100) ||
        (kMinimumGraphicsRenderScalePercent <= 40 && percent == 40) ||
        (kMinimumGraphicsRenderScalePercent == 33 && percent == 33);
}
inline constexpr int ClampGraphicsRenderScalePercent(const int percent) noexcept
{
    if (percent >= 50) return std::min(percent, 100);
    return ValidGraphicsRenderScalePercent(percent) ? percent : 50;
}
inline constexpr bool ExperimentalGraphicsRenderScalePercent(const int percent) noexcept
{
    return percent == 33 || percent == 40;
}
// Windows trackbar positions keep the two experimental tiers selectable while
// leaving every unapproved 34..49 value unreachable: 0=33, 1=40, 2..52=50..100.
inline constexpr int GraphicsRenderScalePercentFromSliderPosition(const int position) noexcept
{
    const int bounded = std::clamp(position, 0, 52);
    return bounded == 0 ? 33 : (bounded == 1 ? 40 : bounded + 48);
}
inline constexpr int GraphicsRenderScaleSliderPositionFromPercent(const int percent) noexcept
{
    const int admitted = ClampGraphicsRenderScalePercent(percent);
    return admitted == 33 ? 0 : (admitted == 40 ? 1 : admitted - 48);
}
inline constexpr int StepGraphicsRenderScalePercent(const int percent, const bool increase) noexcept
{
    const int target = ClampGraphicsRenderScalePercent(percent) + (increase ? 5 : -5);
    int best = ClampGraphicsRenderScalePercent(percent);
    int bestDistance = std::numeric_limits<int>::max();
    for (int candidate = 33; candidate <= 100; ++candidate)
    {
        if (!ValidGraphicsRenderScalePercent(candidate)) continue;
        const int distance = candidate > target ? candidate - target : target - candidate;
        if (distance < bestDistance ||
            (distance == bestDistance && (increase ? candidate > best : candidate < best)))
        {
            best = candidate;
            bestDistance = distance;
        }
    }
    return best;
}

enum class GraphicsPlatform : std::uint8_t { Android, Windows };
// Values deliberately match the legacy persisted water setting.
enum class WaterQuality : std::uint8_t { Off = 0u, Mobile = 1u, High = 2u };
enum class FireDetail : std::uint8_t { Mobile = 0u, High = 1u, Low = 2u };
enum class ShadowQuality : std::uint8_t { Lower = 0u, Current = 1u, Higher = 2u };
enum class OpticalProfile : std::uint8_t { Mobile, High };
enum class GraphicsBackend : std::uint8_t { Unsupported, RayTracingPipeline, RayQueryCompute };
enum class GraphicsPreset : std::uint8_t { AcceptedBaseline, ReducedEffects, Custom, PlatformDefault };
enum class GraphicsScene : std::uint8_t { Showcase, Preview, EntryMenu };

struct GraphicsSettings
{
    int renderScalePercent = 75;
    WaterQuality waterQuality = WaterQuality::Mobile;
    FireDetail fireDetail = FireDetail::Mobile;
    int previewFrameCap = 30;
    // Enables the physical glass supported by the immutable compiled profile.
    // It cannot add High lantern panes to a Mobile build.
    bool glassEnabled = true;
    ShadowQuality shadowQuality = ShadowQuality::Current;
    bool mistEnabled = true;
    DustQuality dustQuality = DustQuality::Off;
    bool operator==(const GraphicsSettings&) const = default;
};

inline bool ValidGraphicsSettings(const GraphicsSettings& settings) noexcept
{
    return ValidGraphicsRenderScalePercent(settings.renderScalePercent) &&
        static_cast<unsigned>(settings.waterQuality) <= 2u &&
        static_cast<unsigned>(settings.fireDetail) <= 2u &&
        static_cast<unsigned>(settings.shadowQuality) <= 2u &&
        ValidDustQuality(settings.dustQuality) &&
        settings.previewFrameCap >= 15 && settings.previewFrameCap <= 60;
}

inline GraphicsSettings BaselineGraphicsSettings(const GraphicsPlatform platform) noexcept
{
    return platform == GraphicsPlatform::Android ? GraphicsSettings{} :
        GraphicsSettings{100, WaterQuality::High, FireDetail::High, 30};
}

// Fresh installs and explicit Reset use these defaults. Historical baseline
// and migration remain separate so existing confirmed choices are preserved.
inline GraphicsSettings PlatformDefaultGraphicsSettings(const GraphicsPlatform platform) noexcept
{
    return platform == GraphicsPlatform::Android ?
        GraphicsSettings{50, WaterQuality::Mobile, FireDetail::Mobile, 30, false,
            ShadowQuality::Current, true, DustQuality::Low} :
        GraphicsSettings{100, WaterQuality::High, FireDetail::High, 30, true,
            ShadowQuality::Current, true, DustQuality::Low};
}

inline GraphicsSettings ReducedEffectsGraphicsSettings(const GraphicsPlatform platform) noexcept
{
    auto result = BaselineGraphicsSettings(platform);
    result.waterQuality = WaterQuality::Mobile;
    result.fireDetail = FireDetail::Low;
    result.shadowQuality = ShadowQuality::Lower;
    return result;
}

inline GraphicsPreset MatchGraphicsPreset(const GraphicsSettings& settings,
                                         const GraphicsPlatform platform) noexcept
{
    if (settings == BaselineGraphicsSettings(platform)) return GraphicsPreset::AcceptedBaseline;
    if (settings == PlatformDefaultGraphicsSettings(platform)) return GraphicsPreset::PlatformDefault;
    if (settings == ReducedEffectsGraphicsSettings(platform)) return GraphicsPreset::ReducedEffects;
    return GraphicsPreset::Custom;
}

inline std::string_view GraphicsPresetName(const GraphicsPreset preset) noexcept
{
    switch (preset)
    {
    case GraphicsPreset::AcceptedBaseline: return "Accepted 1.6.1 baseline";
    case GraphicsPreset::ReducedEffects: return "Reduced effects";
    case GraphicsPreset::PlatformDefault: return "Platform defaults";
    default: return "Custom";
    }
}

struct GraphicsCapabilities
{
    OpticalProfile compiledOpticalProfile = OpticalProfile::Mobile;
    GraphicsBackend backend = GraphicsBackend::Unsupported;
    // False until the production adapter independently applies fire detail.
    bool independentFireDetail = false;
};

enum class GraphicsReason : std::uint32_t
{
    None = 0u,
    InvalidSettings = 1u << 0u,
    UnsupportedRt = 1u << 1u,
    FireFollowsWater = 1u << 2u,
    InvalidExtent = 1u << 3u,
    InterruptedApply = 1u << 4u,
    InvalidStoredSettings = 1u << 5u,
    ResourceFailure = 1u << 6u,
    ConfirmationExpired = 1u << 7u,
};
inline GraphicsReason operator|(const GraphicsReason a, const GraphicsReason b) noexcept
{
    return static_cast<GraphicsReason>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}
inline bool HasGraphicsReason(const GraphicsReason reasons, const GraphicsReason reason) noexcept
{
    return (static_cast<std::uint32_t>(reasons) & static_cast<std::uint32_t>(reason)) != 0u;
}

struct GraphicsExtent
{
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    bool operator==(const GraphicsExtent&) const = default;
};

inline GraphicsExtent ScaledGraphicsExtent(const GraphicsExtent output, const int percent) noexcept
{
    if (output.width == 0u || output.height == 0u || !ValidGraphicsRenderScalePercent(percent)) return {};
    // Positive round-to-nearest matches the production fixed-scale sizing,
    // with integer arithmetic avoiding float/32-bit multiplication overflow.
    const auto dimension = [percent](const std::uint32_t value) {
        return std::max(1u, static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(value) * static_cast<unsigned>(percent) + 50u) / 100u));
    };
    return {dimension(output.width), dimension(output.height)};
}

struct GraphicsResolution
{
    GraphicsSettings requested{};
    GraphicsSettings effective{};
    OpticalProfile opticalProfile = OpticalProfile::Mobile;
    GraphicsBackend backend = GraphicsBackend::Unsupported;
    GraphicsExtent requestedInternalExtent{}; // Planned size, never claimed as presented.
    GraphicsExtent outputExtent{};
    GraphicsReason reasons = GraphicsReason::None;
    bool valid = false;
};

inline GraphicsResolution ResolveGraphicsSettings(const GraphicsSettings& requested,
                                                  const GraphicsCapabilities capabilities,
                                                  const GraphicsExtent output) noexcept
{
    GraphicsResolution result{requested, requested, capabilities.compiledOpticalProfile,
        capabilities.backend, {}, output};
    if (!ValidGraphicsSettings(requested)) result.reasons = result.reasons | GraphicsReason::InvalidSettings;
    if (capabilities.backend != GraphicsBackend::RayTracingPipeline &&
        capabilities.backend != GraphicsBackend::RayQueryCompute)
        result.reasons = result.reasons | GraphicsReason::UnsupportedRt;
    if (output.width == 0u || output.height == 0u)
        result.reasons = result.reasons | GraphicsReason::InvalidExtent;
    if (!capabilities.independentFireDetail)
    {
        result.effective.fireDetail = requested.waterQuality == WaterQuality::High ? FireDetail::High : FireDetail::Mobile;
        result.reasons = result.reasons | GraphicsReason::FireFollowsWater;
    }
    result.valid = !HasGraphicsReason(result.reasons, GraphicsReason::InvalidSettings) &&
        !HasGraphicsReason(result.reasons, GraphicsReason::UnsupportedRt) &&
        !HasGraphicsReason(result.reasons, GraphicsReason::InvalidExtent);
    if (result.valid) result.requestedInternalExtent = ScaledGraphicsExtent(output, requested.renderScalePercent);
    return result;
}

// Enabling glass preserves the exact compiled optical/geometry profile.
// Disabling it removes physical glass from every ray path, independently of quality.
inline std::string_view OpticalProfileHelp(const OpticalProfile profile) noexcept
{
    return profile == OpticalProfile::Mobile ?
        "Mobile optical build: lantern panes are absent from geometry. Glass On enables other supported glass; full lantern panes require a High build." :
        "High optical build: Glass On retains physical lantern panes. Optical profile is fixed by this build.";
}
inline constexpr std::string_view kGraphicsCostHelp = "Cost: not yet measured for this candidate.";

struct LegacyGraphicsSettings
{
    std::optional<int> renderScalePercent;
    std::optional<int> waterQuality;
};
inline GraphicsSettings MigrateLegacyGraphicsSettings(const LegacyGraphicsSettings& legacy,
                                                      const GraphicsPlatform platform) noexcept
{
    auto result = BaselineGraphicsSettings(platform);
    if (legacy.renderScalePercent) result.renderScalePercent = ClampGraphicsRenderScalePercent(*legacy.renderScalePercent);
    if (legacy.waterQuality) result.waterQuality = static_cast<WaterQuality>(std::clamp(*legacy.waterQuality, 0, 2));
    result.fireDetail = result.waterQuality == WaterQuality::High ? FireDetail::High : FireDetail::Mobile;
    return result;
}

// Platform adapters persist this as graphics-only keys, independently of
// audio/control/consent/progression storage. pending must be written before
// issuing a GPU Apply and cleared only after Confirm or completed Revert.
struct GraphicsPersistenceRecord
{
    std::uint32_t schema = kGraphicsSettingsSchema;
    GraphicsSettings confirmed{};
    std::optional<GraphicsSettings> pending;
};
struct GraphicsRecovery
{
    GraphicsSettings startup{};
    std::optional<GraphicsSettings> retainedRequested;
    GraphicsReason reasons = GraphicsReason::None;
};
inline GraphicsRecovery RecoverGraphicsSettings(const GraphicsPersistenceRecord& record,
                                                const GraphicsPlatform platform) noexcept
{
    GraphicsRecovery result;
    const bool legacySchema = record.schema == 1u;
    const bool oldSchema = legacySchema || record.schema == 2u;
    const bool beforeMistSchema = oldSchema || record.schema == 3u;
    const bool beforeDustSchema = record.schema >= 1u && record.schema <= 4u;
    auto confirmed = record.confirmed;
    if (oldSchema) confirmed.shadowQuality = ShadowQuality::Current;
    if (beforeMistSchema) confirmed.mistEnabled = true;
    if (beforeDustSchema) confirmed.dustQuality = DustQuality::Off;
    if ((!beforeDustSchema && record.schema != kGraphicsSettingsSchema) || !ValidGraphicsSettings(confirmed) ||
        (oldSchema && static_cast<unsigned>(confirmed.fireDetail) > 1u))
    {
        result.startup = BaselineGraphicsSettings(platform);
        result.reasons = GraphicsReason::InvalidStoredSettings;
    }
    else
    {
        result.startup = confirmed;
        if (legacySchema) result.startup.glassEnabled = true;
    }
    if (record.pending)
    {
        result.retainedRequested = record.pending;
        if (legacySchema) result.retainedRequested->glassEnabled = true;
        if (oldSchema) result.retainedRequested->shadowQuality = ShadowQuality::Current;
        if (beforeMistSchema) result.retainedRequested->mistEnabled = true;
        if (beforeDustSchema) result.retainedRequested->dustQuality = DustQuality::Off;
        if (!ValidGraphicsSettings(*result.retainedRequested) ||
            (oldSchema && static_cast<unsigned>(result.retainedRequested->fireDetail) > 1u))
        {
            result.retainedRequested.reset();
            result.reasons = result.reasons | GraphicsReason::InvalidStoredSettings;
        }
        result.reasons = result.reasons | GraphicsReason::InterruptedApply;
    }
    return result;
}

enum class GraphicsCommandKind : std::uint8_t { Apply, Revert };
struct GraphicsCommand
{
    std::uint64_t serial = 0u;
    std::uint64_t lifecycleGeneration = 0u;
    GraphicsCommandKind kind = GraphicsCommandKind::Apply;
    GraphicsSettings requested{};
};
struct GraphicsAppliedSnapshot
{
    std::uint64_t serial = 0u;
    std::uint64_t lifecycleGeneration = 0u;
    GraphicsSettings requested{};
    GraphicsSettings effective{};
    OpticalProfile opticalProfile = OpticalProfile::Mobile;
    GraphicsBackend backend = GraphicsBackend::Unsupported;
    GraphicsExtent internalExtent{}; // Actual DispatchExtent, filled by render owner.
    GraphicsExtent outputExtent{};   // Actual swapchain extent.
    GraphicsScene scene = GraphicsScene::Showcase;
    GraphicsReason reasons = GraphicsReason::None;
    bool rtPresented = false;
};

enum class GraphicsEditState : std::uint8_t
{
    Editing, Applying, AwaitingConfirmation, Committed, Reverting, Failed,
};

class GraphicsEditSession
{
public:
    explicit GraphicsEditSession(const GraphicsSettings confirmed,
                                 const std::uint64_t serialFloor = 0u) noexcept
        : committed_(confirmed), draft_(confirmed), lastSerial_(serialFloor) {}

    bool Stage(const GraphicsSettings draft) noexcept
    {
        if (state_ == GraphicsEditState::Applying || state_ == GraphicsEditState::Reverting ||
            state_ == GraphicsEditState::AwaitingConfirmation || !ValidGraphicsSettings(draft)) return false;
        draft_ = draft;
        state_ = GraphicsEditState::Editing;
        return true;
    }
    bool ResetDraft(const GraphicsPlatform platform) noexcept { return Stage(PlatformDefaultGraphicsSettings(platform)); }
    std::optional<GraphicsCommand> RequestApply(const std::uint64_t generation) noexcept
    {
        if ((state_ != GraphicsEditState::Editing && state_ != GraphicsEditState::Failed &&
             state_ != GraphicsEditState::Committed) || !ValidGraphicsSettings(draft_)) return std::nullopt;
        auto command = Request(GraphicsCommandKind::Apply, draft_, generation);
        if (command) failureReason_ = GraphicsReason::None;
        return command;
    }
    std::optional<GraphicsCommand> RequestRevert(const std::uint64_t generation) noexcept
    {
        // Supersede even an in-flight Apply. Its eventual acknowledgement is
        // stale and cannot commit after this newer restore command.
        return Request(GraphicsCommandKind::Revert, committed_, generation);
    }
    bool Acknowledge(const GraphicsAppliedSnapshot& snapshot, const bool resourceSuccess) noexcept
    {
        if ((state_ != GraphicsEditState::Applying && state_ != GraphicsEditState::Reverting) ||
            !pending_ || pending_->serial != snapshot.serial ||
            pending_->lifecycleGeneration != snapshot.lifecycleGeneration ||
            !(pending_->requested == snapshot.requested)) return false;
        if (!resourceSuccess)
        {
            failureReason_ = snapshot.reasons | GraphicsReason::ResourceFailure;
            state_ = GraphicsEditState::Failed;
            // Keep pending persistence until successful restore or explicit
            // confirmation; a failed revert must never look like restored.
            pending_.reset();
            return true;
        }
        if (!snapshot.rtPresented) return false;
        const bool supportedBackend = snapshot.backend == GraphicsBackend::RayTracingPipeline ||
            snapshot.backend == GraphicsBackend::RayQueryCompute;
        const bool explainedFire = HasGraphicsReason(snapshot.reasons, GraphicsReason::FireFollowsWater) &&
            snapshot.effective.fireDetail == (snapshot.requested.waterQuality == WaterQuality::High ? FireDetail::High : FireDetail::Mobile);
        if (!ValidGraphicsSettings(snapshot.effective) || !supportedBackend ||
            snapshot.effective.renderScalePercent != snapshot.requested.renderScalePercent ||
            snapshot.effective.waterQuality != snapshot.requested.waterQuality ||
            snapshot.effective.previewFrameCap != snapshot.requested.previewFrameCap ||
            snapshot.effective.glassEnabled != snapshot.requested.glassEnabled ||
            snapshot.effective.shadowQuality != snapshot.requested.shadowQuality ||
            snapshot.effective.mistEnabled != snapshot.requested.mistEnabled ||
            snapshot.effective.dustQuality != snapshot.requested.dustQuality ||
            (snapshot.effective.fireDetail != snapshot.requested.fireDetail && !explainedFire) ||
            (HasGraphicsReason(snapshot.reasons, GraphicsReason::FireFollowsWater) && !explainedFire) ||
            HasGraphicsReason(snapshot.reasons, GraphicsReason::InvalidSettings) ||
            HasGraphicsReason(snapshot.reasons, GraphicsReason::UnsupportedRt) ||
            HasGraphicsReason(snapshot.reasons, GraphicsReason::InvalidExtent) ||
            snapshot.internalExtent.width == 0u || snapshot.internalExtent.height == 0u ||
            snapshot.outputExtent.width == 0u || snapshot.outputExtent.height == 0u) return false;
        effective_ = snapshot;
        if (pending_->kind == GraphicsCommandKind::Revert)
        {
            draft_ = committed_;
            persistencePending_.reset();
            pending_.reset();
            state_ = GraphicsEditState::Editing;
        }
        else
        {
            state_ = GraphicsEditState::AwaitingConfirmation;
            foregroundSeconds_ = 0.0;
        }
        return true;
    }
    bool Confirm() noexcept
    {
        if (state_ != GraphicsEditState::AwaitingConfirmation || !pending_ || !effective_) return false;
        committed_ = pending_->requested; // Retain requested intent if effective values are constrained.
        draft_ = committed_;
        pending_.reset();
        persistencePending_.reset();
        state_ = GraphicsEditState::Committed;
        return true;
    }
    std::optional<GraphicsCommand> AdvanceConfirmation(const double seconds,
                                                       const bool foreground,
                                                       const std::uint64_t generation) noexcept
    {
        if (state_ != GraphicsEditState::AwaitingConfirmation || !foreground ||
            !std::isfinite(seconds) || seconds <= 0.0) return std::nullopt;
        foregroundSeconds_ = std::min(kGraphicsConfirmationSeconds, foregroundSeconds_ + seconds);
        if (foregroundSeconds_ < kGraphicsConfirmationSeconds) return std::nullopt;
        failureReason_ = GraphicsReason::ConfirmationExpired;
        return RequestRevert(generation);
    }
    GraphicsPersistenceRecord Persistence() const noexcept
    {
        return {kGraphicsSettingsSchema, committed_, persistencePending_};
    }
    const GraphicsSettings& Committed() const noexcept { return committed_; }
    const GraphicsSettings& Draft() const noexcept { return draft_; }
    const std::optional<GraphicsAppliedSnapshot>& Effective() const noexcept { return effective_; }
    GraphicsEditState State() const noexcept { return state_; }
    GraphicsReason FailureReason() const noexcept { return failureReason_; }
    std::uint64_t LastSerial() const noexcept { return lastSerial_; }

private:
    std::optional<GraphicsCommand> Request(const GraphicsCommandKind kind,
                                         const GraphicsSettings settings,
                                         const std::uint64_t generation) noexcept
    {
        if (generation == 0u || lastSerial_ == std::numeric_limits<std::uint64_t>::max() ||
            !ValidGraphicsSettings(settings)) return std::nullopt;
        pending_ = GraphicsCommand{++lastSerial_, generation, kind, settings};
        if (kind == GraphicsCommandKind::Apply) persistencePending_ = settings;
        state_ = kind == GraphicsCommandKind::Apply ? GraphicsEditState::Applying : GraphicsEditState::Reverting;
        return pending_;
    }
    GraphicsSettings committed_;
    GraphicsSettings draft_;
    std::uint64_t lastSerial_ = 0u;
    GraphicsEditState state_ = GraphicsEditState::Editing;
    std::optional<GraphicsCommand> pending_;
    std::optional<GraphicsSettings> persistencePending_;
    std::optional<GraphicsAppliedSnapshot> effective_;
    GraphicsReason failureReason_ = GraphicsReason::None;
    double foregroundSeconds_ = 0.0;
};

// One coherent copied snapshot; no JNI/environment pointer is retained. A
// mutex is intentionally simpler than independent quality/dimension atomics.
template <typename T>
class GraphicsMailbox
{
public:
    void Publish(const T& value) { std::lock_guard lock(mutex_); value_ = value; }
    T Read() const { std::lock_guard lock(mutex_); return value_; }
private:
    mutable std::mutex mutex_;
    T value_{};
};
} // namespace horde::graphics
