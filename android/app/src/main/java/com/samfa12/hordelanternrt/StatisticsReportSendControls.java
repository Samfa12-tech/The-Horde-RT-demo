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

/** Foreground submission controls; consent, transport and verification retain their owners. */
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
        static Services production(Activity activity) {
            Verification verifier = new Verification() {
                PlaytestReportVerification.Handle handle;
                long generation;
                public void request(String purpose, long attempt, TokenCallback callback) {
                    cancel();
                    if (!"horde_report".equals(purpose) && !"horde_benchmark_publish".equals(purpose))
                        throw new IllegalArgumentException("Unsupported verification purpose");
                    final long current = ++generation;
                    handle = PlaytestReportVerification.show(activity, current, value -> value == generation,
                        new PlaytestReportVerification.Callback() {
                            public void onVerified(long owner, String token) { if (owner == generation) callback.completed(token); }
                            public void onFailure(long owner, PlaytestReportVerification.Failure failure) { if (owner == generation) callback.completed(null); }
                            public void onCancelled(long owner) { if (owner == generation) callback.completed(null); }
                        }, "horde_benchmark_publish".equals(purpose));
                }
                public void cancel() { ++generation; if (handle != null) { handle.cancel(); handle = null; } }
            };
            return new Services(PlaytestReportSubmission.HttpsTransport.statistics(), verifier, false);
        }
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
        return benchmark(activity,panel,worker,local,services,null);
    }
    static StatisticsReportSendControls benchmark(Activity activity, LinearLayout panel, Executor worker,
            BenchmarkSummaryReview local, Services services, LinearLayout technicalDetails) {
        return new StatisticsReportSendControls(activity, panel, worker, local::isPrepared,
                (owner, consent) -> local.isPublicPreview() ? owner.beginPublicBenchmarkSummary(local, consent) :
                    owner.beginPrivateBenchmarkSummary(local, consent), local.isPublicPreview(), services, technicalDetails);
    }
    static StatisticsReportSendControls compatibility(Activity activity, LinearLayout panel, Executor worker,
            CompatibilityReportReview local, Services services) {
        return new StatisticsReportSendControls(activity, panel, worker, local::isPrepared,
                (owner, consent) -> owner.beginPrivateCompatibility(local, consent), false, services, null);
    }
    private StatisticsReportSendControls(Activity activity, LinearLayout panel, Executor worker,
            BooleanSupplier prepared, Admission admission, boolean publicResult, Services services, LinearLayout technicalDetails) {
        this.activity = activity; this.prepared = prepared; this.admission = admission;
        this.publicResult = publicResult; this.services = services == null ? Services.unavailable() : services;
        submission = new PlaytestReportSubmission(worker::execute, this.services.transport);
        consent = new CheckBox(activity); consent.setText(publicResult ? R.string.statistics_publish_consent : R.string.statistics_private_send_consent);
        consent.setTextColor(HordeUiTokens.PARCHMENT); consent.setMinHeight(dp(48)); panel.addView(consent);
        review = button(panel, publicResult ? R.string.statistics_review_public_submission : R.string.statistics_review_private_submission);
        if(publicResult)review.setVisibility(View.GONE); // The explicit Publish action admits the immutable envelope.
        final LinearLayout raw=technicalDetails==null?panel:technicalDetails;
        text(raw, R.string.statistics_submission_preview);
        preview = text(raw, 0); preview.setTypeface(Typeface.MONOSPACE); preview.setTextIsSelectable(true);
        send = button(panel, publicResult ? R.string.statistics_publish_send : R.string.statistics_private_send);
        status = text(panel, this.services.verification == null ? R.string.statistics_send_unavailable :
                publicResult ? R.string.statistics_public_available : R.string.statistics_live_available);
        status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        consent.setOnCheckedChangeListener((view, checked) -> refresh());
        if(!publicResult)review.setOnClickListener(view -> prepareSubmission());
        send.setOnClickListener(view -> verifyAndSend());
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
        review.setEnabled(!publicResult && consent.isEnabled() && consent.isChecked());
        final boolean retry = state == PlaytestReportSubmission.State.RETRYABLE;
        final boolean pendingPublic = publicResult && state == PlaytestReportSubmission.State.QUEUED;
        send.setText(pendingPublic ? R.string.statistics_check_publication : retry ? R.string.statistics_retry_send :
                publicResult ? R.string.statistics_publish_send : R.string.statistics_private_send);
        final boolean directPublic=publicResult && state==PlaytestReportSubmission.State.DRAFT &&
                prepared.getAsBoolean() && consent.isChecked() && services.transport.admitsPublicBenchmarkReports();
        send.setEnabled(foreground && !verifying && services.verification != null &&
                (directPublic || (submission.statisticsSendAvailable() &&
                (state == PlaytestReportSubmission.State.READY || retry || pendingPublic))));
        if (!verifying) renderReceiptState(state);
    }
    private void renderReceiptState(PlaytestReportSubmission.State state) {
        // Receipt ownership can complete before its main-loop notification.
        // Lifecycle cancellation revokes callbacks, not an already verified receipt.
        switch (state) {
            case IN_FLIGHT: if (!services.synthetic) status.setText(R.string.statistics_live_sending); break;
            case QUEUED: status.setText(publicResult ? (services.synthetic ? R.string.statistics_mock_pending : R.string.statistics_live_pending) : (services.synthetic ? R.string.statistics_mock_private_queued : R.string.statistics_live_private_queued)); break;
            case SENT: status.setText((services.synthetic ? R.string.statistics_mock_sent : R.string.statistics_live_sent)); break;
            case PUBLISHED: status.setText((services.synthetic ? R.string.statistics_mock_published : R.string.statistics_live_published)); break;
            case REJECTED: case CONFLICT: status.setText((services.synthetic ? R.string.statistics_mock_rejected : R.string.statistics_live_rejected)); break;
            case RETRYABLE: status.setText((services.synthetic ? R.string.statistics_mock_uncertain : R.string.statistics_live_uncertain)); break;
            default: break;
        }
    }
    private void prepareSubmission() {
        if (closed || !foreground || !prepared.getAsBoolean() || !consent.isChecked() ||
                !admission.begin(submission, true)) return;
        final byte[] envelope = submission.statisticsJsonForReview();
        preview.setText(envelope == null ? "" : new String(envelope, StandardCharsets.UTF_8));
        status.setText(services.synthetic ? R.string.statistics_mock_ready : services.verification == null ? R.string.statistics_send_unavailable : R.string.statistics_live_ready);
        refresh();
    }
    private void verifyAndSend() {
        if (closed || !foreground || verifying || !send.isEnabled() || services.verification == null) return;
        // Public consent alone never constructs or sends anything. Only this
        // explicit final action freezes the already reviewed local preview.
        if(publicResult && submission.state()==PlaytestReportSubmission.State.DRAFT) {
            if(!consent.isChecked() || !prepared.getAsBoolean() || !services.transport.admitsPublicBenchmarkReports())return;
            prepareSubmission();
            if(submission.state()!=PlaytestReportSubmission.State.READY)return;
        }
        final PlaytestReportSubmission.State state = submission.state();
        if ((state == PlaytestReportSubmission.State.RETRYABLE ||
                (publicResult && state == PlaytestReportSubmission.State.QUEUED)) && submission.retry() < 0) return;
        final long attempt = submission.attempt(), current = ++generation;
        verifying = true; status.setText((services.synthetic ? R.string.statistics_mock_verifying : R.string.statistics_live_verifying)); refresh();
        try {
            services.verification.request(publicResult ? "horde_benchmark_publish" : "horde_report", attempt, token -> ui.post(() -> {
                if (closed || !foreground || generation != current || submission.attempt() != attempt || !verifying) return;
                verifying = false;
                if (token == null || token.isEmpty()) { status.setText((services.synthetic ? R.string.statistics_mock_verification_cancelled : R.string.statistics_live_verification_cancelled)); refresh(); return; }
                final boolean started = submission.submit(attempt, token, (completed, result) -> ui.post(() -> {
                    if (closed || generation != current || submission.attempt() != completed) return;
                    refresh();
                }));
                if (!started) status.setText(R.string.statistics_send_unavailable);
                refresh();
            }));
        } catch (RuntimeException failure) { verifying = false; status.setText((services.synthetic ? R.string.statistics_mock_verification_cancelled : R.string.statistics_live_verification_cancelled)); refresh(); }
    }
    void interrupt() {
        if (closed) return;
        final boolean revokedVerification = verifying;
        foreground = false; ++generation; verifying = false;
        final boolean revokedAttempt = submission.interruptInFlight();
        cancelVerification();
        if (revokedVerification || revokedAttempt) status.setText(services.synthetic ? R.string.statistics_mock_interrupted : R.string.statistics_live_interrupted);
        refresh();
    }
    void resume() { if (!closed) { foreground = true; refresh(); } }
    boolean canChangeDestination() {
        final PlaytestReportSubmission.State state=submission.state();
        // Changing audience cannot withdraw an accepted/uncertain request or
        // silently mint a replacement identity after a network attempt.
        return !closed && foreground && !verifying && (state==PlaytestReportSubmission.State.DRAFT ||
                state==PlaytestReportSubmission.State.READY || state==PlaytestReportSubmission.State.REJECTED);
    }
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
