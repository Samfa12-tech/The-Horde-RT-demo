#include "reporting/CompatibilityStartupOwnership.h"

#include <iostream>

int main()
{
    using horde::reporting::CompatibilityStartupOwnership;
    bool passed = true;
    const auto check = [&](const bool value, const char* message) {
        if (!value) { std::cerr << message << '\n'; passed = false; }
    };
    CompatibilityStartupOwnership owner;
    check(owner.AcceptProbe(0u, false), "legacy pre-attempt diagnostic probe remains available");
    const auto first = owner.Begin();
    check(first != 0u && owner.IsCurrent(first), "asset preparation receives an owned monotonic token");
    check(!owner.AcceptProbe(0u, false) && !owner.AcceptProbe(first, true),
        "untagged getters and unrequested surface work cannot publish into an asset attempt");
    check(owner.FailAssets(first) && !owner.FailAssets(first) && !owner.RequestSurface(first),
        "observed asset failure is terminal for that attempt and cannot be replaced by a probe");
    const auto second = owner.Begin();
    check(second > first && !owner.FailAssets(first) && !owner.RequestSurface(first) &&
        !owner.AcceptProbe(first, true), "old queued completions and surface probes cannot bind a retry");
    check(owner.RequestSurface(second) && owner.AcceptProbe(second, true) && !owner.FailAssets(second),
        "a bound current surface owns later startup evidence, not an old Java asset callback");
    check(owner.RequestSurface(second), "ordinary surface recreation can retain the prepared asset attempt");
    const auto third = owner.Begin();
    check(third > second && !owner.AcceptProbe(second, true) && !owner.AcceptProbe(0u, false) &&
        owner.FailAssets(third), "new Java preparation revokes the old surface and diagnostic publication owners");
    return passed ? 0 : 1;
}
