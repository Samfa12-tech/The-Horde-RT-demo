#pragma once

#include <span>
#include <string>
#include <string_view>
#include "gameplay/BenchmarkWorkload.h"

namespace horde::platform::windows
{

struct WindowsBenchmarkLaunch
{
    bool requested = false;
    std::wstring outputDirectory;
    std::string error;
    horde::gameplay::BenchmarkWorkload workload = horde::gameplay::BenchmarkWorkload::ShowcaseRoute;
};

// Arguments exclude argv[0]. This only selects the existing player benchmark;
// it grants no checkpoint mutation, diagnostic instrumentation or quality override.
inline WindowsBenchmarkLaunch ParseWindowsBenchmarkLaunch(
    const std::span<const std::wstring_view> arguments)
{
    WindowsBenchmarkLaunch result;
    bool captureOrCheckpoint = false;
    bool workloadSpecified = false;
    for (std::size_t index = 0u; index < arguments.size(); ++index)
    {
        const auto argument = arguments[index];
        if (argument == L"--benchmark-workload")
        {
            if (workloadSpecified || ++index == arguments.size())
            {
                result.error = "--benchmark-workload requires one unique workload name.";
                return result;
            }
            workloadSpecified = true;
            bool found = false;
            for (const auto workload : horde::gameplay::kBenchmarkWorkloads)
            {
                const auto name = horde::gameplay::BenchmarkWorkloadName(workload);
                if (arguments[index] == std::wstring(name.begin(), name.end()))
                {
                    result.workload = workload;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                result.error = "Unknown versioned benchmark workload.";
                return result;
            }
            continue;
        }
        if (argument == L"--capture-showcase" || argument == L"--development-checkpoint")
        {
            captureOrCheckpoint = true;
        }
        if (argument != L"--benchmark-showcase")
        {
            continue;
        }
        if (result.requested)
        {
            result.error = "--benchmark-showcase may only be specified once.";
            return result;
        }
        result.requested = true;
        if (++index == arguments.size() || arguments[index].empty() ||
            arguments[index].front() == L'-')
        {
            result.error = "--benchmark-showcase requires an output directory.";
            return result;
        }
        result.outputDirectory = arguments[index];
    }
    if (result.requested && captureOrCheckpoint)
    {
        result.error = "--benchmark-showcase cannot be combined with capture or checkpoint automation.";
    }
    if (workloadSpecified && !result.requested)
        result.error = "--benchmark-workload requires --benchmark-showcase.";
    return result;
}

} // namespace horde::platform::windows
