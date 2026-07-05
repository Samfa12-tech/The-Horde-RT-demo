#pragma once

#include <string>

namespace horde::app
{

class HordeLanternApp
{
public:
    HordeLanternApp() = default;

    // Scaffold helper for tests or early platform wiring. This does not launch a
    // renderer. It returns the current diagnostic text from the capability layer.
    std::string BuildPhase0DiagnosticString() const;
};

} // namespace horde::app
