package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.app.AlertDialog;
import android.content.Context;
import android.content.res.Configuration;
import android.hardware.input.InputManager;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowSystemClock;
import java.time.Duration;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=AndroidControllerIntegrationTest.Bridge.class)
public class AndroidControllerIntegrationTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static class Bridge {
        static float x,z,yaw,pitch; static int attacks,parries,dodges,interacts,lanterns;
        static boolean paused; static int context,retries;
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static int getSurfaceRuntimeState(long generation){return 1;}
        @Implementation protected static int getContextualControlState(){return context;}
        @Implementation protected static void setViewControls(float y,float p,float light,float strafe,float forward){yaw=y;pitch=p;x=strafe;z=forward;}
        @Implementation protected static void setSimulationPaused(boolean p){paused=p;}
        @Implementation protected static int retryEncounter(){retries++;return 2;}
        @Implementation protected static void beginGraphicsEdit(int scale,int water,int fire,int cap,boolean glass,int shadow,boolean mist,int dust){}
        @Implementation protected static void setEntryMenu(boolean enabled,boolean side,boolean reduced,boolean play,long generation){}
        @Implementation protected static void requestAttack(){attacks++;}
        @Implementation protected static void requestParry(){parries++;}
        @Implementation protected static void requestDodge(){dodges++;}
        @Implementation protected static void requestInteract(){interacts++;}
        @Implementation protected static void requestToggleHeldLightPose(){lanterns++;}
    }
    private MainActivity a;
    static Field field(String name)throws Exception {Field f=MainActivity.class.getDeclaredField(name);f.setAccessible(true);return f;}
    private void set(String name,Object value)throws Exception {field(name).set(a,value);}
    private Object call(String name,Class<?>[] types,Object...args)throws Exception {
        Method m=MainActivity.class.getDeclaredMethod(name,types);m.setAccessible(true);return m.invoke(a,args);
    }
    private Button button(String name)throws Exception{return (Button)field(name).get(a);}
    @Before public void setup()throws Exception {
        Bridge.attacks=Bridge.parries=Bridge.dodges=Bridge.interacts=Bridge.lanterns=Bridge.context=0;
        Bridge.x=Bridge.z=Bridge.yaw=Bridge.pitch=0;
        a=Robolectric.buildActivity(MainActivity.class).get();
        a.setContentView(R.layout.activity_main);
        set("surfaceView",a.findViewById(R.id.scene_surface)); set("menuScrim",a.findViewById(R.id.menu_scrim));
        set("preferences",a.getSharedPreferences("controller-integration",Context.MODE_PRIVATE));
        a.getSharedPreferences("controller-integration",Context.MODE_PRIVATE).edit().clear().apply();
        set("resumed",true);set("menuVisible",false);
        String[] names={"menuButton","attackButton","parryButton","dodgeButton","interactButton","toggleHeldLightPoseButton"};
        int[] ids={R.id.menu_button,R.id.attack_button,R.id.parry_button,R.id.dodge_button,R.id.interact_button,R.id.toggle_held_light_pose_button};
        for(int i=0;i<names.length;i++)set(names[i],a.findViewById(ids[i]));
        set("diagnosticsPanel",a.findViewById(R.id.diagnostics_panel));set("developerOverlay",a.findViewById(R.id.developer_overlay));
        set("vitalityStatus",a.findViewById(R.id.vitality_status));set("rtStatus",a.findViewById(R.id.rt_status));
        set("controllerPrompt",a.findViewById(R.id.controller_prompt));
        call("configureGameplayActionButtons",new Class<?>[]{});
    }
    private KeyEvent key(int action,int code,int repeat) {
        ShadowSystemClock.advanceBy(Duration.ofMillis(2)); long now=SystemClock.uptimeMillis();
        return new KeyEvent(now,now,action,code,repeat,0,9,0,0,InputDevice.SOURCE_GAMEPAD);
    }
    private void press(int code){a.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,code,0));a.dispatchKeyEvent(key(KeyEvent.ACTION_UP,code,0));}
    private void motion(float x,float y,float rx,float ry) {
        MotionEvent.PointerProperties prop=new MotionEvent.PointerProperties();prop.id=0;
        MotionEvent.PointerCoords coords=new MotionEvent.PointerCoords();
        coords.setAxisValue(MotionEvent.AXIS_X,x);coords.setAxisValue(MotionEvent.AXIS_Y,y);
        coords.setAxisValue(MotionEvent.AXIS_Z,rx);coords.setAxisValue(MotionEvent.AXIS_RZ,ry);
        long t=SystemClock.uptimeMillis(); MotionEvent e=MotionEvent.obtain(t,t,MotionEvent.ACTION_MOVE,1,
                new MotionEvent.PointerProperties[]{prop},new MotionEvent.PointerCoords[]{coords},0,0,1,1,9,0,InputDevice.SOURCE_JOYSTICK,0);
        a.dispatchGenericMotionEvent(e);e.recycle();
    }
    @Test public void pressDownActionsRemainIndependentWithoutDuplicateReleases()throws Exception {
        for(int code:new int[]{KeyEvent.KEYCODE_BUTTON_R2,KeyEvent.KEYCODE_BUTTON_L2,KeyEvent.KEYCODE_BUTTON_B,KeyEvent.KEYCODE_BUTTON_A,KeyEvent.KEYCODE_BUTTON_Y}){
            a.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,code,0));a.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,code,1));a.dispatchKeyEvent(key(KeyEvent.ACTION_UP,code,0));
        }
        assertEquals(1,Bridge.attacks);assertEquals(1,Bridge.parries);assertEquals(1,Bridge.dodges);assertEquals(1,Bridge.interacts);assertEquals(1,Bridge.lanterns);
        assertTrue(field("controllerMode").getBoolean(a));
    }
    @Test public void takeoverRetainsHealthAndContextWhileLeavingPreferencesUntouched()throws Exception {
        Bridge.context=1|(2<<3)|2;
        android.content.SharedPreferences p=a.getSharedPreferences("controller-integration",Context.MODE_PRIVATE);
        p.edit().putBoolean("show_hud",true).putInt("interface_opacity",73).apply();java.util.Map<String,?> before=p.getAll();
        press(KeyEvent.KEYCODE_BUTTON_X);
        for(String name:new String[]{"attackButton","parryButton","dodgeButton","menuButton","interactButton","toggleHeldLightPoseButton"})assertEquals(name,View.GONE,button(name).getVisibility());
        assertEquals(View.VISIBLE,((TextView)field("vitalityStatus").get(a)).getVisibility());
        TextView prompt=(TextView)field("controllerPrompt").get(a);
        assertEquals(View.VISIBLE,prompt.getVisibility());assertTrue(prompt.getText().toString().contains("A:"));
        assertEquals(before,p.getAll());
        p.edit().putBoolean("show_hud",false).apply();call("refreshControllerHud",new Class<?>[]{});
        assertEquals(View.GONE,prompt.getVisibility());assertEquals(View.GONE,((TextView)field("vitalityStatus").get(a)).getVisibility());
    }
    @Test public void connectionAndDriftDoNotTakeOverButMoveLookAndDodgeCanCoexist()throws Exception {
        ((InputManager.InputDeviceListener)field("controllerDevices").get(a)).onInputDeviceAdded(9);
        motion(0,0,0,0);motion(.04f,-.03f,0,0);assertFalse(field("controllerMode").getBoolean(a));
        motion(.5f,-.6f,.5f,-.5f);assertTrue(field("controllerMode").getBoolean(a));assertTrue(Bridge.x>0);assertTrue(Bridge.z>0);
        Runnable frame=(Runnable)field("controllerFrame").get(a);frame.run();ShadowSystemClock.advanceBy(Duration.ofMillis(32));frame.run();
        assertTrue(Bridge.yaw>0);assertTrue("upward right stick raises pitch",Bridge.pitch>0);
        press(KeyEvent.KEYCODE_BUTTON_B);assertEquals(1,Bridge.dodges);assertTrue(Bridge.z>0);
    }
    @Test public void firstIntentionalTouchRestoresTargetsAndNeutralizesControllerAxes()throws Exception {
        motion(0,0,0,0);motion(.7f,0,0,0);assertTrue(Bridge.x>0);
        MotionEvent e=MotionEvent.obtain(1,2,MotionEvent.ACTION_DOWN,2,2,0);a.dispatchTouchEvent(e);e.recycle();
        assertFalse(field("controllerMode").getBoolean(a));assertEquals(0,Bridge.x,0);assertEquals(0,Bridge.z,0);
        assertEquals(View.VISIBLE,button("attackButton").getVisibility());assertEquals(0,Bridge.attacks);
    }
    @Test public void menuConfirmActivatesFocusedNativeControlAndNeverLeaksIntoCombat()throws Exception {
        set("menuVisible",true);FrameLayout root=(FrameLayout)field("menuScrim").get(a);root.setVisibility(View.VISIBLE);
        LinearLayout panel=new LinearLayout(a);panel.setOrientation(LinearLayout.VERTICAL);int[] clicks={0};
        Button play=new Button(a);play.setText("Play");play.setOnClickListener(v->clicks[0]++);panel.addView(play);
        Button disabled=new Button(a);disabled.setText("Unavailable");disabled.setEnabled(false);panel.addView(disabled);root.addView(panel);
        press(KeyEvent.KEYCODE_BUTTON_A);assertEquals(1,clicks[0]);assertTrue(play.hasFocus());
        press(KeyEvent.KEYCODE_BUTTON_R2);press(KeyEvent.KEYCODE_BUTTON_L2);assertEquals(0,Bridge.attacks);assertEquals(0,Bridge.parries);
    }
    private static void focusDialog(AlertDialog dialog,boolean focused) {
        View decor=dialog.getWindow().getDecorView();decor.dispatchWindowFocusChanged(focused);
        // ViewRootImpl also dispatches this observer event; Robolectric's View alone does not.
        org.robolectric.util.ReflectionHelpers.callInstanceMethod(decor.getViewTreeObserver(),
                "dispatchOnWindowFocusChange",org.robolectric.util.ReflectionHelpers.ClassParameter.from(boolean.class,focused));
    }
    @Test public void nativeDialogConfirmAndCancelPreserveButtonCallbacks()throws Exception {
        set("menuVisible",true);int[] accepted={0};AlertDialog dialog=new AlertDialog.Builder(a).setTitle("Confirm")
                .setNegativeButton("Stay",null).setPositiveButton("Accept",(d,w)->accepted[0]++).create();dialog.show();
        call("installControllerDialog",new Class<?>[]{AlertDialog.class},dialog);focusDialog(dialog,true);set("controllerMode",true);
        Button accept=dialog.getButton(AlertDialog.BUTTON_POSITIVE);accept.setFocusableInTouchMode(true);accept.requestFocus();
        dialog.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_BUTTON_A,0));org.robolectric.shadows.ShadowLooper.idleMainLooper();assertEquals(1,accepted[0]);assertFalse(dialog.isShowing());
        AlertDialog cancel=new AlertDialog.Builder(a).setTitle("Cancel").setPositiveButton("Close",null).create();cancel.show();
        call("installControllerDialog",new Class<?>[]{AlertDialog.class},cancel);focusDialog(cancel,true);
        dialog.dispatchKeyEvent(key(KeyEvent.ACTION_UP,KeyEvent.KEYCODE_BUTTON_A,0));
        cancel.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_BUTTON_B,0));assertFalse(cancel.isShowing());assertEquals(0,Bridge.dodges);
    }
    @Test public void dialogFocusLossNeutralizesHeldNavigationAndRequiresRelease()throws Exception {
        set("menuVisible",true);set("controllerMode",true);set("controllerWindowFocused",false);
        AlertDialog dialog=new AlertDialog.Builder(a).setPositiveButton("Keep",null).create();dialog.show();
        call("installControllerDialog",new Class<?>[]{AlertDialog.class},dialog);
        View decor=dialog.getWindow().getDecorView();focusDialog(dialog,true);
        dialog.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DPAD_DOWN,0));
        focusDialog(dialog,false);
        AndroidControllerInput policy=(AndroidControllerInput)field("controllerInput").get(a);
        assertEquals("unfocused dialog must not repeat held navigation",0,policy.navigation(SystemClock.uptimeMillis()+600).vertical);
        focusDialog(dialog,true);
        dialog.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DPAD_DOWN,0));
        assertEquals("held navigation cannot replay on focus return",0,policy.navigation(SystemClock.uptimeMillis()+1200).vertical);
        dialog.dispatchKeyEvent(key(KeyEvent.ACTION_UP,KeyEvent.KEYCODE_DPAD_DOWN,0));
        dialog.dismiss();
    }
    @Test public void dialogFocusLossRejectsConfirmUntilWindowReturns()throws Exception {
        set("menuVisible",true);set("controllerMode",true);set("controllerWindowFocused",false);
        int[] accepted={0};AlertDialog dialog=new AlertDialog.Builder(a).setPositiveButton("Keep",(d,w)->accepted[0]++).create();dialog.show();
        call("installControllerDialog",new Class<?>[]{AlertDialog.class},dialog);
        Button keep=dialog.getButton(AlertDialog.BUTTON_POSITIVE);keep.setFocusableInTouchMode(true);keep.requestFocus();
        View decor=dialog.getWindow().getDecorView();focusDialog(dialog,true);focusDialog(dialog,false);
        dialog.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_BUTTON_A,0));
        dialog.dispatchKeyEvent(key(KeyEvent.ACTION_UP,KeyEvent.KEYCODE_BUTTON_A,0));
        org.robolectric.shadows.ShadowLooper.idleMainLooper();
        assertEquals("unfocused native window cannot accept a simulated confirm",0,accepted[0]);assertTrue(dialog.isShowing());
        focusDialog(dialog,true);keep.requestFocus();
        dialog.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_BUTTON_A,0));
        org.robolectric.shadows.ShadowLooper.idleMainLooper();assertEquals(1,accepted[0]);assertFalse(dialog.isShowing());
        assertEquals(0,Bridge.interacts);assertEquals(0,Bridge.attacks);
    }
    @Test public void rotationNeutralizesAxesWithoutChangingSavedSettings()throws Exception {
        motion(0,0,0,0);motion(.6f,-.5f,0,0);
        android.content.SharedPreferences p=a.getSharedPreferences("controller-integration",Context.MODE_PRIVATE);p.edit().putInt("render_scale",50).apply();
        a.onConfigurationChanged(new Configuration(a.getResources().getConfiguration()));
        assertEquals(0,Bridge.x,0);assertEquals(0,Bridge.z,0);assertEquals(50,p.getInt("render_scale",0));
        motion(.6f,-.5f,0,0);assertEquals("held axes cannot replay across rotation",0,Bridge.x,0);
        motion(0,0,0,0);motion(.6f,-.5f,0,0);assertTrue(Bridge.x>0);
    }
    private static Button findButton(View view,String text) {
        if(view instanceof Button && ((Button)view).getText().toString().equals(text))return (Button)view;
        if(view instanceof android.view.ViewGroup) {
            android.view.ViewGroup g=(android.view.ViewGroup)view;
            for(int i=0;i<g.getChildCount();i++){Button b=findButton(g.getChildAt(i),text);if(b!=null)return b;}
        }
        return null;
    }
    private void confirmLabel(String text)throws Exception {
        View root=(View)field("menuScrim").get(a);Button b=findButton(root,text);assertNotNull(text,b);
        b.setFocusableInTouchMode(true);assertTrue(b.requestFocus());press(KeyEvent.KEYCODE_BUTTON_A);
    }
    @Test public void startPauseSettingsAndGraphicsAreRealNativeControllerRoutes()throws Exception {
        set("firstMenu",false);press(KeyEvent.KEYCODE_BUTTON_START);assertTrue(field("menuVisible").getBoolean(a));assertTrue(Bridge.paused);
        confirmLabel(a.getString(R.string.settings));
        confirmLabel(a.getString(R.string.graphics_settings));assertTrue(field("graphicsVisible").getBoolean(a));
        assertNotNull(findButton((View)field("menuScrim").get(a),"Use"));
        assertFalse(((Button)field("graphicsConfirm").get(a)).isEnabled());
        assertFalse(a.getSharedPreferences("controller-integration",Context.MODE_PRIVATE).contains("graphics_pending"));
        assertEquals(0,Bridge.attacks);assertEquals(0,Bridge.interacts);
    }
    @Test public void disconnectPausesAndReconnectCannotReplayHeldMotion()throws Exception {
        set("firstMenu",false);motion(0,0,0,0);motion(.7f,0,0,0);assertTrue(Bridge.x>0);
        ((InputManager.InputDeviceListener)field("controllerDevices").get(a)).onInputDeviceRemoved(9);
        assertEquals(0,Bridge.x,0);assertTrue(field("menuVisible").getBoolean(a));assertTrue(Bridge.paused);assertFalse(field("controllerMode").getBoolean(a));
        assertNotNull(findButton((View)field("menuScrim").get(a),a.getString(R.string.resume_demo)));
        motion(.7f,0,0,0);assertFalse(field("controllerMode").getBoolean(a));assertEquals(0,Bridge.x,0);
        confirmLabel(a.getString(R.string.resume_demo));assertFalse(field("menuVisible").getBoolean(a));assertFalse(Bridge.paused);
        motion(.7f,0,0,0);assertEquals(0,Bridge.x,0);motion(0,0,0,0);motion(.7f,0,0,0);assertTrue(Bridge.x>0);
    }
    @Test public void deathRetryAndEndingContinueAreReachableWithoutCombatLeakage()throws Exception {
        set("firstMenu",false);Bridge.retries=0;call("showDeathOverlay",new Class<?>[]{});
        confirmLabel(a.getString(R.string.retry_encounter));assertEquals(1,Bridge.retries);assertTrue(field("retryPending").getBoolean(a));
        set("deathOverlayVisible",false);set("retryPending",false);set("menuVisible",false);
        call("showEndingOverlay",new Class<?>[]{});assertTrue(field("endingOverlayVisible").getBoolean(a));
        press(KeyEvent.KEYCODE_BUTTON_B);assertFalse(field("endingOverlayVisible").getBoolean(a));assertFalse(field("menuVisible").getBoolean(a));assertEquals(0,Bridge.dodges);
    }
    @Test public void coldEntryMoreSettingsPlayAndPendingBackAreControllerAccessible()throws Exception {
        set("entryMenuEnabled",true);call("showEntryMenu",new Class<?>[]{boolean.class},false);
        confirmLabel(a.getString(R.string.entry_more));assertTrue(field("entryMenuSidePage").getBoolean(a));
        press(KeyEvent.KEYCODE_BUTTON_B);assertFalse(field("entryMenuSidePage").getBoolean(a));
        confirmLabel(a.getString(R.string.settings));assertTrue(field("entryMenuSidePage").getBoolean(a));
        press(KeyEvent.KEYCODE_BUTTON_B);assertFalse(field("entryMenuSidePage").getBoolean(a));
        confirmLabel(a.getString(R.string.entry_play));assertTrue(field("entryPlayRequested").getBoolean(a));
        assertTrue(field("menuVisible").getBoolean(a));assertTrue(Bridge.paused);
        press(KeyEvent.KEYCODE_BUTTON_B);assertFalse(field("entryPlayRequested").getBoolean(a));assertTrue(field("menuVisible").getBoolean(a));
    }
    @Test public void sidePagesResizeInBothLandscapeDirectionsAndPortraitWithoutRebuild()throws Exception {
        set("menuVisible",true);call("showSettings",new Class<?>[]{});press(KeyEvent.KEYCODE_BUTTON_Y);
        FrameLayout root=(FrameLayout)field("menuScrim").get(a);View focus=root.findFocus();assertNotNull(focus);
        android.content.SharedPreferences prefs=a.getSharedPreferences("controller-integration",Context.MODE_PRIVATE);java.util.Map<String,?> saved=prefs.getAll();
        for(int[] size:new int[][]{{640,360},{360,640},{640,360}}) {
            root.layout(0,0,size[0],size[1]);call("relayoutMenuForViewport",new Class<?>[]{});
            assertTrue(root.getChildAt(0).getLayoutParams().width<=size[0]);assertEquals(focus,root.findFocus());assertEquals(saved,prefs.getAll());
        }
    }
    @Test public void dismissedNestedDialogReturnsToTheStillVisibleParent()throws Exception {
        set("menuVisible",true);set("controllerMode",true);AlertDialog parent=new AlertDialog.Builder(a).setPositiveButton("Parent",null).create();parent.show();
        call("installControllerDialog",new Class<?>[]{AlertDialog.class},parent);
        AlertDialog child=new AlertDialog.Builder(a).setPositiveButton("Child",null).create();child.show();call("installControllerDialog",new Class<?>[]{AlertDialog.class},child);
        child.dismiss();assertEquals(parent,call("currentControllerDialog",new Class<?>[]{}));focusDialog(parent,true);
        parent.dispatchKeyEvent(key(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_BUTTON_B,0));assertFalse(parent.isShowing());assertEquals(0,Bridge.dodges);
    }
    @Test public void focusLossNeutralizesTouchRolesAndPausesActiveController()throws Exception {
        set("firstMenu",false);motion(0,0,0,0);motion(.6f,0,0,0);
        int[] pointers=(int[])field("activePointers").get(a);pointers[0]=3;pointers[1]=7;set("parryTouchActive",true);
        a.onWindowFocusChanged(false);assertArrayEquals(new int[]{-1,-1},pointers);assertFalse(field("parryTouchActive").getBoolean(a));
        assertEquals(0,Bridge.x,0);assertTrue(field("menuVisible").getBoolean(a));assertTrue(Bridge.paused);
        press(KeyEvent.KEYCODE_BUTTON_R2);assertEquals(0,Bridge.attacks);
        a.onWindowFocusChanged(true);confirmLabel(a.getString(R.string.resume_demo));assertFalse(field("menuVisible").getBoolean(a));
    }
    @Test public void rapidDpadPressNavigatesBeforeItsRelease()throws Exception {
        set("menuVisible",true);FrameLayout root=(FrameLayout)field("menuScrim").get(a);root.setVisibility(View.VISIBLE);
        LinearLayout panel=new LinearLayout(a);panel.setOrientation(LinearLayout.VERTICAL);root.addView(panel);
        Button one=new Button(a),two=new Button(a);panel.addView(one);panel.addView(two);
        press(KeyEvent.KEYCODE_BUTTON_Y);assertTrue(one.hasFocus());
        press(KeyEvent.KEYCODE_DPAD_DOWN);assertTrue(two.hasFocus());assertEquals(0,Bridge.lanterns);
    }
    @Test public void suspensionAndKeyboardDoNotProduceControllerActions()throws Exception {
        press(KeyEvent.KEYCODE_BUTTON_X);set("controllerWindowFocused",false);
        press(KeyEvent.KEYCODE_BUTTON_R2);assertEquals(1,Bridge.attacks);assertEquals(0,Bridge.x,0);
        a.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DPAD_CENTER));assertEquals(1,Bridge.attacks);
        set("controllerWindowFocused",true);press(KeyEvent.KEYCODE_BUTTON_X);assertEquals(2,Bridge.attacks);
    }
}
