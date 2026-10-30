package com.samfa12.hordelanternrt;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executor;
import java.util.function.BooleanSupplier;

/** Foreground statistics submission seam. Production remains unavailable. */
final class StatisticsReportSendControls {
    interface TokenCallback { void completed(String token); }
    interface Verification {
        void request(String purpose, long attempt, TokenCallback callback);
        void cancel();
    }
    static final class Services {
        final PlaytestReportSubmission.Transport transport;
        final Verification verification;
        final boolean synthetic;
        private Services(PlaytestReportSubmission.Transport transport, Verification verification, boolean synthetic) {
            this.transport = transport; this.verification = verification; this.synthetic = synthetic;
        }
        static Services unavailable() { return new Services(new PlaytestReportSubmission.HttpsTransport(), null, false); }
        static Services localValidation(PlaytestReportSubmission.Transport transport, Verification verification) {
            if (transport == null || verification == null) throw new IllegalArgumentException("Local test dependencies required");
            return new Services(transport, verification, true);
        }
    }
    private interface Admission { boolean begin(PlaytestReportSubmission owner, boolean consent); }
    private final Activity activity;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final BooleanSupplier prepared;
    private final Admission admission;
    private final Services services;
    private final boolean publicResult;
    final PlaytestReportSubmission submission;
    final CheckBox consent;
    final Button review, send;
    final TextView preview, status;
    private boolean closed, verifying, foreground = true;
    private long generation;

