#pragma once

#include <cstdint>
#include <limits>

namespace horde::reporting
{
// Admission only; the platform owns snapshots, revisions and surface currency.
// Call under the same lock as the compatibility snapshot publication.
class CompatibilityStartupOwnership
{
public:
    [[nodiscard]] std::uint64_t Begin() noexcept
    {
        if (exhausted_ || attempt_ == static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        { exhausted_ = true; return 0u; }
        ++attempt_;
        surfaceRequested_ = assetFailed_ = false;
        return attempt_;
    }
    [[nodiscard]] std::uint64_t Attempt() const noexcept { return exhausted_ ? 0u : attempt_; }
    [[nodiscard]] bool IsCurrent(const std::uint64_t attempt) const noexcept
    { return !exhausted_ && attempt == attempt_; }
    [[nodiscard]] bool FailAssets(const std::uint64_t attempt) noexcept
    {
        if (attempt == 0u || !IsCurrent(attempt) || surfaceRequested_ || assetFailed_) return false;
        assetFailed_ = true;
        return true;
    }
    [[nodiscard]] bool RequestSurface(const std::uint64_t attempt) noexcept
    {
        if (!IsCurrent(attempt) || assetFailed_) return false;
        surfaceRequested_ = true;
        return true;
    }
    [[nodiscard]] bool AcceptProbe(const std::uint64_t attempt, const bool surfaceProbe) const noexcept
    {
        // Untagged diagnostic getters cannot borrow a newer Java asset token.
        if (!surfaceProbe) return !exhausted_ && attempt_ == 0u;
        return IsCurrent(attempt) && !assetFailed_ && (attempt_ == 0u || surfaceRequested_);
    }
private:
    std::uint64_t attempt_ = 0u;
    bool surfaceRequested_ = false, assetFailed_ = false, exhausted_ = false;
};
} // namespace horde::reporting
