package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ScrollView;
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
        static long[] entryState;
        static String directionsText;
        static boolean paused,entry,crouch;
        static float strafe,forward;
        static int lastCampaignAction,lastCampaignKind,resets,campaignRequests,attacks,parries,dodges,interacts,lanterns;
        static final List<String> order=new ArrayList<>();
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static long[] getLookoutObjectiveState(long generation){return objective==null?null:objective.clone();}
        @Implementation protected static String getLookoutObjectiveText(long surface,long world,int id) {
            if(objective==null || objective[1]!=surface || objective[2]!=world || objective[3]!=id)return "";
            if (directionsText != null) return directionsText;
            switch(id) {
                case 1: return "Follow the road down to Bellwether. The tavern is across the square.";
                case 2: return "Ask the record keeper by the northern church about the wayside shrine.\r\n\r\n"
                        + "For a breathing rig, follow the MILL signs east and inspect the gearing at the artificer's workbench.\r\n\r\n"
                        + "Shore practice: Meet the salvager at the river fork.";
                case 3: return "Recover the road register from the opened wayside archive.\r\n\r\n"
                        + "Purse: 0 coins.\r\nOrdinary silver: sell it at the stall west of the square.";
                case 4: return "Show the road register to the record keeper by the northern church.";
                case 5: return "The Abbey lies downstream. The Foundry is east, across the gorge. Both are leads for future expeditions.";
                case 7: return "Follow the road to the covered well in Bellwether square. At the fingerpost, take the ramp with the oak rail. The lit tankard sign hangs beside the tavern's south door.";
                case 8: return "Meet Kit by the tavern hearth. Choose Talk when the keeper joins him and the prompt appears.";
                case 9: return "The wayside archive is open. Stand near the opening and choose Listen to the lantern.";
                case 10: return "The fingerpost by the covered well";
                case 11: return "The ledger is beside the oil lamp on the sorting table. Follow the ramp back to the road.";
                default: return "";
            }
        }
        @Implementation protected static int getSurfaceRuntimeState(long generation){return 1;}
        @Implementation protected static long[] getEntryMenuState(){return entryState==null?null:entryState.clone();}
        @Implementation protected static int getContextualControlState(){return 0;}
        @Implementation protected static void setSimulationPaused(boolean value){paused=value;order.add(value?"pause":"resume");}
        @Implementation protected static void setViewControls(float y,float p,float light,float x,float z,boolean c,boolean analogueMovement){strafe=x;forward=z;crouch=c;order.add("input");}
        @Implementation protected static void setRunHeld(boolean held){}
        @Implementation protected static void clearRunIntent(){}
        @Implementation protected static void clearCrouchIntent(){crouch=false;}
        @Implementation protected static boolean isRtLabUnlockEligible(){return false;}
        @Implementation protected static void setEntryMenu(boolean enabled,boolean side,boolean reduced,boolean play,long generation){
            // The native owner invalidates prior presentation synchronously on a profile-enable change.
            if(enabled!=entry && entryState!=null)entryState[4]=0;
            entry=enabled;order.add("entry");
        }
        @Implementation protected static long requestCampaign(int action,int slot,int kind,long generation){campaignRequests++;lastCampaignAction=action;lastCampaignKind=kind;return 1;}
        @Implementation protected static void requestRouteReset(){resets++;}
        @Implementation protected static boolean setDiagnosticSurfaceSuspended(long generation,boolean suspended){return true;}
        @Implementation protected static void requestAttack(){attacks++;}
        @Implementation protected static void requestParry(){parries++;}
        static boolean guardHeld;
        @Implementation protected static void setGuardHeld(boolean held) {
            if (held && !guardHeld) ++parries;
            guardHeld = held;
        }
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
        Bridge.objective=new long[]{2,31,81,1};Bridge.entryState=null;Bridge.guardHeld=false;
        Bridge.directionsText=null;
        Bridge.paused=Bridge.entry=Bridge.crouch=false;
        Bridge.strafe=Bridge.forward=0;Bridge.order.clear();
        Bridge.resets=Bridge.campaignRequests=Bridge.attacks=Bridge.parries=Bridge.dodges=Bridge.interacts=Bridge.lanterns=0;
        a=Robolectric.buildActivity(MainActivity.class).get();a.setContentView(R.layout.activity_main);
        set("preferences",a.getSharedPreferences("lookout-menu",Context.MODE_PRIVATE));
        a.getSharedPreferences("lookout-menu",Context.MODE_PRIVATE).edit().clear().putBoolean("graphics_mist_enabled",false).putInt("owner_marker",17).commit();
        set("resumed",true);set("firstMenu",false);set("startupAssetsPrepared",true);set("surfaceStarted",true);set("surfaceRequestGeneration",31L);
        String[] names={"surfaceView","menuScrim","menuButton","attackButton","parryButton","dodgeButton","runButton","crouchButton",
                "interactButton","toggleHeldLightPoseButton","diagnosticsPanel","developerOverlay","vitalityStatus","rtStatus","controllerPrompt"};
        int[] ids={R.id.scene_surface,R.id.menu_scrim,R.id.menu_button,R.id.attack_button,R.id.parry_button,R.id.dodge_button,R.id.run_button,R.id.crouch_button,
                R.id.interact_button,R.id.toggle_held_light_pose_button,R.id.diagnostics_panel,R.id.developer_overlay,R.id.vitality_status,R.id.rt_status,R.id.controller_prompt};
        for(int i=0;i<names.length;i++)set(names[i],a.findViewById(ids[i]));
    }
    @Test public void malformedStaleAndUnavailablePublicationsCannotOfferThePage()throws Exception {
        for(long[] value:new long[][]{null,new long[4],new long[]{1,31,81,1},new long[]{2,30,81,1},
                new long[]{2,31,0,1},new long[]{2,31,81,12},new long[]{2,31,81,0}}) {
            Bridge.objective=value;pause();assertNull(find("Lookout objective"));
            assertNull(find("Bellwether directions"));
            assertFalse(field("lookoutObjectiveVisible").getBoolean(a));
        }
        Bridge.objective=new long[]{2,31,81,1};set("deathOverlayVisible",true);pause();assertNull(find("Lookout objective"));
    }
    @Test public void arrivalOnlyOffersAnExplicitPauseChoiceAndContinueClearsHeldInput()throws Exception {
        assertFalse(field("lookoutObjectiveVisible").getBoolean(a));pause();
        assertNotNull(find("Lookout objective"));assertNull(find("Continue exploring"));
        int[] pointers=(int[])field("activePointers").get(a);pointers[0]=5;pointers[1]=8;
        float[] controls=(float[])field("viewControls").get(a);controls[7]=.7f;controls[8]=.8f;
        set("parryTouchActive",true);set("crouchControlKeys",1);
        find("Lookout objective").performClick();
        assertTrue(field("lookoutObjectiveVisible").getBoolean(a));assertTrue(Bridge.paused);
        assertTrue(text(a.findViewById(R.id.menu_scrim)).contains("The tavern is across the square"));
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
    @Test public void bellwetherDirectionsFollowTheEarnedStageAndBackStaysPaused()throws Exception {
        Bridge.objective=new long[]{2,31,81,4};pause();
        assertNull(find("Lookout objective"));
        find("Bellwether directions").performClick();
        assertTrue(text(a.findViewById(R.id.menu_scrim)).contains("Show the road register"));
        assertNull(find("Return to menu"));
        a.onBackPressed();
        assertTrue(Bridge.paused);assertNotNull(find("Bellwether directions"));
        find("Bellwether directions").performClick();Button old=find("Continue exploring");
        Bridge.objective[3]=5;old.performClick();
        assertTrue(Bridge.paused);assertFalse(field("lookoutObjectiveVisible").getBoolean(a));
        find("Bellwether directions").performClick();
        assertTrue(text(a.findViewById(R.id.menu_scrim)).contains("future expeditions"));
        find("Back to pause menu").performClick();assertTrue(Bridge.paused);
        find("Bellwether directions").performClick();find("Continue exploring").performClick();
        assertFalse(Bridge.paused);assertEquals(0,Bridge.campaignRequests);assertEquals(0,actionCount());
    }

    @Test public void bellwetherMenuReadsPreparationAndTradeDirectionsFromTheCurrentPublication()throws Exception {
        Bridge.objective[3]=2;
        pause();
        find("Bellwether directions").performClick();
        String opening=text(a.findViewById(R.id.menu_scrim));
        assertTrue(opening.contains("artificer's workbench"));
        assertTrue(opening.contains("Meet the salvager at the river fork"));
        find("Back to pause menu").performClick();
        assertTrue(Bridge.paused);
        Bridge.objective[3]=3;
        pause();
        find("Bellwether directions").performClick();
        String archive=text(a.findViewById(R.id.menu_scrim));
        assertTrue(archive.contains("Recover the road register"));
        assertTrue(archive.contains("sell it at the stall west of the square"));
        assertEquals(0,Bridge.campaignRequests);
        assertEquals(0,actionCount());
    }
    @Test public void bellwetherDirectionsOpenForTheRoadHearthLanternSquareAndStockRoom()throws Exception {
        String[] expected={"lit tankard sign","Choose Talk","Listen to the lantern",
                "fingerpost by the covered well","oil lamp on the sorting table"};
        for(int i=0;i<expected.length;i++) {
            Bridge.objective[3]=7+i;
            pause();
            assertNull(find("Lookout objective"));
            find("Bellwether directions").performClick();
            assertTrue(text(a.findViewById(R.id.menu_scrim)).contains(expected[i]));
            find("Back to pause menu").performClick();
            assertTrue(Bridge.paused);
        }
        assertEquals(0,Bridge.campaignRequests);
        assertEquals(0,actionCount());
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
        Bridge.entryState=new long[]{31,2,0,0,1}; // Retained old Entry publication cannot certify a new request.
        open();find("Return to menu").performClick();
        assertTrue(Bridge.paused);assertTrue(Bridge.entry);assertTrue(field("entryMenuEnabled").getBoolean(a));
        assertEquals(0,Bridge.resets);assertEquals(1,Bridge.campaignRequests);assertEquals(0,actionCount());
        assertEquals("return releases ownership only; it does not save/load/reset",CampaignMenuState.END_GAME,Bridge.lastCampaignAction);
        assertEquals("ordinary return never acknowledges a failed save automatically",0,Bridge.lastCampaignKind);
        assertEquals(before,a.getSharedPreferences("lookout-menu",Context.MODE_PRIVATE).getAll());
        assertEquals("request invalidates old same-generation presentation",0,Bridge.entryState[4]);
        assertTrue("return publishes the request but waits for the Entry frame",field("startupUiVisible").getBoolean(a));
        assertNull(find(a.getString(R.string.entry_play)));
        Bridge.entryState=new long[]{31,2,0,0,1};
        call("showEntryMenu",new Class<?>[]{boolean.class},false);
        assertFalse(field("startupUiVisible").getBoolean(a));
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

    private ScrollView openLongDirections() throws Exception {
        Bridge.objective[3] = 5;
        Bridge.directionsText = "The Abbey lies downstream. The Foundry is east, across the gorge.\n\n"
                + "The fingerpost by the covered well\nTankard - up the oak-railed ramp to the tavern.\n"
                + "Scales - west to the dealer, then on to the kitchen gardens.\n"
                + "Wheel - east to the mill and the river fork.\nAnvil - southeast to the smith yard.\n\n"
                + "Purse: 2 coins.\nLantern hinge fitted.\nThe mill's tally stamp has been returned.\n\n"
                + "For an air ward, return to the wayside shrine and restore its binding.\n\n"
                + "Follow the stone causeway south of the mill to the river rail. Start the leak test "
                + "in the shallows and stay there for two seconds. Return to dry stone to confirm the seal.\n\n"
                + "Shore practice: Return to the salvager at the river fork to report back.";
        pause();
        find("Bellwether directions").performClick();
        ViewGroup root = a.findViewById(R.id.menu_scrim);
        root.measure(View.MeasureSpec.makeMeasureSpec(720, View.MeasureSpec.EXACTLY),
                View.MeasureSpec.makeMeasureSpec(260, View.MeasureSpec.EXACTLY));
        root.layout(0, 0, 720, 260);
        return (ScrollView) root.getChildAt(0);
    }

    @Test public void controllerReadsLongDirectionsBeforeReachingTheActions() throws Exception {
        set("controllerMode", true);
        ScrollView scroll = openLongDirections();
        View root = a.findViewById(R.id.menu_scrim);
        View heading = ControllerNavigation.ensureFocus(root);
        assertEquals("Directions", ((TextView) heading).getText().toString());
        assertEquals(0, scroll.getScrollY());
        assertFalse(ControllerNavigation.confirm(root));
        assertTrue(Bridge.paused);

        ControllerNavigation.navigate(root, 0, 1);
        assertTrue(scroll.getScrollY() > 0);
        assertTrue(heading.hasFocus());
        ControllerNavigation.navigate(root, 0, -1);
        assertEquals(0, scroll.getScrollY());
        for (int i = 0; i < 30 && heading.hasFocus(); ++i) {
            ControllerNavigation.navigate(root, 0, 1);
        }
        assertTrue(find("Continue exploring").hasFocus());
        ControllerNavigation.navigate(root, 0, 1);
        assertTrue(find("Back to pause menu").hasFocus());
        assertTrue(ControllerNavigation.confirm(root));
        assertTrue(Bridge.paused);
        assertEquals(0, Bridge.campaignRequests);
        assertEquals(0, actionCount());
    }

    @Test public void keyboardArrowsCanReadLongDirectionsFromTheTop() throws Exception {
        ScrollView scroll = openLongDirections();
        View heading = scroll.findFocus();
        assertEquals("Directions", ((TextView) heading).getText().toString());
        assertEquals(0, scroll.getScrollY());
        assertTrue(heading.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_DOWN)));
        assertTrue(scroll.getScrollY() > 0);
        assertTrue(heading.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_UP)));
        assertEquals(0, scroll.getScrollY());
        assertTrue(Bridge.paused);
    }
}
