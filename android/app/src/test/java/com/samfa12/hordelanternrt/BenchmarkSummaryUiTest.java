package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.app.Activity;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.Spinner;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.concurrent.ExecutorService;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=BenchmarkSummaryUiTest.SummaryBridgeShadow.class)
public final class BenchmarkSummaryUiTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class SummaryBridgeShadow {
        static String runId;
        static int calls, legacyCalls;
        static boolean summaryRequestReady;
        static byte[] lastPrepared;
        @Implementation protected static void __staticInitializer__() { }
        @Implementation protected static String getReadyBenchmarkSummaryRunId() { return runId; }
        @Implementation protected static byte[] prepareBenchmarkSummaryReport(String run,String report,String utc,
                boolean consent,boolean hardware,int cooling) {
            ++calls;
            if(!run.equals(runId)) return new byte[]{2};
            lastPrepared=BenchmarkSummaryReviewTest.prepared(run,report,hardware,cooling);
            return lastPrepared;
        }
        @Implementation protected static boolean requestBenchmarkWithSummaryId(String run,String model) {
            assertTrue(BenchmarkSummaryReview.validUuid(run)); return summaryRequestReady;
        }
        @Implementation protected static boolean requestBenchmark() { ++legacyCalls; return true; }
    }

    private MainActivity activity;
    private FrameLayout scrim;
    @Before public void setUp() throws Exception {
        activity=Robolectric.buildActivity(MainActivity.class).get();
        scrim=new FrameLayout(activity);
        activity.setContentView(scrim);
        ((ClipboardManager)activity.getSystemService(Context.CLIPBOARD_SERVICE)).clearPrimaryClip();
        set("menuScrim",scrim);
        SummaryBridgeShadow.runId=BenchmarkSummaryReviewTest.RUN;
        SummaryBridgeShadow.calls=0; SummaryBridgeShadow.legacyCalls=0;
        SummaryBridgeShadow.summaryRequestReady=true; SummaryBridgeShadow.lastPrepared=null;
    }
    @After public void tearDown() throws Exception {
        call("closeBenchmarkSummaryReview");
        get("reportExecutor",ExecutorService.class).shutdownNow();
        get("updateExecutor",ExecutorService.class).shutdownNow();
    }
    private void set(String name,Object value) throws Exception {
        Field field=MainActivity.class.getDeclaredField(name); field.setAccessible(true); field.set(activity,value);
    }
    private <T> T get(String name,Class<T> type) throws Exception {
        Field field=MainActivity.class.getDeclaredField(name); field.setAccessible(true); return type.cast(field.get(activity));
    }
    private Object call(String name) throws Exception {
        Method method=MainActivity.class.getDeclaredMethod(name); method.setAccessible(true); return method.invoke(activity);
    }
    private void prepare() throws Exception {
        get("benchmarkSummaryConsent",CheckBox.class).setChecked(true);
        get("benchmarkSummaryPrepare",Button.class).performClick();
    }

    @Test public void nativeFormStartsWithoutConsentAndRemainsAccessibleAtLargeFonts() throws Exception {
        for(float font:new float[]{1f,1.7f,2f}) {
            Configuration config=new Configuration(activity.getResources().getConfiguration());
            config.fontScale=font;
            activity.getResources().updateConfiguration(config,activity.getResources().getDisplayMetrics());
            call("showBenchmarkSummaryReview");
            assertFalse(get("benchmarkSummaryConsent",CheckBox.class).isChecked());
            assertFalse(get("benchmarkSummaryHardware",CheckBox.class).isChecked());
            assertEquals(0,get("benchmarkSummaryCooling",Spinner.class).getSelectedItemPosition());
            assertFalse(get("benchmarkSummaryPrepare",Button.class).isEnabled());
            scrim.measure(View.MeasureSpec.makeMeasureSpec(1080,View.MeasureSpec.EXACTLY),
                    View.MeasureSpec.makeMeasureSpec(1800,View.MeasureSpec.EXACTLY));
            scrim.layout(0,0,1080,1800);
            int minimum=Math.round(48*activity.getResources().getDisplayMetrics().density);
            assertTrue(get("benchmarkSummaryConsent",CheckBox.class).getMeasuredHeight()>=minimum);
            assertTrue(get("benchmarkSummaryHardware",CheckBox.class).getMeasuredHeight()>=minimum);
            assertTrue(get("benchmarkSummaryCooling",Spinner.class).getMeasuredHeight()>=minimum);
            assertTrue(get("benchmarkSummaryPrepare",Button.class).getMeasuredHeight()>=minimum);
            assertEquals(0,SummaryBridgeShadow.calls);
            assertNull(get("playtestSubmission",PlaytestReportSubmission.class));
            assertFalse(hasSendButton(scrim));
        }
    }

    @Test public void preparationLocksChoicesAndExplicitCopyUsesLiteralNativeJson() throws Exception {
        call("showBenchmarkSummaryReview"); prepare();
        BenchmarkSummaryReview owner=get("benchmarkSummaryReview",BenchmarkSummaryReview.class);
        assertTrue(owner.isPrepared());
        assertFalse(get("benchmarkSummaryConsent",CheckBox.class).isEnabled());
        assertFalse(get("benchmarkSummaryHardware",CheckBox.class).isEnabled());
        assertFalse(get("benchmarkSummaryCooling",Spinner.class).isEnabled());
        assertEquals(owner.json(),get("benchmarkSummaryJson",TextView.class).getText().toString());
        assertArrayEquals(java.util.Arrays.copyOfRange(SummaryBridgeShadow.lastPrepared,1,
                SummaryBridgeShadow.lastPrepared.length),owner.exactJsonBytes());
        ClipboardManager clipboard=(ClipboardManager)activity.getSystemService(Context.CLIPBOARD_SERVICE);
        assertFalse(clipboard.hasPrimaryClip());
        get("benchmarkSummaryCopy",Button.class).performClick();
        assertEquals(owner.json(),clipboard.getPrimaryClip().getItemAt(0).getText().toString());
        assertEquals(1,SummaryBridgeShadow.calls);
        assertNull(get("playtestSubmission",PlaytestReportSubmission.class));
    }

    @Test public void editingRequiresFreshConsentAndReportIdButKeepsImmutableRun() throws Exception {
        call("showBenchmarkSummaryReview"); prepare();
        BenchmarkSummaryReview old=get("benchmarkSummaryReview",BenchmarkSummaryReview.class);
        String id=old.reportId();
        Button oldCopy=get("benchmarkSummaryCopy",Button.class);
        CheckBox oldConsent=get("benchmarkSummaryConsent",CheckBox.class);
        get("benchmarkSummaryEdit",Button.class).performClick();
        assertNull(old.json());
        assertFalse(get("benchmarkSummaryConsent",CheckBox.class).isChecked());
        assertFalse(get("benchmarkSummaryHardware",CheckBox.class).isChecked());
        assertFalse(get("benchmarkSummaryPrepare",Button.class).isEnabled());
        oldConsent.setChecked(false); oldConsent.setChecked(true);
        oldCopy.performClick(); // Detached controls cannot authorize a replacement form.
        assertFalse(get("benchmarkSummaryPrepare",Button.class).isEnabled());
        prepare();
        BenchmarkSummaryReview next=get("benchmarkSummaryReview",BenchmarkSummaryReview.class);
        assertEquals(old.runId(),next.runId());
        assertNotEquals(id,next.reportId());
        assertEquals(2,SummaryBridgeShadow.calls);
    }

    @Test public void oldPickerResultCannotSaveNewFormAndDrainsBeforeNextPicker() throws Exception {
        call("showBenchmarkSummaryReview"); prepare();
        get("benchmarkSummarySave",Button.class).performClick();
        BenchmarkSummaryReview old=get("benchmarkSummaryReview",BenchmarkSummaryReview.class);
        assertEquals(PlaytestReportExport.State.CHOOSING,old.exportOwner().state());
        call("closeBenchmarkSummaryReview");
        call("showBenchmarkSummaryReview"); prepare();
        BenchmarkSummaryReview next=get("benchmarkSummaryReview",BenchmarkSummaryReview.class);
        get("benchmarkSummarySave",Button.class).performClick();
        assertNull(next.exportOwner()); // Outstanding platform picker blocks a replacement launch.
        Method finish=MainActivity.class.getDeclaredMethod("finishBenchmarkSummaryPicker",int.class,Intent.class);
        finish.setAccessible(true);
        finish.invoke(activity,Activity.RESULT_OK,new Intent().setData(Uri.parse("content://approved/stale")));
        assertNull(next.exportOwner());
        assertEquals(PlaytestReportExport.State.CANCELLED,old.exportOwner().state());
        get("benchmarkSummarySave",Button.class).performClick();
        assertEquals(PlaytestReportExport.State.CHOOSING,next.exportOwner().state());
        finish.invoke(activity,Activity.RESULT_CANCELED,null);
        assertEquals(PlaytestReportExport.State.RETRYABLE,next.exportOwner().state());
        String id=next.reportId();
        get("benchmarkSummarySave",Button.class).performClick();
        assertEquals(id,next.reportId());
        assertEquals(2,SummaryBridgeShadow.calls); // Retry never re-prepares native bytes.
    }

    @Test public void unavailableSummaryCannotOpenFormOrBlockOrdinaryBenchmark() throws Exception {
        SummaryBridgeShadow.runId="";
        call("showBenchmarkSummaryReview");
        assertNull(get("benchmarkSummaryReview",BenchmarkSummaryReview.class));
        assertFalse(get("benchmarkSummaryVisible",Boolean.class));
        SummaryBridgeShadow.summaryRequestReady=false;
        assertEquals(Boolean.TRUE,call("requestInteractiveBenchmark"));
        assertEquals(1,SummaryBridgeShadow.legacyCalls);
        SummaryBridgeShadow.summaryRequestReady=true;
        assertEquals(Boolean.TRUE,call("requestInteractiveBenchmark"));
        assertEquals(1,SummaryBridgeShadow.legacyCalls);
    }

    private static boolean hasSendButton(View view) {
        if(view instanceof Button && ((Button)view).getText().toString().toLowerCase(java.util.Locale.ROOT).contains("send")) return true;
        if(view instanceof ViewGroup) {
            ViewGroup group=(ViewGroup)view;
            for(int i=0;i<group.getChildCount();++i) if(hasSendButton(group.getChildAt(i))) return true;
        }
        return false;
    }
}
