#pragma once
#include <cstdint>
#include <optional>

namespace horde::platform::windows {
struct PresentReturnInterval {
    std::uint64_t nanoseconds=0;
    bool available=false;
};
// Consume only actual successful present-call timestamps from one route.
// Invalid/missing stamps preserve the previous owner and fail the measurement.
class WindowsPresentIntervals {
public:
    explicit WindowsPresentIntervals(std::uint64_t start):start_(start) {}
    std::optional<PresentReturnInterval> Consume(std::uint64_t stamp) {
        if(stamp<=start_ || (previous_ && stamp<=previous_))return std::nullopt;
        const PresentReturnInterval interval{previous_?stamp-previous_:0,previous_!=0};
        previous_=stamp;
        return interval;
    }
private:
    std::uint64_t start_=0,previous_=0;
};
}
