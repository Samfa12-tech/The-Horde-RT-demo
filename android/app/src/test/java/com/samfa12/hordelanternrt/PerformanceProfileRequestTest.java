package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import android.content.Intent;
import android.content.SharedPreferences;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34, shadows=PerformanceProfileRequestTest.ProfileBridge.class)
public class PerformanceProfileRequestTest {
    final GraphicsPreferences.Values saved=new GraphicsPreferences.Values(40,1,0,30,false,1,true,1);
    Intent intent(String profile) {return new Intent().putExtra("horde.debug.motion","chapter-performance-route")
            .putExtra("horde.debug.motion_id","quality-1").putExtra(PerformanceProfileRequest.EXTRA,profile);}
    GraphicsPreferences.Values resolve(String profile) {return PerformanceProfileRequest.resolve(intent(profile),true,true,false,saved);}
    @Test public void singleProfilesOnlyChangeTheirNamedQuality() {
        assertTrue(resolve("baseline").same(saved));
        assertTrue(resolve("mist-off").same(new GraphicsPreferences.Values(40,1,0,30,false,1,false,1)));
        assertTrue(resolve("fire-low").same(new GraphicsPreferences.Values(40,1,2,30,false,1,true,1)));
        assertTrue(resolve("dust-off").same(new GraphicsPreferences.Values(40,1,0,30,false,1,true,0)));
        assertTrue(resolve("combined").same(new GraphicsPreferences.Values(40,1,2,30,false,1,false,0)));
    }
    @Test public void rejectsReleaseWrongScopeUnknownAndWrongTypes() {
        assertNull(resolve("unknown"));
        assertNull(PerformanceProfileRequest.resolve(intent("combined"),false,true,false,saved));
        assertNull(PerformanceProfileRequest.resolve(intent("combined"),true,false,false,saved));
        assertNull(PerformanceProfileRequest.resolve(intent("combined"),true,true,true,saved));
        assertNull(PerformanceProfileRequest.resolve(intent("combined").putExtra("horde.debug.motion","keeper-first-entry"),true,true,false,saved));
        assertNull(PerformanceProfileRequest.resolve(intent("combined").putExtra(PerformanceProfileRequest.EXTRA,1),true,true,false,saved));
        assertNull(PerformanceProfileRequest.resolve(intent("combined"),true,true,false,GraphicsPreferences.mobileDefaults()));
        for(String key:new String[]{"horde.debug.checkpoint","horde.debug.replay","horde.debug.dust","horde.debug.scale"})
            assertNull(PerformanceProfileRequest.resolve(intent("combined").putExtra(key,"unexpected"),true,true,false,saved));
    }
    @Test public void startupMistOffPreservesExplicitSavedOnAndOffWithoutWrites() {
        SharedPreferences p=RuntimeEnvironment.getApplication().getSharedPreferences("mist-default-test",0);
        p.edit().clear().commit();assertFalse(GraphicsPreferences.confirmed(p).mistEnabled);assertTrue(p.getAll().isEmpty());
        for(boolean mist:new boolean[]{true,false}) {
            p.edit().putInt("graphics_schema",5).putInt("render_scale",40).putInt("water_quality",1)
                .putInt(GraphicsPreferences.FIRE,0).putInt(GraphicsPreferences.CAP,30).putBoolean(GraphicsPreferences.GLASS,false)
                .putInt(GraphicsPreferences.SHADOW,1).putBoolean(GraphicsPreferences.MIST,mist).putInt(GraphicsPreferences.DUST,1).commit();
            java.util.Map<String,?> before=p.getAll();assertEquals(mist,GraphicsPreferences.confirmed(p).mistEnabled);
            assertNotNull(resolve("combined"));assertEquals(before,p.getAll());
        }
        assertTrue(GraphicsPreferences.baseline().mistEnabled);
    }
    long[] presented() {
        return new long[]{7,3,0,40,1,2,30,576,1188,1440,2971,0,1,1,0,40,1,2,30,0,0,0,1,1,1,1,1,1};
    }
    @Test public void motionWaitsForOwningPresentedQualityAndRejectsStaleOrFailedOwnership() {
        PerformanceProfileRequest.PresentationGate gate=new PerformanceProfileRequest.PresentationGate(resolve("fire-low"),7,3,100);
        assertEquals(0,gate.poll(null,3,101));
        long[] frame=presented(); frame[0]=6; assertEquals(0,gate.poll(frame,3,101));
        frame=presented(); frame[13]=0; assertEquals(0,gate.poll(frame,3,101));
        frame=presented(); frame[5]=0; assertEquals(0,gate.poll(frame,3,101));
        frame=presented(); frame[17]=0; assertEquals(0,gate.poll(frame,3,101));
        frame=presented(); frame[19]=1; assertEquals(0,gate.poll(frame,3,101));
        frame=presented(); frame[12]=0; assertEquals(0,gate.poll(frame,3,101));
        assertEquals(1,gate.poll(presented(),3,101));
        frame=presented(); frame[0]=8; assertEquals(2,gate.poll(frame,3,101));
        frame=presented(); frame[14]=64; assertEquals(2,gate.poll(frame,3,101));
        assertEquals(2,gate.poll(presented(),4,101));
        assertEquals(2,gate.poll(presented(),3,99));
        assertEquals(2,gate.poll(presented(),3,30100));
    }

    @Test public void coldLaunchBindsOneSurfaceWithoutExtendingItsDeadline() {
        PerformanceProfileRequest.PresentationGate gate=new PerformanceProfileRequest.PresentationGate(resolve("fire-low"),7,0,100);
        assertEquals(0,gate.poll(null,0,101));
        assertEquals(1,gate.poll(presented(),3,102));
        assertEquals(2,gate.poll(presented(),4,103));
        gate=new PerformanceProfileRequest.PresentationGate(resolve("fire-low"),7,0,100);
        assertEquals(2,gate.poll(null,0,30100));
    }

    @Implements(ProbeBridge.class)
    public static class ProfileBridge {
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static void setRequiredRayQueryCompute(boolean required) {}
        @Implementation protected static void setGpuTimingEnabled(boolean enabled) {}
        @Implementation protected static void markRtLabDebugAutomation() {}
        @Implementation protected static long setGraphicsSettings(int scale,int water,int fire,int cap,
                boolean glass,int shadow,boolean mist,int dust) {return 7;}
    }
    static java.lang.reflect.Field field(String name) throws Exception {
        java.lang.reflect.Field f=MainActivity.class.getDeclaredField(name);f.setAccessible(true);return f;
    }
    @Test public void rejectedReplacementRetainsTheOriginalMotionAndItsPresentationGate() throws Exception {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        a.getApplicationInfo().flags|=android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE;
        field("graphicsConfirmed").set(a,saved);field("surfaceRequestGeneration").setLong(a,3);
        java.lang.reflect.Method consume=MainActivity.class.getDeclaredMethod("consumeDebugAutomationIntent",Intent.class);
        consume.setAccessible(true);consume.invoke(a,intent("fire-low"));
        Object gate=field("pendingDebugMotionGraphics").get(a);assertNotNull(gate);
        consume.invoke(a,intent("unknown").putExtra("horde.debug.motion_id","rejected-replacement"));
        assertSame(gate,field("pendingDebugMotionGraphics").get(a));
        assertEquals("quality-1",field("pendingDebugMotionId").get(a));
        assertEquals("chapter-performance-route",field("pendingDebugMotion").get(a));
    }

}
