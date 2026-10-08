#include "graphics/GraphicsSettings.h"
#include "vulkan/raytracing/RtSceneTuning.h"

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
using namespace horde::graphics;
bool passed = true;
void Check(const bool condition, const char* message)
{
    if (!condition) { passed = false; std::cerr << "Graphics settings: " << message << '\n'; }
}
GraphicsAppliedSnapshot Presented(const GraphicsCommand& command)
{
    GraphicsAppliedSnapshot result;
    result.serial = command.serial;
    result.lifecycleGeneration = command.lifecycleGeneration;
    result.requested = command.requested;
    result.effective = command.requested;
    result.backend = GraphicsBackend::RayTracingPipeline;
    result.internalExtent = ScaledGraphicsExtent({1920u, 1080u}, command.requested.renderScalePercent);
    result.outputExtent = {1920u, 1080u};
    result.rtPresented = true;
    return result;
}
void TestMigrationAndProfiles()
{
    Check(kMinimumGraphicsRenderScalePercent == 33, "normal build exposes the two explicit experimental tiers");
    Check(MigrateLegacyGraphicsSettings({}, GraphicsPlatform::Android) == GraphicsSettings{}, "missing Android keys use accepted 75 percent Mobile baseline");
    const auto desktop = MigrateLegacyGraphicsSettings({}, GraphicsPlatform::Windows);
    Check(desktop.renderScalePercent == 100 && desktop.waterQuality == WaterQuality::High && desktop.fireDetail == FireDetail::High,
          "missing Windows keys preserve accepted desktop appearance");
    auto preserved = MigrateLegacyGraphicsSettings({63, 0}, GraphicsPlatform::Android);
    Check(preserved.renderScalePercent == 63 && preserved.waterQuality == WaterQuality::Off && preserved.fireDetail == FireDetail::Mobile,
          "legacy non-preset and Off choice survives migration with existing coupled fire");
    Check(MatchGraphicsPreset(preserved, GraphicsPlatform::Android) == GraphicsPreset::Custom, "legacy custom is not rewritten to a preset");
    Check(MigrateLegacyGraphicsSettings({-4, 9}, GraphicsPlatform::Windows).renderScalePercent == 50 &&
          MigrateLegacyGraphicsSettings({-4, 9}, GraphicsPlatform::Windows).waterQuality == WaterQuality::High,
          "legacy corrupt values retain established clamping");
    Check(ClampGraphicsRenderScalePercent(34) == 50 && ClampGraphicsRenderScalePercent(49) == 50 &&
          ClampGraphicsRenderScalePercent(-1) == 50 && ClampGraphicsRenderScalePercent(40) ==
              (ValidGraphicsRenderScalePercent(40) ? 40 : 50),
          "only explicit low tiers survive loading; malformed gaps and negatives recover at50");
    Check(PlatformDefaultGraphicsSettings(GraphicsPlatform::Android).renderScalePercent == 50 &&
          PlatformDefaultGraphicsSettings(GraphicsPlatform::Android).waterQuality == WaterQuality::Mobile &&
          !PlatformDefaultGraphicsSettings(GraphicsPlatform::Android).glassEnabled &&
          PlatformDefaultGraphicsSettings(GraphicsPlatform::Windows).renderScalePercent == 100,
          "experimental choices do not change fresh/reset platform defaults");
    Check(GraphicsRenderScalePercentFromSliderPosition(0) == 33 &&
          GraphicsRenderScalePercentFromSliderPosition(1) == 40 &&
          GraphicsRenderScalePercentFromSliderPosition(2) == 50 &&
          GraphicsRenderScalePercentFromSliderPosition(52) == 100 &&
          GraphicsRenderScalePercentFromSliderPosition(-10) == 33 &&
          GraphicsRenderScalePercentFromSliderPosition(999) == 100,
          "scale menu positions expose only33,40,50..100 and clamp at endpoints");
    for (const int percent : {33, 40, 50, 68, 100})
    {
        if (!ValidGraphicsRenderScalePercent(percent)) continue;
        Check(GraphicsRenderScalePercentFromSliderPosition(GraphicsRenderScaleSliderPositionFromPercent(percent)) == percent,
              "persisted admitted scale round-trips through the menu position");
    }
    Check(StepGraphicsRenderScalePercent(33, false) == 33 &&
          StepGraphicsRenderScalePercent(33, true) == 40 &&
          StepGraphicsRenderScalePercent(40, true) == 50 &&
          StepGraphicsRenderScalePercent(50, false) == 40 &&
          StepGraphicsRenderScalePercent(51, false) == 50 &&
          StepGraphicsRenderScalePercent(75, false) == 70 &&
          StepGraphicsRenderScalePercent(75, true) == 80 &&
          StepGraphicsRenderScalePercent(100, true) == 100,
          "controller steps preserve five-point movement while crossing only admitted experimental tiers");
    for (const auto platform : {GraphicsPlatform::Android, GraphicsPlatform::Windows})
    {
        Check(MatchGraphicsPreset(BaselineGraphicsSettings(platform), platform) == GraphicsPreset::AcceptedBaseline, "baseline is deterministic");
        auto altered = BaselineGraphicsSettings(platform);
        altered.renderScalePercent -= 1;
        Check(MatchGraphicsPreset(altered, platform) == GraphicsPreset::Custom, "scale override changes preset to Custom");
    }
    Check(OpticalProfileHelp(OpticalProfile::Mobile).find("absent from geometry") != std::string_view::npos,
          "Mobile physical panes omission is explicit");
    Check(kGraphicsCostHelp.find("not yet measured") != std::string_view::npos, "no numerical cost invented");
}
void TestResolutionAndEffectiveValues()
{
    auto requested = GraphicsSettings{63, WaterQuality::Mobile, FireDetail::High, 30};
    auto resolved = ResolveGraphicsSettings(requested, {OpticalProfile::Mobile, GraphicsBackend::RayQueryCompute, false}, {1440u, 2980u});
    Check(resolved.valid && resolved.requestedInternalExtent == GraphicsExtent{907u, 1877u}, "round-to-nearest dimensions match owner 63 percent example");
    Check(resolved.requested == requested && resolved.effective.fireDetail == FireDetail::Mobile &&
          HasGraphicsReason(resolved.reasons, GraphicsReason::FireFollowsWater), "constrained fire preserves request and explains effective value");
    Check(resolved.opticalProfile == OpticalProfile::Mobile, "High fire or water does not invent High optics");
    resolved = ResolveGraphicsSettings(requested, {OpticalProfile::High, GraphicsBackend::RayTracingPipeline, true}, {1919u, 1079u});
    Check(resolved.valid && resolved.effective == requested && !HasGraphicsReason(resolved.reasons, GraphicsReason::FireFollowsWater),
          "independent production fire is honored only when adapter admits it");
    Check(!ResolveGraphicsSettings(requested, {}, {1920u, 1080u}).valid, "no RT backend means unsupported, never a fake rendering mode");
    Check(!ResolveGraphicsSettings(requested, {OpticalProfile::Mobile, GraphicsBackend::RayTracingPipeline, true}, {}).valid, "zero output size is pending/unavailable");
    requested.renderScalePercent = 49;
    Check(!ResolveGraphicsSettings(requested, {OpticalProfile::Mobile, GraphicsBackend::RayTracingPipeline, true}, {1920u, 1080u}).valid,
          "invalid new settings are rejected, not silently quality-clamped");
    const auto maximum = std::numeric_limits<std::uint32_t>::max();
    Check(ScaledGraphicsExtent({maximum, maximum}, 100) == GraphicsExtent{maximum, maximum}, "extent arithmetic handles maximum input without overflow");
    Check(ScaledGraphicsExtent({1u, 1u}, 50) == GraphicsExtent{1u, 1u}, "tiny outputs remain at least one pixel");
    for (const int percent : {33, 40})
    {
        if (!ValidGraphicsRenderScalePercent(percent)) continue;
        requested.renderScalePercent = percent;
        for (const auto backend : {GraphicsBackend::RayTracingPipeline, GraphicsBackend::RayQueryCompute})
        {
            const auto experimental = ResolveGraphicsSettings(requested, {OpticalProfile::Mobile, backend, true}, {1440u, 2980u});
            const auto expected = GraphicsExtent{
                static_cast<std::uint32_t>((1440 * percent + 50) / 100),
                static_cast<std::uint32_t>((2980 * percent + 50) / 100)};
            Check(experimental.valid && experimental.backend == backend && experimental.requestedInternalExtent == expected &&
                  experimental.outputExtent == GraphicsExtent{1440u, 2980u},
                  "experimental internal extent applies on either RT backend while preserving native output extent");
        }
    }
}
void TestApplyConfirmAndCancel()
{
    const auto baseline = BaselineGraphicsSettings(GraphicsPlatform::Windows);
    GraphicsEditSession session(baseline);
    auto draft = baseline; draft.renderScalePercent = 75; draft.fireDetail = FireDetail::Mobile;
    Check(session.Stage(draft) && session.Committed() == baseline, "staging does not change saved settings");
    auto apply = session.RequestApply(1u);
    Check(apply && session.Persistence().pending == draft && !session.Confirm(), "pending marker exists before GPU work and early confirm is rejected");
    auto snapshot = Presented(*apply); snapshot.rtPresented = false;
    Check(!session.Acknowledge(snapshot, true) && session.State() == GraphicsEditState::Applying, "allocated/submitted without RT presentation is not usable setting proof");
    snapshot = Presented(*apply); snapshot.serial += 1u;
    Check(!session.Acknowledge(snapshot, true), "wrong serial ignored");
    snapshot = Presented(*apply); snapshot.lifecycleGeneration += 1u;
    Check(!session.Acknowledge(snapshot, true), "wrong lifecycle generation ignored");
    snapshot = Presented(*apply); snapshot.internalExtent.width = 0u;
    Check(!session.Acknowledge(snapshot, true), "present acknowledgement requires actual dimensions");
    snapshot = Presented(*apply);
    Check(session.Acknowledge(snapshot, true) && session.State() == GraphicsEditState::AwaitingConfirmation, "actual RT-present ack starts confirmation");
    Check(!session.Acknowledge(snapshot, true), "duplicate present ack cannot restart confirmation timer");
    Check(session.Confirm() && session.Committed() == draft && !session.Persistence().pending, "only confirm updates persisted values");
    Check(session.ResetDraft(GraphicsPlatform::Windows) && session.Committed() == draft, "reset is scoped draft until apply/confirm");
    apply = session.RequestApply(2u);
    const auto revert = session.RequestRevert(2u);
    Check(apply && revert && revert->serial > apply->serial && revert->requested == draft, "cancel restores confirmed choice with newer identity");
    Check(!session.Acknowledge(Presented(*apply), true), "superseded apply cannot commit after cancel");
    Check(session.Acknowledge(Presented(*revert), true) && session.State() == GraphicsEditState::Editing &&
          session.Draft() == draft && !session.Persistence().pending, "successful revert restores prior confirmed value and clears pending marker");
}
void TestFailureDeadlineAndRecovery()
{
    const auto baseline = BaselineGraphicsSettings(GraphicsPlatform::Android);
    GraphicsEditSession session(baseline, 99u);
    auto draft = baseline; draft.renderScalePercent = 100;
    session.Stage(draft);
    const auto apply = session.RequestApply(5u);
    Check(apply && apply->serial == 100u, "serial floor survives reopen/lifecycle");
    auto failure = Presented(*apply); failure.rtPresented = false;
    Check(session.Acknowledge(failure, false) && session.Committed() == baseline && !session.Effective(), "allocation failure preserves old configuration, no invented new effective output");
    auto recovery = RecoverGraphicsSettings(session.Persistence(), GraphicsPlatform::Android);
    Check(recovery.startup == baseline && recovery.retainedRequested == draft &&
          HasGraphicsReason(recovery.reasons, GraphicsReason::InterruptedApply), "crash/failure recovery keeps last confirmed and retained requested choice");
    auto retry = session.RequestApply(6u);
    Check(retry && session.Acknowledge(Presented(*retry), true), "failed allocation can be retried without changing user choice");
    Check(!session.AdvanceConfirmation(999.0, false, 6u), "background time cannot confirm or expire foreground deadline");
    Check(!session.AdvanceConfirmation(std::numeric_limits<double>::quiet_NaN(), true, 6u), "invalid clock sample ignored");
    Check(!session.AdvanceConfirmation(14.0, true, 6u), "confirmation survives before deadline");
    Check(!session.Acknowledge(Presented(*retry), true), "repeated telemetry cannot extend deadline");
    const auto revert = session.AdvanceConfirmation(1.0, true, 6u);
    Check(revert && revert->kind == GraphicsCommandKind::Revert && session.Committed() == baseline, "foreground expiry requests GPU restore, does not claim restore already happened");
    failure = Presented(*revert); failure.rtPresented = false;
    Check(session.Acknowledge(failure, false) && session.State() == GraphicsEditState::Failed &&
          session.Effective()->effective == draft && session.Persistence().pending,
          "failed restore retains actual candidate and recovery marker instead of reporting baseline effective");
    const auto restore = session.RequestRevert(7u);
    Check(restore && session.Acknowledge(Presented(*restore), true) && session.Effective()->effective == baseline,
          "new surface can restore confirmed output with generation-scoped acknowledgement");
    auto bad = session.Persistence(); bad.schema = 999u;
    recovery = RecoverGraphicsSettings(bad, GraphicsPlatform::Android);
    Check(recovery.startup == baseline && HasGraphicsReason(recovery.reasons, GraphicsReason::InvalidStoredSettings), "future/corrupt schema has explicit baseline recovery");
    GraphicsEditSession exhausted(baseline, std::numeric_limits<std::uint64_t>::max());
    Check(!exhausted.RequestApply(1u), "serial exhaustion cannot wrap and accept stale acknowledgements");
}
void TestEffectiveAcknowledgement()
{
    GraphicsEditSession session(BaselineGraphicsSettings(GraphicsPlatform::Windows));
    const auto requested = GraphicsSettings{63, WaterQuality::Mobile, FireDetail::High, 60};
    session.Stage(requested);
    const auto command = session.RequestApply(1u);
    auto snapshot = Presented(*command);
    snapshot.effective.previewFrameCap = 30;
    Check(!session.Acknowledge(snapshot, true), "old live preview cap cannot acknowledge a new requested cap");
    snapshot = Presented(*command); snapshot.effective.glassEnabled = false;
    Check(!session.Acknowledge(snapshot, true), "old or constrained glass geometry cannot acknowledge requested glass On");
    snapshot = Presented(*command); snapshot.effective.renderScalePercent = 100;
    Check(!session.Acknowledge(snapshot, true), "old scale cannot silently acknowledge requested scale");
    snapshot = Presented(*command); snapshot.effective.waterQuality = WaterQuality::High;
    Check(!session.Acknowledge(snapshot, true), "unexplained water fallback cannot be confirmed");
    snapshot = Presented(*command); snapshot.backend = static_cast<GraphicsBackend>(999);
    Check(!session.Acknowledge(snapshot, true), "unknown backend cannot be relabeled successful RT acknowledgement");
    snapshot = Presented(*command); snapshot.effective.fireDetail = FireDetail::Mobile;
    Check(!session.Acknowledge(snapshot, true), "unexplained fire fallback cannot be confirmed");
    snapshot.reasons = GraphicsReason::FireFollowsWater;
    Check(session.Acknowledge(snapshot, true) && session.Confirm() && session.Committed() == requested,
          "explicit admitted legacy fire constraint preserves requested intent on confirmation");
}
void TestGlassMigrationAndTransactions()
{
    for (const auto platform : {GraphicsPlatform::Android, GraphicsPlatform::Windows})
    {
        Check(BaselineGraphicsSettings(platform).glassEnabled &&
              ReducedEffectsGraphicsSettings(platform).glassEnabled &&
              MigrateLegacyGraphicsSettings({63, 0}, platform).glassEnabled,
              "all existing defaults and legacy appearance preserve glass On");
        const GraphicsSettings oldConfirmed{63, WaterQuality::Off, FireDetail::High, 42, false};
        const GraphicsSettings oldPending{92, WaterQuality::High, FireDetail::Mobile, 15, false};
        const auto migrated = RecoverGraphicsSettings({1u, oldConfirmed, oldPending}, platform);
        auto expectedConfirmed = oldConfirmed; expectedConfirmed.glassEnabled = true;
        auto expectedPending = oldPending; expectedPending.glassEnabled = true;
        Check(migrated.startup == expectedConfirmed && migrated.retainedRequested == expectedPending &&
              HasGraphicsReason(migrated.reasons, GraphicsReason::InterruptedApply) &&
              !HasGraphicsReason(migrated.reasons, GraphicsReason::InvalidStoredSettings),
              "schema1 keeps every confirmed/pending quality and custom value, defaults only new glass field On");
        const auto current = RecoverGraphicsSettings({kGraphicsSettingsSchema, oldConfirmed, oldPending}, platform);
        Check(current.startup == oldConfirmed && current.retainedRequested == oldPending,
              "schema3 preserves explicit Off through interrupted apply recovery");
        auto off = BaselineGraphicsSettings(platform); off.glassEnabled = false;
        Check(MatchGraphicsPreset(off, platform) == GraphicsPreset::Custom, "glass-only override is Custom");
        const auto resolved = ResolveGraphicsSettings(off, {OpticalProfile::Mobile, GraphicsBackend::RayQueryCompute, true}, {800u, 600u});
        Check(resolved.valid && resolved.effective == off && resolved.opticalProfile == OpticalProfile::Mobile,
              "glass Off honors intent without claiming a different compiled optical quality");
        GraphicsEditSession session(BaselineGraphicsSettings(platform));
        Check(session.Stage(off), "glass-only candidate can be staged");
        const auto apply = session.RequestApply(9u);
        auto stale = Presented(*apply); stale.effective.glassEnabled = true;
        Check(!session.Acknowledge(stale, true) && !session.Confirm(),
              "previous On frame cannot confirm Off merely because dimensions match");
        auto failed = Presented(*apply); failed.rtPresented = false;
        Check(session.Acknowledge(failed, false) && session.Committed().glassEnabled && session.Persistence().pending == off,
              "failed geometry replacement retains last confirmed and pending recovery intent");
        const auto retry = session.RequestApply(9u);
        Check(retry && session.Acknowledge(Presented(*retry), true), "actual Off presentation admits confirmation");
        Check(!session.AdvanceConfirmation(99.0, false, 9u), "background does not expire glass confirmation");
        const auto restore = session.AdvanceConfirmation(15.0, true, 9u);
        Check(restore && restore->requested.glassEnabled && session.Persistence().pending == off,
              "15 visible seconds requests physical On restore and retains marker until presented");
        Check(session.Acknowledge(Presented(*restore), true) && !session.Persistence().pending && session.Draft().glassEnabled,
              "actual restored On presentation clears pending marker");
        Check(session.Stage(off), "Off can be staged after restore");
        const auto kept = session.RequestApply(10u);
        Check(kept && session.Acknowledge(Presented(*kept), true) && session.Confirm() &&
               !session.Persistence().confirmed.glassEnabled && session.Persistence().schema == 5u,
              "explicit Keep persists Off in schema5 only after current presentation");
        Check(session.ResetDraft(platform) && session.Draft() == PlatformDefaultGraphicsSettings(platform) && !session.Committed().glassEnabled,
              "Reset stages platform defaults without rewriting confirmed Off");
    }
}
void TestIndependentShadowAndLowFire()
{
    for (const auto oldSchema : {1u, 2u})
    {
        auto old = BaselineGraphicsSettings(GraphicsPlatform::Android);
        old.glassEnabled = false; old.shadowQuality = ShadowQuality::Higher;
        const auto recovery = RecoverGraphicsSettings({oldSchema, old, old}, GraphicsPlatform::Android);
        Check(recovery.startup.shadowQuality == ShadowQuality::Current &&
              recovery.startup.glassEnabled == (oldSchema == 1u) &&
              recovery.retainedRequested && recovery.retainedRequested->shadowQuality == ShadowQuality::Current,
              "historical confirmed and pending migrate Current preserving schema-specific glass");
        old.fireDetail = FireDetail::Low;
        Check(HasGraphicsReason(RecoverGraphicsSettings({oldSchema, old, old}, GraphicsPlatform::Android).reasons,
              GraphicsReason::InvalidStoredSettings), "old schemas never reinterpret unknown fire2 as an admitted legacy choice");
    }
    auto draft = GraphicsSettings{};
    draft.fireDetail = FireDetail::Low; draft.shadowQuality = ShadowQuality::Higher;
    Check(RecoverGraphicsSettings({3u, draft, draft}, GraphicsPlatform::Android).startup == draft,
          "schema3 retains Low and Higher without coupling or rewriting");
    GraphicsEditSession edit(GraphicsSettings{});
    Check(edit.Stage(draft), "independent Low/Higher draft accepted");
    const auto command = edit.RequestApply(9u);
    if (command)
    {
        auto snapshot = Presented(*command);
        snapshot.effective.shadowQuality = ShadowQuality::Current;
        Check(!edit.Acknowledge(snapshot, true), "old shadow upload cannot acknowledge Higher request");
        snapshot.effective = draft;
        Check(edit.Acknowledge(snapshot, true) && edit.Confirm() && edit.Committed() == draft,
              "exact six-field presented ACK commits independent choices");
    }
    using namespace horde::vulkan::raytracing;
    for (const bool high : {false, true})
        for (const auto workload : {RtWorkloadPreset::Lean, RtWorkloadPreset::Authored, RtWorkloadPreset::Max})
            for (const auto mode : {ShadowQuality::Lower, ShadowQuality::Current, ShadowQuality::Higher})
            {
                const auto policy = ResolveRtQualityControls(mode, workload, high);
                Check(policy && policy->controls[0] == static_cast<std::uint32_t>(mode) &&
                    policy->controls[1] == (mode == ShadowQuality::Higher ? (high ? 4u : 2u) : 1u) &&
                    policy->controls[2] == (mode == ShadowQuality::Higher ? 2u : 1u) && policy->controls[3] == 0u,
                    "independent shadow budgets ignore mist/workload and obey compiled profile");
            }
    Check(!ResolveRtQualityControls(static_cast<ShadowQuality>(3u), RtWorkloadPreset::Authored, true),
          "unknown production shadow enum is rejected rather than diagnostic legacy");
    Check(ResolveRtQualityControls(std::nullopt, RtWorkloadPreset::Max, true)->controls ==
          std::array<std::uint32_t, 4u>{{3u,4u,2u,0u}}, "absent diagnostic choice explicitly retains legacy Max");
    draft.shadowQuality = static_cast<ShadowQuality>(99u);
    Check(!ValidGraphicsSettings(draft), "unknown shadow settings rejected before JNI/persistence admission");
}

