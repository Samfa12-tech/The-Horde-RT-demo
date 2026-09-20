#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace horde::gameplay
{

enum class BenchmarkWorkload : std::uint8_t
{
    ShowcaseRoute,
    LanternHeldHigh,
    LanternHeldLow,
    LanternGrazing,
    LanternMotionExtreme,
    LanternRevealSequence,
};

inline constexpr std::array<BenchmarkWorkload, 6> kBenchmarkWorkloads{{
    BenchmarkWorkload::ShowcaseRoute,
    BenchmarkWorkload::LanternHeldHigh,
    BenchmarkWorkload::LanternHeldLow,
    BenchmarkWorkload::LanternGrazing,
    BenchmarkWorkload::LanternMotionExtreme,
    BenchmarkWorkload::LanternRevealSequence}};

constexpr std::string_view BenchmarkWorkloadName(const BenchmarkWorkload workload)
{
    switch (workload)
    {
    case BenchmarkWorkload::ShowcaseRoute: return "showcase-route-v1";
    case BenchmarkWorkload::LanternHeldHigh: return "lantern-held-high-v1";
    case BenchmarkWorkload::LanternHeldLow: return "lantern-held-low-v1";
    case BenchmarkWorkload::LanternGrazing: return "lantern-grazing-v1";
    case BenchmarkWorkload::LanternMotionExtreme: return "lantern-motion-extreme-v1";
    case BenchmarkWorkload::LanternRevealSequence: return "lantern-reveal-sequence-v1";
    }
    return "invalid";
}

inline bool ParseBenchmarkWorkload(const std::string_view name, BenchmarkWorkload& result)
{
    for (const auto workload : kBenchmarkWorkloads)
    {
        if (BenchmarkWorkloadName(workload) == name)
        {
            result = workload;
            return true;
        }
    }
    return false;
}

constexpr bool IsLanternBenchmark(const BenchmarkWorkload workload)
{
    return workload != BenchmarkWorkload::ShowcaseRoute &&
           BenchmarkWorkloadName(workload) != "invalid";
}

constexpr bool IsFrozenBenchmark(const BenchmarkWorkload workload)
{
    return IsLanternBenchmark(workload) &&
           workload != BenchmarkWorkload::LanternRevealSequence;
}

inline constexpr std::uint32_t kLanternBenchmarkFramesPerLap = 600u;
inline constexpr float kLanternBenchmarkX = -10.65f;
inline constexpr float kLanternBenchmarkZ = -15.20f;
inline constexpr float kLanternBenchmarkYaw = -1.57079632679f;
inline constexpr float kLanternBenchmarkPitch = -0.30f;

} // namespace horde::gameplay
