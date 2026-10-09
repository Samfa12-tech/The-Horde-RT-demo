package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;
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
@Config(sdk=34, shadows=GameplayTouchCancellationTest.Bridge.class)
public final class GameplayTouchCancellationTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class Bridge {
        static float strafe,forward;
        static int attacks,parries,dodges;
        static int runtimeState=1;
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static int getSurfaceRuntimeState(long generation) { return runtimeState; }
        @Implementation protected static long getPlayerVitalityState() { return (3L << 32) | 3L; }
        @Implementation protected static int getPlayerLifePhase() { return 0; }
        @Implementation protected static int getFinaleEndingPhase() { return 0; }
        @Implementation protected static int getContextualControlState() { return 0; }
        @Implementation protected static void setViewControls(float yaw,float pitch,float light,float x,float z) {
            strafe=x; forward=z;
        }
        @Implementation protected static void setSimulationPaused(boolean paused) {}
        @Implementation protected static void requestAttack() { ++attacks; }
        @Implementation protected static void requestParry() { ++parries; }
        @Implementation protected static void requestDodge() { ++dodges; }
    }
    private static Field field(String name) throws Exception {
        Field f=MainActivity.class.getDeclaredField(name); f.setAccessible(true); return f;
    }
    private static void invoke(MainActivity a,String name) throws Exception {
        Method m=MainActivity.class.getDeclaredMethod(name); m.setAccessible(true); m.invoke(a);
    }
    private static MotionEvent event(int action,boolean lookFirst,boolean moved) {
        MotionEvent.PointerProperties[] pp=new MotionEvent.PointerProperties[2];
        MotionEvent.PointerCoords[] pc=new MotionEvent.PointerCoords[2];
        for(int i=0;i<2;++i) {
            pp[i]=new MotionEvent.PointerProperties(); pp[i].id=i==0?3:7; pp[i].toolType=MotionEvent.TOOL_TYPE_FINGER;
            pc[i]=new MotionEvent.PointerCoords(); boolean look=(i==0)==lookFirst;
            pc[i].x=look?(moved?330:300):(moved?115:80); pc[i].y=moved?370:400;
            pc[i].pressure=1; pc[i].size=1;
        }
        int count=action==MotionEvent.ACTION_DOWN?1:2;
        return MotionEvent.obtain(1,10,action,count,pp,pc,0,0,1,1,0,0,0,0);
    }
    private static MainActivity prepare() throws Exception {
        Bridge.runtimeState=1;
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        SurfaceView surface=new SurfaceView(a); surface.layout(0,0,360,640);
        field("surfaceView").set(a,surface); field("menuVisible").setBoolean(a,false);
        field("preferences").set(a,a.getSharedPreferences("touch-fixture",Context.MODE_PRIVATE));
        for(String button:new String[]{"attackButton","parryButton","dodgeButton","interactButton","toggleHeldLightPoseButton"}) field(button).set(a,new Button(a));
        invoke(a,"configureGameplayActionButtons");
        invoke(a,"configureTouchControls"); return a;
    }
    @Test public void nativeReadyAfterPlayOrSurfaceRecoveryRestoresAllThreeActionButtons() throws Exception {
        MainActivity a=prepare();
        TextView status=new TextView(a);
        status.setLayoutParams(new FrameLayout.LayoutParams(100,48));
        field("rtStatus").set(a,status);
        field("developerOverlay").set(a,new TextView(a));
        field("vitalityStatus").set(a,new TextView(a));
        field("lastPlayerVitality").setInt(a,3);
        Runnable poll=(Runnable)field("runtimePoll").get(a);
        // Play can close the menu before the native surface is ready. The
        // same transition occurs after lifecycle surface recovery.
        for(int attempt=0;attempt<2;attempt++) {
            Bridge.runtimeState=0;
            for(String name:new String[]{"attackButton","parryButton","dodgeButton"})
                ((Button)field(name).get(a)).setVisibility(View.GONE);
            poll.run();
            assertEquals(View.GONE,((Button)field("dodgeButton").get(a)).getVisibility());
            Bridge.runtimeState=1; poll.run();
            for(String name:new String[]{"attackButton","parryButton","dodgeButton"})
                assertEquals(name+" must recover when native becomes ready",View.VISIBLE,
                        ((Button)field(name).get(a)).getVisibility());
        }
    }
    private static void drag(MainActivity a,boolean lookFirst) throws Exception {
        SurfaceView surface=(SurfaceView)field("surfaceView").get(a);
        for(int action:new int[]{MotionEvent.ACTION_DOWN,
                MotionEvent.ACTION_POINTER_DOWN|(1<<MotionEvent.ACTION_POINTER_INDEX_SHIFT),MotionEvent.ACTION_MOVE}) {
            MotionEvent e=event(action,lookFirst,action==MotionEvent.ACTION_MOVE);
            surface.dispatchTouchEvent(e); e.recycle();
        }
        assertTrue(Math.abs(Bridge.forward)>0); assertTrue(Math.abs(Bridge.strafe)>0);
    }
    private static void assertCleared(MainActivity a) throws Exception {
        assertArrayEquals(new int[]{-1,-1},(int[])field("activePointers").get(a));
        assertEquals(0,Bridge.strafe,0); assertEquals(0,Bridge.forward,0);
        assertFalse(field("parryTouchActive").getBoolean(a));
    }
    @Test public void productionWholeCancelClearsLookFirstAndMovementFirstGestures() throws Exception {
        for(boolean lookFirst:new boolean[]{true,false}) {
            MainActivity a=prepare(); drag(a,lookFirst);
            MotionEvent cancel=event(MotionEvent.ACTION_CANCEL,lookFirst,true);
            ((SurfaceView)field("surfaceView").get(a)).dispatchTouchEvent(cancel); cancel.recycle(); assertCleared(a);
            drag(a,!lookFirst); // A new gesture cannot inherit either cancelled role.
        }
    }
    @Test public void productionMenuCleanupAndLifecyclePauseClearHeldGestureBeforeRecovery() throws Exception {
        for(String boundary:new String[]{"clearTouchState","onPause"}) {
            MainActivity a=prepare(); drag(a,true); field("parryTouchActive").setBoolean(a,true);
            invoke(a,boundary); assertCleared(a);
            field("menuVisible").setBoolean(a,false); drag(a,false);
        }
    }
    private static void buttonEvent(Button button,int action) {
        MotionEvent e=MotionEvent.obtain(1,10,action,10,10,0);
        button.dispatchTouchEvent(e); e.recycle();
    }
    @Test public void moveAndLookCanCoexistWithEachPressDownActionWithoutDuplicateRelease() throws Exception {
        MainActivity a=prepare(); drag(a,true);
        Bridge.attacks=Bridge.parries=Bridge.dodges=0;
        for(String name:new String[]{"attackButton","parryButton","dodgeButton"}) {
            Button b=(Button)field(name).get(a);
            buttonEvent(b,MotionEvent.ACTION_DOWN);
            buttonEvent(b,MotionEvent.ACTION_DOWN);
            assertEquals(name+" must fire on press-down",1,name.equals("attackButton")?Bridge.attacks:
                    name.equals("parryButton")?Bridge.parries:Bridge.dodges);
            assertTrue(Math.abs(Bridge.forward)>0); assertTrue(Math.abs(Bridge.strafe)>0);
            buttonEvent(b,MotionEvent.ACTION_UP);
        }
        assertEquals(1,Bridge.attacks); assertEquals(1,Bridge.parries); assertEquals(1,Bridge.dodges);
        for(String name:new String[]{"attackButton","parryButton","dodgeButton"}) {
            Button b=(Button)field(name).get(a); b.performClick();
        }
        assertEquals(2,Bridge.attacks); assertEquals(2,Bridge.parries); assertEquals(2,Bridge.dodges);
    }
    @Test public void menuEndingAndCancelledReleaseCannotPublishExtraActions() throws Exception {
        MainActivity a=prepare(); Bridge.attacks=Bridge.parries=Bridge.dodges=0;
        for(String name:new String[]{"attackButton","parryButton","dodgeButton"}) {
            Button b=(Button)field(name).get(a);
            buttonEvent(b,MotionEvent.ACTION_DOWN); invoke(a,"clearTouchState");
            buttonEvent(b,MotionEvent.ACTION_UP); assertFalse(b.isPressed());
            buttonEvent(b,MotionEvent.ACTION_DOWN); buttonEvent(b,MotionEvent.ACTION_CANCEL);
            buttonEvent(b,MotionEvent.ACTION_UP); assertFalse(b.isPressed());
        }
        assertEquals(2,Bridge.attacks); assertEquals(2,Bridge.parries); assertEquals(2,Bridge.dodges);
        for(String overlay:new String[]{"menuVisible","endingOverlayVisible","deathOverlayVisible","diagnosticsVisible"}) {
            field(overlay).setBoolean(a,true);
            for(String name:new String[]{"attackButton","parryButton","dodgeButton"}) {
                Button b=(Button)field(name).get(a);
                buttonEvent(b,MotionEvent.ACTION_DOWN); buttonEvent(b,MotionEvent.ACTION_UP); b.performClick();
            }
            field(overlay).setBoolean(a,false);
        }
        assertEquals(2,Bridge.attacks); assertEquals(2,Bridge.parries); assertEquals(2,Bridge.dodges);
    }
}
