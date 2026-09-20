#pragma once

#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{

struct WindowsBenchmarkLaunch
{
    bool requested = false;
    std::wstring outputDirectory;
    std::string error;
};

// Arguments exclude argv[0]. This only selects the existing player benchmark;
// it grants no checkpoint mutation, diagnostic instrumentation or quality override.
inline WindowsBenchmarkLaunch ParseWindowsBenchmarkLaunch(
    const std::span<const std::wstring_view> arguments)
{
    WindowsBenchmarkLaunch result;
    bool captureOrCheckpoint = false;
    for (std::size_t index = 0u; index < arguments.size(); ++index)
    {
        const auto argument = arguments[index];
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
    return result;
}

} // namespace horde::platform::windows
