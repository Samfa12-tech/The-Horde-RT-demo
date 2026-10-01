#include "platform/windows/WindowsPlaytestSubmission.h"

#include <algorithm>
#include <atomic>
#include <array>
#include <condition_variable>
#include <cstdint>
#include <iterator>
#include <mutex>
#include <new>
#include <thread>
#include <utility>

#include <winhttp.h>

namespace horde::platform::windows
{
namespace
{
constexpr std::size_t kMaxRequestBytes = reporting::kPlaytestSubmissionMaxBytes;
constexpr std::size_t kMaxResponseBytes = 8u * 1024u;
constexpr auto kTotalTimeout = std::chrono::seconds(30);
constexpr DWORD kIoTimeoutMs = 10000u;
detail::PlaytestCallbackContextLeasePool gCallbackContextLeases;

void Wipe(std::string& value) noexcept
{
    if (!value.empty()) SecureZeroMemory(value.data(), value.size());
    value.clear();
}

class WinHttpHandle final
{
public:
    WinHttpHandle() = default;
    explicit WinHttpHandle(HINTERNET value) : value_(value) {}
    ~WinHttpHandle() { if (value_ != nullptr) WinHttpCloseHandle(value_); }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    [[nodiscard]] HINTERNET Get() const noexcept { return value_; }
    [[nodiscard]] explicit operator bool() const noexcept { return value_ != nullptr; }
private:
    HINTERNET value_ = nullptr;
};

struct CancellationEvent final
{
    CancellationEvent() : handle(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {}
    ~CancellationEvent() { if (handle != nullptr) CloseHandle(handle); }
    CancellationEvent(const CancellationEvent&) = delete;
    CancellationEvent& operator=(const CancellationEvent&) = delete;
    HANDLE handle = nullptr;
};

struct AsyncRequestContext final
{
    explicit AsyncRequestContext(detail::PlaytestCallbackContextLeasePool& pool)
        : lifetime(pool), operationComplete(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {}
    ~AsyncRequestContext()
    {
        if (operationComplete != nullptr) CloseHandle(operationComplete);
        Wipe(requestBody);
        SecureZeroMemory(response.data(), response.size());
        SecureZeroMemory(readBuffer.data(), readBuffer.size());
    }
    detail::PlaytestCallbackContextLifetime lifetime;
    HANDLE operationComplete = nullptr;
    std::mutex mutex;
    DWORD completion = 0u;
    DWORD error = ERROR_SUCCESS;
    DWORD available = 0u;
    std::array<char, kMaxResponseBytes + 1u> response{};
    std::size_t responseSize = 0u;
    std::array<char, kMaxResponseBytes + 1u> readBuffer{};
    std::string requestBody;
};

void ReleaseContextReference(AsyncRequestContext* context, const bool final) noexcept
{
    if (final) delete context;
}

struct ContextWorkerReference final
{
    explicit ContextWorkerReference(AsyncRequestContext* value) noexcept : context(value) {}
    ~ContextWorkerReference()
    {
        if (context != nullptr)
            ReleaseContextReference(context, context->lifetime.ReleaseWorkerReference());
    }
    ContextWorkerReference(const ContextWorkerReference&) = delete;
    ContextWorkerReference& operator=(const ContextWorkerReference&) = delete;
    AsyncRequestContext* context = nullptr;
};

bool SetRemainingTimeouts(const HINTERNET request,
    std::chrono::steady_clock::time_point deadline) noexcept;

void CALLBACK WinHttpStatusCallback(HINTERNET, const DWORD_PTR rawContext, const DWORD status,
    void* information, const DWORD informationLength)
{
    auto* context = reinterpret_cast<AsyncRequestContext*>(rawContext);
    if (context == nullptr) return;
    if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)
    {
        ReleaseContextReference(context, context->lifetime.ReleaseHandleReference());
        return;
    }
    if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
    {
        std::lock_guard lock(context->mutex);
        const auto* result = static_cast<const WINHTTP_ASYNC_RESULT*>(information);
        context->error = result == nullptr || informationLength < sizeof(WINHTTP_ASYNC_RESULT) ?
            ERROR_WINHTTP_INTERNAL_ERROR : result->dwError;
        context->completion = status;
        SetEvent(context->operationComplete);
        return;
    }
    if (status != WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE &&
        status != WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE &&
        status != WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE &&
        status != WINHTTP_CALLBACK_STATUS_READ_COMPLETE) return;

