#include "platform/windows/WindowsBenchmarkMenuOwner.h"
#include "platform/android/AndroidOpeningBenchmarkOwner.h"

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
} // namespace

int main()
{
    TestSequentialOwnership();
    TestEligibilityRaces();
    TestFailedProofs();
    TestAndroidOpeningOwner();
    std::cout << checks << " opening benchmark ownership checks, " << failures << " failures\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
