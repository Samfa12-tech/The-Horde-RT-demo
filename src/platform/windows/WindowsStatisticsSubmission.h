#pragma once
#include "platform/windows/WindowsPlaytestSubmission.h"

namespace horde::platform::windows
{
// Only local/staged fixtures supply these dependencies today. An empty service
// is the production default. Verification receives purpose, never payload bytes.
// Enabling live endpoints requires separate deployment/E2E admission.
struct WindowsStatisticsSendServices
{
    WindowsPlaytestExchange privateExchange, publicExchange;
    std::function<std::string(HWND, bool publicBoard)> verify;
};

class WindowsStatisticsSubmission final
{
public:
    WindowsStatisticsSendServices services;
    [[nodiscard]] bool Available(bool publicBoard) const noexcept
    { return services.verify && (publicBoard ? bool(services.publicExchange) : bool(services.privateExchange)); }
    [[nodiscard]] bool Prepare(const reporting::PreparedBenchmarkSummaryReport& report, bool consent)
    { return PreparePrivate(reporting::PreparePrivateDeveloperReport(report, consent)); }
    [[nodiscard]] bool Prepare(const reporting::PreparedCompatibilityReport& report, bool consent)
    { return PreparePrivate(reporting::PreparePrivateDeveloperReport(report, consent)); }
    [[nodiscard]] bool Prepare(const reporting::PreparedPublicBenchmarkReport& report, bool consent)
    {
        Clear(); public_ = reporting::PreparePublicBenchmarkSubmission(report, consent);
        publicBoard_ = true; return public_.IsReady();
    }
    [[nodiscard]] std::string_view Json() const noexcept
    { return publicBoard_ ? public_.Json() : private_.Json(); }
    [[nodiscard]] bool Send(HWND window)
    {
        if (!Available(publicBoard_) || Json().empty() || (owner_ && !owner_->CanRetry())) return false;
        std::string token;
        try { token = services.verify(window, publicBoard_); } catch (...) { return false; }
        bool began = false;
        if (owner_) began = owner_->Retry(window, token);
        else
        {
            owner_ = std::make_unique<WindowsPlaytestSubmission>(publicBoard_ ? services.publicExchange : services.privateExchange);
            began = publicBoard_ ? owner_->BeginPublicBenchmarkForLocalValidation(window, public_, token) :
                owner_->BeginDeveloperReportForLocalValidation(window, private_, token);
            if (!began) owner_.reset();
        }
        if (!token.empty()) SecureZeroMemory(token.data(), token.size());
        return began;
    }
    void CancelAttempt() noexcept { if (owner_) owner_->CancelAttempt(); }
    void Clear() noexcept { owner_.reset(); private_ = {}; public_ = {}; publicBoard_ = false; }
    [[nodiscard]] bool Busy() const noexcept { return owner_ && owner_->IsBusy(); }
    [[nodiscard]] bool CanSend() const noexcept
    { return Available(publicBoard_) && !Json().empty() && (!owner_ || owner_->CanRetry()); }
    [[nodiscard]] bool Handle(UINT message, WPARAM token, WindowsPlaytestSubmissionResult& result) noexcept
    { return owner_ && owner_->HandleMessage(message, token, result); }
    [[nodiscard]] WindowsPlaytestSubmissionResult Result() const noexcept
    { return owner_ ? owner_->LastResult() : WindowsPlaytestSubmissionResult::None; }
    [[nodiscard]] const wchar_t* StatusText() const noexcept
    {
        // These services are local mocks only. Never present their receipt as
        // production delivery, an inbox acknowledgement, or a published score.
        switch (Result())
        {
        case WindowsPlaytestSubmissionResult::Queued: return publicBoard_ ?
            L"Local test: accepted for review, NOT published. Check receipt explicitly later." :
            L"Local test: accepted, developer delivery is not confirmed.";
        case WindowsPlaytestSubmissionResult::Sent: return L"Local test: provider accepted. No real report or email was sent.";
        case WindowsPlaytestSubmissionResult::Published: return L"Local test: matching published receipt. No real board was changed.";
        case WindowsPlaytestSubmissionResult::Conflict: return L"Local test: ID/content conflict. Nothing can be claimed delivered.";
        case WindowsPlaytestSubmissionResult::Rejected: return L"Local test: rejected. No successful delivery or publication receipt.";
        case WindowsPlaytestSubmissionResult::VerificationExpired: return L"Local test: verify again, then retry the same payload explicitly.";
        case WindowsPlaytestSubmissionResult::RateLimited: return L"Local test: rate limited. Wait before an explicit same-ID retry.";
        case WindowsPlaytestSubmissionResult::Uncertain: return L"Local test: outcome unknown. Retry explicitly with the same ID and fresh verification.";
        default: return Busy() ? L"Local test request in progress. Cancel cannot prove it was not accepted." :
            L"Sending is unavailable. You can still copy or save the local report.";
        }
    }
private:
    bool PreparePrivate(reporting::PreparedDeveloperReportSubmission report)
    { Clear(); private_ = std::move(report); return private_.IsReady(); }
    bool publicBoard_ = false;
    reporting::PreparedDeveloperReportSubmission private_;
    reporting::PreparedPublicBenchmarkSubmission public_;
    std::unique_ptr<WindowsPlaytestSubmission> owner_;
};
}