    static StatisticsReportSendControls benchmark(Activity activity, LinearLayout panel, Executor worker,
            BenchmarkSummaryReview local, Services services) {
        return new StatisticsReportSendControls(activity, panel, worker, local::isPrepared,
                (owner, consent) -> local.isPublicPreview() ? owner.beginPublicBenchmarkSummary(local, consent) :
                    owner.beginPrivateBenchmarkSummary(local, consent), local.isPublicPreview(), services);
    }
    static StatisticsReportSendControls compatibility(Activity activity, LinearLayout panel, Executor worker,
            CompatibilityReportReview local, Services services) {
        return new StatisticsReportSendControls(activity, panel, worker, local::isPrepared,
                (owner, consent) -> owner.beginPrivateCompatibility(local, consent), false, services);
    }
    private StatisticsReportSendControls(Activity activity, LinearLayout panel, Executor worker,
            BooleanSupplier prepared, Admission admission, boolean publicResult, Services services) {
        this.activity = activity; this.prepared = prepared; this.admission = admission;
        this.publicResult = publicResult; this.services = services == null ? Services.unavailable() : services;
        submission = new PlaytestReportSubmission(worker::execute, this.services.transport);
        consent = new CheckBox(activity); consent.setText(publicResult ? R.string.statistics_publish_consent : R.string.statistics_private_send_consent);
        consent.setTextColor(HordeUiTokens.PARCHMENT); consent.setMinHeight(dp(48)); panel.addView(consent);
        review = button(panel, publicResult ? R.string.statistics_review_public_submission : R.string.statistics_review_private_submission);
        text(panel, R.string.statistics_submission_preview);
        preview = text(panel, 0); preview.setTypeface(Typeface.MONOSPACE); preview.setTextIsSelectable(true);
        send = button(panel, publicResult ? R.string.statistics_publish_send : R.string.statistics_private_send);
        status = text(panel, R.string.statistics_send_unavailable);
        status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        consent.setOnCheckedChangeListener((view, checked) -> refresh());
        review.setOnClickListener(view -> prepareSubmission()); send.setOnClickListener(view -> verifyAndSend());
        refresh();
    }
    private int dp(int value) { return Math.round(activity.getResources().getDisplayMetrics().density * value); }
    private TextView text(LinearLayout panel, int message) {
        TextView text = new TextView(activity); text.setTextColor(HordeUiTokens.PARCHMENT); text.setTextSize(14);
        text.setPadding(0, dp(8), 0, dp(8)); if (message != 0) text.setText(message); panel.addView(text); return text;
    }
    private Button button(LinearLayout panel, int label) {
        Button button = new Button(activity); button.setText(label); button.setMinHeight(dp(48)); panel.addView(button); return button;
    }
    void refresh() {
        if (closed) return;
        final PlaytestReportSubmission.State state = submission.state();
        consent.setEnabled(foreground && prepared.getAsBoolean() && state == PlaytestReportSubmission.State.DRAFT);
        review.setEnabled(consent.isEnabled() && consent.isChecked());
        final boolean retry = state == PlaytestReportSubmission.State.RETRYABLE;
        final boolean pendingPublic = publicResult && state == PlaytestReportSubmission.State.QUEUED;
        send.setText(pendingPublic ? R.string.statistics_check_publication : retry ? R.string.statistics_retry_send :
                publicResult ? R.string.statistics_publish_send : R.string.statistics_private_send);
        send.setEnabled(foreground && !verifying && services.verification != null && submission.statisticsSendAvailable() &&
                (state == PlaytestReportSubmission.State.READY || retry || pendingPublic));
        if (services.synthetic && !verifying) renderReceiptState(state);
    }
    private void renderReceiptState(PlaytestReportSubmission.State state) {
        // Receipt ownership can complete before its main-loop notification.
        // Lifecycle cancellation revokes callbacks, not an already verified receipt.
        switch (state) {
            case QUEUED: status.setText(publicResult ? R.string.statistics_mock_pending : R.string.statistics_mock_private_queued); break;
            case SENT: status.setText(R.string.statistics_mock_sent); break;
            case PUBLISHED: status.setText(R.string.statistics_mock_published); break;
            case REJECTED: case CONFLICT: status.setText(R.string.statistics_mock_rejected); break;
            case RETRYABLE: status.setText(R.string.statistics_mock_uncertain); break;
            default: break;
        }
    }
    private void prepareSubmission() {
        if (closed || !foreground || !prepared.getAsBoolean() || !consent.isChecked() ||
                !admission.begin(submission, true)) return;
        final byte[] envelope = submission.statisticsJsonForReview();
        preview.setText(envelope == null ? "" : new String(envelope, StandardCharsets.UTF_8));
        status.setText(services.synthetic ? R.string.statistics_mock_ready : R.string.statistics_send_unavailable);
        refresh();
    }
    private void verifyAndSend() {
        if (closed || !foreground || verifying || !send.isEnabled() || services.verification == null) return;
        final PlaytestReportSubmission.State state = submission.state();
        if ((state == PlaytestReportSubmission.State.RETRYABLE ||
                (publicResult && state == PlaytestReportSubmission.State.QUEUED)) && submission.retry() < 0) return;
        final long attempt = submission.attempt(), current = ++generation;
        verifying = true; status.setText(R.string.statistics_mock_verifying); refresh();
        try {
            services.verification.request(publicResult ? "horde_benchmark_publish" : "horde_report", attempt, token -> ui.post(() -> {
                if (closed || !foreground || generation != current || submission.attempt() != attempt || !verifying) return;
                verifying = false;
                if (token == null || token.isEmpty()) { status.setText(R.string.statistics_mock_verification_cancelled); refresh(); return; }
                final boolean started = submission.submit(attempt, token, (completed, result) -> ui.post(() -> {
                    if (closed || generation != current || submission.attempt() != completed) return;
                    refresh();
                }));
                if (!started) status.setText(R.string.statistics_send_unavailable);
                refresh();
            }));
        } catch (RuntimeException failure) { verifying = false; status.setText(R.string.statistics_mock_verification_cancelled); refresh(); }
    }
    void interrupt() {
        if (closed) return;
        final boolean revokedVerification = verifying;
        foreground = false; ++generation; verifying = false;
        final boolean revokedAttempt = submission.interruptInFlight();
        cancelVerification();
        if (services.synthetic && (revokedVerification || revokedAttempt)) status.setText(R.string.statistics_mock_interrupted);
        refresh();
    }
    void resume() { if (!closed) { foreground = true; refresh(); } }
    void close() {
        if (closed) return;
        closed = true; foreground = false; ++generation; verifying = false; submission.cancel();
        cancelVerification();
        preview.setText(""); consent.setChecked(false); consent.setEnabled(false); review.setEnabled(false); send.setEnabled(false);
    }
    private void cancelVerification() {
        if (services.verification != null) try { services.verification.cancel(); }
        catch (RuntimeException ignored) { /* The generation/owner is already revoked. */ }
    }
}
