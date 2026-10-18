#include "platform/android/AndroidCampaignLoadBarrier.h"

#include <iostream>
#include <limits>

namespace
{
using Barrier = horde::platform::android::AndroidCampaignLoadBarrier;
bool passed = true;
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        passed = false;
        std::cerr << "Android campaign load barrier: " << message << '\n';
    }
}

void CurrentLoadPresentation()
{
    Barrier barrier;
    Check(!barrier.KeepsPaused(), "idle does not retain a load pause");
    Check(!barrier.Begin(0, 1) && !barrier.Begin(1, 0), "zero identities rejected");
    Check(barrier.Begin(1, 10), "begin current request");
    Check(barrier.KeepsPaused(), "preparation retains pause");
    Check(!barrier.Begin(2, 10), "cannot supersede preparation");
    Check(!barrier.CommitLoaded(2, 10, 1.2f, .1f, 40, 9), "wrong request cannot commit");
    Check(!barrier.CommitLoaded(1, 9, 1.2f, .1f, 40, 9), "old surface cannot commit");
    Check(!barrier.CommitLoaded(1, 10, std::numeric_limits<float>::quiet_NaN(), .1f, 40, 9),
        "nonfinite pose rejected");
    Check(!barrier.CommitLoaded(1, 10, 1.2f, .1f, 40, 0), "zero world rejected");
    Check(barrier.CommitLoaded(1, 10, 1.2f, .1f, 40, 9), "matching restore commits barrier");
    const auto token = barrier.State().rebaseToken;
    Check(token && barrier.State().yaw == 1.2f && barrier.State().pitch == .1f,
        "rebase token and loaded pose retained");
    Check(barrier.HasPendingCommittedLoad() && barrier.State().MatchesRebasePose(1.2f, .1f),
        "committed preparation requires its exact camera handshake");
    Check(!barrier.State().MatchesRebasePose(std::nextafter(1.2f, 2.f), .1f) &&
        !barrier.State().MatchesRebasePose(1.2f, std::nextafter(.1f, 1.f)) &&
        !barrier.State().MatchesRebasePose(std::numeric_limits<float>::quiet_NaN(), .1f),
        "one-ULP and nonfinite poses cannot acknowledge a blocked simulation floor");
    Check(!barrier.Presented(10, 40, 9), "presentation before Java acknowledgement rejected");
    Check(!barrier.Acknowledge(token + 1, 10) && !barrier.Acknowledge(token, 9),
        "wrong token or surface rejected");
    Check(!barrier.Cancel(1) && !barrier.Fail(1) && barrier.KeepsPaused(),
        "postcommit cancel and failure cannot abandon pause obligation");
    Check(barrier.Acknowledge(token, 10) && barrier.KeepsPaused(), "ack still retains pause");
    Check(!barrier.Acknowledge(token, 10), "duplicate ack rejected");
    Check(!barrier.Presented(9, 40, 9), "retired surface output rejected");
    Check(!barrier.Presented(10, 40, 8), "old world output rejected");
    Check(!barrier.Presented(10, 39, 9), "preload tick rejected");
    Check(barrier.Presented(10, 40, 9), "actual loaded-frame identities complete barrier");
    Check(!barrier.KeepsPaused() && barrier.State().phase == Barrier::Phase::Complete,
        "only completion releases load pause");
    Check(!barrier.Fail(2) && barrier.State().loaded && !barrier.HasPendingCommittedLoad(),
        "failed later NewGame cannot inherit a completed load pause obligation");
    Check(!barrier.Presented(10, 41, 9) && !barrier.SurfaceRetired(10),
        "terminal load does not repeat or bind ordinary lifecycle");
    Check(!barrier.Begin(1, 10) && barrier.Begin(2, 10), "new request must be monotonic");
    Check(barrier.CommitLoaded(2, 10, 0, 0, 0, 10), "zero loaded tick is valid");
    Check(barrier.State().rebaseToken > token, "tokens remain monotonic across loads");
    Check(!barrier.Acknowledge(token, 10), "previous load ack rejected on same surface");
}

