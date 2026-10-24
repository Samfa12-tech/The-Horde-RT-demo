package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
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

/** Activity/native protocol boundaries only; no Vulkan or physical acceptance. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=OpeningBenchmarkMenuLifecycleTest.Bridge.class)
@LooperMode(LooperMode.Mode.PAUSED)
public final class OpeningBenchmarkMenuLifecycleTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class Bridge {
        static long[] entry,opening;
        static int requests,legacyCancels,plays;
        static boolean paused,accept;
        static final List<String> cancels=new ArrayList<>();
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static long[] getEntryMenuState(){return entry;}
        @Implementation protected static long requestOpeningBenchmark(long generation){++requests;return accept?7:0;}
        @Implementation protected static void cancelOpeningBenchmark(long request,long origin){cancels.add(request+":"+origin);}
        @Implementation protected static long[] getOpeningBenchmarkState(){return opening;}
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
    }
    private MainActivity activity;
    private Field field(String name)throws Exception {Field f=MainActivity.class.getDeclaredField(name);f.setAccessible(true);return f;}
    private void set(String name,Object value)throws Exception {field(name).set(activity,value);}
    private Object call(String name)throws Exception {Method m=MainActivity.class.getDeclaredMethod(name);m.setAccessible(true);return m.invoke(activity);}
    private Object call(String name,boolean value)throws Exception {Method m=MainActivity.class.getDeclaredMethod(name,boolean.class);m.setAccessible(true);return m.invoke(activity,value);}
    @Before public void prepare()throws Exception {
        Bridge.entry=new long[]{31,2,0,0,1}; Bridge.opening=new long[]{7,31,1,0,0,31};
        Bridge.requests=Bridge.legacyCancels=Bridge.plays=0;Bridge.paused=false;Bridge.accept=true;Bridge.cancels.clear();
        activity=Robolectric.buildActivity(MainActivity.class).get();
        activity.setContentView(LayoutInflater.from(activity).inflate(R.layout.activity_main,null));
        set("preferences",activity.getSharedPreferences("opening-benchmark-menu",Context.MODE_PRIVATE));
        set("entryMenuEnabled",true);set("entryMenuSidePage",true);set("menuVisible",true);set("resumed",true);
        set("surfaceRequestGeneration",31L);set("surfaceStarted",true);set("startupUpdateCheckCompleted",true);
        String[] names={"surfaceView","menuScrim","menuButton","attackButton","parryButton","dodgeButton","runButton","crouchButton",
                "interactButton","toggleHeldLightPoseButton","diagnosticsPanel","developerOverlay","vitalityStatus","rtStatus","controllerPrompt"};
        int[] ids={R.id.scene_surface,R.id.menu_scrim,R.id.menu_button,R.id.attack_button,R.id.parry_button,R.id.dodge_button,R.id.run_button,R.id.crouch_button,
                R.id.interact_button,R.id.toggle_held_light_pose_button,R.id.diagnostics_panel,R.id.developer_overlay,R.id.vitality_status,R.id.rt_status,R.id.controller_prompt};
        for(int i=0;i<names.length;i++)set(names[i],activity.findViewById(ids[i]));
    }
    @After public void finish()throws Exception {
        ((ExecutorService)field("reportExecutor").get(activity)).shutdownNow();
        ((ExecutorService)field("updateExecutor").get(activity)).shutdownNow();
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
}
