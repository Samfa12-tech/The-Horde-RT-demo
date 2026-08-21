#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "gameplay/simulation/GameplayEvent.h"

namespace horde::gameplay
{

// Preserve the authored gap between the sword impact and the body-fall cue on
// every platform. The queue retains the complete semantic event, including its
// fixed-tick source and listener state, until playback becomes due.
inline constexpr std::uint64_t kEnemyImpactFallDelayMilliseconds = 140u;

class DelayedGameplayFeedbackQueue
{
public:
    static constexpr std::size_t kCapacity = simulation::BoundedGameplayEventQueue::kCapacity;

    bool Enqueue(const simulation::GameplayEvent& event, const std::uint64_t dueMilliseconds)
    {
        if (count_ >= entries_.size())
        {
            ++overflowCount_;
            return false;
        }
        entries_[count_++] = {dueMilliseconds, event};
        if (count_ > highWaterMark_)
        {
            highWaterMark_ = count_;
        }
        return true;
    }

    template <typename Consumer>
    std::size_t DrainDue(const std::uint64_t nowMilliseconds, Consumer&& consumer)
    {
        std::size_t retained = 0u;
        std::size_t drained = 0u;
        for (std::size_t index = 0u; index < count_; ++index)
        {
            if (entries_[index].dueMilliseconds <= nowMilliseconds)
            {
                consumer(entries_[index].event);
                ++drained;
            }
            else
            {
                if (retained != index)
                {
                    entries_[retained] = entries_[index];
                }
                ++retained;
            }
        }
        count_ = retained;
        return drained;
    }

    void Clear() { count_ = 0u; }
    std::size_t Size() const { return count_; }
    std::size_t HighWaterMark() const { return highWaterMark_; }
    std::uint64_t OverflowCount() const { return overflowCount_; }

private:
    struct Entry
    {
        std::uint64_t dueMilliseconds = 0u;
        simulation::GameplayEvent event{};
    };

    std::array<Entry, kCapacity> entries_{};
    std::size_t count_ = 0u;
    std::size_t highWaterMark_ = 0u;
    std::uint64_t overflowCount_ = 0u;
};

} // namespace horde::gameplay
