package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.app.Activity;
import android.os.Looper;
import android.widget.LinearLayout;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class StatisticsReportSendControlsTest {
    static final class Verification implements StatisticsReportSendControls.Verification {
        final List<String> purposes=new ArrayList<>();
        StatisticsReportSendControls.TokenCallback pending;
        int requests,cancels;
        public void request(String purpose,long attempt,StatisticsReportSendControls.TokenCallback callback){
            requests++;purposes.add(purpose);pending=callback;
        }
        void complete(){StatisticsReportSendControls.TokenCallback callback=pending;pending=null;callback.completed("synthetic-token-"+requests);}
        public void cancel(){cancels++;}
    }
    static void idle(){Shadows.shadowOf(Looper.getMainLooper()).idle();}
    static LinearLayout panel(Activity activity){LinearLayout panel=new LinearLayout(activity);panel.setOrientation(LinearLayout.VERTICAL);activity.setContentView(panel);return panel;}
    private void publicUiRoundTrip(){
        Activity activity=Robolectric.buildActivity(Activity.class).setup().get();
        PublicBenchmarkReportFlowTest.Worker worker=new PublicBenchmarkReportFlowTest.Worker();
        PublicBenchmarkReportFlowTest.PublicRelay relay=new PublicBenchmarkReportFlowTest.PublicRelay();Verification verifier=new Verification();
        BenchmarkSummaryReview local=PublicBenchmarkReportFlowTest.review();byte[] original=local.exactJsonBytes();
        StatisticsReportSendControls controls=StatisticsReportSendControls.benchmark(activity,panel(activity),worker::execute,local,
            StatisticsReportSendControls.Services.localValidation(relay,verifier));
        assertFalse(controls.consent.isChecked());assertFalse(controls.review.isEnabled());assertFalse(controls.send.isEnabled());
        controls.send.performClick();idle();assertEquals(0,verifier.requests);
        controls.consent.setChecked(true);assertTrue(controls.review.isEnabled());controls.review.performClick();
        assertFalse(controls.consent.isEnabled());assertTrue(controls.send.isEnabled());
        assertTrue(controls.preview.getText().toString().contains("\"consentToPublish\":true"));
        assertFalse(controls.preview.getText().toString().contains("turnstileToken"));assertArrayEquals(original,local.exactJsonBytes());
        controls.send.performClick();assertEquals(1,verifier.requests);assertEquals("horde_benchmark_publish",verifier.purposes.get(0));
        assertTrue(worker.jobs.isEmpty());verifier.complete();idle();worker.drain();idle();
        assertEquals(PlaytestReportSubmission.State.QUEUED,controls.submission.state());
        assertEquals(activity.getString(R.string.statistics_mock_pending),controls.status.getText().toString());
        assertEquals(activity.getString(R.string.statistics_check_publication),controls.send.getText().toString());
        assertTrue(controls.send.isEnabled());assertEquals(1,relay.publicOpens);idle();assertEquals(1,relay.publicOpens);
        relay.next=PublicBenchmarkReportFlowTest.ack(200,"published","public-board","public-benchmark-result",PublicBenchmarkReportFlowTest.ID);
        controls.send.performClick();assertEquals(2,verifier.requests);verifier.complete();idle();worker.drain();idle();
        assertEquals(PlaytestReportSubmission.State.PUBLISHED,controls.submission.state());assertFalse(controls.send.isEnabled());
        assertEquals(activity.getString(R.string.statistics_mock_published),controls.status.getText().toString());
        assertEquals(0,relay.privateOpens);assertArrayEquals(original,local.exactJsonBytes());
        assertTrue(controls.send.getMinHeight()>=48*activity.getResources().getDisplayMetrics().density);
        controls.close();assertEquals("",controls.preview.getText().toString());activity.finish();
    }
    @Test @Config(qualifiers="port") public void portraitPublicControlsConsentQueuedRecheckAndSyntheticPublished(){publicUiRoundTrip();}
    @Test @Config(qualifiers="land") public void landscapePublicControlsConsentQueuedRecheckAndSyntheticPublished(){publicUiRoundTrip();}

    @Test public void closeAndPauseRevokePendingVerificationAndRequireExplicitResume(){
        Activity activity=Robolectric.buildActivity(Activity.class).setup().get();
        PublicBenchmarkReportFlowTest.Worker worker=new PublicBenchmarkReportFlowTest.Worker();
        PublicBenchmarkReportFlowTest.PublicRelay relay=new PublicBenchmarkReportFlowTest.PublicRelay();Verification verifier=new Verification();
        StatisticsReportSendControls controls=StatisticsReportSendControls.benchmark(activity,panel(activity),worker::execute,
            PublicBenchmarkReportFlowTest.review(),StatisticsReportSendControls.Services.localValidation(relay,verifier));
        controls.consent.setChecked(true);controls.review.performClick();controls.send.performClick();
        StatisticsReportSendControls.TokenCallback stale=verifier.pending;
        controls.interrupt();assertFalse(controls.send.isEnabled());stale.completed("stale-token");idle();assertTrue(worker.jobs.isEmpty());
        controls.resume();assertTrue(controls.send.isEnabled());assertEquals(1,verifier.requests);
        controls.send.performClick();assertEquals(2,verifier.requests);verifier.complete();idle();
        controls.interrupt();worker.drain();idle();assertEquals(0,relay.publicOpens);
        controls.resume();controls.send.performClick();
        controls.close();verifier.complete();idle();worker.drain();assertEquals(0,relay.publicOpens);
        assertEquals(PlaytestReportSubmission.State.CANCELLED,controls.submission.state());activity.finish();
    }
    @Test public void wrongReceiptShowsSyntheticUncertaintyAndDoesNotPublish(){
        Activity activity=Robolectric.buildActivity(Activity.class).setup().get();
        PublicBenchmarkReportFlowTest.Worker worker=new PublicBenchmarkReportFlowTest.Worker();
        PublicBenchmarkReportFlowTest.PublicRelay relay=new PublicBenchmarkReportFlowTest.PublicRelay();Verification verifier=new Verification();
        relay.next=PublicBenchmarkReportFlowTest.ack(200,"published","developer-private","public-benchmark-result",PublicBenchmarkReportFlowTest.ID);
        StatisticsReportSendControls controls=StatisticsReportSendControls.benchmark(activity,panel(activity),worker::execute,
            PublicBenchmarkReportFlowTest.review(),StatisticsReportSendControls.Services.localValidation(relay,verifier));
        controls.consent.setChecked(true);controls.review.performClick();controls.send.performClick();verifier.complete();idle();worker.drain();idle();
        assertEquals(PlaytestReportSubmission.State.RETRYABLE,controls.submission.state());
        assertEquals(activity.getString(R.string.statistics_mock_uncertain),controls.status.getText().toString());
        assertEquals(1,relay.publicOpens);assertTrue(controls.send.isEnabled());controls.close();activity.finish();
    }
    @Test public void productionUnavailableStillAllowsExactSeparatePreviewWithoutSending(){
        Activity activity=Robolectric.buildActivity(Activity.class).setup().get();
        BenchmarkSummaryReview local=PublicBenchmarkReportFlowTest.review();byte[] original=local.exactJsonBytes();
        StatisticsReportSendControls controls=StatisticsReportSendControls.benchmark(activity,panel(activity),work->fail("must not enqueue"),
            local,StatisticsReportSendControls.Services.unavailable());
        controls.consent.setChecked(true);controls.review.performClick();assertFalse(controls.send.isEnabled());
        assertEquals(activity.getString(R.string.statistics_send_unavailable),controls.status.getText().toString());
        assertTrue(controls.preview.getText().toString().contains("consentToPublish"));
        assertArrayEquals(original,local.exactJsonBytes());controls.send.performClick();assertEquals(PlaytestReportSubmission.State.READY,controls.submission.state());
        controls.close();activity.finish();
    }
    @Test public void privateCompatibilityUsesIndependentConsentPurposeAndCannotBecomePublic(){
        Activity activity=Robolectric.buildActivity(Activity.class).setup().get();
        DeveloperReportFlowTest.Executor worker=new DeveloperReportFlowTest.Executor();DeveloperReportFlowTest.LocalRelay relay=new DeveloperReportFlowTest.LocalRelay();
        relay.loseFirstReceipt=false;Verification verifier=new Verification();CompatibilityReportReview local=DeveloperReportFlowTest.review("Example model");
        byte[] original=local.exactJsonBytes();
        StatisticsReportSendControls controls=StatisticsReportSendControls.compatibility(activity,panel(activity),worker::execute,local,
            StatisticsReportSendControls.Services.localValidation(relay,verifier));
        controls.consent.setChecked(true);controls.review.performClick();
        assertTrue(controls.preview.getText().toString().contains("\"publishToBoard\":false"));
        assertFalse(controls.preview.getText().toString().contains("consentToPublish"));assertArrayEquals(original,local.exactJsonBytes());
        controls.send.performClick();assertEquals("horde_report",verifier.purposes.get(0));verifier.complete();idle();worker.drain();idle();
        assertEquals(PlaytestReportSubmission.State.QUEUED,controls.submission.state());assertFalse(controls.send.isEnabled());
        assertEquals(activity.getString(R.string.statistics_mock_private_queued),controls.status.getText().toString());
        controls.close();activity.finish();
    }
    @Test public void pendingAndPublishedReceiptsSurvivePauseAndLateUiCallback(){
        Activity activity=Robolectric.buildActivity(Activity.class).setup().get();
        PublicBenchmarkReportFlowTest.Worker worker=new PublicBenchmarkReportFlowTest.Worker();
        PublicBenchmarkReportFlowTest.PublicRelay relay=new PublicBenchmarkReportFlowTest.PublicRelay();Verification verifier=new Verification();
        StatisticsReportSendControls controls=StatisticsReportSendControls.benchmark(activity,panel(activity),worker::execute,
            PublicBenchmarkReportFlowTest.review(),StatisticsReportSendControls.Services.localValidation(relay,verifier));
        controls.consent.setChecked(true);controls.review.performClick();controls.send.performClick();verifier.complete();idle();worker.drain();idle();
        controls.interrupt();controls.resume();idle();
        assertEquals(PlaytestReportSubmission.State.QUEUED,controls.submission.state());
        assertEquals(activity.getString(R.string.statistics_mock_pending),controls.status.getText().toString());
        assertTrue(controls.send.isEnabled());assertEquals(1,relay.publicOpens);
        relay.next=PublicBenchmarkReportFlowTest.ack(200,"published","public-board","public-benchmark-result",PublicBenchmarkReportFlowTest.ID);
        controls.send.performClick();verifier.complete();idle();worker.drain(); // Receipt committed; main-loop notification has not run.
        assertEquals(PlaytestReportSubmission.State.PUBLISHED,controls.submission.state());
        controls.interrupt();controls.resume();idle();
        assertEquals(activity.getString(R.string.statistics_mock_published),controls.status.getText().toString());
        assertFalse(controls.send.isEnabled());assertEquals(2,relay.publicOpens);
        controls.close();activity.finish();
    }
    @Test public void privateReceiptAndUncertaintySurvivePauseBeforeUiDelivery(){
        int[] codes={202,200,409,422,503};
        PlaytestReportSubmission.State[] states={PlaytestReportSubmission.State.QUEUED,PlaytestReportSubmission.State.SENT,
            PlaytestReportSubmission.State.CONFLICT,PlaytestReportSubmission.State.REJECTED,PlaytestReportSubmission.State.RETRYABLE};
        int[] messages={R.string.statistics_mock_private_queued,R.string.statistics_mock_sent,R.string.statistics_mock_rejected,
            R.string.statistics_mock_rejected,R.string.statistics_mock_uncertain};
        for(int i=0;i<codes.length;i++){
            Activity activity=Robolectric.buildActivity(Activity.class).setup().get();final int code=codes[i];
            PublicBenchmarkReportFlowTest.Worker worker=new PublicBenchmarkReportFlowTest.Worker();Verification verifier=new Verification();
            PlaytestReportSubmission.Transport relay=new PlaytestReportSubmission.Transport(){
                public boolean admitsPrivateDeveloperReports(){return true;}
                public PlaytestReportSubmission.Call open(byte[] body){return new PlaytestReportSubmission.Call(){
                    public PlaytestReportSubmission.Response execute(){return PublicBenchmarkReportFlowTest.ack(code,code==200?"sent":"accepted",
                        "developer-private","compatibility",DeveloperReportFlowTest.ID);}
                    public void cancel(){java.util.Arrays.fill(body,(byte)0);}
                };}
            };
            StatisticsReportSendControls controls=StatisticsReportSendControls.compatibility(activity,panel(activity),worker::execute,
                DeveloperReportFlowTest.review("Example model"),StatisticsReportSendControls.Services.localValidation(relay,verifier));
            controls.consent.setChecked(true);controls.review.performClick();controls.send.performClick();verifier.complete();idle();worker.drain();
            assertEquals(states[i],controls.submission.state());
            controls.interrupt();controls.resume();idle();
            assertEquals(activity.getString(messages[i]),controls.status.getText().toString());
            assertEquals(code==503,controls.send.isEnabled());assertEquals(1,verifier.requests);
            controls.close();activity.finish();
        }
    }
}
