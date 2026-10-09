#include "platform/android/SurfaceSessionMailbox.h"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

namespace
{
using horde::platform::android::SurfaceSessionMailbox;

bool passed = true;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        passed = false;
        std::cerr << "Surface session mailbox: " << message << '\n';
    }
}

struct ReleaseLedger
{
    void Release(int id)
    {
        std::lock_guard lock(mutex);
        ++releases[id];
    }

    int Count(int id)
    {
        std::lock_guard lock(mutex);
        return releases[id];
    }

    std::mutex mutex;
    std::map<int, int> releases;
};

struct FakeWindow
{
    FakeWindow(std::shared_ptr<ReleaseLedger> ledger, int id)
        : ledger(std::move(ledger)), id(id), owns(true) {}

    FakeWindow(const FakeWindow&) = delete;
    FakeWindow& operator=(const FakeWindow&) = delete;

    FakeWindow(FakeWindow&& other) noexcept
        : ledger(std::move(other.ledger)), id(other.id), owns(std::exchange(other.owns, false)) {}

    FakeWindow& operator=(FakeWindow&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            ledger = std::move(other.ledger);
            id = other.id;
            owns = std::exchange(other.owns, false);
        }
        return *this;
    }

    ~FakeWindow() { Release(); }

    void Release()
    {
        if (owns)
        {
            ledger->Release(id);
            owns = false;
        }
    }

    std::shared_ptr<ReleaseLedger> ledger;
    int id;
    bool owns;
};

using Mailbox = SurfaceSessionMailbox<FakeWindow>;
constexpr auto kWaitLimit = std::chrono::seconds(3);
constexpr auto kParkObservation = std::chrono::milliseconds(25);
static_assert(!std::is_copy_constructible_v<FakeWindow>);
static_assert(std::is_move_constructible_v<FakeWindow>);

struct AsyncWaiters
{
    explicit AsyncWaiters(Mailbox& mailbox, const std::uint64_t generation)
    {
        render = std::thread([this, &mailbox, generation]
        {
            {
                std::lock_guard lock(mutex);
                renderEntered = true;
            }
            changed.notify_all();
            mailbox.WaitWhileSuspended(generation);
            {
                std::lock_guard lock(mutex);
                renderReturned = true;
            }
            changed.notify_all();
        });
        taker = std::thread([this, &mailbox]
        {
            {
                std::lock_guard lock(mutex);
                takeEntered = true;
            }
            changed.notify_all();
            auto action = mailbox.Take();
            {
                std::lock_guard lock(mutex);
                takeReturned = true;
                takeHadAction = action.has_value();
                takeHadRequest = action && action->request.has_value();
                takeGeneration = action ? action->generation : 0u;
            }
            changed.notify_all();
        });
    }

    bool WaitUntilParked()
    {
        std::unique_lock lock(mutex);
        const bool entered = changed.wait_for(lock, kWaitLimit, [&] {
            return renderEntered && takeEntered;
        });
        if (!entered) return false;
        changed.wait_for(lock, kParkObservation, [&] { return renderReturned || takeReturned; });
        return !renderReturned && !takeReturned;
    }

    bool WaitUntilReturned()
    {
        std::unique_lock lock(mutex);
        return changed.wait_for(lock, kWaitLimit, [&] { return renderReturned && takeReturned; });
    }

    bool WaitUntilRenderReturned()
    {
        std::unique_lock lock(mutex);
        return changed.wait_for(lock, kWaitLimit, [&] { return renderReturned; });
    }

    bool TakeRemainsBlocked()
    {
        std::unique_lock lock(mutex);
        changed.wait_for(lock, kParkObservation, [&] { return takeReturned; });
        return !takeReturned;
    }

    void Join()
    {
        if (render.joinable()) render.join();
        if (taker.joinable()) taker.join();
    }

    std::mutex mutex;
    std::condition_variable changed;
    bool renderEntered = false;
    bool renderReturned = false;
    bool takeEntered = false;
    bool takeReturned = false;
    bool takeHadAction = false;
    bool takeHadRequest = false;
    std::uint64_t takeGeneration = 0u;
    std::thread render;
    std::thread taker;
};

