#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>

namespace horde::platform::android
{
// One latest, move-owned surface request. No driver work or resource destruction
// runs under the queue lock. The lifecycle worker serializes initialization,
// joining the old render owner and cleanup; callers never wait for those steps.
template<class Request>
class SurfaceSessionMailbox
{
public:
    struct Action
    {
        std::uint64_t generation = 0;
        std::optional<Request> request;
    };

    std::uint64_t Start(Request request)
    {
        std::optional<Request> superseded;
        std::uint64_t generation = 0;
        {
            std::lock_guard lock(mutex_);
            if (closed_ || !AdvanceGeneration()) return 0;
            generation = generation_;
            superseded.swap(pending_);
            pending_.emplace(std::move(request));
            changed_ = true;
            suspended_ = false;
        }
        ready_.notify_all();
        return generation;
    }

    bool Stop(const std::uint64_t expectedGeneration = 0)
    {
        std::optional<Request> superseded;
        {
            std::lock_guard lock(mutex_);
            if (closed_ || (expectedGeneration != 0 && expectedGeneration != generation_)) return false;
            if (!AdvanceGeneration()) closed_ = true; // Never wrap identities.
            superseded.swap(pending_);
            changed_ = true;
            suspended_ = false;
        }
        ready_.notify_all();
        return true;
    }

    void Close()
    {
        std::optional<Request> superseded;
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
            (void)AdvanceGeneration();
            superseded.swap(pending_);
            suspended_ = false;
        }
        ready_.notify_all();
    }

    std::optional<Action> Take()
    {
        std::unique_lock lock(mutex_);
        ready_.wait(lock, [this] { return closed_ || changed_; });
        if (closed_) return std::nullopt;
        changed_ = false;
        Action action{generation_, std::move(pending_)};
        pending_.reset(); // Only the moved-from request is destroyed here.
        return action;
    }

    bool IsCurrent(const std::uint64_t generation) const
    {
        return generation != 0 && (state_.load(std::memory_order_acquire) >> 2u) == generation;
    }

    // Suspension belongs to the current surface generation, but does not
    // create a lifecycle action or alter its published runtime status.
    bool SetSuspended(const std::uint64_t generation, const bool suspended)
    {
        {
            std::lock_guard lock(mutex_);
            if (closed_ || generation == 0 || generation != generation_) return false;
            suspended_ = suspended;
        }
        ready_.notify_all();
        return true;
    }

    bool IsSuspended(const std::uint64_t generation) const
    {
        std::lock_guard lock(mutex_);
        return !closed_ && generation != 0 && generation == generation_ && suspended_;
    }

    // Render-owner wait only. It performs no lifecycle or Vulkan work and
    // returns when resumed, cancelled/replaced, or closed.
    void WaitWhileSuspended(const std::uint64_t generation)
    {
        std::unique_lock lock(mutex_);
        ready_.wait(lock, [this, generation] {
            return closed_ || generation == 0 || generation != generation_ || !suspended_;
        });
    }

    // Generation and runtime status share one atomic: a stale completion cannot
    // resurrect readiness after Stop, including a check/store cancellation race.
    // 0 starting/stopped, 1 successfully RT-presented, 2 unsupported, 3 error.
    bool Publish(const std::uint64_t generation, const unsigned status)
    {
        if (generation == 0 || status > 3) return false;
        auto expected = state_.load(std::memory_order_acquire);
        while ((expected >> 2u) == generation)
        {
            if (state_.compare_exchange_weak(expected, (generation << 2u) | status,
                                           std::memory_order_acq_rel)) return true;
        }
        return false;
    }

    unsigned State() const { return static_cast<unsigned>(state_.load(std::memory_order_acquire) & 3u); }
    unsigned State(const std::uint64_t generation) const
    {
        const auto state = state_.load(std::memory_order_acquire);
        return generation != 0 && (state >> 2u) == generation ? static_cast<unsigned>(state & 3u) : 0u;
    }

private:
    bool AdvanceGeneration()
    {
        if (generation_ == (std::numeric_limits<std::uint64_t>::max() >> 2u))
        {
            state_.store(0, std::memory_order_release);
            return false;
        }
        ++generation_;
        state_.store(generation_ << 2u, std::memory_order_release);
        return true;
    }
    mutable std::mutex mutex_;
    std::condition_variable ready_;
    std::optional<Request> pending_;
    std::uint64_t generation_ = 0;
    bool changed_ = false;
    bool closed_ = false;
    bool suspended_ = false;
    std::atomic<std::uint64_t> state_{0};
};
}
