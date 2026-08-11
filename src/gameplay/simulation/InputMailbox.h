#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <thread>

#include "gameplay/simulation/InputSnapshot.h"

namespace horde::gameplay::simulation
{

struct PublishedInput
{
    InputSnapshot snapshot{};
    std::uint64_t publicationSequence = 0;
};

// Single-writer, multi-reader coherent POD mailbox. The access word pins a
// reader before it copies a slot and gives the writer exclusive ownership of
// the unpublished slot. This closes the stale-index race that a bare two-slot
// index swap would otherwise have when publications lap a reader.
class InputMailbox
{
public:
    std::uint64_t Publish(const InputSnapshot& snapshot)
    {
        const std::uint64_t current = publishedToken_.load(std::memory_order_acquire);
        const std::size_t slotIndex = 1u - static_cast<std::size_t>(current & 1u);
        Slot& slot = slots_[slotIndex];
        AcquireWriter(slot);
        slot.snapshot = snapshot;
        slot.access.store(0u, std::memory_order_release);

        const std::uint64_t sequence = nextPublicationSequence_.fetch_add(1u, std::memory_order_relaxed) + 1u;
        publishedToken_.store((sequence << 1u) | slotIndex, std::memory_order_release);
        return sequence;
    }

    PublishedInput ConsumeLatest() const
    {
        for (;;)
        {
            const std::uint64_t before = publishedToken_.load(std::memory_order_acquire);
            const std::size_t slotIndex = static_cast<std::size_t>(before & 1u);
            const Slot& slot = slots_[slotIndex];
            if (!AcquireReader(slot))
            {
                std::this_thread::yield();
                continue;
            }

            const std::uint64_t after = publishedToken_.load(std::memory_order_acquire);
            if (before != after)
            {
                ReleaseReader(slot);
                continue;
            }

            PublishedInput result;
            result.snapshot = slot.snapshot;
            result.publicationSequence = before >> 1u;
            ReleaseReader(slot);
            return result;
        }
    }

    std::uint64_t PublishedSequence() const
    {
        return publishedToken_.load(std::memory_order_acquire) >> 1u;
    }

private:
    static constexpr std::uint32_t kWriterOwned = std::numeric_limits<std::uint32_t>::max();

    struct Slot
    {
        InputSnapshot snapshot{};
        mutable std::atomic<std::uint32_t> access{0u};
    };

    static void AcquireWriter(Slot& slot)
    {
        for (;;)
        {
            std::uint32_t expected = 0u;
            if (slot.access.compare_exchange_weak(expected,
                                                  kWriterOwned,
                                                  std::memory_order_acquire,
                                                  std::memory_order_relaxed))
            {
                return;
            }
            std::this_thread::yield();
        }
    }

    static bool AcquireReader(const Slot& slot)
    {
        std::uint32_t readers = slot.access.load(std::memory_order_acquire);
        while (readers != kWriterOwned && readers < kWriterOwned - 1u)
        {
            if (slot.access.compare_exchange_weak(readers,
                                                  readers + 1u,
                                                  std::memory_order_acquire,
                                                  std::memory_order_relaxed))
            {
                return true;
            }
        }
        return false;
    }

    static void ReleaseReader(const Slot& slot)
    {
        slot.access.fetch_sub(1u, std::memory_order_release);
    }

    mutable std::array<Slot, 2u> slots_{};
    std::atomic<std::uint64_t> publishedToken_{0u};
    std::atomic<std::uint64_t> nextPublicationSequence_{0u};
};

} // namespace horde::gameplay::simulation
