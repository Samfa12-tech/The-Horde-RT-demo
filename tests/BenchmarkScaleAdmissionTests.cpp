#include "graphics/GraphicsSettings.h"
#include <iostream>

using namespace horde::graphics;
int main()
{
    int failures = 0;
    const auto check = [&](bool ok, const char* message) {
        if (!ok) { ++failures; std::cerr << message << '\n'; }
    };
    constexpr bool experimental = kMinimumGraphicsRenderScalePercent == 33;
    check(BaselineGraphicsSettings(GraphicsPlatform::Android).renderScalePercent == 75,
          "benchmark admission must preserve the Android default");
    for (int scale : {33, 40, 50, 68, 100})
    {
        auto requested = BaselineGraphicsSettings(GraphicsPlatform::Android);
        requested.renderScalePercent = scale;
        const bool admitted = scale >= 50 || experimental;
        const auto resolution = ResolveGraphicsSettings(requested,
            {OpticalProfile::Mobile, GraphicsBackend::RayTracingPipeline, true}, {1440, 2980});
        check(ValidGraphicsSettings(requested) == admitted && resolution.valid == admitted,
              "shared validation and actual planned sizing must agree with build admission");
        const auto extent = ScaledGraphicsExtent({1440, 2980}, scale);
        check(extent == (admitted ? GraphicsExtent{static_cast<unsigned>((1440 * scale + 50) / 100),
            static_cast<unsigned>((2980 * scale + 50) / 100)} : GraphicsExtent{}),
            "effective internal dimensions must reflect the admitted percentage");
        const auto recovery = RecoverGraphicsSettings({kGraphicsSettingsSchema, requested, requested}, GraphicsPlatform::Android);
        check(recovery.startup.renderScalePercent == (admitted ? scale : 75) && recovery.retainedRequested == requested,
              "invalid stored experiments must restore ordinary baseline while preserving interrupted intent");
        check(MigrateLegacyGraphicsSettings({scale, 1}, GraphicsPlatform::Android).renderScalePercent ==
              (admitted ? scale : 50), "legacy values must share explicit floor admission");
    }
    for (int scale : {-1, 0, 32, 34, 39, 41, 49, 101})
    {
        auto requested = GraphicsSettings{}; requested.renderScalePercent = scale;
        check(!ValidGraphicsSettings(requested) && ScaledGraphicsExtent({1440, 2980}, scale) == GraphicsExtent{},
              "unrequested sub50 tiers and malformed extents must remain rejected");
    }
    if (experimental)
    {
        check(ScaledGraphicsExtent({1440, 2980}, 33) == GraphicsExtent{475, 983} &&
              ScaledGraphicsExtent({1440, 2980}, 40) == GraphicsExtent{576, 1192}, "33/40 rounded extents");
        GraphicsEditSession edit(GraphicsSettings{});
        auto candidate = GraphicsSettings{}; candidate.renderScalePercent = 33;
        check(edit.Stage(candidate), "experimental draft must use the established edit transaction");
        const auto command = edit.RequestApply(7);
        check(command.has_value(), "experimental apply requires an ordinary serial/generation");
        if (command)
        {
            GraphicsAppliedSnapshot a;
            a.serial = command->serial; a.lifecycleGeneration = 8; a.requested = candidate; a.effective = candidate;
            a.backend = GraphicsBackend::RayTracingPipeline; a.outputExtent = {1440, 2980};
            a.internalExtent = {475, 983}; a.rtPresented = true;
            check(!edit.Acknowledge(a, true), "wrong surface must not confirm an experiment");
            a.lifecycleGeneration = 7; a.rtPresented = false;
            check(!edit.Acknowledge(a, true), "allocation without actual RT presentation cannot confirm");
            a.rtPresented = true; a.effective.renderScalePercent = 50;
            check(!edit.Acknowledge(a, true), "clamped50 cannot acknowledge a requested33");
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