    std::lock_guard lock(context->mutex);
    context->error = ERROR_SUCCESS;
    context->completion = status;
    if (status == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE)
    {
        if (information == nullptr || informationLength < sizeof(DWORD))
            context->error = ERROR_WINHTTP_INTERNAL_ERROR;
        else context->available = *static_cast<const DWORD*>(information);
    }
    else if (status == WINHTTP_CALLBACK_STATUS_READ_COMPLETE && informationLength != 0u)
    {
        if (information == nullptr || informationLength > context->readBuffer.size() ||
            informationLength > context->response.size() - context->responseSize)
            context->error = ERROR_INSUFFICIENT_BUFFER;
        else
        {
            std::copy_n(static_cast<const char*>(information), informationLength,
                context->response.data() + context->responseSize);
            context->responseSize += informationLength;
        }
    }
    SetEvent(context->operationComplete);
}

bool IsCancelledOrExpired(HANDLE cancellationEvent,
    const std::chrono::steady_clock::time_point deadline) noexcept
{
    return cancellationEvent == nullptr || WaitForSingleObject(cancellationEvent, 0u) == WAIT_OBJECT_0 ||
        std::chrono::steady_clock::now() >= deadline;
}

bool WaitForCompletion(AsyncRequestContext& context, const HANDLE cancellationEvent,
    const std::chrono::steady_clock::time_point deadline, const DWORD expectedStatus) noexcept
{
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now()).count();
    if (remaining <= 0) return false;
    const HANDLE handles[]{cancellationEvent, context.operationComplete};
    const DWORD wait = WaitForMultipleObjects(2u, handles, FALSE,
        static_cast<DWORD>(std::min<std::int64_t>(remaining, INFINITE - 1u)));
    if (wait != WAIT_OBJECT_0 + 1u) return false;
    std::lock_guard lock(context.mutex);
    return context.error == ERROR_SUCCESS && context.completion == expectedStatus;
}

bool PrepareAsyncOperation(AsyncRequestContext& context, const HINTERNET request,
    const std::chrono::steady_clock::time_point deadline) noexcept
{
    if (!SetRemainingTimeouts(request, deadline)) return false;
    ResetEvent(context.operationComplete);
    std::lock_guard lock(context.mutex);
    context.error = ERROR_SUCCESS;
    context.completion = 0u;
    context.available = 0u;
    return true;
}

bool StartAsyncOperation(const BOOL apiResult) noexcept
{
    return apiResult != FALSE || GetLastError() == ERROR_IO_PENDING;
}

class AsyncRequestHandle final
{
public:
    explicit AsyncRequestHandle(HINTERNET handle) : handle_(handle) {}
    ~AsyncRequestHandle() { Close(); }
    AsyncRequestHandle(const AsyncRequestHandle&) = delete;
    AsyncRequestHandle& operator=(const AsyncRequestHandle&) = delete;
    [[nodiscard]] HINTERNET Get() const noexcept { return handle_; }
    void Close() noexcept
    {
        if (handle_ == nullptr) return;
        // A close failure does not justify freeing callback-owned memory. If
        // HANDLE_CLOSING never arrives, the two-slot lease pool fails closed.
        (void)WinHttpCloseHandle(handle_);
        handle_ = nullptr;
    }
private:
    HINTERNET handle_ = nullptr;
};

bool SetRemainingTimeouts(const HINTERNET request,
    const std::chrono::steady_clock::time_point deadline) noexcept
{
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now()).count();
    if (remaining <= 0) return false;
    const auto timeout = static_cast<int>(std::min<std::int64_t>(remaining, kIoTimeoutMs));
    return WinHttpSetTimeouts(request, timeout, timeout, timeout, timeout) != FALSE;
}

bool HasExactAck(std::string_view json, const std::string_view expectedId,
    const std::string_view expectedStatus) noexcept
{
    // The acknowledgement grammar is intentionally narrower than general JSON:
    // exactly one object, exactly three unique fields, no escapes or nested data.
    if (expectedId.size() < 8u || expectedId.size() > 96u ||
        !std::all_of(expectedId.begin(), expectedId.end(), [](const unsigned char ch) {
            return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        })) return false;
    std::size_t at = 0u;
    const auto space = [&]() { while (at < json.size() && (json[at] == ' ' || json[at] == '\t' ||
        json[at] == '\r' || json[at] == '\n')) ++at; };
    const auto quoted = [&](std::string_view& value) {
        if (at >= json.size() || json[at++] != '"') return false;
        const auto begin = at;
        while (at < json.size() && json[at] != '"')
        {
            const auto ch = static_cast<unsigned char>(json[at]);
            if (ch < 0x20u || ch == '\\') return false;
            ++at;
        }
        if (at >= json.size()) return false;
        value = json.substr(begin, at - begin);
        ++at;
        return true;
    };
    space();
    if (at >= json.size() || json[at++] != '{') return false;
    bool haveOk = false, haveId = false, haveStatus = false;
    bool ok = false;
    std::string_view id, status;
    space();
    while (at < json.size() && json[at] != '}')
    {
        std::string_view key;
        if (!quoted(key)) return false;
        space();
        if (at >= json.size() || json[at++] != ':') return false;
        space();
        if (key == "ok")
        {
            if (haveOk) return false;
            haveOk = true;
            if (json.substr(at, 4u) == "true") { ok = true; at += 4u; }
            else if (json.substr(at, 5u) == "false") { at += 5u; }
            else return false;
        }
        else if (key == "reportId")
        {
            return false;
        }
        else if (key == "id")
        {
            if (haveId || !quoted(id)) return false;
            haveId = true;
        }
        else if (key == "status")
        {
            if (haveStatus || !quoted(status)) return false;
            haveStatus = true;
        }
        else return false;
        space();
        if (at < json.size() && json[at] == ',')
        {
            ++at;
            space();
            if (at >= json.size() || json[at] == '}') return false;
            continue;
        }
        break;
    }
    if (at >= json.size() || json[at++] != '}') return false;
    space();
    return at == json.size() && haveOk && haveId && haveStatus && ok &&
        id == expectedId && status == expectedStatus;
}

class SingleWorker final
{
public:
    SingleWorker() : thread_([this] { Run(); }) {}
    ~SingleWorker()
    {
        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
        }
        wake_.notify_one();
        if (thread_.joinable()) thread_.join();
    }
    bool Submit(std::function<void()> work)
    {
        std::lock_guard lock(mutex_);
        if (stopping_ || pending_) return false;
        pending_ = std::move(work);
        wake_.notify_one();
        return true;
    }
private:
    void Run()
    {
        for (;;)
        {
            std::function<void()> work;
            {
                std::unique_lock lock(mutex_);
                wake_.wait(lock, [this] { return stopping_ || pending_; });
                if (stopping_ && !pending_) return;
                work = std::move(pending_);
                active_ = true;
            }
            try { work(); } catch (...) {}
            {
                std::lock_guard lock(mutex_);
                active_ = false;
            }
        }
    }
    std::mutex mutex_;
    std::condition_variable wake_;
    std::function<void()> pending_;
    std::thread thread_;
    bool active_ = false;
    bool stopping_ = false;
};