void TestPlatformDefaultsAndMist()
{
    const auto mobile = PlatformDefaultGraphicsSettings(GraphicsPlatform::Android);
    const auto desktop = PlatformDefaultGraphicsSettings(GraphicsPlatform::Windows);
    Check(mobile == GraphicsSettings{50, WaterQuality::Mobile, FireDetail::Mobile, 30, false, ShadowQuality::Current, true, DustQuality::Low},
          "fresh mobile defaults keep50/Mobile/Mobile/30/GlassOff/Current/MistOn with owner-approved DustLow");
    Check(desktop == GraphicsSettings{100, WaterQuality::High, FireDetail::High, 30, true, ShadowQuality::Current, true, DustQuality::Low},
          "fresh desktop defaults preserve High appearance and MistOn with owner-approved DustLow");
    Check(MatchGraphicsPreset(mobile, GraphicsPlatform::Android) == GraphicsPreset::PlatformDefault &&
          GraphicsPresetName(GraphicsPreset::PlatformDefault) == "Platform defaults" &&
          MatchGraphicsPreset(desktop, GraphicsPlatform::Windows) == GraphicsPreset::PlatformDefault,
          "both Low-dust platform defaults have a truthful label distinct from the historical Off-dust baseline");
    Check(BaselineGraphicsSettings(GraphicsPlatform::Android) == GraphicsSettings{} &&
          BaselineGraphicsSettings(GraphicsPlatform::Android).renderScalePercent == 75 &&
          BaselineGraphicsSettings(GraphicsPlatform::Android).glassEnabled,
          "accepted historical mobile baseline remains75/GlassOn independently of fresh defaults");
    for (const auto schema : {1u, 2u, 3u})
    {
        auto legacy = GraphicsSettings{63, WaterQuality::Off, FireDetail::High, 42, false, ShadowQuality::Higher, false};
        const auto migrated = RecoverGraphicsSettings({schema, legacy, legacy}, GraphicsPlatform::Android);
        Check(migrated.startup.renderScalePercent == 63 && migrated.startup.waterQuality == WaterQuality::Off &&
              migrated.startup.fireDetail == FireDetail::High && migrated.startup.previewFrameCap == 42 &&
              migrated.startup.glassEnabled == (schema == 1u) && migrated.startup.mistEnabled &&
              migrated.startup.shadowQuality == (schema < 3u ? ShadowQuality::Current : ShadowQuality::Higher) &&
              migrated.retainedRequested && migrated.retainedRequested->mistEnabled,
              "schemas1/2/3 preserve historical tuple and pending intent while migrating only MistOn");
    }
    auto off = mobile; off.mistEnabled = false; off.dustQuality = DustQuality::Off;
    auto legacyMobile = mobile; legacyMobile.dustQuality = DustQuality::Off;
    const auto migrated = RecoverGraphicsSettings({4u, off, mobile}, GraphicsPlatform::Android);
    Check(migrated.startup == off && migrated.retainedRequested == legacyMobile,
          "schema4 preserves independently saved Off and interrupted On candidate while migrating DustOff");
    GraphicsEditSession edit(off);
    Check(edit.ResetDraft(GraphicsPlatform::Android) && edit.Draft() == mobile && edit.Committed() == off &&
          !edit.Persistence().pending, "Reset stages defaults; it does not save or issue native work");
    const auto apply = edit.RequestApply(10u);
    if (apply)
    {
        auto old = Presented(*apply); old.effective.mistEnabled = false;
        Check(!edit.Acknowledge(old, true) && !edit.Confirm(), "old Off frame cannot acknowledge requested MistOn");
        Check(edit.Persistence().pending == mobile && edit.Committed() == off,
              "pending Reset intent preserves last saved Off until current presented ACK and Keep");
        Check(edit.Acknowledge(Presented(*apply), true) && edit.Committed() == off && edit.Confirm() &&
              edit.Committed() == mobile && !edit.Persistence().pending, "only current exact MistOn presentation and Keep saves Reset");
    }
    using namespace horde::vulkan::raytracing;
    for (const bool high : {false, true})
        for (const auto shadow : {ShadowQuality::Lower, ShadowQuality::Current, ShadowQuality::Higher})
            for (const auto workload : {RtWorkloadPreset::Lean, RtWorkloadPreset::Authored, RtWorkloadPreset::Max})
            {
                const auto on = ResolveRtQualityControls(shadow, workload, high);
                const auto offPolicy = ResolveRtQualityControls(shadow, workload, high, false);
                Check(on && offPolicy && on->controls[3] == 0u && offPolicy->controls[3] == 1u &&
                      on->controls[0] == offPolicy->controls[0] && on->controls[1] == offPolicy->controls[1] &&
                      on->controls[2] == offPolicy->controls[2], "Mist flag changes onlyw; all independent shadow budgets remain exact");
            }
}