void RetiredPendingLoad(bool acknowledged)
{
    Barrier barrier;
    Check(barrier.Begin(1, 20) && barrier.CommitLoaded(1, 20, -.4f, .2f, 100, 12),
        "prepare pending retirement case");
    const auto token = barrier.State().rebaseToken;
    if (acknowledged) Check(barrier.Acknowledge(token, 20), "ack before retirement");
    Check(!barrier.SurfaceRetired(19) && barrier.SurfaceRetired(20), "retire matching surface only");
    Check(barrier.KeepsPaused() && barrier.State().loaded && !barrier.State().rebaseToken &&
        !barrier.State().surfaceGeneration, "retirement retains loaded game and revokes token");
    Check(!barrier.Cancel(1) && !barrier.Begin(2, 21), "cannot discard or supersede committed load");
    Check(!barrier.Acknowledge(token, 20) && !barrier.Presented(20, 100, 12),
        "retired callbacks cannot advance barrier");
    Check(!barrier.ResumeOnSurface(20) && !barrier.ResumeOnSurface(19), "stale surfaces cannot resume");
    Check(barrier.ResumeOnSurface(21), "new surface resumes pending loaded game");
    const auto freshToken = barrier.State().rebaseToken;
    Check(freshToken > token && barrier.State().loadedTick == 100 &&
        barrier.State().loadedWorldGeneration == 12 && barrier.State().yaw == -.4f,
        "new handshake preserves loaded identities and pose");
    Check(!barrier.Acknowledge(token, 21) && !barrier.Presented(21, 100, 12),
        "old token and presentation without fresh ack rejected");
    Check(barrier.Acknowledge(freshToken, 21) && barrier.Presented(21, 101, 12),
        "fresh ack and loaded presentation recover without freezing game");
    Check(!barrier.KeepsPaused(), "recovered load can release pause");
}

