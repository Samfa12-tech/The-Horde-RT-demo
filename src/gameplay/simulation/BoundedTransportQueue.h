#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace horde::gameplay::simulation
{

// Fixed-capacity platform handoff queue. Callers provide synchronization when
// producer and consumer live on different threads. Overflow drops only the
// newest publication and remains permanently visible in diagnostics.
template <typename Value, std::size_t Capacity>
class BoundedTransportQueue
{
public:
    static_assert(Capacity > 0u);

    bool Push(const Value& value)
    {
        if (count_ >= values_.size())
        {
            ++overflowCount_;
            return false;
        }
        values_[count_++] = value;
        if (count_ > highWaterMark_)
        {
            highWaterMark_ = count_;
        }
        return true;
    }

    void Clear() { count_ = 0u; }
    std::span<const Value> Values() const { return {values_.data(), count_}; }
    const Value& operator[](const std::size_t index) const { return values_[index]; }
    std::size_t Size() const { return count_; }
    std::size_t HighWaterMark() const { return highWaterMark_; }
    std::uint64_t OverflowCount() const { return overflowCount_; }

private:
    std::array<Value, Capacity> values_{};
    std::size_t count_ = 0u;
    std::size_t highWaterMark_ = 0u;
    std::uint64_t overflowCount_ = 0u;
};

} // namespace horde::gameplay::simulation