void TestPendingSupersededAndLatestWins()
{
    auto ledger = std::make_shared<ReleaseLedger>();
    Mailbox mailbox;
    Check(mailbox.Start(FakeWindow{ledger, 1}) != 0, "first request accepted");
    Check(mailbox.Start(FakeWindow{ledger, 2}) != 0, "new request supersedes pending request");
    Check(ledger->Count(1) == 1, "superseded pending window released exactly once");

    auto action = mailbox.Take();
    Check(action && action->request && action->request->id == 2,
          "Take returns the newest pending window");
    Check(action && mailbox.IsCurrent(action->generation), "taken generation is current");
    action.reset();
    Check(ledger->Count(2) == 1, "taken window releases exactly once when owner drops it");
}

void TestStopBeforeTakeAndWorkerOwnedCancellation()
{
    auto ledger = std::make_shared<ReleaseLedger>();
    Mailbox mailbox;
    Check(mailbox.Start(FakeWindow{ledger, 3}) != 0, "pending request accepted before Stop");
    Check(mailbox.Stop(), "unscoped Stop cancels the current generation");
    Check(ledger->Count(3) == 1, "Stop releases a pending request exactly once");
    auto stopped = mailbox.Take();
    Check(stopped && !stopped->request, "Stop-before-Take yields an empty action");

    Check(mailbox.Start(FakeWindow{ledger, 4}) != 0, "request accepted after Stop");
    auto owned = mailbox.Take();
    Check(owned && owned->request && owned->request->id == 4, "worker takes request ownership");
    const auto generation = owned ? owned->generation : 0u;
    mailbox.Stop();
    Check(!mailbox.IsCurrent(generation), "Stop cancels worker-owned generation");
    Check(!mailbox.Publish(generation, 1u), "cancelled worker cannot publish ready status");
    Check(ledger->Count(4) == 0, "Stop does not destroy worker-owned window");
    owned.reset();
    Check(ledger->Count(4) == 1, "worker-owned window releases once after owner drops it");
}

void TestStaleStatusRejectedAcrossRestart()
{
    auto ledger = std::make_shared<ReleaseLedger>();
    Mailbox mailbox;
    const auto oldGeneration = mailbox.Start(FakeWindow{ledger, 5});
    Check(oldGeneration != 0, "first lifecycle request accepted");
    auto first = mailbox.Take();
    Check(mailbox.Publish(oldGeneration, 1u) && mailbox.State() == 1u,
          "current generation may publish ready status");
    Check(mailbox.Stop(oldGeneration), "Activity may stop its own current generation");
    Check(mailbox.State() == 0u, "Stop clears published status");
    Check(!mailbox.Publish(oldGeneration, 3u), "stale completion after Stop is rejected");

    const auto newGeneration = mailbox.Start(FakeWindow{ledger, 6});
    Check(newGeneration != 0, "restart request accepted");
    auto second = mailbox.Take();
    Check(second->generation == newGeneration && newGeneration != oldGeneration,
          "restart receives a new generation");
    Check(!mailbox.Publish(oldGeneration, 1u) && mailbox.State() == 0u,
          "old completion cannot overwrite restarted status");
    Check(mailbox.Publish(second->generation, 2u) && mailbox.State() == 2u,
          "new generation can publish its own status");
    Check(mailbox.State(oldGeneration) == 0u && mailbox.State(newGeneration) == 2u,
          "token-scoped State hides stale generations");
    Check(!mailbox.Publish(second->generation, 4u), "invalid status is rejected");
    Check(!mailbox.Stop(oldGeneration), "stale Activity Stop is rejected");
    Check(mailbox.IsCurrent(newGeneration) && mailbox.State(newGeneration) == 2u && mailbox.State() == 2u,
          "stale Activity teardown leaves the new ready session intact");
    mailbox.Close();
    Check(mailbox.State() == 0u && mailbox.State(newGeneration) == 0u,
          "Close leaves both global and token-scoped status nonready");
}

