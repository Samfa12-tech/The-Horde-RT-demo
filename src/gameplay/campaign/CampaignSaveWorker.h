#pragma once
#include "gameplay/campaign/CampaignSlots.h"
#include <condition_variable>
#include <mutex>
#include <thread>

namespace horde::gameplay::campaign::async {
// One active IO + one queued immutable request. Directory is explicitly supplied
// by the platform/test owner; no default save root or directory creation.
class Worker {
public:
    explicit Worker(std::filesystem::path directory,storage::FaultInjector* faults=nullptr);
    ~Worker();
    Worker(const Worker&)=delete;
    Worker& operator=(const Worker&)=delete;
    bool Submit(Request request);
    // Drop queued work and join the owned thread. The caller must revoke its
    // coordinators first. Already-issued OS IO can finish; it cannot be killed.
    void Stop();
    bool Idle() const;
private:
    void Run();
    void Execute(const Request& request);
    const std::filesystem::path directory_;
    storage::FaultInjector* const faults_;
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::optional<Request> queued_;
    bool stopping_=false,active_=false;
    std::thread thread_;
};
} // namespace horde::gameplay::campaign::async
