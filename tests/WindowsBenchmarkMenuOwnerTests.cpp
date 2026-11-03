#include "platform/windows/WindowsBenchmarkMenuOwner.h"
#include "platform/android/AndroidOpeningBenchmarkOwner.h"
#include "platform/android/AndroidBenchmarkOutputTransaction.h"

#include <cstdlib>
#include <iostream>

namespace {
using namespace horde::platform::windows;
using Transfer = OpeningBenchmarkOwnerTransfer;
using Phase = OpeningBenchmarkOwnerPhase;
int failures = 0;
int checks = 0;
void Check(const bool passed, const char* message)
{
    ++checks;
    if (!passed) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
constexpr OpeningBenchmarkEligibility Ready() { return {true, true}; }

void TestSequentialOwnership()
{
    for (const auto choice : {OpeningBenchmarkChoice::WindowsProvisional, OpeningBenchmarkChoice::Custom}) {
        Transfer transfer;
        unsigned liveRenderers = 1;
        Check(transfer.Queue(choice, Ready()), "eligible opening menu admits an explicit choice");
        Check(!transfer.Queue(choice, Ready()) && liveRenderers == 1,
              "duplicate native command cannot allocate another owner");
        --liveRenderers; // Actual native retirement completed before the policy proof.
        Check(transfer.RecheckAndRetireEntry(Ready(), true) && liveRenderers == 0 &&
              transfer.Phase() == Phase::Benchmark && transfer.Choice() == choice,
              "only retired Entry admits benchmark ownership, preserving the explicit choice");
        ++liveRenderers;
        Check(!transfer.CancelQueued() && !transfer.Queue(choice, Ready()) && liveRenderers == 1,
              "late cancellation or queued selection cannot re-enter an acquired benchmark owner");
        --liveRenderers;
        Check(transfer.BenchmarkRetired(true, true) && liveRenderers == 0 &&
              transfer.Phase() == Phase::ReturningEntry,
              "restoration and benchmark retirement jointly admit the new Entry owner");
        ++liveRenderers;
        Check(!transfer.Queue(choice, Ready()), "returning Entry cannot start another benchmark before RT presentation");
        Check(transfer.EntryPresented(true) && transfer.Phase() == Phase::Entry &&
              transfer.Choice() == OpeningBenchmarkChoice::None && liveRenderers == 1,
              "acknowledged new Entry presentation completes the ownership cycle");
        Check(transfer.Queue(choice, Ready()), "a completed return admits a subsequent independent run");
    }
}

void TestEligibilityRaces()
{
    // Each represents a different native message winning before the queued
    // owner's next frame: hiding Entry, loading/starting a game, or graphics IO.
    for (unsigned blocker = 0; blocker < 7; ++blocker) {
        auto changed = Ready();
        switch (blocker) {
        case 0: changed.entryVisible = false; break;
        case 1: changed.entryScene = false; break;
        case 2: changed.activeCampaign = true; break;
        case 3: changed.pendingCampaign = true; break;
        case 4: changed.graphicsTransition = true; break;
        case 5: changed.playHandoff = true; break;
        case 6: changed.benchmarkBusy = true; break;
        }
        Transfer direct, queued;
        Check(!direct.Queue(OpeningBenchmarkChoice::Custom, changed) && direct.Phase() == Phase::Entry,
              "non-opening or busy owner rejects direct/injected benchmark commands");
        Check(queued.Queue(OpeningBenchmarkChoice::WindowsProvisional, Ready()) &&
              !queued.RecheckAndRetireEntry(changed, false) && queued.Phase() == Phase::Entry &&
              queued.Choice() == OpeningBenchmarkChoice::None,
              "eligibility race cancels before retirement and leaves the existing Entry owner usable");
        Check(queued.Queue(OpeningBenchmarkChoice::Custom, Ready()),
              "cancelled pre-retirement request does not poison a later eligible choice");
    }
    Transfer cancelled;
    Check(!cancelled.Queue(OpeningBenchmarkChoice::None, Ready()) &&
          !cancelled.Queue(static_cast<OpeningBenchmarkChoice>(99), Ready()),
          "absent and unknown choices cannot silently select a benchmark profile");
    Check(cancelled.Queue(OpeningBenchmarkChoice::Custom, Ready()) && cancelled.CancelQueued() &&
          !cancelled.CancelQueued() && !cancelled.RecheckAndRetireEntry(Ready(), true) &&
          cancelled.Phase() == Phase::Entry,
          "explicit cancellation consumes the queued request and prevents a stale retirement callback");
}

void TestFailedProofs()
{
    Transfer entryFailure;
    Check(entryFailure.Queue(OpeningBenchmarkChoice::Custom, Ready()) &&
          !entryFailure.RecheckAndRetireEntry(Ready(), false) && entryFailure.Phase() == Phase::Blocked,
          "unproved Entry retirement forbids benchmark allocation");
    Check(!entryFailure.Queue(OpeningBenchmarkChoice::Custom, Ready()) &&
          !entryFailure.CancelQueued() && !entryFailure.RecheckAndRetireEntry(Ready(), true),
          "retained resources cannot be recovered through a late success or another menu command");

    for (const auto proofs : {0u, 1u, 2u}) {
        Transfer failed;
        (void)failed.Queue(OpeningBenchmarkChoice::Custom, Ready());
        (void)failed.RecheckAndRetireEntry(Ready(), true);
        Check(!failed.BenchmarkRetired((proofs & 1u) != 0, (proofs & 2u) != 0) &&
              failed.Phase() == Phase::Blocked && !failed.EntryPresented(true),
              "failed restoration or retirement forbids a new Entry renderer despite a late presentation");
    }
    Transfer presentationFailure;
    (void)presentationFailure.Queue(OpeningBenchmarkChoice::WindowsProvisional, Ready());
    (void)presentationFailure.RecheckAndRetireEntry(Ready(), true);
    (void)presentationFailure.BenchmarkRetired(true, true);
    Check(!presentationFailure.EntryPresented(false) && presentationFailure.Phase() == Phase::Blocked &&
          !presentationFailure.EntryPresented(true),
          "failed return presentation cannot be erased by a later unrelated completion");
    Transfer wrongOwner;
    Check(!wrongOwner.BenchmarkRetired(true, true) && wrongOwner.Phase() == Phase::Blocked,
          "unmatched benchmark retirement cannot create an Entry successor");
    Transfer wrongPresentation;
    Check(!wrongPresentation.EntryPresented(true) && wrongPresentation.Phase() == Phase::Blocked,
          "unmatched Entry completion cannot certify an ownership return");
}
void TestAndroidOpeningOwner()
{
    using Owner=horde::platform::android::AndroidOpeningBenchmarkOwner;
    using AP=Owner::Phase;
    Owner owner;
    auto ineligible=Ready();ineligible.activeCampaign=true;
    Check(!owner.Queue(1,7,ineligible) && !owner.Queue(0,7,Ready()) && !owner.Queue(1,0,Ready()),
          "Android requires explicit identity and actual opening eligibility, never an active campaign");
    Check(owner.Queue(1,7,Ready()) && !owner.Queue(2,7,Ready()) && owner.State()==AP::Queued,
          "Android admits one generation-tagged Custom request and rejects duplication");
    Check(!owner.TakeStart() && owner.Prepare(Ready()) && !owner.ShowcasePresented(7),
          "queued and initialized state cannot manufacture Entry retirement or start measurement");
    Check(owner.EntryRetired(true) && !owner.ShowcasePresented(6) && !owner.TakeStart(),
          "retirement alone and stale surface presentation cannot admit Android measurement");
    Check(owner.ShowcasePresented(7) && owner.TakeStart() && !owner.TakeStart(),
          "only actual current Showcase presentation admits one benchmark start");
    Check(!owner.Cancel(2,7) && !owner.Cancel(1,6),"stale request or origin cannot cancel the current benchmark");
    owner.BeginReturn(2);
    Check(!owner.EntryPresented(7,true) && owner.State()==AP::Blocked,
          "fresh Entry cannot be claimed before benchmark retirement is proved");

    Owner success;
    Check(success.Queue(3,8,Ready()) && success.Prepare(Ready()) && success.EntryRetired(true) &&
          success.ShowcasePresented(8) && success.TakeStart(),"Android success scenario owns one presented Showcase");
    success.BeginReturn(2);
    Check(success.ReturnRetired(true,true) && !success.EntryPresented(7,true) &&
          success.State()==AP::Returning,"restoration/retirement still await the current Entry presentation");
    Check(success.EntryPresented(8,true) && success.Snapshot()[3]==2 && success.Snapshot()[4]==1,
          "terminal success is published only with fresh Entry presentation");

    Owner queued;
    Check(queued.Queue(4,9,Ready()) && queued.Cancel(4,9) && !queued.Prepare(Ready()) &&
          !queued.WantsShowcase() && !queued.TakeStart(),"queued Back cancels before any benchmark scene or measurement");
    Check(queued.ReturnRetired(true,true) && queued.EntryPresented(9,true) && queued.Snapshot()[3]==3,
          "queued cancellation retains its Entry owner and yields a truthful cancelled result");

    Owner suspended;
    Check(suspended.Queue(5,10,Ready()) && suspended.Prepare(Ready()) && suspended.EntryRetired(true) &&
          suspended.ShowcasePresented(10) && suspended.TakeStart(),"surface-loss case starts only after normal ownership proofs");
    suspended.SurfaceRetired(10,true);suspended.BindSurface(11);
    Check(suspended.Returning() && suspended.Cancelled() && !suspended.WantsShowcase() && !suspended.TakeStart() &&
          !suspended.EntryPresented(10,true),"surface replacement makes cancellation sticky and cannot restart measurement");
    Check(suspended.EntryPresented(11,true) && suspended.Snapshot()[3]==3 && suspended.Snapshot()[5]==10,
          "replacement surface acknowledges fresh Entry while retaining original request identity");
    Owner failed;
    Check(failed.Queue(6,12,Ready()) && failed.Prepare(Ready()) && !failed.EntryRetired(false) &&
          failed.State()==AP::Blocked && !failed.Queue(7,13,Ready()),"unproved GPU retirement blocks successor ownership and restart");
}
void TestAndroidFixedOutputTransaction()
{
    using T=horde::platform::android::AndroidBenchmarkOutputTransaction;
    using P=T::Phase;using Scene=T::EvidenceScene;
    using namespace horde;
    const auto spec=*gameplay::BenchmarkProfileSpecification(gameplay::BenchmarkProfileId::MobileStandardV2);
    auto original=graphics::PlatformDefaultGraphicsSettings(graphics::GraphicsPlatform::Android);
    original.previewFrameCap=45;
    gameplay::BenchmarkProfileConfiguration actual;
    actual.platform=graphics::GraphicsPlatform::Android;actual.optical=graphics::OpticalProfile::Mobile;
    actual.backend=graphics::GraphicsBackend::RayTracingPipeline;actual.graphics=spec.graphics;
    actual.graphics.previewFrameCap=45;actual.internal=spec.internal;actual.output=spec.output;
    actual.cameraAspect=16.0/9;actual.outputPolicy=gameplay::BenchmarkOutputPolicy::FixedBufferPreserveAspect;
    T transaction;const auto requested=actual.graphics;
    Check(transaction.Begin(1,9,original) && !transaction.Ready() && transaction.Snapshot()[4]==0,
          "fixed output cannot command UI before native drain or measure from desired pixels");
    Check(transaction.Drained(true) && transaction.Snapshot()[4]==1 &&
          !transaction.Acknowledge(1,9,8,1,true,1280,720,true) &&
          !transaction.Acknowledge(1,9,9,2,true,1280,720,true),
          "only generation and resize-sequence matched UI acknowledgment is consumed");
    Check(transaction.Acknowledge(1,9,9,1,true,1280,720,true) && transaction.Recreating() &&
          !transaction.Observe(Scene::Showcase,actual,requested,2,3,4,5),
          "holder ACK alone never certifies RT resources or completed dimensions");
    Check(transaction.Recreated(2,3,true) && !transaction.Observe(Scene::Showcase,actual,requested,1,3,4,5) &&
          !transaction.Observe(Scene::Showcase,actual,requested,2,2,4,5) &&
          !transaction.Observe(Scene::Showcase,actual,requested,2,3,0,5),
          "applied frame requires current recreated owner and actual submission/completion identities");
    auto wrong=actual;wrong.internal.width=639;
    Check(!transaction.Observe(Scene::Showcase,wrong,requested,2,3,4,5) && !transaction.Ready(),
          "requested scale does not override a mismatched recorded extent");
    wrong=actual;wrong.presentationTransform=1;wrong.cameraAspect=9.0/16;
    Check(!transaction.Observe(Scene::Showcase,wrong,requested,2,3,4,5),
          "nonidentity metadata without transposed images cannot become a standard profile");
    wrong=actual;wrong.dynamicResolutionEnabled=true;
    Check(!transaction.Observe(Scene::Showcase,wrong,requested,2,3,4,5),"DRS cannot masquerade as a fixed profile");
    auto entry=actual;entry.graphics.waterQuality=graphics::WaterQuality::Off;
    Check(!transaction.Observe(Scene::EntryMenu,entry,requested,2,3,4,5) &&
          !transaction.Observe(Scene::EntryMenu,actual,requested,2,3,4,5) &&
          !transaction.Observe(Scene::Unknown,actual,requested,2,3,4,5) && !transaction.Ready(),
          "Entry geometry or even matching quality cannot substitute for actual Showcase measurement proof");
    Check(!transaction.Observe(Scene::Showcase,actual,original,2,3,4,5) &&
          transaction.Observe(Scene::Showcase,actual,requested,2,3,4,5) && transaction.Ready(),
          "full actual Showcase and separately applied configuration jointly admit measurement");
    auto driftCap=requested;++driftCap.previewFrameCap;
    auto driftActual=actual;driftActual.graphics.previewFrameCap=driftCap.previewFrameCap;
    Check(gameplay::MatchesBenchmarkProfile(gameplay::BenchmarkProfileId::MobileStandardV2,driftActual) &&
          !transaction.Observe(Scene::Showcase,driftActual,driftCap,2,3,6,7) && transaction.Ready() &&
          driftCap!=transaction.Requested(),
          "fresh preview-cap drift is benchmark-profile-neutral but must trigger the owner's exact configured-tuple cancellation");
    // A stale completion alone is not configuration drift and must not cancel.
    Check(!transaction.Observe(Scene::Showcase,actual,requested,2,3,4,5) && transaction.Ready() &&
          requested==transaction.Requested(),"stale evidence with unchanged settings does not manufacture a drift cancellation");
    transaction.Restore();
    Check(transaction.Draining() && transaction.Drained(true) && transaction.Snapshot()[4]==2 &&
          !transaction.Acknowledge(1,9,9,1,true,2400,1080,true),
          "restoration drains independently and rejects a late apply ACK");
    Check(transaction.Acknowledge(1,9,9,2,true,2400,1080,true) && transaction.Recreated(4,5,true),
          "restoration acknowledges actual current layout before explicit resource recreation");
    actual.graphics=original;actual.graphics.waterQuality=graphics::WaterQuality::Off;
    actual.output={2400,1080};actual.internal=graphics::ScaledGraphicsExtent(actual.output,original.renderScalePercent);
    actual.cameraAspect=double(actual.internal.width)/actual.internal.height;actual.outputPolicy=gameplay::BenchmarkOutputPolicy::Native;
    wrong=actual;wrong.graphics.mistEnabled=!original.mistEnabled;
    auto staleOutput=actual;staleOutput.output={1280,720};
    staleOutput.internal=graphics::ScaledGraphicsExtent(staleOutput.output,original.renderScalePercent);
    staleOutput.cameraAspect=double(staleOutput.internal.width)/staleOutput.internal.height;
    Check(!transaction.Observe(Scene::EntryMenu,wrong,original,4,5,6,7) &&
          !transaction.Observe(Scene::EntryMenu,staleOutput,original,4,5,6,7) &&
          !transaction.Observe(Scene::EntryMenu,actual,original,4,5,6,5) &&
          !transaction.Observe(Scene::Showcase,actual,original,4,5,6,7) &&
          transaction.Observe(Scene::EntryMenu,actual,original,4,5,6,7) && transaction.Restored(),
          "restored Entry keeps truthful water Off while proving original configured quality and fresh completion");
    T failedApply;failedApply.Begin(2,10,original);failedApply.Drained(true);
    Check(failedApply.Acknowledge(2,10,10,1,true,1279,720,true) && failedApply.Cancelled() && failedApply.Draining() &&
          !failedApply.Acknowledge(2,10,10,1,true,1280,720,true),
          "failed fixed-size ACK is sticky and requires restoration, never late success");
    T lost;lost.Begin(3,11,original);lost.Drained(true);lost.SurfaceLost(11);lost.BindSurface(12);
    Check(lost.Cancelled() && lost.Draining() && lost.Drained(true) && lost.Snapshot()[2]==12 && lost.Snapshot()[4]==2 &&
          !lost.Acknowledge(3,11,11,2,true,1280,720,true),
          "surface replacement only restores on the new generation and never resumes measurement");
    actual.presentationTransform=1;actual.output={2400,1080};
    actual.internal=graphics::ScaledGraphicsExtent(actual.output,original.renderScalePercent);
    actual.cameraAspect=graphics::RtPresentationCameraAspect(actual.internal.width,actual.internal.height,
        graphics::RtPresentationTransform::Rotate90);
    Check(lost.Acknowledge(3,11,12,2,true,1080,2400,true) && lost.Recreated(8,9,true) &&
          lost.Observe(Scene::EntryMenu,actual,original,8,9,10,11) && lost.Restored(),
          "cancelled rotation restores current layout and actual axes without forcing original orientation");
    // Same logical standard in every Vulkan orientation, with non-default saved
    // settings and three independent configured water choices on restoration.
    for(std::uint32_t transform=0;transform<8;++transform)for(bool cancel:{false,true})
    for(auto water:{graphics::WaterQuality::Off,graphics::WaterQuality::Mobile,graphics::WaterQuality::High}) {
        auto personal=original;personal.renderScalePercent=37;personal.glassEnabled=true;
        personal.waterQuality=water;personal.fireDetail=graphics::FireDetail::High;
        personal.shadowQuality=graphics::ShadowQuality::Higher;personal.mistEnabled=true;
        personal.dustQuality=graphics::DustQuality::Standard;
        const bool swapped=graphics::RtPresentationTransformSwapsAxes(graphics::RtPresentationTransformFromIndex(transform));
        auto applied=actual;applied.graphics=spec.graphics;applied.graphics.previewFrameCap=personal.previewFrameCap;
        applied.presentationTransform=transform;
        applied.internal=swapped?graphics::GraphicsExtent{360,640}:spec.internal;
        applied.output=swapped?graphics::GraphicsExtent{720,1280}:spec.output;
        applied.cameraAspect=graphics::RtPresentationCameraAspect(applied.internal.width,applied.internal.height,
            graphics::RtPresentationTransformFromIndex(transform));
        applied.outputPolicy=gameplay::BenchmarkOutputPolicy::FixedBufferPreserveAspect;
        T oriented;
        Check(oriented.Begin(10+transform,30,personal) && oriented.Drained(true) &&
            oriented.Acknowledge(10+transform,30,30,1,true,1280,720,true) && oriented.Recreated(10,11,true) &&
            oriented.Observe(Scene::Showcase,applied,oriented.Requested(),10,11,12,13) && oriented.Ready() &&
            oriented.MatchesAppliedTransform(transform),
            "actual oriented image and Showcase quality proof admit standard without changing the holder contract");
        if(cancel) {
            auto drift=applied;drift.presentationTransform=transform^2u;
            Check(gameplay::MatchesBenchmarkProfile(gameplay::BenchmarkProfileId::MobileStandardV2,drift) &&
                !oriented.MatchesAppliedTransform(drift.presentationTransform) &&
                !oriented.Observe(Scene::Showcase,drift,oriented.Requested(),10,11,14,15),
                "otherwise-valid orientation drift cannot silently continue an active measurement");
        }
        oriented.Restore(cancel);
        Check(oriented.Drained(true) && oriented.Acknowledge(10+transform,30,30,2,true,2400,1080,true) &&
            oriented.Recreated(20,21,true),"success and cancellation require their own restored holder and resource owner");
        auto restored=applied;restored.graphics=personal;restored.graphics.waterQuality=graphics::WaterQuality::Off;
        restored.output=swapped?graphics::GraphicsExtent{1080,2400}:graphics::GraphicsExtent{2400,1080};
        restored.internal=graphics::ScaledGraphicsExtent(restored.output,personal.renderScalePercent);
        restored.cameraAspect=graphics::RtPresentationCameraAspect(restored.internal.width,restored.internal.height,
            graphics::RtPresentationTransformFromIndex(transform));
        restored.outputPolicy=gameplay::BenchmarkOutputPolicy::Native;
        auto wrongPreference=personal;
        wrongPreference.waterQuality=water==graphics::WaterQuality::Off?graphics::WaterQuality::Mobile:graphics::WaterQuality::Off;
        auto wrongQuality=restored;wrongQuality.graphics.fireDetail=graphics::FireDetail::Mobile;
        auto wrongDispatch=restored;++wrongDispatch.internal.width;
        wrongDispatch.cameraAspect=graphics::RtPresentationCameraAspect(wrongDispatch.internal.width,wrongDispatch.internal.height,
            graphics::RtPresentationTransformFromIndex(transform));
        Check(!oriented.Observe(Scene::EntryMenu,restored,wrongPreference,20,21,22,23) &&
            !oriented.Observe(Scene::EntryMenu,wrongQuality,personal,20,21,22,23) &&
            !oriented.Observe(Scene::EntryMenu,wrongDispatch,personal,20,21,22,23) &&
            !oriented.Observe(Scene::Showcase,restored,personal,20,21,22,23) &&
            oriented.Observe(Scene::EntryMenu,restored,personal,20,21,22,23) && oriented.Restored() &&
            oriented.Cancelled()==cancel && oriented.Original()==personal,
            "Entry restoration keeps observed Off separate from exact saved water preference and verifies every other field");
    }
    T unsafe;unsafe.Begin(4,13,original);
    Check(!unsafe.Drained(false) && unsafe.State()==P::Failed && unsafe.Snapshot()[4]==0,
          "unproved GPU drain cannot issue any buffer mutation command");
}
} // namespace

int main()
{
    TestSequentialOwnership();
    TestEligibilityRaces();
    TestFailedProofs();
    TestAndroidOpeningOwner();
    TestAndroidFixedOutputTransaction();
    std::cout << checks << " opening benchmark ownership checks, " << failures << " failures\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