void TestBlockedInitializationDoesNotBlockStopOrStart()
{
    auto ledger = std::make_shared<ReleaseLedger>();
    Mailbox mailbox;
    Check(mailbox.Start(FakeWindow{ledger, 7}) != 0, "initial request accepted");

    std::mutex mutex;
    std::condition_variable changed;
    bool initializationEntered = false;
    bool allowInitializationToFinish = false;
    bool lifecycleCallsFinished = false;
    bool stopReturned = false;
    bool restartAccepted = false;
    bool oldPublished = true;
    std::thread owner([&]
    {
        auto action = mailbox.Take();
        {
            std::lock_guard lock(mutex);
            initializationEntered = true;
        }
        changed.notify_all();
        {
            std::unique_lock lock(mutex);
            changed.wait(lock, [&] { return allowInitializationToFinish; });
        }
        const bool published = mailbox.Publish(action->generation, 1u);
        action.reset();
        {
            std::lock_guard lock(mutex);
            oldPublished = published;
        }
        changed.notify_all();
    });

    {
        std::unique_lock lock(mutex);
        if (!changed.wait_for(lock, kWaitLimit, [&] { return initializationEntered; }))
        {
            passed = false;
            std::cerr << "Surface session mailbox: owner entered blocked initialization before timeout\n";
        }
    }

    std::thread caller([&]
    {
        mailbox.Stop();
        const bool accepted = mailbox.Start(FakeWindow{ledger, 8});
        {
            std::lock_guard lock(mutex);
            stopReturned = true;
            restartAccepted = accepted;
            lifecycleCallsFinished = true;
        }
        changed.notify_all();
    });

    bool callsFinishedInTime = false;
    {
        std::unique_lock lock(mutex);
        callsFinishedInTime = changed.wait_for(lock, kWaitLimit, [&] { return lifecycleCallsFinished; });
    }
    if (!callsFinishedInTime)
    {
        passed = false;
        std::cerr << "Surface session mailbox: Stop and Start return while initialization is blocked\n";
    }
    {
        std::lock_guard lock(mutex);
        allowInitializationToFinish = true;
    }
    changed.notify_all();
    caller.join();
    owner.join();
    Check(stopReturned && restartAccepted, "Stop returned and newest request was accepted");
    Check(!oldPublished, "blocked old initialization cannot publish after cancellation");
    auto newest = mailbox.Take();
    Check(newest && newest->request && newest->request->id == 8,
          "serial owner takes the newest request after old initialization exits");
    Check(newest && mailbox.IsCurrent(newest->generation), "newest owner generation remains current");
    Check(newest && mailbox.Publish(newest->generation, 1u),
          "newest serialized initialization publishes successfully");
    newest.reset();
    Check(ledger->Count(7) == 1 && ledger->Count(8) == 1,
          "old and newest windows are each released exactly once");
}

void TestCloseWakesTake()
{
    Mailbox mailbox;
    std::mutex mutex;
    std::condition_variable changed;
    bool takeEntered = false;
    bool takeReturned = false;
    std::thread taker([&]
    {
        {
            std::lock_guard lock(mutex);
            takeEntered = true;
        }
        changed.notify_all();
        auto action = mailbox.Take();
        {
            std::lock_guard lock(mutex);
            takeReturned = !action;
        }
        changed.notify_all();
    });
    {
        std::unique_lock lock(mutex);
        if (!changed.wait_for(lock, kWaitLimit, [&] { return takeEntered; }))
        {
            passed = false;
            std::cerr << "Surface session mailbox: Take worker starts before timeout\n";
        }
    }
    mailbox.Close();
    bool returnedInTime = false;
    {
        std::unique_lock lock(mutex);
        returnedInTime = changed.wait_for(lock, kWaitLimit, [&] { return takeReturned; });
    }
    if (!returnedInTime)
    {
        passed = false;
        std::cerr << "Surface session mailbox: Close wakes blocked Take\n";
        mailbox.Close(); // A second notification lets cleanup proceed if the first was lost.
    }
    taker.join();
    Check(takeReturned, "Close causes Take to return no action");
}

