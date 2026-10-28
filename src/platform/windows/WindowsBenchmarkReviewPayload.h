#pragma once

#include "reporting/BenchmarkSummaryReport.h"
#include "reporting/PublicBenchmarkReport.h"
#include <string_view>
#include <utility>

namespace horde::platform::windows
{
// Local review only. Selection never grants consent, transport or publication.
// The two immutable payload types cannot be admitted to the other's audience.
class WindowsBenchmarkReviewPayload
{
public:
    [[nodiscard]] bool IsPublicPreview() const noexcept { return publicPreview_; }
    [[nodiscard]] bool IsReady() const noexcept
    { return publicPreview_ ? public_.IsReady() : private_.IsReady(); }
    [[nodiscard]] std::string_view Json() const noexcept
    { return publicPreview_ ? public_.Json() : private_.Json(); }
    void SelectPublic(bool selected)
    {
        private_ = {}; public_ = {};
        publicPreview_ = selected;
    }
    [[nodiscard]] bool Admit(reporting::PreparedBenchmarkSummaryReport prepared)
    {
        if (publicPreview_ || IsReady() || !prepared.IsReady()) return false;
        private_ = std::move(prepared); return true;
    }
    [[nodiscard]] bool Admit(reporting::PreparedPublicBenchmarkReport prepared)
    {
        if (!publicPreview_ || IsReady() || !prepared.IsReady()) return false;
        public_ = std::move(prepared); return true;
    }
private:
    bool publicPreview_ = false;
    reporting::PreparedBenchmarkSummaryReport private_;
    reporting::PreparedPublicBenchmarkReport public_;
};
}
