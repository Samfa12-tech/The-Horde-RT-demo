#pragma once

#include <string_view>

#if __has_include("reporting/BuildIdentityStamp.generated.h")
#include "reporting/BuildIdentityStamp.generated.h"
#else
#define HORDE_RT_PROVENANCE_COMMIT "unavailable"
#define HORDE_RT_PROVENANCE_INPUT_STATE "unavailable"
#define HORDE_RT_PROVENANCE_INPUT_SHA256 "unavailable"
#define HORDE_RT_PROVENANCE_CONFIGURATION "unavailable"
#define HORDE_RT_PROVENANCE_LABEL "g:unavailable;i:unavailable;c:U;s:U"
#endif

namespace horde::reporting
{
// Exact typed grammar for the build field's privacy-filter exception. This
// validates format, not authenticity. It accepts no free text or extra fields.
// c: Debug/Release/RelWithDebInfo/MinSizeRel/unknown; s: scoped clean/dirty/unknown.
[[nodiscard]] constexpr bool IsBuildIdentityReportLabel(const std::string_view label) noexcept
{
    if (label.size() > 118u || !label.starts_with("g:")) return false;
    const auto inputAt = label.find(";i:", 2u);
    const auto configAt = inputAt == label.npos ? label.npos : label.find(";c:", inputAt + 3u);
    const auto stateAt = configAt == label.npos ? label.npos : label.find(";s:", configAt + 3u);
    if (inputAt == label.npos || configAt == label.npos || stateAt == label.npos) return false;
    const auto isHex = [](const std::string_view value, const std::size_t length) constexpr
    {
        if (value.size() != length) return false;
        for (const char c : value)
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        return true;
    };
    const auto commit = label.substr(2u, inputAt - 2u);
    const auto digest = label.substr(inputAt + 3u, configAt - inputAt - 3u);
    const auto config = label.substr(configAt + 3u, stateAt - configAt - 3u);
    const auto state = label.substr(stateAt + 3u);
    if (!(commit == "unavailable" || isHex(commit, 40u)) ||
        !(digest == "unavailable" || isHex(digest, 64u)) ||
        !(config == "D" || config == "R" || config == "RD" || config == "M" || config == "U") ||
        !(state == "C" || state == "D" || state == "U")) return false;
    return state == "U" || (isHex(commit, 40u) && isHex(digest, 64u));
}

// Provenance of source and explicitly declared native build inputs. A clean
// inputState describes ONLY that scope relative to HEAD, never the whole
// checkout. nativeInputSha256 is NOT the executable/APK/artifact SHA-256.
// Missing Git, missing input files or an unattached target remain explicit.
struct BuildIdentity
{
    std::string_view sourceCommit;
    std::string_view inputState; // clean, dirty or unavailable
    std::string_view nativeInputSha256;
    std::string_view configuration;
    std::string_view reportLabel;

    [[nodiscard]] constexpr std::string_view ReportLabel() const noexcept { return reportLabel; }
};

// Internal linkage deliberately keeps a platform target's configured stamp
// local when libraries or host tests were built with a different/no stamp.
[[nodiscard]] static constexpr BuildIdentity CurrentBuildIdentity() noexcept
{
    return {HORDE_RT_PROVENANCE_COMMIT, HORDE_RT_PROVENANCE_INPUT_STATE,
        HORDE_RT_PROVENANCE_INPUT_SHA256, HORDE_RT_PROVENANCE_CONFIGURATION,
        HORDE_RT_PROVENANCE_LABEL};
}
} // namespace horde::reporting

#undef HORDE_RT_PROVENANCE_COMMIT
#undef HORDE_RT_PROVENANCE_INPUT_STATE
#undef HORDE_RT_PROVENANCE_INPUT_SHA256
#undef HORDE_RT_PROVENANCE_CONFIGURATION
#undef HORDE_RT_PROVENANCE_LABEL
