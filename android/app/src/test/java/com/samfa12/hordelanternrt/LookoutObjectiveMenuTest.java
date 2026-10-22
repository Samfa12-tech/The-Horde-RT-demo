package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.LooperMode;

/** Real native Android menu methods; publications are fake, no device or RT claim. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=LookoutObjectiveMenuTest.Bridge.class)
@LooperMode(LooperMode.Mode.PAUSED)
public final class LookoutObjectiveMenuTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class Bridge {
        static long[] objective;
        static boolean paused,entry,crouch;
        static float strafe,forward;
        static int resets,campaignRequests,attacks,parries,dodges,interacts,lanterns;
        static final List<String> order=new ArrayList<>();
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static long[] getLookoutObjectiveState(long generation){return objective==null?null:objective.clone();}
        @Implementation protected static int getSurfaceRuntimeState(long generation){return 1;}
        @Implementation protected static int getContextualControlState(){return 0;}
        @Implementation protected static void setSimulationPaused(boolean value){paused=value;order.add(value?"pause":"resume");}
        @Implementation protected static void setViewControls(float y,float p,float light,float x,float z,boolean c,boolean analogueMovement){strafe=x;forward=z;crouch=c;order.add("input");}
        @Implementation protected static void setRunHeld(boolean held){}
        @Implementation protected static void clearRunIntent(){}
        @Implementation protected static void clearCrouchIntent(){crouch=false;}
        @Implementation protected static boolean isRtLabUnlockEligible(){return false;}
        @Implementation protected static void setEntryMenu(boolean enabled,boolean side,boolean reduced,boolean play,long generation){entry=enabled;order.add("entry");}
        @Implementation protected static long requestCampaign(int action,int slot,int kind,long generation){campaignRequests++;return 1;}
        @Implementation protected static void requestRouteReset(){resets++;}
        @Implementation protected static boolean setDiagnosticSurfaceSuspended(long generation,boolean suspended){return true;}
        @Implementation protected static void requestAttack(){attacks++;}
        @Implementation protected static void requestParry(){parries++;}
        @Implementation protected static void requestDodge(){dodges++;}
        @Implementation protected static void requestInteract(){interacts++;}
        @Implementation protected static void requestToggleHeldLightPose(){lanterns++;}
    }
    private MainActivity a;
    private static Field field(String name)throws Exception {Field f=MainActivity.class.getDeclaredField(name);f.setAccessible(true);return f;}
    private void set(String name,Object value)throws Exception {field(name).set(a,value);}
    private Object call(String name,Class<?>[] types,Object...args)throws Exception {
        Method m=MainActivity.class.getDeclaredMethod(name,types);m.setAccessible(true);return m.invoke(a,args);
    }
    private void call(String name)throws Exception {call(name,new Class<?>[]{});}
    private Button find(String label){return find((ViewGroup)a.findViewById(R.id.menu_scrim),label);}
    private Button find(ViewGroup group,String label) {
        for(int i=0;i<group.getChildCount();i++) {
            View v=group.getChildAt(i);
            if(v instanceof Button && ((Button)v).getText().toString().equals(label))return (Button)v;
            if(v instanceof ViewGroup){Button b=find((ViewGroup)v,label);if(b!=null)return b;}
        }
        return null;
    }
    private String text(View v) {
        String result=v instanceof TextView?((TextView)v).getText().toString()+"\n":"";
        if(v instanceof ViewGroup)for(int i=0;i<((ViewGroup)v).getChildCount();i++)result+=text(((ViewGroup)v).getChildAt(i));
        return result;
    }
    private void pause()throws Exception {call("showMainMenu",new Class<?>[]{boolean.class},false);}
    private void open()throws Exception {pause();assertNotNull(find("Lookout objective"));find("Lookout objective").performClick();}
    private int actionCount(){return Bridge.attacks+Bridge.parries+Bridge.dodges+Bridge.interacts+Bridge.lanterns;}
    @Before public void setup()throws Exception {
        Bridge.objective=new long[]{1,31,81,1};Bridge.paused=Bridge.entry=Bridge.crouch=false;
        Bridge.strafe=Bridge.forward=0;Bridge.order.clear();
        Bridge.resets=Bridge.campaignRequests=Bridge.attacks=Bridge.parries=Bridge.dodges=Bridge.interacts=Bridge.lanterns=0;
        a=Robolectric.buildActivity(MainActivity.class).get();a.setContentView(R.layout.activity_main);
        set("preferences",a.getSharedPreferences("lookout-menu",Context.MODE_PRIVATE));
        a.getSharedPreferences("lookout-menu",Context.MODE_PRIVATE).edit().clear().putBoolean("graphics_mist_enabled",false).putInt("owner_marker",17).commit();
        set("resumed",true);set("firstMenu",false);set("surfaceStarted",true);set("surfaceRequestGeneration",31L);
        String[] names={"surfaceView","menuScrim","menuButton","attackButton","parryButton","dodgeButton","runButton","crouchButton",
                "interactButton","toggleHeldLightPoseButton","diagnosticsPanel","developerOverlay","vitalityStatus","rtStatus","controllerPrompt"};
        int[] ids={R.id.scene_surface,R.id.menu_scrim,R.id.menu_button,R.id.attack_button,R.id.parry_button,R.id.dodge_button,R.id.run_button,R.id.crouch_button,
                R.id.interact_button,R.id.toggle_held_light_pose_button,R.id.diagnostics_panel,R.id.developer_overlay,R.id.vitality_status,R.id.rt_status,R.id.controller_prompt};
        for(int i=0;i<names.length;i++)set(names[i],a.findViewById(ids[i]));
    }
    @Test public void malformedStaleAndUnavailablePublicationsCannotOfferThePage()throws Exception {
        for(long[] value:new long[][]{null,new long[4],new long[]{2,31,81,1},new long[]{1,30,81,1},
                new long[]{1,31,0,1},new long[]{1,31,81,2},new long[]{1,31,81,0}}) {
            Bridge.objective=value;pause();assertNull(find("Lookout objective"));
            assertFalse(field("lookoutObjectiveVisible").getBoolean(a));
        }
        Bridge.objective=new long[]{1,31,81,1};set("deathOverlayVisible",true);pause();assertNull(find("Lookout objective"));
    }
    @Test public void arrivalOnlyOffersAnExplicitPauseChoiceAndContinueClearsHeldInput()throws Exception {
        assertFalse(field("lookoutObjectiveVisible").getBoolean(a));pause();
        assertNotNull(find("Lookout objective"));assertNull(find("Continue exploring"));
        int[] pointers=(int[])field("activePointers").get(a);pointers[0]=5;pointers[1]=8;
        float[] controls=(float[])field("viewControls").get(a);controls[7]=.7f;controls[8]=.8f;
        set("parryTouchActive",true);set("crouchControlKeys",1);
        find("Lookout objective").performClick();
        assertTrue(field("lookoutObjectiveVisible").getBoolean(a));assertTrue(Bridge.paused);
        assertTrue(text(a.findViewById(R.id.menu_scrim)).contains("future chapter"));
        assertArrayEquals(new int[]{-1,-1},pointers);assertFalse(field("parryTouchActive").getBoolean(a));
        Bridge.order.clear();find("Continue exploring").performClick();
        assertFalse(Bridge.paused);assertFalse(field("menuVisible").getBoolean(a));
        assertEquals(0,Bridge.strafe,0);assertEquals(0,Bridge.forward,0);assertFalse(Bridge.crouch);
        assertTrue(Bridge.order.indexOf("input")<Bridge.order.indexOf("resume"));
        assertEquals(0,actionCount());assertEquals(0,Bridge.resets);assertEquals(0,Bridge.campaignRequests);
    }
    @Test public void backReopensCleanlyAndOldButtonsCannotControlTheNewPanel()throws Exception {
        open();Button old=find("Continue exploring");a.onBackPressed();assertFalse(Bridge.paused);
        open();assertTrue(Bridge.paused);old.performClick();assertTrue(Bridge.paused);
        assertTrue(field("lookoutObjectiveVisible").getBoolean(a));
        find("Continue exploring").performClick();assertFalse(Bridge.paused);
    }
    @Test public void changedWorldOrSurfaceCannotBeResumedByTheOldPanel()throws Exception {
        open();Button old=find("Continue exploring");Bridge.objective[2]++;
        old.performClick();assertTrue(Bridge.paused);assertFalse(field("lookoutObjectiveVisible").getBoolean(a));
        open();old=find("Continue exploring");set("surfaceRequestGeneration",32L);old.performClick();
        assertTrue(Bridge.paused);assertFalse(field("lookoutObjectiveVisible").getBoolean(a));
        assertEquals(0,actionCount());
    }
    @Test public void homeRevokesPanelActionsAndReturnsToOrdinaryPausedMenu()throws Exception {
        open();Button old=find("Continue exploring");a.onPause();
        assertTrue(Bridge.paused);assertFalse(field("lookoutObjectiveVisible").getBoolean(a));
        old.performClick();assertTrue(Bridge.paused);assertFalse(field("resumed").getBoolean(a));
        assertEquals(0,Bridge.resets);assertEquals(0,Bridge.campaignRequests);
    }
    @Test public void replacementPageAndLostWindowFocusRevokeDetachedPanelButtons()throws Exception {
        open();Button old=find("Continue exploring");call("showControls");
        old.performClick();assertTrue(Bridge.paused);assertFalse(field("lookoutObjectiveVisible").getBoolean(a));
        open();old=find("Continue exploring");a.onWindowFocusChanged(false);old.performClick();
        assertTrue(Bridge.paused);assertFalse(field("lookoutObjectiveVisible").getBoolean(a));
        assertEquals(0,actionCount());
    }
    @Test public void returnToBrandedMenuDoesNotResetWriteSaveOrChangePreferences()throws Exception {
        Map<String,?> before=a.getSharedPreferences("lookout-menu",Context.MODE_PRIVATE).getAll();
        open();find("Return to menu").performClick();
        assertTrue(Bridge.paused);assertTrue(Bridge.entry);assertTrue(field("entryMenuEnabled").getBoolean(a));
        assertEquals(0,Bridge.resets);assertEquals(0,Bridge.campaignRequests);assertEquals(0,actionCount());
        assertEquals(before,a.getSharedPreferences("lookout-menu",Context.MODE_PRIVATE).getAll());
    }
    @Test public void keyboardEscapeClosesOnlyThePlayerOpenedPanel()throws Exception {
        open();long now=SystemClock.uptimeMillis();
        a.dispatchKeyEvent(new KeyEvent(now,now,KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_ESCAPE,0,0,1,0,0,InputDevice.SOURCE_KEYBOARD));
        assertTrue(Bridge.paused);
        a.dispatchKeyEvent(new KeyEvent(now,now+1,KeyEvent.ACTION_UP,KeyEvent.KEYCODE_ESCAPE,0,0,1,0,0,InputDevice.SOURCE_KEYBOARD));
        assertFalse(Bridge.paused);assertFalse(field("lookoutObjectiveVisible").getBoolean(a));assertEquals(0,actionCount());
    }
    @Test public void controllerConfirmationCannotAlsoDeliverItsGameplayAction()throws Exception {
        set("controllerMode",true);open();
        View root=a.findViewById(R.id.menu_scrim);
        root.measure(View.MeasureSpec.makeMeasureSpec(1000,View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(1500,View.MeasureSpec.EXACTLY));
        root.layout(0,0,1000,1500);
        ControllerNavigation.ensureFocus(root);assertTrue(find("Continue exploring").hasFocus());
        AndroidControllerInput.Result result=new AndroidControllerInput.Result(true,true,
                AndroidControllerInput.CONFIRM|AndroidControllerInput.INTERACT|AndroidControllerInput.SWING,0,0,0,0,0,0);
        call("handleControllerResult",new Class<?>[]{AndroidControllerInput.Result.class},result);
        assertFalse(Bridge.paused);assertEquals(0,actionCount());assertFalse(Bridge.crouch);
    }
}
