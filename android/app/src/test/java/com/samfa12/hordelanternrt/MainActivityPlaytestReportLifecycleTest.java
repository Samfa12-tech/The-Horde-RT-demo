package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowAlertDialog;

/** Exercises the Activity's UI reconciliation without running onCreate/JNI. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28)
public final class MainActivityPlaytestReportLifecycleTest {
    private static final String ID = "opaque-123";

    private static final class QueuedExecutor implements PlaytestReportSubmission.Executor {
        final List<Runnable> jobs = new ArrayList<>();
        @Override public void execute(Runnable job) { jobs.add(job); }
        void runNext() { jobs.remove(0).run(); }
    }

    private static final class Fixture {
        final QueuedExecutor executor = new QueuedExecutor();
        final PlaytestReportSubmission owner;
        Fixture() {
            owner = new PlaytestReportSubmission(executor, request -> call(
                    new PlaytestReportSubmission.Response(202,
                            ("{\"ok\":true,\"id\":\"" + ID + "\",\"status\":\"accepted\"}")
                                    .getBytes(StandardCharsets.UTF_8))),
                    (expired, delay) -> () -> { }, cleanup -> { });
            assertTrue(owner.begin(prepared(), true));
        }
    }

    private MainActivity activity;
    private Context context;
    private Button remoteSend, edit, save, remotePrepare;
    private TextView status;

    @Before public void setUp() throws Exception {
        // get() constructs/attaches an Activity instance without dispatching onCreate,
        // whose normal startup intentionally requires the native runtime.
        activity = Robolectric.buildActivity(MainActivity.class).get();
        context = RuntimeEnvironment.getApplication();
        remoteSend = new Button(context);
        edit = new Button(context);
        save = new Button(context);
        remotePrepare = new Button(context);
        status = new TextView(context);
        put("playtestReportVisible", true);
        put("playtestFormGeneration", 7L);
        put("playtestRemoteSend", remoteSend);
        put("playtestEdit", edit);
        put("playtestSave", save);
        put("playtestRemotePrepare", remotePrepare);
        put("playtestStatus", status);
        put("playtestNote", new EditText(context));
        put("playtestCategory", new Spinner(context));
        put("playtestImpact", new Spinner(context));
        put("playtestConsent", new CheckBox(context));
        put("playtestContext", new CheckBox(context));
        put("playtestRemoteConsent", new CheckBox(context));
        put("playtestRemoteContext", new CheckBox(context));
        put("playtestRemoteScreenshot", new CheckBox(context));
    }

    @After public void tearDown() throws Exception {
        get("reportExecutor", java.util.concurrent.ThreadPoolExecutor.class).shutdownNow();
    }

    @Test public void pauseInterruptKeepsSameIdRetryableAndRestoresControls() throws Exception {
        Fixture fixture = new Fixture();
        assertTrue(fixture.owner.submit(fixture.owner.attempt(), "one-use-token", (attempt, result) -> { }));
        assertEquals(PlaytestReportSubmission.State.IN_FLIGHT, fixture.owner.state());
        remoteSend.setEnabled(false);
        edit.setEnabled(false);
        put("playtestSubmission", fixture.owner);

        activity.reconcilePlaytestReportForPause();

        assertEquals(PlaytestReportSubmission.State.RETRYABLE, fixture.owner.state());
        assertEquals(ID, fixture.owner.reportId());
        assertTrue(fixture.owner.retry() > 0L); // Frozen report survived interruption.
        assertTrue(remoteSend.isEnabled());
        assertTrue(edit.isEnabled());
        assertEquals(context.getText(R.string.playtest_lifecycle_uncertain), status.getText());
    }

    @Test public void pauseReconcilesRetryableAttemptWhoseUiCallbackWasNotApplied() throws Exception {
        PlaytestReportSubmission owner = completedOwner(503, "accepted");
        String reportId = owner.reportId();
        remoteSend.setEnabled(false);
        put("playtestSubmission", owner);

        activity.reconcilePlaytestReportForPause();

        assertEquals(PlaytestReportSubmission.State.RETRYABLE, owner.state());
        assertEquals(reportId, owner.reportId());
        assertTrue(remoteSend.isEnabled());
        assertEquals(context.getText(R.string.playtest_retry_ready), status.getText());
    }

    @Test public void pauseRendersQueuedAndSentStatesEvenWhenOldSendingUiWasPosted() throws Exception {
        for (boolean sent : new boolean[] {false, true}) {
            PlaytestReportSubmission owner = completedOwner(sent ? 200 : 202, sent ? "sent" : "accepted");
            assertEquals(sent ? PlaytestReportSubmission.State.SENT : PlaytestReportSubmission.State.QUEUED,
                    owner.state());
            remoteSend.setVisibility(View.VISIBLE);
            remoteSend.setEnabled(false);
            edit.setEnabled(false);
            status.setText(R.string.playtest_sending);
            put("playtestSubmission", owner);

            activity.reconcilePlaytestReportForPause();

            assertEquals(sent ? context.getText(R.string.playtest_sent) : context.getText(R.string.playtest_queued),
                    status.getText());
            assertEquals(View.GONE, remoteSend.getVisibility());
            assertTrue(edit.isEnabled());
        }
    }

    @Test public void editHandlerCannotAbandonInFlightSubmission() throws Exception {
        Fixture fixture = new Fixture();
        assertTrue(fixture.owner.submit(fixture.owner.attempt(), "one-use-token", (attempt, result) -> { }));
        put("playtestSubmission", fixture.owner);
        edit.setEnabled(false);
        long generation = fixture.owner.attempt();

        activity.requestPlaytestEdit();

        assertEquals(PlaytestReportSubmission.State.IN_FLIGHT, fixture.owner.state());
        assertEquals(generation, fixture.owner.attempt());
        assertEquals(ID, fixture.owner.reportId());
    }

    @Test public void cancellingRetryableEditConfirmationKeepsFrozenReport() throws Exception {
        PlaytestReportSubmission owner = completedOwner(503, "accepted");
        final String reportId = owner.reportId();
        put("playtestSubmission", owner);

        activity.requestPlaytestEdit();

        android.app.AlertDialog dialog = ShadowAlertDialog.getLatestAlertDialog();
        assertNotNull(dialog);
        dialog.getButton(android.app.AlertDialog.BUTTON_NEGATIVE).performClick();
        assertEquals(PlaytestReportSubmission.State.RETRYABLE, owner.state());
        assertEquals(reportId, owner.reportId());
        assertTrue(owner.retry() > 0L);
    }

    @Test public void pauseDismissesUncertainEditAndStaleConfirmationCannotReplaceOwner() throws Exception {
        PlaytestReportSubmission owner = completedOwner(503, "accepted");
        put("playtestSubmission", owner);
        activity.requestPlaytestEdit();
        android.app.AlertDialog dialog = ShadowAlertDialog.getLatestAlertDialog();
        assertNotNull(dialog);
        Button staleConfirm = dialog.getButton(android.app.AlertDialog.BUTTON_POSITIVE);

        activity.reconcilePlaytestReportForPause();
        assertFalse(dialog.isShowing());
        staleConfirm.performClick();

        assertEquals(owner, get("playtestSubmission", PlaytestReportSubmission.class));
        assertEquals(PlaytestReportSubmission.State.RETRYABLE, owner.state());
        assertEquals(ID, owner.reportId());
        assertTrue(remoteSend.isEnabled());
    }

    @Test public void reportFormStartsWithAllOptInsOffAndRemotePrecedesOfflineFallback() throws Exception {
        put("menuScrim", new FrameLayout(context));
        java.lang.reflect.Method show = MainActivity.class.getDeclaredMethod("showPlaytestReport");
        show.setAccessible(true);
        show.invoke(activity);

        assertFalse(get("playtestRemoteConsent", CheckBox.class).isChecked());
        assertFalse(get("playtestRemoteContext", CheckBox.class).isChecked());
        assertFalse(get("playtestRemoteScreenshot", CheckBox.class).isChecked());
        assertFalse(get("playtestContext", CheckBox.class).isChecked());
        assertFalse(get("playtestConsent", CheckBox.class).isChecked());

        FrameLayout root = get("menuScrim", FrameLayout.class);
        List<String> labels = new ArrayList<>();
        collectText(root, labels);
        assertTrue(labels.indexOf(context.getString(R.string.playtest_remote_section)) >= 0);
        assertTrue(labels.indexOf(context.getString(R.string.playtest_local_section)) >= 0);
        assertTrue(labels.indexOf(context.getString(R.string.playtest_remote_section)) <
                labels.indexOf(context.getString(R.string.playtest_local_section)));
    }

    private static void collectText(View view, List<String> labels) {
        if (view instanceof TextView) labels.add(((TextView) view).getText().toString());
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); ++i) collectText(group.getChildAt(i), labels);
        }
    }

    private PlaytestReportSubmission completedOwner(int status, String ackStatus) {
        QueuedExecutor queued = new QueuedExecutor();
        PlaytestReportSubmission owner = new PlaytestReportSubmission(queued,
                request -> call(status == 503
                        ? new PlaytestReportSubmission.Response(status, new byte[0])
                        : new PlaytestReportSubmission.Response(status,
                                ("{\"ok\":true,\"id\":\"" + ID + "\",\"status\":\"" + ackStatus + "\"}")
                                        .getBytes(StandardCharsets.UTF_8))),
                (expired, delay) -> () -> { }, cleanup -> { });
        assertTrue(owner.begin(prepared(), true));
        assertTrue(owner.submit(owner.attempt(), "fresh-token", (attempt, result) -> { }));
        queued.runNext();
        return owner;
    }

    private void put(String name, Object value) throws Exception {
        Field field = MainActivity.class.getDeclaredField(name);
        field.setAccessible(true);
        field.set(activity, value);
    }

    @SuppressWarnings("unchecked")
    private <T> T get(String name, Class<T> type) throws Exception {
        Field field = MainActivity.class.getDeclaredField(name);
        field.setAccessible(true);
        return (T) field.get(activity);
    }

    private static byte[] prepared() {
        byte[] json = ("{\"schemaVersion\":1,\"product\":\"horde-lantern-rt\",\"report\":{" +
                "\"schemaVersion\":1,\"reportId\":\"" + ID + "\",\"note\":\"test\"}}")
                .getBytes(StandardCharsets.UTF_8);
        byte[] envelope = new byte[json.length + 1];
        System.arraycopy(json, 0, envelope, 1, json.length);
        return envelope;
    }

    private static PlaytestReportSubmission.Call call(PlaytestReportSubmission.Response response) {
        return new PlaytestReportSubmission.Call() {
            @Override public PlaytestReportSubmission.Response execute() { return response; }
            @Override public void cancel() { }
        };
    }
}
