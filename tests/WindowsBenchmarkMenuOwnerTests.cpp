#include "platform/windows/WindowsBenchmarkMenuOwner.h"

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
} // namespace

int main()
{
    TestSequentialOwnership();
    TestEligibilityRaces();
    TestFailedProofs();
    std::cout << checks << " opening benchmark ownership checks, " << failures << " failures\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