void TestDustMigrationAndAcknowledgement()
{
    using namespace horde::graphics;
    Check(GraphicsSettings{}.dustQuality == DustQuality::Off &&
          PlatformDefaultGraphicsSettings(GraphicsPlatform::Android).dustQuality == DustQuality::Low &&
          PlatformDefaultGraphicsSettings(GraphicsPlatform::Windows).dustQuality == DustQuality::Low,
          "fresh/reset defaults use Low while historical/migration structs keep Off");
    for (const auto platform : {GraphicsPlatform::Android, GraphicsPlatform::Windows})
    {
        for (const auto dust : {DustQuality::Off, DustQuality::Low, DustQuality::Standard})
        {
            auto saved = BaselineGraphicsSettings(platform);
            saved.renderScalePercent = 33; saved.dustQuality = dust; saved.mistEnabled = false;
            const auto reset = PlatformDefaultGraphicsSettings(platform);
            const auto recovered = RecoverGraphicsSettings({kGraphicsSettingsSchema, saved, reset}, platform);
            Check(recovered.startup == saved && recovered.retainedRequested == reset,
                  "saved Off/custom settings remain authoritative over an interrupted Low reset");
            GraphicsEditSession session(saved);
            Check(session.Stage(reset) && session.Committed() == saved,
                  "Reset only stages Low; saved choices require an exact acknowledgement and Keep");
        }
    }
    auto oldTuple = BaselineGraphicsSettings(GraphicsPlatform::Windows);
    oldTuple.dustQuality = DustQuality::Standard; // Stale future field must not leak through old records.
    for (const auto schema : {1u, 2u, 3u, 4u})
    {
        const auto recovered = RecoverGraphicsSettings({schema, oldTuple, oldTuple}, GraphicsPlatform::Windows);
        Check(recovered.startup.dustQuality == DustQuality::Off && recovered.retainedRequested &&
              recovered.retainedRequested->dustQuality == DustQuality::Off,
              "schemas1-4 ignore stale dust values in confirmed and pending tuples");
    }
    auto invalid = BaselineGraphicsSettings(GraphicsPlatform::Windows);
    invalid.dustQuality = static_cast<DustQuality>(3u);
    Check(!ValidGraphicsSettings(invalid), "unknown dust enum rejected at settings admission");

    const auto saved = BaselineGraphicsSettings(GraphicsPlatform::Windows);
    GraphicsEditSession edit(saved);
    auto draft = saved;
    draft.dustQuality = DustQuality::Standard;
    Check(edit.Stage(draft), "valid Standard dust setting stages as requested intent");
    const auto command = edit.RequestApply(1u);
    if (command)
    {
        auto stale = Presented(*command);
        stale.effective.dustQuality = DustQuality::Low;
        Check(!edit.Acknowledge(stale, true) && !edit.Confirm(),
              "uploaded Low or prior dust frame cannot acknowledge requested Standard");
        Check(edit.Persistence().pending && edit.Persistence().pending->dustQuality == DustQuality::Standard &&
              edit.Committed().dustQuality == DustQuality::Off,
              "pending Standard remains requested while saved dust stays Off");
        Check(edit.Acknowledge(Presented(*command), true) && edit.Confirm() &&
              edit.Committed().dustQuality == DustQuality::Standard,
              "exact presented dust quality is the only value Keep commits");
    }
    auto schema5 = GraphicsPersistenceRecord{};
    schema5.confirmed = saved;
    schema5.confirmed.dustQuality = DustQuality::Low;
    schema5.pending = draft;
    const auto recovered = RecoverGraphicsSettings(schema5, GraphicsPlatform::Windows);
    Check(recovered.startup.dustQuality == DustQuality::Low && recovered.retainedRequested &&
          recovered.retainedRequested->dustQuality == DustQuality::Standard,
          "schema5 preserves independently confirmed and pending dust qualities");
}

