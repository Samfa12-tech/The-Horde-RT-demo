#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace horde::reporting
{

inline constexpr std::uint32_t kPlaytestReportSchemaVersion = 1u;
inline constexpr std::size_t kPlaytestReportMaxNoteBytes = 4096u;
inline constexpr std::size_t kPlaytestReportMaxContextStringBytes = 512u;
inline constexpr std::size_t kPlaytestReportMaxJsonBytes = 16u * 1024u;

enum class PlaytestReportCategory : std::uint8_t
{
    Gameplay,
    Visuals,
    Performance,
    Controls,
    Audio,
    Other,
};

enum class PlaytestReportImpact : std::uint8_t
{
    BlocksProgress,
    MajorFriction,
    MinorFriction,
    Polish,
};

enum class PlaytestReportStatus : std::uint8_t
{
    Ready,
    ConsentRequired,
    InvalidReportId,
    InvalidTimestamp,
    InvalidCategory,
    InvalidImpact,
    EmptyNote,
    NoteTooLarge,
    InvalidUtf8,
    ControlCharacter,
    SensitiveContent,
    InvalidContext,
    JsonTooLarge,
    InvalidPreparedReport,
};

struct PlaytestReportContext
{
    std::string_view product;
    std::string_view version;
    std::string_view build;
    std::string_view platform;
    std::string_view rawModel;
    std::string_view gpu;
    std::string_view backend;
    std::string_view quality;
    double renderScale = 0.0;
    std::uint32_t internalWidth = 0u;
    std::uint32_t internalHeight = 0u;
    // Set true only after an RT-produced frame was successfully presented.
    bool rtPresented = false;
};

struct PlaytestReportInput
{
    // Caller supplies an opaque CSPRNG-generated URL-safe ID, not a device or
    // user identifier. This module validates syntax but cannot prove entropy.
    std::string_view reportId;
    std::string_view capturedAtUtc;
    PlaytestReportCategory category = PlaytestReportCategory::Other;
    PlaytestReportImpact impact = PlaytestReportImpact::Polish;
    std::string_view note;
    bool consentToSubmit = false;
    bool includeBasicContext = false;
    PlaytestReportContext context{};
};

struct PreparedPlaytestReport
{
    PlaytestReportStatus status = PlaytestReportStatus::InvalidPreparedReport;
    std::string reportId;
    std::string json;

    [[nodiscard]] bool IsReady() const noexcept
    {
        return status == PlaytestReportStatus::Ready;
    }
};

[[nodiscard]] PreparedPlaytestReport PreparePlaytestReport(const PlaytestReportInput& input);
[[nodiscard]] std::string_view PlaytestReportStatusName(PlaytestReportStatus status) noexcept;

enum class PlaytestReportDeliveryState : std::uint8_t
{
    Idle,
    InFlight,
    RetryableFailure,
    Accepted,
    Failed,
    Cancelled,
};

enum class PlaytestReportDeliveryResult : std::uint8_t
{
    Accepted,
    RetryableFailure,
    PermanentFailure,
};

struct PlaytestReportAttempt
{
    std::uint64_t token = 0u;
    std::string reportId;
    std::string json;
};

// UI/platform-owner seam for a synchronous transport callback. Retry explicitly
// reuses identical ID/JSON bytes; Cancel invalidates the token so a late result
// cannot complete a newer/cancelled submission. This class is not thread-safe,
// performs no transport, and never schedules background retries.
class PlaytestReportDelivery
{
public:
    [[nodiscard]] bool Begin(const PreparedPlaytestReport& report, PlaytestReportAttempt& attempt);
    [[nodiscard]] bool Retry(PlaytestReportAttempt& attempt);
    [[nodiscard]] bool Complete(std::uint64_t token, PlaytestReportDeliveryResult result) noexcept;
    void Cancel() noexcept;

    [[nodiscard]] PlaytestReportDeliveryState State() const noexcept { return state_; }

private:
    [[nodiscard]] bool MakeAttempt(PlaytestReportAttempt& attempt);

    PlaytestReportDeliveryState state_ = PlaytestReportDeliveryState::Idle;
    std::uint64_t token_ = 0u;
    std::string reportId_;
    std::string json_;
};

} // namespace horde::reporting
