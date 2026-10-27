#pragma once

#include <cstddef>
#include <string_view>

namespace horde::reporting
{
// Shared syntax policy, preserved from IsBenchmarkSummaryUuid. The platform
// supplies CSPRNG entropy; syntax alone cannot prove random report identity.
[[nodiscard]] inline bool IsReportUuidV4(const std::string_view uuid) noexcept
{
    if (uuid.size() != 36u || uuid[14] != '4' ||
        (uuid[19] != '8' && uuid[19] != '9' && uuid[19] != 'a' && uuid[19] != 'b')) return false;
    for (std::size_t i = 0u; i < uuid.size(); ++i)
    {
        if (i == 8u || i == 13u || i == 18u || i == 23u) { if (uuid[i] != '-') return false; }
        else if (!((uuid[i] >= '0' && uuid[i] <= '9') || (uuid[i] >= 'a' && uuid[i] <= 'f'))) return false;
    }
    return true;
}
} // namespace horde::reporting
