package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.FrameLayout;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
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
import org.robolectric.annotation.LooperMode;
import org.robolectric.shadows.ShadowToast;

/** Activity/native protocol boundaries only; no Vulkan or physical acceptance. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=OpeningBenchmarkMenuLifecycleTest.Bridge.class)
@LooperMode(LooperMode.Mode.PAUSED)
public final class OpeningBenchmarkMenuLifecycleTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class Bridge {
        static long[] entry,opening,output;
        static int requests,provisionalRequests,legacyCancels,plays,settingsWrites,starts;
        static boolean paused,accept;
        static final List<String> cancels=new ArrayList<>();
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static long[] getEntryMenuState(){return entry;}
        @Implementation protected static long requestOpeningBenchmark(long generation){++requests;return accept?7:0;}
        @Implementation protected static long requestOpeningProvisionalBenchmark(long generation){++provisionalRequests;return accept?7:0;}
        @Implementation protected static void cancelOpeningBenchmark(long request,long origin){cancels.add(request+":"+origin);}
        @Implementation protected static long[] getOpeningBenchmarkState(){return opening;}
        @Implementation protected static long[] getOpeningBenchmarkOutputState(){return output;}
        @Implementation protected static int getSurfaceRuntimeState(long generation){return 1;}
        @Implementation protected static String getBenchmarkProgress(){return "1 / 2";}
        @Implementation protected static String getBenchmarkReport(){return "Custom, unranked, local report";}
        @Implementation protected static String getReadyBenchmarkSummaryRunId(){return "";}
        @Implementation protected static void cancelBenchmark(){++legacyCancels;}
        @Implementation protected static void setEntryMenu(boolean enabled,boolean side,boolean reduced,boolean play,long generation){if(play)++plays;}
        @Implementation protected static boolean setDiagnosticSurfaceSuspended(long generation,boolean suspended){return true;}
        @Implementation protected static void stopDiagnosticSurface(long generation){}
        @Implementation protected static void setSimulationPaused(boolean value){paused=value;}
        @Implementation protected static int getContextualControlState(){return 0;}
        @Implementation protected static boolean isRtLabUnlockEligible(){return false;}
        @Implementation protected static void setViewControls(float yaw,float pitch,float light,float x,float z,boolean crouch,boolean analogueMovement){}
        @Implementation protected static void resetRtSceneTuning(){++settingsWrites;}
        @Implementation protected static void setCombatTeachingOptions(boolean enabled,boolean slowdown){++settingsWrites;}
        @Implementation protected static long setGraphicsSettings(int scale,int water,int fire,int cap,boolean glass,int shadow,boolean mist,int dust){++settingsWrites;return 1;}
        @Implementation protected static long startDiagnosticSurface(android.view.Surface surface,String root){++starts;return 42;}
    }
    private MainActivity activity;
    private Field field(String name)throws Exception {Field f=MainActivity.class.getDeclaredField(name);f.setAccessible(true);return f;}
    private void set(String name,Object value)throws Exception {field(name).set(activity,value);}
    private Object call(String name)throws Exception {Method m=MainActivity.class.getDeclaredMethod(name);m.setAccessible(true);return m.invoke(activity);}
    private Object call(String name,boolean value)throws Exception {Method m=MainActivity.class.getDeclaredMethod(name,boolean.class);m.setAccessible(true);return m.invoke(activity,value);}
    private Object callBundle(String name,Bundle value)throws Exception {Method m=MainActivity.class.getDeclaredMethod(name,Bundle.class);m.setAccessible(true);return m.invoke(activity,value);}
    @Before public void prepare()throws Exception {
        Bridge.entry=new long[]{31,2,0,0,1}; Bridge.opening=new long[]{7,31,1,0,0,31};Bridge.output=new long[0];
        Bridge.requests=Bridge.provisionalRequests=Bridge.legacyCancels=Bridge.plays=0;Bridge.paused=false;Bridge.accept=true;Bridge.cancels.clear();
        Bridge.settingsWrites=Bridge.starts=0;field("openingBenchmarkClientFallback").set(null,null);
        prepareViews();
    }
    private void prepareViews()throws Exception {
        activity=Robolectric.buildActivity(MainActivity.class).get();
        activity.setContentView(LayoutInflater.from(activity).inflate(R.layout.activity_main,null));
        set("preferences",activity.getSharedPreferences("opening-benchmark-menu",Context.MODE_PRIVATE));
        set("graphicsConfirmed",GraphicsPreferences.mobileDefaults());
        set("entryMenuEnabled",true);set("entryMenuSidePage",true);set("menuVisible",true);set("resumed",true);
        set("surfaceRequestGeneration",31L);set("surfaceStarted",true);set("startupUpdateCheckCompleted",true);
        String[] names={"surfaceView","menuScrim","menuButton","attackButton","parryButton","dodgeButton","runButton","crouchButton",
                "interactButton","toggleHeldLightPoseButton","diagnosticsPanel","developerOverlay","vitalityStatus","rtStatus","controllerPrompt"};
        int[] ids={R.id.scene_surface,R.id.menu_scrim,R.id.menu_button,R.id.attack_button,R.id.parry_button,R.id.dodge_button,R.id.run_button,R.id.crouch_button,
                R.id.interact_button,R.id.toggle_held_light_pose_button,R.id.diagnostics_panel,R.id.developer_overlay,R.id.vitality_status,R.id.rt_status,R.id.controller_prompt};
        for(int i=0;i<names.length;i++)set(names[i],activity.findViewById(ids[i]));
    }
    private void replace(Bundle saved)throws Exception {
        ((ExecutorService)field("reportExecutor").get(activity)).shutdownNow();
        ((ExecutorService)field("updateExecutor").get(activity)).shutdownNow();
        prepareViews();set("surfaceRequestGeneration",0L);set("surfaceStarted",false);set("resumed",false);
        callBundle("selectOpeningBenchmarkRecovery",saved);call("recoverOpeningBenchmarkClient");
    }
    @After public void finish()throws Exception {
        ((ExecutorService)field("reportExecutor").get(activity)).shutdownNow();
        ((ExecutorService)field("updateExecutor").get(activity)).shutdownNow();
        field("openingBenchmarkClientFallback").set(null,null);
    }
    private void start()throws Exception {call("requestOpeningCustomBenchmark");assertEquals(7L,field("openingBenchmarkRequest").getLong(activity));}
    private boolean hasBenchmarkButton(View view) {
        if(view instanceof Button && ((Button)view).getText().toString().toLowerCase(java.util.Locale.ROOT).contains("benchmark")) return true;
        if(view instanceof ViewGroup)for(int i=0;i<((ViewGroup)view).getChildCount();i++)
            if(hasBenchmarkButton(((ViewGroup)view).getChildAt(i))) return true;
        return false;
    }
    @Test public void entryMoreActionPublishesCustomRequestThroughActualButton()throws Exception {
        call("showEntryMorePage");
        Button action=(Button)field("openingBenchmarkButton").get(activity);
        assertEquals("Custom benchmark (unranked)",action.getText().toString());assertTrue(action.isEnabled());
        action.performClick();
        assertEquals(1,Bridge.requests);assertEquals(7L,field("openingBenchmarkRequest").getLong(activity));
        assertEquals(0,Bridge.plays);
    }
    @Test public void ordinaryActiveGameMenuHasNoInteractiveBenchmarkAction()throws Exception {
        set("entryMenuEnabled",false);set("firstMenu",false);call("showMainMenu",false);
        assertFalse(hasBenchmarkButton((View)field("menuScrim").get(activity)));
        call("requestOpeningCustomBenchmark");assertEquals(0,Bridge.requests);
    }
    @Test public void provisionalHasSeparateActionAndUnavailableNeverFallsBackToCustom()throws Exception {
        call("showEntryMorePage");
        Button action=(Button)field("openingBenchmarkProvisionalButton").get(activity);
        assertEquals("Mobile provisional benchmark (unranked)",action.getText().toString());
        action.performClick();
        assertEquals(1,Bridge.provisionalRequests);assertEquals(0,Bridge.requests);
        assertTrue(field("openingBenchmarkProvisional").getBoolean(activity));
        Bridge.opening=new long[]{7,31,5,4,1,31};call("pollOpeningBenchmark");
        assertEquals(0,Bridge.requests);assertFalse(field("benchmarkRunning").getBoolean(activity));
        assertFalse(field("benchmarkReportVisible").getBoolean(activity));
        assertEquals("Provisional unavailable; choose Custom.",ShadowToast.getTextOfLatestToast());
    }
    @Test public void onlyCurrentPresentedEntryCanPublishInteractiveRequest()throws Exception {
        Bridge.entry=new long[]{30,2,0,0,1};call("requestOpeningCustomBenchmark");assertEquals(0,Bridge.requests);
        Bridge.entry=new long[]{31,0,0,0,1};call("requestOpeningCustomBenchmark");assertEquals(0,Bridge.requests);
        Bridge.entry=new long[]{31,2,0,0,0};call("requestOpeningCustomBenchmark");assertEquals(0,Bridge.requests);
        Bridge.entry=new long[]{31,2,0,0,1};set("entryPlayRequested",true);
        call("requestOpeningCustomBenchmark");assertEquals(0,Bridge.requests);
        set("entryPlayRequested",false);start();
        assertEquals(1,Bridge.requests);assertEquals(0,Bridge.plays);assertFalse(Bridge.paused);
        assertTrue(field("entryMenuEnabled").getBoolean(activity));assertTrue(field("benchmarkRunning").getBoolean(activity));
        assertFalse(field("entryPlayRequested").getBoolean(activity));
    }
    @Test public void backAndPauseCancelSameOwnerWithoutUnpausingOrReleasingUi()throws Exception {
        start();activity.onBackPressed();
        assertTrue(Bridge.cancels.contains("7:31"));assertTrue(field("openingBenchmarkCancelRequested").getBoolean(activity));
        call("onPause");
        assertEquals(0,Bridge.legacyCancels);assertTrue(field("benchmarkRunning").getBoolean(activity));
        assertEquals(7L,field("openingBenchmarkRequest").getLong(activity));assertTrue(Bridge.paused);
        for(String cancellation:Bridge.cancels)assertEquals("7:31",cancellation);
    }
    @Test public void terminalCourseResultWaitsForCurrentReturnedEntry()throws Exception {
        start();set("surfaceRequestGeneration",42L);
        for(long[] state:new long[][]{{7,42,4,2,0,31},{7,31,5,2,1,31},{7,42,5,2,0,31},{8,42,5,2,1,31}}) {
            Bridge.opening=state;call("pollOpeningBenchmark");
            assertTrue(field("benchmarkRunning").getBoolean(activity));assertFalse(field("benchmarkReportVisible").getBoolean(activity));
        }
        Bridge.opening=new long[]{7,42,5,2,1,31};call("pollOpeningBenchmark");
        assertFalse(field("benchmarkRunning").getBoolean(activity));assertEquals(0L,field("openingBenchmarkRequest").getLong(activity));
        assertTrue(field("benchmarkReportVisible").getBoolean(activity));assertTrue(field("entryMenuEnabled").getBoolean(activity));
        assertEquals(0,Bridge.plays);
    }
    @Test public void surfaceLossKeepsOriginCancellationAndCannotBorrowOldEntry()throws Exception {
        start();call("stopSurface");
        assertEquals(0L,field("surfaceRequestGeneration").getLong(activity));
        assertEquals(7L,field("openingBenchmarkRequest").getLong(activity));
        assertTrue(Bridge.cancels.contains("7:31"));assertTrue(field("benchmarkRunning").getBoolean(activity));
        Bridge.opening=new long[]{7,31,5,3,1,31};call("pollOpeningBenchmark");
        assertTrue(field("benchmarkRunning").getBoolean(activity));assertFalse(field("benchmarkReportVisible").getBoolean(activity));
    }
    @Test public void competingMenuActionsAndBlockedOwnerCannotExposeGameplay()throws Exception {
        start();call("showSettings");call("requestEntryPlay");call("beginPreparedEntryPlay");call("hideMenu");
        assertFalse(field("entryPlayRequested").getBoolean(activity));assertFalse(field("menuVisible").getBoolean(activity));
        assertEquals(0,Bridge.plays);
        Bridge.opening=new long[]{7,31,6,3,0,31};call("pollOpeningBenchmark");
        assertTrue(field("openingBenchmarkBlocked").getBoolean(activity));assertTrue(field("benchmarkRunning").getBoolean(activity));
        assertFalse(field("benchmarkReportVisible").getBoolean(activity));
        assertTrue(((TextView)field("rtStatus").get(activity)).getText().toString().contains("Could not return"));
    }
    @Test public void savedCustomActivityRecoversReturningOwnerAsCancelOnlyAndWaitsForNewEntry()throws Exception {
        start();Bundle saved=new Bundle();callBundle("onSaveInstanceState",saved);
        Bridge.opening=new long[]{7,31,4,3,0,31};replace(saved);
        assertEquals(7L,field("openingBenchmarkRequest").getLong(activity));
        assertTrue(field("openingBenchmarkRecovered").getBoolean(activity));
        assertTrue(field("openingBenchmarkCancelRequested").getBoolean(activity));assertTrue(Bridge.cancels.contains("7:31"));
        assertNull(field("openingBenchmarkOutput").get(activity));assertEquals(0,Bridge.settingsWrites);
        set("surfaceRequestGeneration",42L);Bridge.opening=new long[]{7,42,4,3,0,31};call("pollOpeningBenchmark");
        assertTrue(field("benchmarkRunning").getBoolean(activity));
        Bridge.opening=new long[]{7,42,5,3,1,31};call("pollOpeningBenchmark");
        assertFalse(field("benchmarkRunning").getBoolean(activity));assertFalse(field("benchmarkReportVisible").getBoolean(activity));
        assertNull(field("openingBenchmarkClientFallback").get(null));
    }
    @Test public void noBundleSameProcessRecoveryUsesImmutableRequestFallback()throws Exception {
        call("requestOpeningProvisionalBenchmark");
        OpeningBenchmarkClient retained=(OpeningBenchmarkClient)field("openingBenchmarkClientFallback").get(null);
        assertNotNull(retained);FrameLayout.LayoutParams original=retained.originalLayout();
        ((android.view.SurfaceView)field("surfaceView").get(activity)).setLayoutParams(new FrameLayout.LayoutParams(200,113));
        Bridge.opening=new long[]{7,31,4,3,0,31};replace(null);
        assertEquals(7L,field("openingBenchmarkRequest").getLong(activity));assertTrue(field("openingBenchmarkProvisional").getBoolean(activity));
        OpeningBenchmarkOutput output=(OpeningBenchmarkOutput)field("openingBenchmarkOutput").get(activity);
        assertNotNull(output);assertFalse(output.canRelease());assertTrue(OpeningBenchmarkOutput.sameLayout(output.originalLayout(),original));
        assertTrue(field("openingBenchmarkCancelRequested").getBoolean(activity));assertEquals(0,Bridge.settingsWrites);
    }
    @Test public void completedOldGenerationRequiresActualCurrentEntryWithoutRewritingNativeReceipt()throws Exception {
        start();Bundle saved=new Bundle();callBundle("onSaveInstanceState",saved);
        Bridge.opening=new long[]{7,31,5,2,1,31};replace(saved);set("surfaceRequestGeneration",42L);
        Bridge.entry=new long[]{31,2,0,0,1};call("pollOpeningBenchmark");assertTrue(field("benchmarkRunning").getBoolean(activity));
        Bridge.entry=new long[]{42,0,0,0,1};call("pollOpeningBenchmark");assertTrue(field("benchmarkRunning").getBoolean(activity));
        Bridge.entry=new long[]{42,2,0,0,0};call("pollOpeningBenchmark");assertTrue(field("benchmarkRunning").getBoolean(activity));
        Bridge.entry=new long[]{42,2,0,0,1};call("pollOpeningBenchmark");assertFalse(field("benchmarkRunning").getBoolean(activity));
        assertFalse(field("benchmarkReportVisible").getBoolean(activity));assertEquals(31,Bridge.opening[1]);assertEquals(0,Bridge.plays);
    }
    @Test public void completionRaceAfterNewBindOnlyObservesOriginalLayoutAndDoesNotResizeIt()throws Exception {
        start();Bundle saved=new Bundle();callBundle("onSaveInstanceState",saved);
        Bridge.opening=new long[]{7,31,4,3,0,31};replace(saved);set("surfaceRequestGeneration",42L);
        View surface=(View)field("surfaceView").get(activity);ViewGroup.LayoutParams untouched=surface.getLayoutParams();
        Bridge.entry=new long[]{42,2,0,0,0};Bridge.opening=new long[]{7,31,5,3,1,31};call("pollOpeningBenchmark");
        assertSame(untouched,surface.getLayoutParams());assertTrue(field("benchmarkRunning").getBoolean(activity));
        assertTrue(field("openingBenchmarkRecoveredComplete").getBoolean(activity));assertEquals(31,Bridge.opening[1]);
        Bridge.entry=new long[]{42,2,0,0,1};call("pollOpeningBenchmark");assertSame(untouched,surface.getLayoutParams());
        assertFalse(field("benchmarkRunning").getBoolean(activity));assertFalse(field("benchmarkReportVisible").getBoolean(activity));
    }
    @Test public void freshNativeIdleRestoresOriginalPolicyAndDiscardsWithoutCancellation()throws Exception {
        call("requestOpeningProvisionalBenchmark");Bundle saved=new Bundle();callBundle("onSaveInstanceState",saved);
        OpeningBenchmarkClient original=OpeningBenchmarkClient.restore(saved);assertNotNull(original);
        Bridge.opening=new long[]{0,0,0,0,0,0};Bridge.cancels.clear();field("openingBenchmarkClientFallback").set(null,null);
        replace(saved);
        assertEquals(0L,field("openingBenchmarkRequest").getLong(activity));assertFalse(field("benchmarkRunning").getBoolean(activity));
        assertTrue(Bridge.cancels.isEmpty());assertNull(field("openingBenchmarkClientFallback").get(null));
        assertTrue(OpeningBenchmarkOutput.sameLayout(((View)field("surfaceView").get(activity)).getLayoutParams(),original.originalLayout()));
    }
    @Test public void differentLiveRequestCannotBeCancelledResizedOrStartedByStaleClient()throws Exception {
        call("requestOpeningProvisionalBenchmark");Bundle saved=new Bundle();callBundle("onSaveInstanceState",saved);
        Bridge.opening=new long[]{8,88,3,0,0,88};Bridge.cancels.clear();replace(saved);
        View surface=(View)field("surfaceView").get(activity);ViewGroup.LayoutParams untouched=surface.getLayoutParams();
        assertTrue(field("openingBenchmarkRecoveryRejected").getBoolean(activity));assertEquals(0L,field("openingBenchmarkRequest").getLong(activity));
        set("surfaceAvailable",true);call("cancelOpeningBenchmarkRequest");call("startSurfaceIfReady");call("showMainMenu",false);
        assertSame(untouched,surface.getLayoutParams());assertTrue(Bridge.cancels.isEmpty());assertEquals(0,Bridge.starts);assertEquals(0,Bridge.settingsWrites);
        assertTrue(((TextView)field("rtStatus").get(activity)).getText().toString().contains("No surface or other run was changed"));
    }
    @Test public void matchingNewerFallbackWinsOverStaleBundle()throws Exception {
        start();Bundle saved=new Bundle();callBundle("onSaveInstanceState",saved);
        field("openingBenchmarkClientFallback").set(null,new OpeningBenchmarkClient(8,88,false,null));
        Bridge.opening=new long[]{8,88,4,3,0,88};Bridge.cancels.clear();replace(saved);
        assertEquals(8L,field("openingBenchmarkRequest").getLong(activity));assertTrue(Bridge.cancels.contains("8:88"));
        assertFalse(Bridge.cancels.contains("7:31"));assertFalse(field("openingBenchmarkRecoveryRejected").getBoolean(activity));
    }
}
