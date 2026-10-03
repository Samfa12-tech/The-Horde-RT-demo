package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34, shadows=DeathRecoveryTest.RecoveryBridgeShadow.class)
public final class DeathRecoveryTest {
    @Implements(ProbeBridge.class)
    public static final class RecoveryBridgeShadow {
        static int resets, unpauses;
        @Implementation protected static void __staticInitializer__() { }
        @Implementation protected static void resetRtSceneTuning() { }
        @Implementation protected static void requestRouteReset() { ++resets; }
        @Implementation protected static void setViewControls(float yaw,float pitch,float light,float strafe,float forward) { }
        @Implementation protected static void setSimulationPaused(boolean paused) { if(!paused) ++unpauses; }
    }
    private static Field field(String name) throws Exception {
        Field result=MainActivity.class.getDeclaredField(name); result.setAccessible(true); return result;
    }
    @Test public void restartKeepsDeadUiPausedUntilNativeAcknowledgesAndIgnoresRepeatedTap() throws Exception {
        RecoveryBridgeShadow.resets=0; RecoveryBridgeShadow.unpauses=0;
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        field("deathOverlayVisible").setBoolean(activity,true);
        field("menuVisible").setBoolean(activity,true);
        field("lastPlayerLifePhase").setInt(activity,2);
        field("lastPlayerVitality").setInt(activity,0);
        Method restart=MainActivity.class.getDeclaredMethod("restartAfterDeath"); restart.setAccessible(true);
        restart.invoke(activity);
        assertEquals(1,RecoveryBridgeShadow.resets);
        assertTrue(field("retryPending").getBoolean(activity));
        assertTrue(field("deathOverlayVisible").getBoolean(activity));
        assertTrue(field("menuVisible").getBoolean(activity));
        assertEquals(2,field("lastPlayerLifePhase").getInt(activity));
        assertEquals(0,field("lastPlayerVitality").getInt(activity));
        assertEquals(0,RecoveryBridgeShadow.unpauses);
        restart.invoke(activity);
        assertEquals(1,RecoveryBridgeShadow.resets);
        assertEquals(0,RecoveryBridgeShadow.unpauses);
    }
}