void TestCoherentPublication()
{
    GraphicsMailbox<GraphicsAppliedSnapshot> mailbox;
    std::atomic<bool> finished{false};
    std::atomic<bool> coherent{true};
    std::thread writer([&] {
        for (std::uint64_t serial = 1u; serial <= 12000u; ++serial)
        {
            GraphicsAppliedSnapshot value;
            value.serial = serial; value.lifecycleGeneration = serial;
            value.internalExtent = {static_cast<std::uint32_t>(serial), static_cast<std::uint32_t>(serial)};
            mailbox.Publish(value);
        }
        finished.store(true);
    });
    std::vector<std::thread> readers;
    for (int index = 0; index < 3; ++index) readers.emplace_back([&] {
        do
        {
            const auto value = mailbox.Read();
            if (value.serial != value.lifecycleGeneration || value.serial != value.internalExtent.width ||
                value.serial != value.internalExtent.height) coherent.store(false);
            std::this_thread::yield();
        } while (!finished.load());
    });
    writer.join(); for (auto& reader : readers) reader.join();
    Check(coherent.load() && mailbox.Read().serial == 12000u, "applied config/dimensions/generation are coherent under concurrent UI reads");
}
void TestExperimentalScaleApplyAndRecovery()
{
    for (const int percent : {33, 40})
    {
        if (!ValidGraphicsRenderScalePercent(percent)) continue;
        for (const auto backend : {GraphicsBackend::RayTracingPipeline, GraphicsBackend::RayQueryCompute})
        {
            const auto baseline = PlatformDefaultGraphicsSettings(GraphicsPlatform::Windows);
            GraphicsEditSession edit(baseline);
            auto draft = baseline;
            draft.renderScalePercent = percent;
            Check(edit.Stage(draft), "an admitted experimental tier stages in the ordinary settings editor");
            const auto command = edit.RequestApply(42u);
            Check(command.has_value(), "experimental apply creates the standard presentation command");
            if (!command) continue;
            auto applied = Presented(*command);
            applied.backend = backend;
            applied.internalExtent = ScaledGraphicsExtent({1920u, 1080u}, percent);
            applied.outputExtent = {1920u, 1080u};
            Check(edit.Acknowledge(applied, true) && edit.Confirm() && edit.Committed() == draft,
                  "only matching RT-present acknowledgement then Keep persists the selected experiment");
            const auto recovered = RecoverGraphicsSettings(edit.Persistence(), GraphicsPlatform::Windows);
            Check(recovered.startup == draft && !recovered.retainedRequested,
                  "confirmed experimental scale survives normal settings recovery");
        }
    }
}
}
int main()
{
    TestMigrationAndProfiles(); TestResolutionAndEffectiveValues(); TestApplyConfirmAndCancel();
    TestFailureDeadlineAndRecovery(); TestExperimentalScaleApplyAndRecovery(); TestEffectiveAcknowledgement(); TestGlassMigrationAndTransactions(); TestIndependentShadowAndLowFire(); TestPlatformDefaultsAndMist(); TestDustMigrationAndAcknowledgement(); TestCoherentPublication();
    return passed ? 0 : 1;
}