SingleWorker& Worker()
{
    static SingleWorker worker;
    return worker;
}
} // namespace

WindowsPlaytestSubmissionResult ClassifyWindowsPlaytestResponse(const int statusCode,
    const std::string_view body, const std::string_view expectedReportId) noexcept
{
    if (body.size() > kMaxResponseBytes) return WindowsPlaytestSubmissionResult::Uncertain;
    if (statusCode == 202 && HasExactAck(body, expectedReportId, "accepted"))
        return WindowsPlaytestSubmissionResult::Queued;
    if (statusCode == 200 && HasExactAck(body, expectedReportId, "sent"))
        return WindowsPlaytestSubmissionResult::Sent;
    if (statusCode == 403) return WindowsPlaytestSubmissionResult::VerificationExpired;
    if (statusCode == 429) return WindowsPlaytestSubmissionResult::RateLimited;
    if (statusCode == 409) return WindowsPlaytestSubmissionResult::Conflict;
    if (statusCode >= 400 && statusCode < 500)
        return WindowsPlaytestSubmissionResult::Rejected;
    return WindowsPlaytestSubmissionResult::Uncertain;
}

WindowsPlaytestHttpResponse ExchangeWindowsPlaytestReport(const std::string_view requestBody,
    const std::chrono::steady_clock::time_point deadline, const HANDLE cancellationEvent)
{
    WindowsPlaytestHttpResponse response;
    if (requestBody.empty() || requestBody.size() > kMaxRequestBytes ||
        IsCancelledOrExpired(cancellationEvent, deadline)) return response;

    WinHttpHandle session(WinHttpOpen(L"HordeLanternRT-Playtest/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC));
    if (!session) return response;
    const auto oldCallback = WinHttpSetStatusCallback(session.Get(), WinHttpStatusCallback,
        WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_REQUEST_ERROR |
            WINHTTP_CALLBACK_FLAG_HANDLES, 0u);
    if (oldCallback == WINHTTP_INVALID_STATUS_CALLBACK) return response;
    if (IsCancelledOrExpired(cancellationEvent, deadline)) return response;
    WinHttpHandle connection(WinHttpConnect(session.Get(), L"briarhold-signal.samfa12.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0u));
    if (!connection) return response;
    if (IsCancelledOrExpired(cancellationEvent, deadline)) return response;
    auto* context = new (std::nothrow) AsyncRequestContext(gCallbackContextLeases);
    if (context == nullptr) return response;
    ContextWorkerReference workerReference(context);
    if (!context->lifetime.IsLeased() || context->operationComplete == nullptr) return response;
    try { context->requestBody.assign(requestBody); }
    catch (...) { return response; }
    const HINTERNET rawRequest = WinHttpOpenRequest(connection.Get(), L"POST", L"/api/horde-reports",
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (rawRequest == nullptr) return response;
    if (!context->lifetime.AcquireHandleReference())
    {
        (void)WinHttpCloseHandle(rawRequest);
        return response;
    }
    DWORD_PTR callbackContext = reinterpret_cast<DWORD_PTR>(context);
    if (!WinHttpSetOption(rawRequest, WINHTTP_OPTION_CONTEXT_VALUE,
        &callbackContext, sizeof(callbackContext)))
    {
        ReleaseContextReference(context, context->lifetime.ReleaseHandleReference());
        (void)WinHttpCloseHandle(rawRequest);
        return response;
    }
    // The handle owns a distinct context reference through HANDLE_CLOSING;
    // close is nonblocking and the worker reference protects this stack frame.
    AsyncRequestHandle requestOwner(rawRequest);
    DWORD disabledFeatures = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_REDIRECTS |
        WINHTTP_DISABLE_AUTHENTICATION;
    DWORD automaticLogonPolicy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
    if (!WinHttpSetOption(requestOwner.Get(), WINHTTP_OPTION_DISABLE_FEATURE,
        &disabledFeatures, sizeof(disabledFeatures)) ||
        !WinHttpSetOption(requestOwner.Get(), WINHTTP_OPTION_AUTOLOGON_POLICY,
            &automaticLogonPolicy, sizeof(automaticLogonPolicy))) return response;
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(requestOwner.Get(), WINHTTP_OPTION_REDIRECT_POLICY,
        &redirectPolicy, sizeof(redirectPolicy))) return response;

    constexpr wchar_t headers[] = L"Content-Type: application/json; charset=utf-8\r\nAccept: application/json\r\n";
    if (IsCancelledOrExpired(cancellationEvent, deadline) ||
        !PrepareAsyncOperation(*context, requestOwner.Get(), deadline)) return response;
    if (!StartAsyncOperation(WinHttpSendRequest(requestOwner.Get(), headers,
        static_cast<DWORD>(std::size(headers) - 1u), context->requestBody.data(),
        static_cast<DWORD>(context->requestBody.size()),
        static_cast<DWORD>(context->requestBody.size()), callbackContext)) ||
        !WaitForCompletion(*context, cancellationEvent, deadline,
            WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)) return response;

    if (IsCancelledOrExpired(cancellationEvent, deadline) ||
        !PrepareAsyncOperation(*context, requestOwner.Get(), deadline) ||
        !StartAsyncOperation(WinHttpReceiveResponse(requestOwner.Get(), nullptr)) ||
        !WaitForCompletion(*context, cancellationEvent, deadline,
            WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE)) return response;

    DWORD status = 0u, statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(requestOwner.Get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX)) return response;
    response.statusCode = static_cast<int>(status);
    for (;;)
    {
        if (IsCancelledOrExpired(cancellationEvent, deadline) ||
            !PrepareAsyncOperation(*context, requestOwner.Get(), deadline) ||
            !StartAsyncOperation(WinHttpQueryDataAvailable(requestOwner.Get(), nullptr)) ||
            !WaitForCompletion(*context, cancellationEvent, deadline,
                WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE)) { response = {}; return response; }
        DWORD available = 0u;
        {
            std::lock_guard lock(context->mutex);
            available = context->available;
        }
        if (available == 0u) break;
        std::size_t responseSize = 0u;
        {
            std::lock_guard lock(context->mutex);
            responseSize = context->responseSize;
        }
        const std::size_t remaining = kMaxResponseBytes + 1u - responseSize;
        const DWORD amount = static_cast<DWORD>(std::min<std::size_t>(available, remaining));
        if (amount == 0u || IsCancelledOrExpired(cancellationEvent, deadline) ||
            !PrepareAsyncOperation(*context, requestOwner.Get(), deadline) ||
            !StartAsyncOperation(WinHttpReadData(requestOwner.Get(), context->readBuffer.data(),
                amount, nullptr)) ||
            !WaitForCompletion(*context, cancellationEvent, deadline,
                WINHTTP_CALLBACK_STATUS_READ_COMPLETE)) { response = {}; return response; }
        {
            std::lock_guard lock(context->mutex);
            if (context->error != ERROR_SUCCESS || context->responseSize > kMaxResponseBytes)
            { response = {}; return response; }
        }
    }
    {
        std::lock_guard lock(context->mutex);
        if (context->responseSize <= kMaxResponseBytes)
            response.body.assign(context->response.data(), context->responseSize);
        else response = {};
    }
    requestOwner.Close();
    return response;
}

struct WindowsPlaytestSubmission::State
{
    mutable std::mutex mutex;
    HWND window = nullptr;
    reporting::PreparedPlaytestSubmission frozen;
    WindowsPlaytestExchange exchange;
    std::chrono::milliseconds attemptTimeout = kTotalTimeout;
    std::shared_ptr<CancellationEvent> cancellation;
    std::chrono::steady_clock::time_point deadline{};
    std::uint64_t generation = 0u;
    std::uintptr_t completionToken = 0u;
    bool busy = false;
    bool retryAllowed = false;
    WindowsPlaytestSubmissionResult result = WindowsPlaytestSubmissionResult::None;
};

namespace
{
std::atomic_uintptr_t gNextCompletionToken{1u};

bool StartAttempt(const std::shared_ptr<WindowsPlaytestSubmission::State>& state,
    const std::string_view turnstileToken, const bool first)
{
    std::lock_guard lock(state->mutex);
    if (state->busy || !state->frozen.IsReady() || (!first && !state->retryAllowed)) return false;
    const auto deadline = std::chrono::steady_clock::now() + state->attemptTimeout;
    auto cancellation = std::make_shared<CancellationEvent>();
    if (cancellation->handle == nullptr) return false;
    std::string requestBody = reporting::BuildPlaytestSubmissionRequest(state->frozen, turnstileToken);
    if (requestBody.empty() || requestBody.size() > kMaxRequestBytes) { Wipe(requestBody); return false; }
    const auto requestBytes = std::shared_ptr<std::string>(new std::string(std::move(requestBody)),
        [](std::string* bytes) { Wipe(*bytes); delete bytes; });
    state->busy = true;
    state->retryAllowed = false;
    state->result = WindowsPlaytestSubmissionResult::None;
    state->cancellation = cancellation;
    state->deadline = deadline;
    const auto generation = ++state->generation;
    const auto window = state->window;
    const auto reportId = state->frozen.reportId;
    const auto exchange = state->exchange;
    const auto accepted = Worker().Submit([state, requestBytes, cancellation, reportId,
        exchange, generation, window, deadline]() mutable {
        {
            std::lock_guard stateLock(state->mutex);
            if (state->generation != generation || IsCancelledOrExpired(cancellation->handle, deadline))
            {
                state->busy = false;
                if (state->frozen.IsReady())
                {
                    state->retryAllowed = true;
                    state->result = WindowsPlaytestSubmissionResult::Uncertain;
                }
                state->cancellation.reset();
                return; // cancelled before dispatch: no request is opened or sent
            }
        }
        auto finish = [state, generation, window](const WindowsPlaytestSubmissionResult outcome) noexcept {
            std::uintptr_t completion = 0u;
            {
                std::lock_guard stateLock(state->mutex);
                if (state->generation != generation)
                {
                    state->busy = false;
                    if (state->frozen.IsReady())
                    {
                        state->retryAllowed = true;
                        state->result = WindowsPlaytestSubmissionResult::Uncertain;
                    }
                    state->cancellation.reset();
                    return;
                }
                state->busy = false;
                state->result = outcome;
                state->cancellation.reset();
                state->retryAllowed = outcome == WindowsPlaytestSubmissionResult::VerificationExpired ||
                    outcome == WindowsPlaytestSubmissionResult::RateLimited ||
                    outcome == WindowsPlaytestSubmissionResult::Uncertain;
                if (window == nullptr || state->window != window) return;
                completion = gNextCompletionToken.fetch_add(1u);
                state->completionToken = completion;
            }
            if (completion != 0u && !PostMessageW(window, kWindowsPlaytestSubmissionCompletedMessage,
                static_cast<WPARAM>(completion), 0u))
            {
                std::lock_guard stateLock(state->mutex);
                if (state->completionToken == completion) state->completionToken = 0u;
            }
        };
        auto outcome = WindowsPlaytestSubmissionResult::Uncertain;
        try
        {
            const auto response = exchange ? exchange(*requestBytes, deadline, cancellation->handle) :
                ExchangeWindowsPlaytestReport(*requestBytes, deadline, cancellation->handle);
            outcome = ClassifyWindowsPlaytestResponse(response.statusCode, response.body, reportId);
        }
        catch (...) { outcome = WindowsPlaytestSubmissionResult::Uncertain; }
        finish(outcome);
    });
    if (!accepted)
    {
        state->busy = false;
        state->retryAllowed = !first;
        state->cancellation.reset();
        state->result = WindowsPlaytestSubmissionResult::Uncertain;
        Wipe(requestBody); // requestBytes is wiped by its shared-pointer deleter, including rejection.
    }
    return accepted;
}
} // namespace

WindowsPlaytestSubmission::WindowsPlaytestSubmission(WindowsPlaytestExchange exchange,
    const std::chrono::milliseconds attemptTimeout)
    : state_(std::make_shared<State>())
{
    state_->exchange = std::move(exchange);
    state_->attemptTimeout = attemptTimeout.count() > 0 ? attemptTimeout : kTotalTimeout;
}

WindowsPlaytestSubmission::~WindowsPlaytestSubmission()
{
    Reset();
}

bool WindowsPlaytestSubmission::Begin(const HWND window,
    const reporting::PreparedPlaytestSubmission& submission, const std::string_view turnstileToken)
{
    if (!submission.IsReady() || submission.json.empty() || submission.json.size() > kMaxRequestBytes ||
        submission.reportId.empty()) return false;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->busy || state_->frozen.IsReady()) return false;
        state_->window = window;
        state_->frozen = submission;
    }
    if (!StartAttempt(state_, turnstileToken, true))
    {
        std::lock_guard lock(state_->mutex);
        state_->frozen = {};
        state_->window = nullptr;
        return false;
    }
    return true;
}

bool WindowsPlaytestSubmission::Retry(const HWND window, const std::string_view turnstileToken)
{
    {
        std::lock_guard lock(state_->mutex);
        if (state_->busy || !state_->retryAllowed) return false;
        state_->window = window;
    }
    return StartAttempt(state_, turnstileToken, false);
}

void WindowsPlaytestSubmission::CancelAttempt() noexcept
{
    std::lock_guard lock(state_->mutex);
    ++state_->generation;
    state_->completionToken = 0u;
    state_->window = nullptr;
    if (state_->busy)
    {
        // Signal only. The worker owns and closes the async WinHTTP handle;
        // UI cancellation never blocks on WinHTTP cleanup.
        if (state_->cancellation) SetEvent(state_->cancellation->handle);
        state_->retryAllowed = false;
        state_->result = WindowsPlaytestSubmissionResult::Uncertain;
    }
    else if (state_->result == WindowsPlaytestSubmissionResult::VerificationExpired ||
        state_->result == WindowsPlaytestSubmissionResult::RateLimited ||
        state_->result == WindowsPlaytestSubmissionResult::Uncertain)
    {
        state_->retryAllowed = true;
    }
}

void WindowsPlaytestSubmission::Reset() noexcept
{
    CancelAttempt();
    std::lock_guard lock(state_->mutex);
    state_->frozen = {};
    state_->result = WindowsPlaytestSubmissionResult::None;
    state_->retryAllowed = false;
}

bool WindowsPlaytestSubmission::IsBusy() const noexcept
{
    std::lock_guard lock(state_->mutex);
    return state_->busy;
}

bool WindowsPlaytestSubmission::CanRetry() const noexcept
{
    std::lock_guard lock(state_->mutex);
    return !state_->busy && state_->retryAllowed;
}

std::string WindowsPlaytestSubmission::ReportId() const
{
    std::lock_guard lock(state_->mutex);
    return state_->frozen.reportId;
}

WindowsPlaytestSubmissionResult WindowsPlaytestSubmission::LastResult() const noexcept
{
    std::lock_guard lock(state_->mutex);
    return state_->result;
}

bool WindowsPlaytestSubmission::HandleMessage(const UINT message, const WPARAM token,
    WindowsPlaytestSubmissionResult& result) noexcept
{
    if (message != kWindowsPlaytestSubmissionCompletedMessage) return false;
    std::lock_guard lock(state_->mutex);
    if (state_->completionToken == 0u || state_->completionToken != static_cast<std::uintptr_t>(token))
        return true;
    state_->completionToken = 0u;
    result = state_->result;
    return true;
}
} // namespace horde::platform::windows
