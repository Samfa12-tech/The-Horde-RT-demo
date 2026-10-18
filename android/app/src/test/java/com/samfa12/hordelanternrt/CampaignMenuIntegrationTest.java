package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.app.AlertDialog;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
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
import org.robolectric.annotation.LooperMode;

/** Real menu/input methods with immutable fake native publications; no filesystem or Vulkan. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=CampaignMenuIntegrationTest.Bridge.class)
@LooperMode(LooperMode.Mode.PAUSED)
public final class CampaignMenuIntegrationTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class Bridge {
        static long[] state;
        static long next, ackToken, ackGeneration;
        static int ackCalls, configurations;
        static String configuredPath;
        static boolean paused, acceptAck;
        static float yaw,pitch,x,z;
        static final List<Integer> actions=new ArrayList<>();
        static final List<String> order=new ArrayList<>();
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static boolean configureCampaign(String path){configurations++;configuredPath=path;return true;}
        @Implementation protected static long[] getCampaignState(){ return state==null ? null : state.clone(); }
        @Implementation protected static long requestCampaign(int a,int s,int k,long g){actions.add(a);return ++next;}
        @Implementation protected static boolean acknowledgeCampaignInput(long t,long g){ackToken=t;ackGeneration=g;ackCalls++;order.add("ack");return acceptAck;}
        @Implementation protected static void setSimulationPaused(boolean p){paused=p;order.add(p?"paused":"unpaused");}
        @Implementation protected static void setViewControls(float y,float p,float light,float xx,float zz,boolean c){yaw=y;pitch=p;x=xx;z=zz;order.add("input");}
        @Implementation protected static void setEntryMenu(boolean e,boolean s,boolean r,boolean p,long g){if(p)order.add("play");}
        @Implementation protected static int getSurfaceRuntimeState(long g){return 1;}
        @Implementation protected static int getContextualControlState(){return 0;}
        @Implementation protected static void clearRunIntent(){}
        @Implementation protected static void clearCrouchIntent(){}
        @Implementation protected static void setRunHeld(boolean h){}
        @Implementation protected static boolean isRtLabUnlockEligible(){return false;}
    }
    private MainActivity a;
    private static Field field(String n)throws Exception {Field f=MainActivity.class.getDeclaredField(n);f.setAccessible(true);return f;}
    private void set(String n,Object v)throws Exception {field(n).set(a,v);}
    private Object call(String n,Class<?>[] types,Object... args)throws Exception {
        Method m=MainActivity.class.getDeclaredMethod(n,types);m.setAccessible(true);return m.invoke(a,args);
    }
    private void call(String n)throws Exception {call(n,new Class<?>[]{});}
    private Button find(String text) {return find((ViewGroup)a.findViewById(R.id.menu_scrim),text);}
    private Button find(ViewGroup group,String text) {
        for(int i=0;i<group.getChildCount();i++) {
            View v=group.getChildAt(i);
            if(v instanceof Button && ((Button)v).getText().toString().equals(text))return (Button)v;
            if(v instanceof ViewGroup) {Button b=find((ViewGroup)v,text);if(b!=null)return b;}
        }
        return null;
    }
    private String menuText(View view) {
        String text=view instanceof TextView ? ((TextView)view).getText().toString()+"\n" : "";
        if(view instanceof ViewGroup) {
            ViewGroup group=(ViewGroup)view;
            for(int i=0;i<group.getChildCount();i++) text+=menuText(group.getChildAt(i));
        }
        return text;
    }
    private void open(int action)throws Exception {
        call("showCampaignMenu",new Class<?>[]{int.class},action);
        Bridge.state[3]=Bridge.next;
        Bridge.state[2]++;
        call("pollCampaignMenu");
    }
    private void choose(int action,int slot,int kind)throws Exception {
        call("chooseCampaignRecord",new Class<?>[]{int.class,int.class,int.class,CampaignMenuState.class},
                action,slot,kind,field("campaignState").get(a));
    }
    @Before public void setup()throws Exception {
        Bridge.state=CampaignMenuStateTest.ready(); Bridge.state[7]=1;
        Bridge.next=Bridge.ackToken=Bridge.ackGeneration=0; Bridge.paused=false; Bridge.acceptAck=true;Bridge.ackCalls=Bridge.configurations=0;Bridge.configuredPath=null;
        Bridge.actions.clear();Bridge.order.clear();Bridge.yaw=Bridge.pitch=Bridge.x=Bridge.z=0;
        a=Robolectric.buildActivity(MainActivity.class).get();a.setContentView(R.layout.activity_main);
        set("preferences",a.getSharedPreferences("campaign-menu",Context.MODE_PRIVATE));
        set("resumed",true);set("firstMenu",false);set("surfaceStarted",true);set("surfaceRequestGeneration",31L);
        set("campaignConfigured",true);
        String[] names={"surfaceView","menuScrim","menuButton","attackButton","parryButton","dodgeButton","runButton","crouchButton",
                "interactButton","toggleHeldLightPoseButton","diagnosticsPanel","developerOverlay","vitalityStatus","rtStatus","controllerPrompt"};
        int[] ids={R.id.scene_surface,R.id.menu_scrim,R.id.menu_button,R.id.attack_button,R.id.parry_button,R.id.dodge_button,R.id.run_button,R.id.crouch_button,
                R.id.interact_button,R.id.toggle_held_light_pose_button,R.id.diagnostics_panel,R.id.developer_overlay,R.id.vitality_status,R.id.rt_status,R.id.controller_prompt};
        for(int i=0;i<names.length;i++)set(names[i],a.findViewById(ids[i]));
    }
    @Test public void entryPlaySelectsSlotBeforeRequestingNativePlay()throws Exception {
        set("entryMenuEnabled",true);
        call("requestEntryPlay");
        assertTrue(field("campaignMenuVisible").getBoolean(a));
        assertFalse(field("entryPlayRequested").getBoolean(a));
        assertEquals(List.of(1),Bridge.actions);
    }
    @Test public void emptyAndInvalidRecordsCannotLoadAndManualSaveUsesNativeSafeGate()throws Exception {
        Bridge.state[12]=4;Bridge.state[15]=5;
        open(CampaignMenuState.LOAD);
        assertFalse(find("Load manual").isEnabled());assertFalse(find("Load latest autosave").isEnabled());
        set("campaignMenuVisible",false);Bridge.state[7]=0;
        open(CampaignMenuState.SAVE_MANUAL);
        assertFalse(find("Save manual in Slot 1").isEnabled());
    }
    @Test public void occupiedManualRequiresFreshExplicitConfirmationAndCancelledButtonCannotWrite()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;
        open(CampaignMenuState.SAVE_MANUAL);choose(2,0,0);
        AlertDialog dialog=(AlertDialog)field("campaignConfirmation").get(a);assertNotNull(dialog);
        a.onBackPressed();assertFalse(dialog.isShowing());
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).performClick();
        assertEquals(List.of(1),Bridge.actions);
        choose(2,0,0);dialog=(AlertDialog)field("campaignConfirmation").get(a);
        // Identical publications keep the reviewable confirmation usable.
        call("pollCampaignMenu");dialog.getButton(AlertDialog.BUTTON_POSITIVE).performClick();
        assertEquals(List.of(1,2),Bridge.actions);
    }
    @Test public void changedCatalogueRevokesConfirmationRatherThanReplacingUnreviewedRecord()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;
        open(2);choose(2,0,0);AlertDialog dialog=(AlertDialog)field("campaignConfirmation").get(a);
        Bridge.state[14]=2;Bridge.state[2]++;call("pollCampaignMenu");
        assertFalse(dialog.isShowing());dialog.getButton(AlertDialog.BUTTON_POSITIVE).performClick();
        assertEquals(List.of(1),Bridge.actions);
    }
    @Test public void loadRebaseClearsHeldInputsPublishesPausedCameraBeforeAckAndWaitsForPresentation()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;
        open(3);choose(3,0,0);
        float[] controls=(float[])field("viewControls").get(a);controls[7]=.7f;controls[8]=.4f;
        int[] pointers=(int[])field("activePointers").get(a);pointers[0]=3;pointers[1]=7;
        set("parryTouchActive",true);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=81;
        Bridge.state[9]=Float.floatToIntBits(12.3f);Bridge.state[10]=Float.floatToIntBits(-.2f);Bridge.state[30]=3;
        Bridge.order.clear();call("pollCampaignMenu");
        assertEquals(81,Bridge.ackToken);assertEquals(31,Bridge.ackGeneration);assertTrue(Bridge.paused);
        assertEquals("ack",Bridge.order.get(Bridge.order.size()-1));assertFalse(Bridge.order.contains("unpaused"));
        assertEquals(12.3f,Bridge.yaw,0);assertEquals(-.2f,Bridge.pitch,0);assertEquals(0,Bridge.x,0);assertEquals(0,Bridge.z,0);
        assertArrayEquals(new int[]{-1,-1},pointers);assertFalse(field("parryTouchActive").getBoolean(a));
        Bridge.state[2]++;Bridge.state[4]=6;call("pollCampaignMenu");assertNull(find("Resume"));
        Bridge.state[2]++;Bridge.state[4]=7;call("pollCampaignMenu");assertNull(find("Resume"));
        Bridge.state[2]++;Bridge.state[11]=1;call("pollCampaignMenu");
        assertNotNull(find("Resume"));assertTrue(Bridge.paused);
        assertFalse(find("Refresh records").isEnabled());
        find("Resume").performClick();assertFalse(Bridge.paused);assertFalse(field("menuVisible").getBoolean(a));
    }
    @Test public void staleOrMalformedSnapshotCannotRebaseCameraOrAcknowledge()throws Exception {
        open(3);Bridge.state[1]=30;Bridge.state[4]=5;Bridge.state[8]=88;Bridge.state[9]=Float.floatToIntBits(9);
        call("pollCampaignMenu");assertEquals(0,Bridge.ackToken);assertEquals(0,Bridge.yaw,0);
        assertFalse(field("campaignPublicationValid").getBoolean(a));
        Bridge.state[1]=31;Bridge.state[9]=Float.floatToIntBits(Float.NaN);call("pollCampaignMenu");assertEquals(0,Bridge.ackToken);
    }
    @Test public void backRequestsCancellationButCannotResumePendingOrCommittedUnrebasedLoad()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;
        open(3);choose(3,0,0);Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=4;Bridge.state[30]=3;
        call("pollCampaignMenu");a.onBackPressed();assertEquals(List.of(1,3,4),Bridge.actions);
        assertTrue(field("campaignMenuVisible").getBoolean(a));assertTrue(Bridge.paused);
        Bridge.state[2]++;Bridge.state[4]=5;Bridge.state[8]=99;call("pollCampaignMenu");
        assertEquals(99,Bridge.ackToken);call("hideMenu");assertTrue(Bridge.paused);assertTrue(field("menuVisible").getBoolean(a));
    }
    @Test public void uncertainWriteNeverDisplaysSavedOrEnablesBlindRetry()throws Exception {
        open(2);choose(2,0,0);Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=8;Bridge.state[5]=10;Bridge.state[30]=2;
        call("pollCampaignMenu");assertEquals(10,((CampaignMenuState)field("campaignState").get(a)).error);
        assertFalse(((CampaignMenuState)field("campaignState").get(a)).message().contains("saved"));assertTrue(Bridge.paused);
        assertFalse(find("Save manual in Slot 1").isEnabled());
    }
    @Test public void onlyMatchingNewGameAcknowledgementStartsExistingEntryHandoff()throws Exception {
        set("entryMenuEnabled",true);open(5);choose(5,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next-1;Bridge.state[4]=7;Bridge.state[30]=5;
        call("pollCampaignMenu");assertFalse(field("entryPlayRequested").getBoolean(a));
        float[] controls=(float[])field("viewControls").get(a);controls[7]=.8f;controls[8]=.6f;
        int[] pointers=(int[])field("activePointers").get(a);pointers[0]=3;pointers[1]=7;
        Bridge.state[9]=Float.floatToIntBits((float)Math.PI);Bridge.state[10]=Float.floatToIntBits(-.15f);
        Bridge.order.clear();Bridge.state[2]++;Bridge.state[3]=Bridge.next;call("pollCampaignMenu");
        assertEquals((float)Math.PI,Bridge.yaw,0);assertEquals(-.15f,Bridge.pitch,0);
        assertEquals(0,Bridge.x,0);assertEquals(0,Bridge.z,0);assertArrayEquals(new int[]{-1,-1},pointers);
        assertTrue(Bridge.order.contains("play"));assertTrue(Bridge.order.lastIndexOf("input")<Bridge.order.indexOf("play"));
        assertFalse(Bridge.order.contains("unpaused"));
        assertTrue(field("entryPlayRequested").getBoolean(a));assertFalse(field("campaignMenuVisible").getBoolean(a));
        assertTrue(Bridge.paused);
    }
    @Test public void deniedRebaseWaitsForFreshPublicationAndPauseRevokesPendingRequest()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;
        open(3);choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=71;Bridge.state[30]=3;
        Bridge.acceptAck=false;call("pollCampaignMenu");assertEquals(1,Bridge.ackCalls);
        call("pollCampaignMenu");assertEquals(1,Bridge.ackCalls);
        Bridge.acceptAck=true;Bridge.state[2]++;call("pollCampaignMenu");assertEquals(2,Bridge.ackCalls);
        a.onPause();assertEquals(List.of(1,3,4),Bridge.actions);assertTrue(Bridge.paused);
        Bridge.state[2]++;Bridge.state[4]=7;Bridge.state[11]=1;
        call("pollCampaignMenu");assertFalse(field("campaignLoadedReady").getBoolean(a));
    }
    @Test public void malformedReplacementSnapshotDisablesFormerlyValidLoadButton()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;open(3);
        assertTrue(find("Load manual").isEnabled());
        Bridge.state[0]=99;call("pollCampaignMenu");assertFalse(find("Load manual").isEnabled());
        choose(3,0,0);assertEquals(List.of(1),Bridge.actions);
        Bridge.state[0]=1;call("pollCampaignMenu");assertTrue(find("Load manual").isEnabled());
    }
    @Test public void cancelAfterCommitRequiresExactRebaseTokenBeforeOfferingResume()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;open(3);choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=71;Bridge.state[30]=3;
        call("pollCampaignMenu");a.onBackPressed();assertEquals(List.of(1,3,4),Bridge.actions);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=7;Bridge.state[11]=1;Bridge.state[8]=70;
        call("pollCampaignMenu");assertNull(find("Resume"));
        Bridge.state[2]++;Bridge.state[8]=71;call("pollCampaignMenu");assertNotNull(find("Resume"));
        a.onBackPressed();assertFalse(field("campaignMenuVisible").getBoolean(a));
        assertTrue(field("menuVisible").getBoolean(a));assertTrue(Bridge.paused);
    }
    @Test public void queuedSecondLoadCannotReusePriorSuccessBeforeCurrentRequestAcknowledgement()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;open(3);choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=71;Bridge.state[30]=3;
        call("pollCampaignMenu");Bridge.state[2]++;Bridge.state[4]=7;Bridge.state[11]=1;call("pollCampaignMenu");
        assertNotNull(find("Resume"));choose(3,0,0);
        call("pollCampaignMenu");assertFalse(field("campaignLoadedReady").getBoolean(a));
        call("resumeLoadedCampaign");assertTrue(Bridge.paused);assertTrue(field("campaignMenuVisible").getBoolean(a));
    }
    @Test public void invalidatedLoadedPublicationDoesNotTrapBackOrReuseResume()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;open(3);choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=71;Bridge.state[30]=3;
        call("pollCampaignMenu");Bridge.state[2]++;Bridge.state[4]=7;Bridge.state[11]=1;call("pollCampaignMenu");
        assertTrue(field("campaignLoadedReady").getBoolean(a));
        Bridge.state[0]=99;call("pollCampaignMenu");assertFalse(field("campaignLoadedReady").getBoolean(a));
        a.onBackPressed();assertFalse(field("campaignMenuVisible").getBoolean(a));assertTrue(Bridge.paused);
    }
    @Test public void delayedOlderDirectoryWorkerCannotConfigureAfterNewActivityExistsEvenBeforeOldDestroy()throws Exception {
        MainActivity old=a;
        MainActivity newer=Robolectric.buildActivity(MainActivity.class).get();
        assertFalse(field("campaignDestroyed").getBoolean(old));
        call("completeCampaignConfiguration",new Class<?>[]{String.class,boolean.class},"synthetic-old-test-root",true);
        assertEquals(0,Bridge.configurations);
        a=newer;call("completeCampaignConfiguration",new Class<?>[]{String.class,boolean.class},"synthetic-new-test-root",true);
        assertEquals(1,Bridge.configurations);assertEquals("synthetic-new-test-root",Bridge.configuredPath);
        set("campaignDestroyed",true);
        call("completeCampaignConfiguration",new Class<?>[]{String.class,boolean.class},"synthetic-destroyed-root",true);
        assertEquals(1,Bridge.configurations);
    }
    @Test public void unavailableDirectoryDoesNotInvokeNativeConfigurationOrClaimReady()throws Exception {
        set("campaignConfigured",false);
        call("completeCampaignConfiguration",new Class<?>[]{String.class,boolean.class},"synthetic-test-root",false);
        assertEquals(0,Bridge.configurations);assertFalse(field("campaignConfigured").getBoolean(a));
        assertTrue(((String)field("campaignLocalMessage").get(a)).contains("unavailable"));
    }
    @Test public void currentReadyCatalogueClearsOnlyStaleSurfaceUnavailableNotice()throws Exception {
        open(CampaignMenuState.NEW_GAME);
        set("surfaceRequestGeneration",42L);
        call("pollCampaignMenu");
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Save state is unavailable for the current game surface."));
        assertFalse(find("Begin in Slot 1").isEnabled());
        Bridge.state[1]=42;Bridge.state[2]++;Bridge.state[4]=CampaignMenuState.READY;Bridge.state[5]=0;
        call("pollCampaignMenu");
        String text=menuText(a.findViewById(R.id.menu_scrim));
        assertFalse(text.contains("Save state is unavailable for the current game surface."));
        assertTrue(text.contains("Manual: Empty"));assertTrue(find("Begin in Slot 1").isEnabled());
        // An unrelated operation notice must survive a harmless catalogue publication.
        set("campaignLocalMessage","Write outcome is uncertain. Refresh before saving again.");
        Bridge.state[2]++;call("pollCampaignMenu");
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Write outcome is uncertain."));
    }
}