void TestColdPendingGenerationCanBeSuspended()
{
    auto ledger = std::make_shared<ReleaseLedger>();
    Mailbox mailbox;
    const auto generation = mailbox.Start(FakeWindow{ledger, 9});
    Check(generation != 0 && mailbox.SetSuspended(generation, true),
          "pending cold generation can be suspended");
    Check(mailbox.IsSuspended(generation) && mailbox.State(generation) == 0u,
          "cold suspension preserves starting status");

    auto action = mailbox.Take();
    Check(action && action->generation == generation && action->request && action->request->id == 9,
          "suspension does not prevent native owner from taking its lifecycle action");
    AsyncWaiters waiters(mailbox, generation);
    Check(waiters.WaitUntilParked(), "render waiter parks on a suspended cold generation");
    Check(mailbox.SetSuspended(generation, false), "cold generation can resume");
    const bool renderReturned = waiters.WaitUntilRenderReturned();
    Check(waiters.TakeRemainsBlocked(), "resume does not wake Take with a lifecycle action");
    mailbox.Close();
    const bool allReturned = waiters.WaitUntilReturned();
    if (!allReturned) mailbox.Close();
    waiters.Join();
    Check(renderReturned && allReturned, "resume releases render; Close releases the still-blocked Take");
    {
        std::lock_guard lock(waiters.mutex);
        Check(!waiters.takeHadAction && waiters.takeGeneration == 0u,
              "suspend/resume creates no extra lifecycle action");
    }
    action.reset();
    mailbox.Close();
    Check(ledger->Count(9) == 1, "cold request ownership still releases exactly once");
}

void TestSameGenerationSuspendResumePreservesPresentedState()
{
    auto ledger = std::make_shared<ReleaseLedger>();
    Mailbox mailbox;
    const auto generation = mailbox.Start(FakeWindow{ledger, 10});
    auto action = mailbox.Take();
    Check(action && mailbox.Publish(generation, 1u), "active generation publishes presented state");
    Check(mailbox.SetSuspended(generation, true) && mailbox.IsSuspended(generation),
          "current presented generation suspends");

    AsyncWaiters waiters(mailbox, generation);
    Check(waiters.WaitUntilParked(), "render waiter and native owner Take waiter park");
    Check(mailbox.State(generation) == 1u && mailbox.State() == 1u,
          "suspension preserves same-generation presented status");
    Check(mailbox.SetSuspended(generation, false), "same generation resumes without replacement");
    const bool renderReturned = waiters.WaitUntilRenderReturned();
    const bool takeStillBlocked = waiters.TakeRemainsBlocked();
    Check(renderReturned && takeStillBlocked, "resume wakes render but creates no Take action");
    {
        std::lock_guard lock(waiters.mutex);
        Check(!waiters.takeHadAction && waiters.takeGeneration == 0u,
              "suspend and resume do not enqueue a native owner action");
    }
    Check(mailbox.IsCurrent(generation) && !mailbox.IsSuspended(generation) &&
          mailbox.State(generation) == 1u,
          "resume keeps the generation and its presented status intact");
    Check(mailbox.Stop(generation), "presented generation stops after resume");
    const bool allReturned = waiters.WaitUntilReturned();
    if (!allReturned) mailbox.Close();
    waiters.Join();
    Check(allReturned, "Stop wakes the still-blocked Take waiter");
    {
        std::lock_guard lock(waiters.mutex);
        Check(waiters.takeHadAction && !waiters.takeHadRequest &&
              waiters.takeGeneration != generation && mailbox.State() == 0u,
          "only Stop produces the next empty lifecycle action and clears readiness");
    }
    action.reset();
    mailbox.Close();
    Check(ledger->Count(10) == 1, "presented request ownership releases exactly once");
}

void TestStaleGenerationCannotResumeReplacement()
{
    auto ledger = std::make_shared<ReleaseLedger>();
    Mailbox mailbox;
    const auto oldGeneration = mailbox.Start(FakeWindow{ledger, 11});
    auto oldAction = mailbox.Take();
    Check(mailbox.Stop(oldGeneration), "old generation stops before replacement");
    auto stopAction = mailbox.Take();
    const auto newGeneration = mailbox.Start(FakeWindow{ledger, 12});
    auto newAction = mailbox.Take();
    Check(newAction && newAction->generation == newGeneration && newGeneration != oldGeneration,
          "replacement starts with a distinct current generation");
    Check(mailbox.Publish(newGeneration, 2u) && mailbox.SetSuspended(newGeneration, true),
          "replacement publishes status and suspends");

    AsyncWaiters waiters(mailbox, newGeneration);
    Check(waiters.WaitUntilParked(), "replacement render and Take waiters park");
    Check(!mailbox.SetSuspended(oldGeneration, false) && mailbox.IsSuspended(newGeneration),
          "stale old token cannot resume the newer suspended generation");
    Check(!mailbox.Publish(oldGeneration, 1u) && mailbox.State(newGeneration) == 2u,
          "stale readiness publication cannot resurrect or overwrite replacement status");
    Check(mailbox.SetSuspended(newGeneration, false), "current generation resumes normally");
    const bool renderReturned = waiters.WaitUntilRenderReturned();
    const bool takeStillBlocked = waiters.TakeRemainsBlocked();
    Check(renderReturned && takeStillBlocked && mailbox.State(newGeneration) == 2u,
          "current resume wakes render, preserves status, and creates no Take action");
    mailbox.Close();
    const bool allReturned = waiters.WaitUntilReturned();
    if (!allReturned) mailbox.Close();
    waiters.Join();
    Check(allReturned, "Close releases the still-blocked Take waiter");
    {
        std::lock_guard lock(waiters.mutex);
        Check(!waiters.takeHadAction, "stale resume attempt does not create a lifecycle action");
    }
    mailbox.Close();
    oldAction.reset();
    stopAction.reset();
    newAction.reset();
    Check(ledger->Count(11) == 1 && ledger->Count(12) == 1,
          "replacement request ownership releases exactly once");
}

