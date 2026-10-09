package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.Surface;
import android.graphics.SurfaceTexture;
import android.view.View;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

/** Real Activity callbacks with a fake native lifecycle, no Vulkan or device claim. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34, shadows=SurfaceSuspensionLifecycleTest.Bridge.class)
public final class SurfaceSuspensionLifecycleTest {
    @Implements(value=ProbeBridge.class, isInAndroidSdk=false)
    public static final class Bridge {
        static boolean accept, fail, paused;
        static int starts, runtime;
        static float strafe, forward;
        static final List<Long> stops=new ArrayList<>();
        static final List<String> suspensions=new ArrayList<>();
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static boolean setDiagnosticSurfaceSuspended(long generation, boolean suspended) {
            suspensions.add(generation+":"+suspended);
            if(fail) throw new IllegalStateException("rejected native request");
            return accept;
        }
        @Implementation protected static void stopDiagnosticSurface(long generation) { stops.add(generation); }
        @Implementation protected static long startDiagnosticSurface(Surface surface,String directory) { ++starts; return 42L; }
        @Implementation protected static int getSurfaceRuntimeState(long generation) { return runtime; }
        @Implementation protected static void setSimulationPaused(boolean value) { paused=value; }
        @Implementation protected static void setViewControls(float yaw,float pitch,float light,float x,float z) { strafe=x; forward=z; }
        @Implementation protected static void setEntryMenu(boolean enabled,boolean side,boolean reduced,boolean play,long generation) {}
    }
    private MainActivity a;
    private static Field field(String name)throws Exception {
        Field f=MainActivity.class.getDeclaredField(name); f.setAccessible(true); return f;
    }
    private void set(String name,Object value)throws Exception { field(name).set(a,value); }
    private void call(String name)throws Exception {
        Method m=MainActivity.class.getDeclaredMethod(name); m.setAccessible(true); m.invoke(a);
    }
    @Before public void prepare()throws Exception {
        Bridge.accept=true; Bridge.fail=false; Bridge.paused=false; Bridge.starts=0; Bridge.runtime=1;
        Bridge.strafe=Bridge.forward=0; Bridge.stops.clear(); Bridge.suspensions.clear();
        a=Robolectric.buildActivity(MainActivity.class).get();
        a.setContentView(LayoutInflater.from(a).inflate(R.layout.activity_main,null));
        set("preferences",a.getSharedPreferences("surface-lifecycle",Context.MODE_PRIVATE));
        set("resumed",true); set("menuVisible",false); set("startupUpdateCheckCompleted",true);
        set("surfaceAvailable",true); set("surfaceStarted",true); set("surfaceRequestGeneration",31L);
        set("currentSurface",new Surface(new SurfaceTexture(0)));
        String[] names={"rtStatus","interactButton","toggleHeldLightPoseButton","parryButton"};
        int[] ids={R.id.rt_status,R.id.interact_button,R.id.toggle_held_light_pose_button,R.id.parry_button};
        for(int i=0;i<names.length;i++)set(names[i],a.findViewById(ids[i]));
    }
    @Test public void briefPauseClearsTouchAndRetainsSamePresentedGeneration()throws Exception {
        int[] pointers=(int[])field("activePointers").get(a); pointers[0]=3; pointers[1]=7;
        float[] controls=(float[])field("viewControls").get(a); controls[7]=0.5f; controls[8]=0.75f;
        set("parryTouchActive",true); ((View)field("parryButton").get(a)).setPressed(true);
        call("onPause");
        assertTrue(Bridge.paused); assertFalse(field("resumed").getBoolean(a));
        assertEquals(31L,field("surfaceRequestGeneration").getLong(a));
        assertTrue(field("surfaceStarted").getBoolean(a));
        assertArrayEquals(new int[]{-1,-1},pointers); assertEquals(0,Bridge.strafe,0); assertEquals(0,Bridge.forward,0);
        assertFalse(field("parryTouchActive").getBoolean(a));
        call("onResume");
        assertEquals(31L,field("surfaceRequestGeneration").getLong(a));
        assertEquals(List.of("31:true","31:false"),Bridge.suspensions);
        assertTrue(Bridge.stops.isEmpty()); assertEquals(0,Bridge.starts); assertFalse(Bridge.paused);
    }
    @Test public void retainedResumeKeepsMenuAndDiagnosticsPaused()throws Exception {
        for(String overlay:new String[]{"menuVisible","diagnosticsVisible"}) {
            set(overlay,true); call("onPause"); call("onResume");
            assertTrue(Bridge.paused); assertEquals(31L,field("surfaceRequestGeneration").getLong(a));
            set(overlay,false);
        }
        assertEquals(0,Bridge.starts); assertTrue(Bridge.stops.isEmpty());
    }
    @Test public void pendingColdStartRetainsTokenWithoutInventingReadiness()throws Exception {
        set("surfaceStarted",false); Bridge.runtime=0;
        call("onPause"); call("onResume");
        assertEquals(31L,field("surfaceRequestGeneration").getLong(a));
        assertFalse(field("surfaceStarted").getBoolean(a)); assertEquals(0,Bridge.starts);
    }
    @Test public void fullStopRetiresOnceAndResumeRequestsFreshGeneration()throws Exception {
        call("onPause"); call("onStop"); call("onStop");
        assertEquals(List.of(31L),Bridge.stops); assertEquals(0L,field("surfaceRequestGeneration").getLong(a));
        assertFalse(field("surfaceStarted").getBoolean(a));
        call("onResume");
        assertEquals(1,Bridge.starts); assertEquals(42L,field("surfaceRequestGeneration").getLong(a));
        assertFalse(field("surfaceStarted").getBoolean(a));
    }
    @Test public void rejectedOrFailedSuspensionFallsBackToFullRetirement()throws Exception {
        for(boolean fail:new boolean[]{false,true}) {
            prepare(); Bridge.accept=false; Bridge.fail=fail;
            call("onPause");
            assertEquals(List.of(31L),Bridge.stops); assertEquals(0L,field("surfaceRequestGeneration").getLong(a));
            call("onResume"); assertEquals(1,Bridge.starts);
        }
    }
    @Test public void rejectedResumeRetiresOldGenerationBeforeFreshStart()throws Exception {
        call("onPause"); Bridge.accept=false; call("onResume");
        assertEquals(List.of(31L),Bridge.stops); assertEquals(1,Bridge.starts);
        assertEquals(42L,field("surfaceRequestGeneration").getLong(a));
    }
    @Test public void destroyStillRetiresRetainedSurface()throws Exception {
        call("onPause"); call("onDestroy");
        assertEquals(List.of(31L),Bridge.stops); assertEquals(0L,field("surfaceRequestGeneration").getLong(a));
    }
}
