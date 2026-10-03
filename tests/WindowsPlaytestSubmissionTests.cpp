#include "platform/windows/WindowsPlaytestSubmission.h"

#include <chrono>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <winhttp.h>

namespace
{
using namespace horde::platform::windows;
using namespace horde::reporting;

bool passed = true;
void Check(const bool value, const char* message)
{
    if (!value) { passed = false; std::cerr << "Windows playtest submission: " << message << '\n'; }
}

PreparedPlaytestSubmission Prepared()
{
    PlaytestReportInput input;
    input.reportId = "random_report_8f21";
    input.capturedAtUtc = "2026-10-02T00:01:02.003Z";
    input.category = PlaytestReportCategory::Visuals;
    input.impact = PlaytestReportImpact::MinorFriction;
    input.note = "A synthetic offline transport fixture.";
    input.consentToSubmit = true;
    return PreparePlaytestSubmission(input);
}

bool WaitIdle(const WindowsPlaytestSubmission& owner)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (owner.IsBusy() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    return !owner.IsBusy();
}

std::string FrozenPayloadFromRequest(const std::string& request)
{
    constexpr std::string_view tokenSuffix = ",\"turnstileToken\":\"";
    const auto tokenAt = request.rfind(tokenSuffix);
    if (tokenAt == std::string::npos || request.empty() || request.back() != '}') return {};
    const auto tokenEnd = request.find('"', tokenAt + tokenSuffix.size());
    if (tokenEnd == std::string::npos || tokenEnd + 1u != request.size() - 1u) return {};
    return request.substr(0u, tokenAt) + "}";
}

struct WinHttpContextFixture final
{
    HANDLE operation = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::atomic_bool callbackContextMatched{false};
};

WinHttpContextFixture& LocalWinHttpContextFixture()
{
    static WinHttpContextFixture fixture;
    return fixture;
}

void CALLBACK VerifyWinHttpCallbackContext(HINTERNET, const DWORD_PTR callbackContext,
    const DWORD status, void*, const DWORD)
{
    auto* fixture = &LocalWinHttpContextFixture();
    if (callbackContext != reinterpret_cast<DWORD_PTR>(fixture)) return;
    if (status == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE ||
        status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
    {
        fixture->callbackContextMatched.store(true, std::memory_order_release);
        SetEvent(fixture->operation);
    }
}

bool WinHttpSendContextCallbackFixture()
{
    // Keep callback context alive for process lifetime in case Windows retires
    // the async handle after this bounded test returns.
    auto& fixture = LocalWinHttpContextFixture();
    if (fixture.operation == nullptr) return false;
    HINTERNET session = WinHttpOpen(L"Horde-playtest-local-context-test/1",
        WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC);
    if (session == nullptr) return false;
    const auto previous = WinHttpSetStatusCallback(session, VerifyWinHttpCallbackContext,
        WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_REQUEST_ERROR |
            WINHTTP_CALLBACK_FLAG_HANDLES, 0u);
    if (previous == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        WinHttpCloseHandle(session);
        return false;
    }
    HINTERNET connection = WinHttpConnect(session, L"127.0.0.1", 1u, 0u);
    HINTERNET request = connection == nullptr ? nullptr : WinHttpOpenRequest(connection,
        L"POST", L"/context-test", nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0u);
    bool observed = false;
    if (request != nullptr)
    {
        // WinHTTP may still own optional request bytes after a bounded timeout.
        static constexpr char body[] = "local-only";
        const auto contextValue = reinterpret_cast<DWORD_PTR>(&fixture);
        const BOOL sent = WinHttpSendRequest(request, L"Content-Length: 10\r\n",
            static_cast<DWORD>(std::size(L"Content-Length: 10\r\n") - 1u),
            const_cast<char*>(body), sizeof(body) - 1u, sizeof(body) - 1u, contextValue);
        const auto error = sent ? ERROR_SUCCESS : GetLastError();
        if (sent || error == ERROR_IO_PENDING)
            observed = WaitForSingleObject(fixture.operation, 3000u) == WAIT_OBJECT_0 &&
                fixture.callbackContextMatched.load(std::memory_order_acquire);
        WinHttpCloseHandle(request);
    }
    if (connection != nullptr) WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return observed;
}
}

int main()
{
    {
        detail::PlaytestCallbackContextLeasePool pool;
        detail::PlaytestCallbackContextLifetime immediate(pool);
        Check(immediate.IsLeased() && pool.LiveCount() == 1u && immediate.AcquireHandleReference(),
            "callback context obtains one bounded lease and handle reference before association");
        Check(!immediate.ReleaseHandleReference() && immediate.ReleaseWorkerReference() &&
            pool.LiveCount() == 0u,
            "HANDLE_CLOSING before the initiating call returns leaves the worker reference safe");

        detail::PlaytestCallbackContextLifetime delayed(pool);
        Check(delayed.IsLeased() && delayed.AcquireHandleReference() &&
            !delayed.ReleaseWorkerReference() && pool.LiveCount() == 1u,
            "cancelled/deadline worker can retire without waiting for OS callback retirement");
        detail::PlaytestCallbackContextLifetime second(pool);
        Check(second.IsLeased() && second.AcquireHandleReference(),
            "one active and one retiring callback context fit the two-slot bound");
        detail::PlaytestCallbackContextLifetime saturated(pool);
        Check(!saturated.IsLeased() && saturated.ReleaseWorkerReference() && pool.LiveCount() == 2u,
            "third callback context fails closed while both bounded leases remain live");
        Check(delayed.ReleaseHandleReference() && pool.LiveCount() == 1u &&
            !second.ReleaseWorkerReference() && second.ReleaseHandleReference() && pool.LiveCount() == 0u,
            "late final callbacks release only their own leases");

        detail::PlaytestCallbackContextLifetime failedAssociation(pool);
        Check(failedAssociation.IsLeased() && failedAssociation.ReleaseWorkerReference() &&
            pool.LiveCount() == 0u,
            "failed context association releases only the worker reference and lease");
    }
    Check(WinHttpSendContextCallbackFixture(),
        "async WinHTTP loopback send reports the explicit non-null request context to its completion callback");

    const std::string id = "random_report_8f21";
    Check(ClassifyWindowsPlaytestResponse(202,
        "{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\"}", id) ==
        WindowsPlaytestSubmissionResult::Queued, "actual strict matching 202 acceptance acknowledgement");
    Check(ClassifyWindowsPlaytestResponse(200,
        "{\"status\":\"sent\",\"ok\":true,\"id\":\"random_report_8f21\"}", id) ==
        WindowsPlaytestSubmissionResult::Sent, "strict matching 200 sent acknowledgement");
    Check(ClassifyWindowsPlaytestResponse(403, {}, id) == WindowsPlaytestSubmissionResult::VerificationExpired,
        "403 requires a fresh explicit verification attempt");
    Check(ClassifyWindowsPlaytestResponse(429, {}, id) == WindowsPlaytestSubmissionResult::RateLimited,
        "429 is retryable after explicit user action");
    Check(ClassifyWindowsPlaytestResponse(409, {}, id) == WindowsPlaytestSubmissionResult::Conflict,
        "409 surfaces same-ID frozen-content conflict");
    for (const auto& body : {
        std::string("{\"ok\":true,\"id\":\"other-id-123\",\"status\":\"accepted\"}"),
        std::string("{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"sent\"}"),
        std::string("{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\",\"extra\":1}"),
        std::string("{\"ok\":true,\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\"}"),
        std::string("{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\",}"),
        std::string("{\"ok\":true,\"reportId\":\"random_report_8f21\",\"status\":\"accepted\"}"),
        std::string("not-json")})
        Check(ClassifyWindowsPlaytestResponse(202, body, id) == WindowsPlaytestSubmissionResult::Uncertain,
            "malformed, extra-field, duplicate-field, wrong-ID or wrong-status ack stays uncertain");
    Check(ClassifyWindowsPlaytestResponse(202, std::string(8193u, 'x'), id) ==
        WindowsPlaytestSubmissionResult::Uncertain, "oversize response stays uncertain");
    Check(ClassifyWindowsPlaytestResponse(202,
        "{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\"}", "éééééééé") ==
        WindowsPlaytestSubmissionResult::Uncertain, "non-ASCII expected report ID is rejected");

    std::mutex mutex;
    std::condition_variable wake;
    std::vector<std::string> requests;
    unsigned calls = 0u;
    WindowsPlaytestExchange exchange = [&](const std::string_view request,
        const std::chrono::steady_clock::time_point, const HANDLE) {
        std::lock_guard lock(mutex);
        requests.emplace_back(request);
        ++calls;
        wake.notify_all();
        return calls == 1u ? WindowsPlaytestHttpResponse{202,
            "{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\"}"} :
            WindowsPlaytestHttpResponse{403, "{}"};
    };
    WindowsPlaytestSubmission owner(exchange);
    auto frozen = Prepared();
    const std::string originalJson = frozen.json;
    Check(frozen.IsReady() && owner.Begin(nullptr, frozen, "fresh-token-one"),
        "synthetic foreground attempt accepted without network");
    const std::string reportId = owner.ReportId();
    Check(WaitIdle(owner) && owner.LastResult() == WindowsPlaytestSubmissionResult::Queued,
        "exchange completes asynchronously and classifies queued");
    Check(reportId == id && owner.ReportId() == id && !owner.CanRetry(),
        "accepted result retains identity and cannot retry");
    WindowsPlaytestSubmissionResult handled = WindowsPlaytestSubmissionResult::None;
    Check(!owner.HandleMessage(kWindowsPlaytestSubmissionCompletedMessage + 1u, 1u, handled) &&
        owner.HandleMessage(kWindowsPlaytestSubmissionCompletedMessage, 1u, handled) &&
        handled == WindowsPlaytestSubmissionResult::None,
        "unrelated messages are rejected and stale completion messages are consumed harmlessly");
    owner.Reset();

    bool started = false;
    unsigned slowAttempt = 0u;
    WindowsPlaytestExchange slow = [&](const std::string_view request,
        const std::chrono::steady_clock::time_point, const HANDLE cancellationEvent) {
        std::unique_lock lock(mutex);
        requests.emplace_back(request);
        started = true;
        ++slowAttempt;
        wake.notify_all();
        if (slowAttempt == 1u)
        {
            lock.unlock();
            WaitForSingleObject(cancellationEvent, 2000u);
            lock.lock();
            return WindowsPlaytestHttpResponse{202,
                "{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\"}"};
        }
        return WindowsPlaytestHttpResponse{403, "{}"};
    };
    WindowsPlaytestSubmission cancelled(slow);
    Check(cancelled.Begin(nullptr, frozen, "fresh-token-two"), "cancellable attempt starts");
    {
        std::unique_lock lock(mutex);
        wake.wait_for(lock, std::chrono::seconds(1), [&] { return started; });
    }
    cancelled.CancelAttempt();
    Check(cancelled.ReportId() == id && cancelled.IsBusy(),
        "cancel is nonblocking and keeps frozen report ID until exchange drains");
    Check(WaitIdle(cancelled) && cancelled.CanRetry() &&
        cancelled.LastResult() == WindowsPlaytestSubmissionResult::Uncertain,
        "cancelled late acceptance is suppressed and remains retryable with same identity");
    Check(cancelled.Retry(nullptr, "fresh-token-three"), "retry obtains a fresh verification token");
    Check(WaitIdle(cancelled) && cancelled.LastResult() == WindowsPlaytestSubmissionResult::VerificationExpired &&
        cancelled.CanRetry(), "403 retry result is reported without changing frozen payload");
    Check(cancelled.ReportId() == id, "same report ID remains owner identity through retries");
    {
        std::lock_guard lock(mutex);
        Check(requests.size() >= 3u && FrozenPayloadFromRequest(requests[0]) == originalJson &&
            FrozenPayloadFromRequest(requests[1]) == originalJson &&
            FrozenPayloadFromRequest(requests[2]) == originalJson &&
            requests[0].find("fresh-token-one") != std::string::npos &&
            requests[1].find("fresh-token-two") != std::string::npos &&
            requests[2].find("fresh-token-three") != std::string::npos,
            "each explicit attempt carries identical frozen envelope and a newly supplied token");
    }

    HANDLE activeStarted = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE releaseActive = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    Check(activeStarted != nullptr && releaseActive != nullptr, "queue lifecycle fixture events allocated");
    std::atomic<unsigned> queuedExchangeCalls{0u};
    WindowsPlaytestExchange activeGate = [&](const std::string_view,
        const std::chrono::steady_clock::time_point, const HANDLE cancellationEvent) {
        SetEvent(activeStarted);
        const HANDLE waits[]{cancellationEvent, releaseActive};
        (void)WaitForMultipleObjects(2u, waits, FALSE, 2000u);
        return WindowsPlaytestHttpResponse{200,
            "{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"sent\"}"};
    };
    WindowsPlaytestExchange shouldNotDispatch = [&](const std::string_view,
        const std::chrono::steady_clock::time_point, const HANDLE) {
        queuedExchangeCalls.fetch_add(1u);
        return WindowsPlaytestHttpResponse{200,
            "{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"sent\"}"};
    };
    WindowsPlaytestSubmission activeOwner(activeGate, std::chrono::seconds(2));
    WindowsPlaytestSubmission deadlineQueued(shouldNotDispatch, std::chrono::milliseconds(35));
    Check(activeOwner.Begin(nullptr, frozen, "queue-active-token"), "active queue fixture starts");
    Check(WaitForSingleObject(activeStarted, 1000u) == WAIT_OBJECT_0, "active worker entered exchange");
    Check(deadlineQueued.Begin(nullptr, frozen, "queue-deadline-token"), "one pending job admitted");
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    SetEvent(releaseActive);
    Check(WaitIdle(activeOwner) && activeOwner.LastResult() == WindowsPlaytestSubmissionResult::Sent,
        "active job drains and preserves terminal success");
    Check(WaitIdle(deadlineQueued) && deadlineQueued.LastResult() == WindowsPlaytestSubmissionResult::Uncertain &&
        deadlineQueued.CanRetry() && queuedExchangeCalls.load() == 0u,
        "queued attempt deadline starts before admission and expired work is not dispatched");

    ResetEvent(activeStarted);
    ResetEvent(releaseActive);
    WindowsPlaytestSubmission activeOwner2(activeGate, std::chrono::seconds(2));
    WindowsPlaytestSubmission cancelledQueued(shouldNotDispatch, std::chrono::seconds(2));
    Check(activeOwner2.Begin(nullptr, frozen, "queue-active-token-2"), "second active queue fixture starts");
    Check(WaitForSingleObject(activeStarted, 1000u) == WAIT_OBJECT_0, "second active worker entered exchange");
    Check(cancelledQueued.Begin(nullptr, frozen, "queue-cancel-token"), "second pending job admitted");
    cancelledQueued.CancelAttempt();
    SetEvent(releaseActive);
    Check(WaitIdle(activeOwner2) && WaitIdle(cancelledQueued) &&
        cancelledQueued.CanRetry() && queuedExchangeCalls.load() == 0u,
        "queued cancellation is observed before opening or sending a request");
    if (activeStarted != nullptr) CloseHandle(activeStarted);
    if (releaseActive != nullptr) CloseHandle(releaseActive);

    bool resetStarted = false;
    bool resetFinished = false;
    WindowsPlaytestExchange resetExchange = [&](const std::string_view,
        const std::chrono::steady_clock::time_point, const HANDLE cancellationEvent) {
        {
            std::lock_guard lock(mutex);
            resetStarted = true;
            wake.notify_all();
        }
        (void)WaitForSingleObject(cancellationEvent, 1000u);
        {
            std::lock_guard lock(mutex);
            resetFinished = true;
            wake.notify_all();
        }
        return WindowsPlaytestHttpResponse{202,
            "{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\"}"};
    };
    WindowsPlaytestSubmission resetOwner(resetExchange);
    Check(resetOwner.Begin(nullptr, frozen, "reset-token"), "reset stale-result attempt starts");
    {
        std::unique_lock lock(mutex);
        wake.wait_for(lock, std::chrono::seconds(1), [&] { return resetStarted; });
    }
    resetOwner.Reset();
    Check(WaitIdle(resetOwner) && resetOwner.LastResult() == WindowsPlaytestSubmissionResult::None &&
        resetOwner.ReportId().empty() && !resetOwner.CanRetry(),
        "late completion cannot mutate an owner after reset");

    std::atomic_bool destroyExchangeFinished{false};
    HANDLE destroyStarted = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    WindowsPlaytestExchange destroyExchange = [&](const std::string_view,
        const std::chrono::steady_clock::time_point, const HANDLE cancellationEvent) {
        SetEvent(destroyStarted);
        (void)WaitForSingleObject(cancellationEvent, 1000u);
        destroyExchangeFinished.store(true);
        return WindowsPlaytestHttpResponse{202,
            "{\"ok\":true,\"id\":\"random_report_8f21\",\"status\":\"accepted\"}"};
    };
    auto destroyedOwner = std::make_unique<WindowsPlaytestSubmission>(destroyExchange);
    Check(destroyedOwner->Begin(nullptr, frozen, "destroy-token"), "destruction stale-result attempt starts");
    Check(WaitForSingleObject(destroyStarted, 1000u) == WAIT_OBJECT_0, "destruction exchange entered");
    destroyedOwner.reset();
    const auto destroyDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!destroyExchangeFinished.load() && std::chrono::steady_clock::now() < destroyDeadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    Check(destroyExchangeFinished.load(), "destruction signals cancellation without UI-thread join or stale owner use");
    if (destroyStarted != nullptr) CloseHandle(destroyStarted);
    return passed ? 0 : 1;
}