enum class WakeAction
{
    Stop,
    Replace,
    Close,
};

void TestLifecycleChangeWakesBothWaiters(const WakeAction wakeAction, const char* label, const int newId)
{
    auto ledger = std::make_shared<ReleaseLedger>();
    Mailbox mailbox;
    const auto generation = mailbox.Start(FakeWindow{ledger, newId});
    auto action = mailbox.Take();
    Check(mailbox.SetSuspended(generation, true), "current generation suspends before lifecycle wake test");
    AsyncWaiters waiters(mailbox, generation);
    Check(waiters.WaitUntilParked(), "render and Take waiters park before lifecycle change");

    std::uint64_t replacementGeneration = 0u;
    if (wakeAction == WakeAction::Stop)
        Check(mailbox.Stop(generation), "Stop cancels the suspended generation");
    else if (wakeAction == WakeAction::Replace)
        replacementGeneration = mailbox.Start(FakeWindow{ledger, newId + 100});
    else
        mailbox.Close();

    const bool returned = waiters.WaitUntilReturned();
    if (!returned) mailbox.Close();
    waiters.Join();
    Check(returned, label);
    Check(!mailbox.IsSuspended(generation), "lifecycle change clears current suspension");
    {
        std::lock_guard lock(waiters.mutex);
        if (wakeAction == WakeAction::Close)
            Check(!waiters.takeHadAction, "Close wakes Take with no action");
        else
            Check(waiters.takeHadAction, "Stop or Start wakes Take with a lifecycle action");
        if (wakeAction == WakeAction::Stop)
            Check(!waiters.takeHadRequest && waiters.takeGeneration != generation,
                  "Stop wakes Take with an empty next-generation action");
        if (wakeAction == WakeAction::Replace)
            Check(waiters.takeHadRequest && waiters.takeGeneration == replacementGeneration,
                  "replacement wakes Take with the newest request");
    }
    if (wakeAction == WakeAction::Replace)
    {
        Check(replacementGeneration != 0u && !mailbox.IsSuspended(replacementGeneration),
              "Start replacement clears suspension for the new generation");
    }
    action.reset();
    mailbox.Close();
    Check(ledger->Count(newId) == 1, "old request releases exactly once after lifecycle wake");
    if (wakeAction == WakeAction::Replace)
        Check(ledger->Count(newId + 100) == 1, "replacement request releases exactly once");
}
}

int main()
{
    TestPendingSupersededAndLatestWins();
    TestStopBeforeTakeAndWorkerOwnedCancellation();
    TestStaleStatusRejectedAcrossRestart();
    TestBlockedInitializationDoesNotBlockStopOrStart();
    TestCloseWakesTake();
    TestColdPendingGenerationCanBeSuspended();
    TestSameGenerationSuspendResumePreservesPresentedState();
    TestStaleGenerationCannotResumeReplacement();
    TestLifecycleChangeWakesBothWaiters(WakeAction::Stop,
        "Stop wakes render and Take waiters", 20);
    TestLifecycleChangeWakesBothWaiters(WakeAction::Replace,
        "Start replacement wakes render and Take waiters", 30);
    TestLifecycleChangeWakesBothWaiters(WakeAction::Close,
        "Close wakes render and Take waiters", 40);
    return passed ? 0 : 1;
}
