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
static_assert(!std::is_copy_constructible_v<FakeWindow>);
static_assert(std::is_move_constructible_v<FakeWindow>);

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
}

int main()
{
    TestPendingSupersededAndLatestWins();
    TestStopBeforeTakeAndWorkerOwnedCancellation();
    TestStaleStatusRejectedAcrossRestart();
    TestBlockedInitializationDoesNotBlockStopOrStart();
    TestCloseWakesTake();
    return passed ? 0 : 1;
}
