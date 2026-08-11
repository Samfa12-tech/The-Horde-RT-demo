#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace horde::gameplay::simulation
{

enum class EntityId : std::uint32_t
{
    Invalid = 0,
    Player = 1,
    SkeletonA = 2,
    Skeleton = SkeletonA,
    Lich = 3,
    SkeletonB = 4,
};

enum class GameplayEventType : std::uint8_t
{
    PlayerFootstep,
    PlayerSwing,
    PlayerDamaged,
    PlayerKilled,
    EnemyFootstep,
    EnemyAttackStarted,
    EnemyHit,
    EnemyDefeated,
    LichChargeStarted,
    LichImpact,
    LichDefeated,
    FinaleCompleted,
};

struct GameplayEvent
{
    std::uint64_t sequence = 0;
    GameplayEventType type = GameplayEventType::PlayerFootstep;
    EntityId source = EntityId::Invalid;
    EntityId target = EntityId::Invalid;
    float worldX = 0.0f;
    float worldY = 0.0f;
    float worldZ = 0.0f;
    float intensity = 1.0f;
    std::int32_t payload = 0;
};

class BoundedGameplayEventQueue
{
public:
    static constexpr std::size_t kCapacity = 64u;

    bool Push(GameplayEvent event)
    {
        if (count_ >= events_.size())
        {
            ++overflowCount_;
            return false;
        }
        event.sequence = nextSequence_++;
        events_[count_++] = event;
        if (count_ > highWaterMark_)
        {
            highWaterMark_ = count_;
        }
        return true;
    }

    void Clear()
    {
        count_ = 0u;
    }

    std::span<const GameplayEvent> Events() const
    {
        return {events_.data(), count_};
    }

    const GameplayEvent& operator[](std::size_t index) const { return events_[index]; }
    std::size_t Size() const { return count_; }
    bool Empty() const { return count_ == 0u; }
    std::uint64_t OverflowCount() const { return overflowCount_; }
    std::size_t HighWaterMark() const { return highWaterMark_; }
    std::uint64_t NextSequence() const { return nextSequence_; }

private:
    std::array<GameplayEvent, kCapacity> events_{};
    std::size_t count_ = 0u;
    std::size_t highWaterMark_ = 0u;
    std::uint64_t overflowCount_ = 0u;
    std::uint64_t nextSequence_ = 1u;
};

} // namespace horde::gameplay::simulation
