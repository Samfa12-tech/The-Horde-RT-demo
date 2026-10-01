#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "reporting/PlaytestSubmission.h"

#include <chrono>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
namespace detail
{
// WinHTTP can deliver HANDLE_CLOSING after the transport deadline and after
// cancellation. Keep its callback-owned memory bounded independently of the
// worker's deadline. This small lease/refcount seam is also exercised without
// network I/O by the unit tests.
class PlaytestCallbackContextLeasePool final
{
public:
    [[nodiscard]] bool TryAcquire() noexcept
    {
        unsigned current = live_.load(std::memory_order_relaxed);
        while (current < 2u)
        {
            if (live_.compare_exchange_weak(current, current + 1u,
                std::memory_order_acq_rel, std::memory_order_relaxed)) return true;
        }
        return false;
    }
    void Release() noexcept { live_.fetch_sub(1u, std::memory_order_acq_rel); }
    [[nodiscard]] unsigned LiveCount() const noexcept { return live_.load(std::memory_order_acquire); }
private:
    std::atomic<unsigned> live_{0u};
};

// Starts with the worker reference. A successful WinHTTP context association
// must first acquire a second reference, released only from HANDLE_CLOSING.
// Release methods return true only for the final reference.
class PlaytestCallbackContextLifetime final
{
public:
    explicit PlaytestCallbackContextLifetime(PlaytestCallbackContextLeasePool& pool) noexcept
        : pool_(pool), leased_(pool_.TryAcquire()) {}
    PlaytestCallbackContextLifetime(const PlaytestCallbackContextLifetime&) = delete;
    PlaytestCallbackContextLifetime& operator=(const PlaytestCallbackContextLifetime&) = delete;
    [[nodiscard]] bool IsLeased() const noexcept { return leased_; }
    [[nodiscard]] bool AcquireHandleReference() noexcept
    {
        bool expected = false;
        if (!leased_ || !handleReference_.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel, std::memory_order_relaxed)) return false;
        references_.fetch_add(1u, std::memory_order_relaxed);
        return true;
    }
    [[nodiscard]] bool ReleaseWorkerReference() noexcept
    {
        if (!workerReference_.exchange(false, std::memory_order_acq_rel)) return false;
        return Release();
    }
    [[nodiscard]] bool ReleaseHandleReference() noexcept
    {
        if (!handleReference_.exchange(false, std::memory_order_acq_rel)) return false;
        return Release();
    }
private:
    [[nodiscard]] bool Release() noexcept
    {
        if (references_.fetch_sub(1u, std::memory_order_acq_rel) != 1u) return false;
        if (leased_) pool_.Release();
        return true;
    }
    PlaytestCallbackContextLeasePool& pool_;
    bool leased_ = false;
    std::atomic<unsigned> references_{1u};
    std::atomic_bool workerReference_{true};
    std::atomic_bool handleReference_{false};
};
} // namespace detail

enum class WindowsPlaytestSubmissionResult : unsigned char
{
    None, Queued, Sent, VerificationExpired, RateLimited, Conflict, Rejected, Uncertain,
};

struct WindowsPlaytestHttpResponse
{
    int statusCode = 0;
    std::string body;
};

// Injectable transport seam. The deadline starts before bounded worker
// admission. Implementations observe the cancellation event and deadline;
// provider error text is never returned to the caller.
using WindowsPlaytestExchange = std::function<WindowsPlaytestHttpResponse(
    std::string_view requestBody, std::chrono::steady_clock::time_point deadline,
    HANDLE cancellationEvent)>;

[[nodiscard]] WindowsPlaytestSubmissionResult ClassifyWindowsPlaytestResponse(
    int statusCode, std::string_view body, std::string_view expectedReportId) noexcept;
[[nodiscard]] WindowsPlaytestHttpResponse ExchangeWindowsPlaytestReport(
    std::string_view requestBody, std::chrono::steady_clock::time_point deadline,
    HANDLE cancellationEvent);

inline constexpr UINT kWindowsPlaytestSubmissionCompletedMessage = WM_APP + 59u;

// One process-wide worker and at most one queued attempt back these foreground
// owners. Cancellation signals an attempt-owned event; the worker closes an
// async WinHTTP request after its current API call returns. Frozen report bytes
// and ID remain owned for explicit fresh-token retries. Destruction never
// joins on UI. The 30-second deadline bounds the attempt, not OS callback
// retirement; at most two callback contexts can remain leased while waiting
// for WinHTTP HANDLE_CLOSING, and later attempts fail closed if both are held.
class WindowsPlaytestSubmission final
{
public:
    struct State;
    explicit WindowsPlaytestSubmission(WindowsPlaytestExchange exchange = {},
        std::chrono::milliseconds attemptTimeout = std::chrono::seconds(30));
    ~WindowsPlaytestSubmission();
    WindowsPlaytestSubmission(const WindowsPlaytestSubmission&) = delete;
    WindowsPlaytestSubmission& operator=(const WindowsPlaytestSubmission&) = delete;

    [[nodiscard]] bool Begin(HWND window,
        const reporting::PreparedPlaytestSubmission& submission, std::string_view turnstileToken);
    [[nodiscard]] bool Retry(HWND window, std::string_view turnstileToken);
    void CancelAttempt() noexcept;
    void Reset() noexcept;
    [[nodiscard]] bool IsBusy() const noexcept;
    [[nodiscard]] bool CanRetry() const noexcept;
    [[nodiscard]] std::string ReportId() const;
    [[nodiscard]] WindowsPlaytestSubmissionResult LastResult() const noexcept;

    // Consume this message family (including stale tokens); false means an
    // unrelated message. A stale token is consumed without changing result.
    [[nodiscard]] bool HandleMessage(UINT message, WPARAM token,
        WindowsPlaytestSubmissionResult& result) noexcept;

private:
    std::shared_ptr<State> state_;
};
} // namespace horde::platform::windows
