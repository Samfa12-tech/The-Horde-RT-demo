#include "graphics/GraphicsSettings.h"
#include <iostream>

using namespace horde::graphics;
int main()
{
    int failures = 0;
    const auto check = [&](bool ok, const char* message) {
        if (!ok) { ++failures; std::cerr << message << '\n'; }
    };
    check(BaselineGraphicsSettings(GraphicsPlatform::Android).renderScalePercent == 75,
          "historical Android75 baseline is separate from fresh/reset50 defaults");
    for (int scale : {33, 40, 50, 68, 100})
    {
        auto requested = BaselineGraphicsSettings(GraphicsPlatform::Android);
        requested.renderScalePercent = scale;
        const bool admitted = ValidGraphicsRenderScalePercent(scale);
        const auto resolution = ResolveGraphicsSettings(requested,
            {OpticalProfile::Mobile, GraphicsBackend::RayTracingPipeline, true}, {1440, 2980});
        check(ValidGraphicsSettings(requested) == admitted && resolution.valid == admitted,
              "shared validation and actual planned sizing must agree with build admission");
        const auto extent = ScaledGraphicsExtent({1440, 2980}, scale);
        check(extent == (admitted ? GraphicsExtent{static_cast<unsigned>((1440 * scale + 50) / 100),
            static_cast<unsigned>((2980 * scale + 50) / 100)} : GraphicsExtent{}),
            "effective internal dimensions must reflect the admitted percentage");
        const auto recovery = RecoverGraphicsSettings({kGraphicsSettingsSchema, requested, requested}, GraphicsPlatform::Android);
        check(recovery.startup.renderScalePercent == (admitted ? scale : 75) &&
              recovery.retainedRequested == (admitted ? std::optional<GraphicsSettings>{requested} : std::nullopt),
              "recovery restores the admitted confirmed tuple and retains only valid interrupted intent");
        check((static_cast<std::uint32_t>(recovery.reasons) &
               static_cast<std::uint32_t>(GraphicsReason::InterruptedApply)) != 0u &&
              ((static_cast<std::uint32_t>(recovery.reasons) &
                static_cast<std::uint32_t>(GraphicsReason::InvalidStoredSettings)) != 0u) == !admitted,
              "invalid interrupted experiments must remain explicitly diagnosed without becoming a draft");
        check(MigrateLegacyGraphicsSettings({scale, 1}, GraphicsPlatform::Android).renderScalePercent ==
              (admitted ? scale : 50), "legacy values must share explicit floor admission");
    }
    auto invalidConfirmed = BaselineGraphicsSettings(GraphicsPlatform::Android);
    invalidConfirmed.renderScalePercent = 32;
    auto validPending = BaselineGraphicsSettings(GraphicsPlatform::Android);
    validPending.renderScalePercent = 50;
    const auto independentRecovery = RecoverGraphicsSettings(
        {kGraphicsSettingsSchema, invalidConfirmed, validPending}, GraphicsPlatform::Android);
    check(independentRecovery.startup.renderScalePercent == 75 &&
          independentRecovery.retainedRequested == validPending,
          "an invalid confirmed tuple must not discard independently valid interrupted intent");
    for (int scale : {-1, 0, 32, 34, 39, 41, 49, 101})
    {
        auto requested = GraphicsSettings{}; requested.renderScalePercent = scale;
        check(!ValidGraphicsSettings(requested) && ScaledGraphicsExtent({1440, 2980}, scale) == GraphicsExtent{},
              "unrequested sub50 tiers and malformed extents must remain rejected");
        check(ClampGraphicsRenderScalePercent(scale) == (scale > 100 ? 100 : 50),
              "corrupt saved values preserve the established50 floor and100 upper clamp");
    }
    const int experimentalScale = ValidGraphicsRenderScalePercent(33) ? 33 : 40;
    if (ValidGraphicsRenderScalePercent(experimentalScale))
    {
        check((!ValidGraphicsRenderScalePercent(33) || ScaledGraphicsExtent({1440, 2980}, 33) == GraphicsExtent{475, 983}) &&
              (!ValidGraphicsRenderScalePercent(40) || ScaledGraphicsExtent({1440, 2980}, 40) == GraphicsExtent{576, 1192}),
              "admitted experimental tiers use rounded native-output-derived extents");
        GraphicsEditSession edit(GraphicsSettings{});
        auto candidate = GraphicsSettings{}; candidate.renderScalePercent = experimentalScale;
        check(edit.Stage(candidate), "experimental draft must use the established edit transaction");
        const auto command = edit.RequestApply(7);
        check(command.has_value(), "experimental apply requires an ordinary serial/generation");
        if (command)
        {
            GraphicsAppliedSnapshot a;
            a.serial = command->serial; a.lifecycleGeneration = 8; a.requested = candidate; a.effective = candidate;
            a.backend = GraphicsBackend::RayTracingPipeline; a.outputExtent = {1440, 2980};
            a.internalExtent = ScaledGraphicsExtent({1440, 2980}, experimentalScale); a.rtPresented = true;
            check(!edit.Acknowledge(a, true), "wrong surface must not confirm an experiment");
            a.lifecycleGeneration = 7; a.rtPresented = false;
            check(!edit.Acknowledge(a, true), "allocation without actual RT presentation cannot confirm");
            a.rtPresented = true; a.effective.renderScalePercent = 50;
            check(!edit.Acknowledge(a, true), "clamped50 cannot acknowledge a requested experimental tier");
            a.effective = candidate;
            check(edit.Acknowledge(a, true), "actual matching experimental tuple can enter confirmation");
            check(!edit.Acknowledge(a, true), "duplicate acknowledgement cannot restart confirmation");
            const auto revert = edit.RequestRevert(7);
            check(revert && revert->requested.renderScalePercent == 75 && !edit.Confirm(),
                  "revert restores the prior default rather than persisting an unconfirmed experiment");
        }
    }
    std::cout << "Scale admission minimum=" << kMinimumGraphicsRenderScalePercent << " failures=" << failures << '\n';
    return failures ? 1 : 0;
}