void RecreatedPendingResources(bool acknowledged)
{
    Barrier barrier;
    Check(!barrier.ResourcesRecreated(30, 1, 1), "idle cannot claim retained loaded resources");
    Check(barrier.Begin(1, 30), "begin recreation case");
    Check(!barrier.ResourcesRecreated(30, 100, 12), "precommit recreation cannot bind a load");
    Check(barrier.CommitLoaded(1, 30, -.4f, .2f, 100, 12), "commit recreation case");
    const auto token = barrier.State().rebaseToken;
    if (acknowledged) Check(barrier.Acknowledge(token, 30), "ack before recreation");
    Check(!barrier.ResourcesRecreated(0, 100, 13) &&
        !barrier.ResourcesRecreated(29, 100, 13) &&
        !barrier.ResourcesRecreated(31, 100, 13), "recreation requires exact same surface");
    Check(!barrier.ResourcesRecreated(30, 100, 0) &&
        !barrier.ResourcesRecreated(30, 100, 12) &&
        !barrier.ResourcesRecreated(30, 100, 11) &&
        !barrier.ResourcesRecreated(30, 99, 13), "zero or stale loaded identities rejected");
    Check(barrier.ResourcesRecreated(30, 100, 13), "renderer-only recreation rebases current world");
    const auto freshToken = barrier.State().rebaseToken;
    Check(barrier.KeepsPaused() && freshToken > token && barrier.State().loaded &&
        barrier.State().loadedTick == 100 && barrier.State().loadedWorldGeneration == 13 &&
        barrier.State().yaw == -.4f && barrier.State().pitch == .2f,
        "recreation retains game and pose with fresh handshake");
    Check(!barrier.Acknowledge(token, 30) && !barrier.Presented(30, 100, 13),
        "old token and fresh-world output before ack rejected");
    Check(barrier.Acknowledge(freshToken, 30), "fresh recreation acknowledgement accepted");
    Check(!barrier.Presented(30, 100, 12) && !barrier.Presented(30, 99, 13),
        "old world and old tick output rejected after fresh acknowledgement");
    Check(barrier.Presented(30, 100, 13) && !barrier.KeepsPaused(),
        "fresh ack and recreated output complete pending load");
    Check(!barrier.ResourcesRecreated(30, 100, 14), "completed load does not bind ordinary recreation");
}
void SuspendedPendingInput(bool acknowledged)
{
    Barrier barrier;
    Check(!barrier.InputSuspended(40), "idle suspension cannot create a load handshake");
    Check(barrier.Begin(1, 40), "begin same-surface suspension case");
    Check(!barrier.InputSuspended(40), "precommit suspension uses cancellation instead");
    Check(barrier.CommitLoaded(1, 40, -.5f, .1f, 200, 15), "commit same-surface suspension case");
    const auto oldToken = barrier.State().rebaseToken;
    if (acknowledged) Check(barrier.Acknowledge(oldToken, 40), "ack before Home suspension");
    Check(!barrier.InputSuspended(0) && !barrier.InputSuspended(39) &&
        !barrier.InputSuspended(41), "suspension requires exact nonzero surface");
    Check(barrier.State().rebaseToken == oldToken, "rejected suspension does not revoke current handshake");
    Check(barrier.InputSuspended(40), "Home suspension revokes pending same-surface handshake");
    const auto freshToken = barrier.State().rebaseToken;
    Check(barrier.KeepsPaused() && freshToken > oldToken && barrier.State().loaded &&
        barrier.State().surfaceGeneration == 40 && barrier.State().loadedTick == 200 &&
        barrier.State().loadedWorldGeneration == 15 && barrier.State().yaw == -.5f &&
        barrier.State().pitch == .1f, "Home preserves loaded identity and pose while paused");
    Check(!barrier.Acknowledge(oldToken, 40) && !barrier.Presented(40, 200, 15),
        "pre-Home ack and presentation without fresh ack rejected");
    Check(barrier.Acknowledge(freshToken, 40) && barrier.KeepsPaused(),
        "fresh post-Home acknowledgement still awaits presentation");
    Check(!barrier.Presented(39, 200, 15) && !barrier.Presented(40, 200, 14),
        "post-Home stale surface and world output rejected");
    Check(barrier.Presented(40, 200, 15) && !barrier.KeepsPaused(),
        "fresh acknowledgement and loaded output finish same-surface recovery");
    Check(!barrier.InputSuspended(40), "completed loads do not bind ordinary Home suspension");
}
void CancelAndFailure()
{
    Barrier barrier;
    Check(barrier.Begin(1, 30) && !barrier.Cancel(2) && barrier.Cancel(1),
        "only matching precommit cancellation succeeds");
    Check(!barrier.KeepsPaused() && !barrier.CommitLoaded(1, 30, 0, 0, 1, 1),
        "cancelled preparation cannot commit late");
    Check(!barrier.Begin(1, 30), "cancelled request identity cannot be reused");
    Check(barrier.Begin(2, 30) && barrier.SurfaceRetired(30), "precommit retirement cancels");
    Check(!barrier.KeepsPaused() && !barrier.ResumeOnSurface(31), "uncommitted load cannot resurrect");
    Check(!barrier.CommitLoaded(2, 30, 0, 0, 1, 1), "retired preparation cannot commit");
    Check(barrier.Begin(3, 31) && !barrier.Fail(2) && barrier.Fail(3), "matching failure recorded");
    Check(barrier.State().phase == Barrier::Phase::Failure && barrier.KeepsPaused(),
        "failure retains menu pause for explicit recovery");
    Check(!barrier.CommitLoaded(3, 31, 0, 0, 1, 1), "failed preparation cannot commit");
    Check(barrier.Begin(4, 31) && barrier.Cancel(4), "new request recovers precommit failure");
    Check(!barrier.Begin(5, 30), "surface generation cannot regress across requests");
}
}

int main()
{
    CurrentLoadPresentation();
    RetiredPendingLoad(false);
    RetiredPendingLoad(true);
    RecreatedPendingResources(false);
    RecreatedPendingResources(true);
    SuspendedPendingInput(false);
    SuspendedPendingInput(true);
    CancelAndFailure();
    if (passed) std::cout << "Android campaign load barrier tests passed\n";
    return passed ? 0 : 1;
}


