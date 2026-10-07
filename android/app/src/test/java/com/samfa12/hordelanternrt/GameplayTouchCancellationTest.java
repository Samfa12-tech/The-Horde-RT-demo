package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.widget.Button;
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
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static int getSurfaceRuntimeState(long generation) { return 1; }
        @Implementation protected static void setViewControls(float yaw,float pitch,float light,float x,float z) {
            strafe=x; forward=z;
        }
        @Implementation protected static void setSimulationPaused(boolean paused) {}
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
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        SurfaceView surface=new SurfaceView(a); surface.layout(0,0,360,640);
        field("surfaceView").set(a,surface); field("menuVisible").setBoolean(a,false);
        field("preferences").set(a,a.getSharedPreferences("touch-fixture",Context.MODE_PRIVATE));
        for(String button:new String[]{"parryButton","interactButton","toggleHeldLightPoseButton"}) field(button).set(a,new Button(a));
        invoke(a,"configureTouchControls"); return a;
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
}
