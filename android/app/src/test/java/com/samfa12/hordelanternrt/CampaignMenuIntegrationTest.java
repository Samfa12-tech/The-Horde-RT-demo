package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.app.AlertDialog;
import android.content.Context;
import android.content.res.Configuration;
import android.util.DisplayMetrics;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.file.Files;
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
import org.robolectric.annotation.GraphicsMode;

/** Real menu/input methods with immutable fake native publications; no filesystem or Vulkan. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=CampaignMenuIntegrationTest.Bridge.class)
@LooperMode(LooperMode.Mode.PAUSED)
public final class CampaignMenuIntegrationTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class Bridge {
        static long[] state;
        static long next, ackToken, ackGeneration;
        static int ackCalls, configurations, lifePhase, retries, resets,loadSlot,loadKind;
        static long vitality;
        static String configuredPath;
        static boolean paused, acceptAck;
        static float yaw,pitch,x,z;
        static final List<Integer> actions=new ArrayList<>();
        static final List<String> order=new ArrayList<>();
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static boolean configureCampaign(String path){configurations++;configuredPath=path;return true;}
        @Implementation protected static long[] getCampaignState(){ return state==null ? null : state.clone(); }
        @Implementation protected static long requestCampaign(int a,int s,int k,long g){actions.add(a);if(a==CampaignMenuState.LOAD){loadSlot=s;loadKind=k;}return ++next;}
        @Implementation protected static boolean acknowledgeCampaignInput(long t,long g){ackToken=t;ackGeneration=g;ackCalls++;order.add("ack");return acceptAck;}
        @Implementation protected static void setSimulationPaused(boolean p){paused=p;order.add(p?"paused":"unpaused");}
        @Implementation protected static void setViewControls(float y,float p,float light,float xx,float zz,boolean c,boolean analogueMovement){yaw=y;pitch=p;x=xx;z=zz;order.add("input");}
        @Implementation protected static void setEntryMenu(boolean e,boolean s,boolean r,boolean p,long g){if(p)order.add("play");}
        @Implementation protected static int getSurfaceRuntimeState(long g){return 1;}
        @Implementation protected static int getPlayerLifePhase(){return lifePhase;}
        @Implementation protected static long getPlayerVitalityState(){return vitality;}
        static boolean confirmedRetryMode;
        @Implementation protected static boolean getConfirmedRetryMode(){return confirmedRetryMode;}
        @Implementation protected static int retryEncounter(){retries++;return 2;}
        @Implementation protected static void requestRouteReset(){resets++;}
        @Implementation protected static boolean setDiagnosticSurfaceSuspended(long generation,boolean suspended){return true;}
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
        Bridge.confirmedRetryMode=false;
        Bridge.loadSlot=Bridge.loadKind=-1;
        Bridge.lifePhase=Bridge.retries=Bridge.resets=0;Bridge.vitality=(3L<<32)|3;
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
    @Test @GraphicsMode(GraphicsMode.Mode.NATIVE)
    public void entryPlaySelectsSlotBeforeRequestingNativePlay()throws Exception {
        set("entryMenuEnabled",true);
        call("requestEntryPlay");
        assertTrue(field("campaignMenuVisible").getBoolean(a));
        assertFalse(field("entryPlayRequested").getBoolean(a));
        assertEquals(List.of(1),Bridge.actions);
        final String evidence=System.getenv("HORDE_CAMPAIGN_UI_EVIDENCE_DIR");
        if(evidence!=null && !evidence.isEmpty()) {
            Bridge.state[3]=Bridge.next;Bridge.state[2]++;call("pollCampaignMenu");
            captureCampaignWidgets(evidence,"empty");
            Bridge.state[12]=CampaignMenuState.VALID;Bridge.state[13]=1;Bridge.state[14]=1;
            Bridge.state[15]=CampaignMenuState.VALID;Bridge.state[16]=3;Bridge.state[17]=2;
            Bridge.state[2]++;call("pollCampaignMenu");captureCampaignWidgets(evidence,"occupied");
        }
    }
    private void captureCampaignWidgets(String directory,String scenario)throws Exception {
        Configuration original=new Configuration(a.getResources().getConfiguration());
        DisplayMetrics originalMetrics=new DisplayMetrics();originalMetrics.setTo(a.getResources().getDisplayMetrics());
        try {
            for(int[] size:new int[][]{{411,801},{804,381}}) {
                Configuration c=new Configuration(original);c.densityDpi=160;c.fontScale=1;
                c.screenWidthDp=size[0];c.screenHeightDp=size[1];
                c.orientation=size[0]>size[1]?Configuration.ORIENTATION_LANDSCAPE:Configuration.ORIENTATION_PORTRAIT;
                DisplayMetrics metrics=new DisplayMetrics();metrics.setTo(originalMetrics);
                metrics.widthPixels=size[0];metrics.heightPixels=size[1];metrics.density=1;metrics.densityDpi=160;metrics.scaledDensity=1;
                a.getResources().updateConfiguration(c,metrics);call("renderCampaignMenu");
                View root=a.findViewById(R.id.menu_scrim);
                ViewGroup scroller=(ViewGroup)((ViewGroup)root).getChildAt(0);
                final int scrollBefore=scroller.getScrollY();
                root.measure(View.MeasureSpec.makeMeasureSpec(size[0],View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(size[1],View.MeasureSpec.EXACTLY));
                root.layout(0,0,size[0],size[1]);
                View title=((ViewGroup)scroller.getChildAt(0)).getChildAt(1);
                Rect titleBounds=new Rect(0,0,title.getWidth(),title.getHeight());
                ((ViewGroup)root).offsetDescendantRectToMyCoords(title,titleBounds);
                assertTrue("host capture must show its actual top title: "+titleBounds,
                    titleBounds.top>=scroller.getTop() && titleBounds.bottom<=scroller.getBottom());
                View focused=root.findFocus();
                String metricsText="viewport="+size[0]+"x"+size[1]+" actualDisplay="+
                    a.getResources().getDisplayMetrics().widthPixels+"x"+a.getResources().getDisplayMetrics().heightPixels+
                    " scroller="+scroller.getLeft()+","+scroller.getTop()+","+scroller.getRight()+","+scroller.getBottom()+
                    " scrollBeforeMeasure="+scrollBefore+" scrollAfterMeasure="+scroller.getScrollY()+
                    " titleBounds="+titleBounds+" focused="+(focused instanceof TextView?((TextView)focused).getText():focused)+"\n";
                Bitmap image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);root.draw(new Canvas(image));
                File file=new File(directory,"android-widget-fixture-"+scenario+"-"+size[0]+"x"+size[1]+".png");
                assertTrue(file.getParentFile().isDirectory() || file.getParentFile().mkdirs());
                try(FileOutputStream stream=new FileOutputStream(file)){assertTrue(image.compress(Bitmap.CompressFormat.PNG,100,stream));}
                Files.write(new File(directory,file.getName()+".txt").toPath(),
                    ("HOST ANDROID WIDGET FIXTURE ONLY; no phone, RT world, touch or save IO evidence.\n"+metricsText+menuText(root)).getBytes(java.nio.charset.StandardCharsets.UTF_8));
            }
        } finally {a.getResources().updateConfiguration(original,originalMetrics);}
    }
    @Test public void emptyAndInvalidRecordsCannotLoadAndManualSaveUsesNativeSafeGate()throws Exception {
        Bridge.state[12]=4;Bridge.state[15]=5;
        open(CampaignMenuState.LOAD);
        assertFalse(find("Load manual save").isEnabled());assertFalse(find("Load autosave").isEnabled());
        set("campaignMenuVisible",false);Bridge.state[7]=0;
        open(CampaignMenuState.SAVE_MANUAL);
        assertFalse(find("Save in Slot 1").isEnabled());
    }
    @Test public void forestAutosaveUsesExistingPausedLoadAndPresentationBarrier()throws Exception {
        Bridge.state[15]=CampaignMenuState.VALID;Bridge.state[16]=3;Bridge.state[17]=2;
        open(CampaignMenuState.LOAD);
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Forest lookout"));
        assertTrue(find("Load autosave").isEnabled());
        choose(CampaignMenuState.LOAD,0,CampaignMenuState.AUTO);
        assertEquals(List.of(CampaignMenuState.REFRESH,CampaignMenuState.LOAD),Bridge.actions);
        assertTrue(Bridge.paused);
        Bridge.state[3]=Bridge.next;Bridge.state[30]=CampaignMenuState.LOAD;
        Bridge.state[4]=CampaignMenuState.SUCCESS;Bridge.state[11]=0;Bridge.state[2]++;
        call("pollCampaignMenu");
        assertNull(find("Resume"));assertTrue(Bridge.paused);
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
        assertFalse(find("Check saves").isEnabled());
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
        assertFalse(find("Save in Slot 1").isEnabled());
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
        assertTrue(find("Load manual save").isEnabled());
        Bridge.state[0]=99;call("pollCampaignMenu");assertFalse(find("Load manual save").isEnabled());
        choose(3,0,0);assertEquals(List.of(1),Bridge.actions);
        Bridge.state[0]=1;call("pollCampaignMenu");assertTrue(find("Load manual save").isEnabled());
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
        assertEquals("Your saves could not be opened.",field("campaignLocalMessage").get(a));
    }
    @Test public void currentReadyCatalogueClearsOnlyStaleSurfaceUnavailableNotice()throws Exception {
        open(CampaignMenuState.NEW_GAME);
        set("surfaceRequestGeneration",42L);
        call("pollCampaignMenu");
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Saved games are unavailable right now. Please try again."));
        assertFalse(find("New game").isEnabled());
        Bridge.state[1]=42;Bridge.state[2]++;Bridge.state[4]=CampaignMenuState.READY;Bridge.state[5]=0;
        call("pollCampaignMenu");
        String text=menuText(a.findViewById(R.id.menu_scrim));
        assertFalse(text.contains("Saved games are unavailable right now. Please try again."));
        assertTrue(text.contains("Empty"));assertTrue(find("New game").isEnabled());
        // An unrelated operation notice must survive a harmless catalogue publication.
        set("campaignLocalMessage","Write outcome is uncertain. Refresh before saving again.");
        Bridge.state[2]++;call("pollCampaignMenu");
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Write outcome is uncertain."));
    }
    private void openDeathLoad()throws Exception {
        Bridge.lifePhase=2;Bridge.vitality=3L<<32;
        set("lastPlayerLifePhase",2);set("lastPlayerVitality",0);
        call("showDeathOverlay");
        Button load=find("Load checkpoint");assertNotNull("Death card needs Load",load);load.performClick();
        Bridge.state[3]=Bridge.next;Bridge.state[2]++;call("pollCampaignMenu");
    }
    private void validLowerManual() {Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=1;}
    private void publishDeathLoadRebase()throws Exception {
        Bridge.lifePhase=0;Bridge.vitality=(3L<<32)|1;
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=95;
        Bridge.state[9]=Float.floatToIntBits(2.5f);Bridge.state[10]=Float.floatToIntBits(-.15f);Bridge.state[30]=3;
        call("pollCampaignMenu");
    }
    private void publishDeathLoadSuccess()throws Exception {
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=7;Bridge.state[11]=1;call("pollCampaignMenu");
    }
    @Test public void deathCardLoadUsesExistingManualAutoChooserWithoutRetryResetOrUnpause()throws Exception {
        validLowerManual();Bridge.state[15]=3;Bridge.state[16]=2;Bridge.state[17]=3;
        float[] controls=(float[])field("viewControls").get(a);controls[7]=.5f;controls[8]=.6f;
        openDeathLoad();
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(field("campaignFromDeath").getBoolean(a));
        assertTrue(find("Load manual save").isEnabled());assertTrue(find("Load autosave").isEnabled());
        assertEquals(0,Bridge.x,0);assertEquals(0,Bridge.z,0);assertTrue(Bridge.paused);
        assertEquals(0,Bridge.retries);assertEquals(0,Bridge.resets);assertFalse(Bridge.order.contains("unpaused"));
    }
    @Test public void deathLoadBackAndPrecommitCancelReturnDeathCardPaused()throws Exception {
        validLowerManual();openDeathLoad();a.onBackPressed();
        assertFalse(field("campaignMenuVisible").getBoolean(a));assertNotNull(find(a.getString(R.string.retry_encounter)));
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(Bridge.paused);
        find("Load checkpoint").performClick();Bridge.state[2]++;Bridge.state[3]=Bridge.next;call("pollCampaignMenu");
        choose(3,0,0);Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=4;Bridge.state[30]=3;call("pollCampaignMenu");
        a.onBackPressed();assertTrue(field("campaignMenuVisible").getBoolean(a));
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=1;Bridge.state[5]=8;call("pollCampaignMenu");
        assertFalse(field("campaignMenuVisible").getBoolean(a));assertNotNull(find(a.getString(R.string.retry_encounter)));
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(Bridge.paused);assertEquals(0,Bridge.retries);
    }
    @Test public void failedDeathLoadReturnsOriginalDeathCardWithVisibleError()throws Exception {
        validLowerManual();openDeathLoad();choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=8;Bridge.state[5]=6;Bridge.state[30]=3;call("pollCampaignMenu");
        assertFalse(field("campaignMenuVisible").getBoolean(a));assertTrue(field("deathOverlayVisible").getBoolean(a));
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Could not save or load"));
        assertNotNull(find(a.getString(R.string.retry_encounter)));assertTrue(Bridge.paused);
        assertEquals(2,field("lastPlayerLifePhase").getInt(a));assertFalse(Bridge.order.contains("unpaused"));
    }
    @Test public void deathLoadAlivePublicationAndHomeCannotTakeOverPendingMenu()throws Exception {
        validLowerManual();openDeathLoad();choose(3,0,0);publishDeathLoadRebase();
        call("updateDeathRecoveryPresentation",new Class<?>[]{int.class},0);
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(field("campaignMenuVisible").getBoolean(a));
        a.onPause();assertTrue(field("campaignFromDeath").getBoolean(a));assertTrue(field("deathOverlayVisible").getBoolean(a));
        assertTrue(field("campaignMenuVisible").getBoolean(a));assertTrue(Bridge.paused);
        int oldAcks=Bridge.ackCalls;Bridge.state[2]++;Bridge.state[8]=96;call("pollCampaignMenu");assertEquals(oldAcks,Bridge.ackCalls);
        set("resumed",true);call("pollCampaignMenu");assertEquals(96,Bridge.ackToken);
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(Bridge.paused);
    }
    @Test public void committedDeathLoadBackCompletesHandshakeAndOnlyExplicitAliveResumeClearsDeath()throws Exception {
        validLowerManual();openDeathLoad();choose(3,0,0);publishDeathLoadRebase();a.onBackPressed();
        assertEquals(95,Bridge.ackToken);assertTrue(field("campaignMenuVisible").getBoolean(a));
        call("updateDeathRecoveryPresentation",new Class<?>[]{int.class},0);publishDeathLoadSuccess();
        a.onBackPressed();assertTrue(field("campaignMenuVisible").getBoolean(a));
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertNotNull(find("Resume"));assertTrue(Bridge.paused);
        float[] controls=(float[])field("viewControls").get(a);controls[7]=.7f;controls[8]=.5f;
        find("Resume").performClick();
        assertFalse(field("deathOverlayVisible").getBoolean(a));assertFalse(field("campaignFromDeath").getBoolean(a));
        assertFalse(field("menuVisible").getBoolean(a));assertFalse(Bridge.paused);
        assertEquals(0,field("lastPlayerLifePhase").getInt(a));assertEquals(1,field("lastPlayerVitality").getInt(a));
        assertEquals(0,Bridge.x,0);assertEquals(0,Bridge.z,0);assertEquals(0,Bridge.retries);assertEquals(0,Bridge.resets);
    }
    @Test public void deathLoadCannotResumeIfNativeStillReportsDead()throws Exception {
        validLowerManual();openDeathLoad();choose(3,0,0);publishDeathLoadRebase();publishDeathLoadSuccess();
        Bridge.lifePhase=2;Bridge.vitality=3L<<32;find("Resume").performClick();
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(field("campaignMenuVisible").getBoolean(a));assertTrue(Bridge.paused);
    }
    @Test public void returnedDeathCardRetryKeepsItsAliveAcknowledgementPolicy()throws Exception {
        validLowerManual();openDeathLoad();a.onBackPressed();find(a.getString(R.string.retry_encounter)).performClick();
        assertEquals(1,Bridge.retries);assertTrue(field("retryPending").getBoolean(a));assertTrue(Bridge.paused);
        call("updateDeathRecoveryPresentation",new Class<?>[]{int.class},0);
        assertFalse(field("deathOverlayVisible").getBoolean(a));assertFalse(Bridge.paused);
    }
    @Test public void homeBeforeDeathLoadCommitReturnsDeathOnlyAfterMatchingCancelResult()throws Exception {
        validLowerManual();openDeathLoad();choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=4;Bridge.state[30]=3;call("pollCampaignMenu");
        a.onPause();assertTrue(field("campaignFromDeath").getBoolean(a));assertTrue(field("deathOverlayVisible").getBoolean(a));
        assertTrue(field("campaignMenuVisible").getBoolean(a));assertEquals(Bridge.next,field("campaignRequest").getLong(a));
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=1;Bridge.state[5]=8;
        set("resumed",true);call("pollCampaignMenu");
        assertFalse(field("campaignMenuVisible").getBoolean(a));assertTrue(field("deathOverlayVisible").getBoolean(a));
        assertNotNull(find(a.getString(R.string.retry_encounter)));assertTrue(Bridge.paused);
        assertFalse(Bridge.order.contains("unpaused"));
    }
    @Test public void detachedDeathRetryRestartCannotMutatePendingLoadAndReturnedRestartKeepsAckPolicy()throws Exception {
        validLowerManual();Bridge.lifePhase=2;Bridge.vitality=3L<<32;set("lastPlayerLifePhase",2);call("showDeathOverlay");
        Button retry=find(a.getString(R.string.retry_encounter)), restart=find(a.getString(R.string.restart_route));
        find("Load checkpoint").performClick();Bridge.state[2]++;Bridge.state[3]=Bridge.next;call("pollCampaignMenu");
        choose(3,0,0);retry.performClick();restart.performClick();assertEquals(0,Bridge.retries);assertEquals(0,Bridge.resets);
        a.onBackPressed();Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=1;Bridge.state[5]=8;call("pollCampaignMenu");
        find(a.getString(R.string.restart_route)).performClick();assertEquals(1,Bridge.resets);
        assertTrue(field("retryPending").getBoolean(a));assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(Bridge.paused);
        call("updateDeathRecoveryPresentation",new Class<?>[]{int.class},0);
        assertFalse(field("deathOverlayVisible").getBoolean(a));assertFalse(Bridge.paused);
    }
    private void beginConfirmedRetry()throws Exception {
        Bridge.confirmedRetryMode=true;
        Bridge.state=java.util.Arrays.copyOf(Bridge.state,34);
        Bridge.lifePhase=2;Bridge.vitality=3L<<32;
        set("lastPlayerLifePhase",2);call("showDeathOverlay");
        float[] controls=(float[])field("viewControls").get(a);controls[0]=.67f;controls[1]=-.11f;
        call("retryEncounter");
        assertEquals(1,Bridge.retries);assertTrue(field("confirmedRetryPending").getBoolean(a));
        assertTrue(field("retryPending").getBoolean(a));assertTrue(Bridge.paused);
        assertEquals("confirmed retry never applies the legacy checkpoint camera",.67f,controls[0],0);
        assertEquals(-.11f,controls[1],0);
    }
    private void publishConfirmedRetry(int phase,int error,boolean presented)throws Exception {
        Bridge.state[2]++;Bridge.state[3]=7;Bridge.state[4]=phase;Bridge.state[5]=error;
        Bridge.state[8]=95;Bridge.state[9]=Float.floatToIntBits(2.5f);
        Bridge.state[10]=Float.floatToIntBits(-.15f);Bridge.state[11]=presented?1:0;
        Bridge.state[30]=CampaignMenuState.LOAD;Bridge.state[33]=1;
        call("pollCampaignMenu");
    }
    @Test public void confirmedRetryUsesFreshOwnerLoadNeutralAckAliveAndActualPresentation()throws Exception {
        beginConfirmedRetry();
        Bridge.lifePhase=0;Bridge.vitality=(3L<<32)|1;
        call("updateDeathRecoveryPresentation",new Class<?>[]{int.class},0);
        call("pollCampaignMenu"); // Old ready publication is not this retry's completion.
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(Bridge.paused);
        publishConfirmedRetry(CampaignMenuState.REBASE,0,false);
        assertEquals(95,Bridge.ackToken);assertEquals(31,Bridge.ackGeneration);
        assertEquals(2.5f,Bridge.yaw,0);assertEquals(0,Bridge.x,0);assertEquals(0,Bridge.z,0);
        publishConfirmedRetry(CampaignMenuState.PRESENTING,0,false);assertTrue(Bridge.paused);
        publishConfirmedRetry(CampaignMenuState.SUCCESS,0,false);assertTrue(Bridge.paused);
        // Merely publishing the exact saved hearts cannot close the pending barrier.
        Bridge.lifePhase=2;Bridge.vitality=3L<<32;
        publishConfirmedRetry(CampaignMenuState.SUCCESS,0,true);assertTrue(Bridge.paused);
        Bridge.lifePhase=0;Bridge.vitality=(3L<<32)|1;call("pollCampaignMenu");
        assertFalse(Bridge.paused);assertFalse(field("deathOverlayVisible").getBoolean(a));
        assertFalse(field("confirmedRetryPending").getBoolean(a));
        assertEquals(1,field("lastPlayerVitality").getInt(a));
        assertEquals(0,Bridge.resets);assertTrue(Bridge.actions.isEmpty());
    }
    @Test public void confirmedRetryFailedOrUnavailableNeverResetsOrDismissesOnAlive()throws Exception {
        beginConfirmedRetry();
        publishConfirmedRetry(CampaignMenuState.FAILED,4,false);
        assertFalse(field("retryPending").getBoolean(a));assertTrue(Bridge.paused);
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("save is empty"));
        Bridge.lifePhase=0;Bridge.vitality=(3L<<32)|1;
        call("updateDeathRecoveryPresentation",new Class<?>[]{int.class},0);
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(Bridge.paused);
        assertEquals(1,Bridge.retries);assertEquals(0,Bridge.resets);
    }
    @Test public void confirmedRetryRequiresAcknowledgedNeutralRebaseEvenIfPresentedAndAlive()throws Exception {
        beginConfirmedRetry();Bridge.acceptAck=false;Bridge.lifePhase=0;Bridge.vitality=(3L<<32)|1;
        publishConfirmedRetry(CampaignMenuState.REBASE,0,false);assertEquals(1,Bridge.ackCalls);
        publishConfirmedRetry(CampaignMenuState.SUCCESS,0,true);
        assertTrue(field("confirmedRetryPending").getBoolean(a));assertTrue(Bridge.paused);
        assertTrue(field("deathOverlayVisible").getBoolean(a));
    }
    @Test public void confirmedRetryHomeBeforeCommitReturnsDeathOnFreshCancellation()throws Exception {
        beginConfirmedRetry();publishConfirmedRetry(CampaignMenuState.PREPARING,0,false);
        Bridge.next=7;a.onPause();assertTrue(Bridge.paused);
        assertEquals(List.of(CampaignMenuState.CANCEL),Bridge.actions);
        set("resumed",true);Bridge.state[3]=8;Bridge.state[2]++;Bridge.state[4]=CampaignMenuState.READY;Bridge.state[5]=8;
        call("pollCampaignMenu");
        assertFalse(field("confirmedRetryPending").getBoolean(a));assertTrue(field("deathOverlayVisible").getBoolean(a));
        assertTrue(Bridge.paused);assertEquals(0,Bridge.resets);
    }

    @Test public void confirmedRetryHomeBeforeOwnerActivationConsumesExactUnmarkedCancel()throws Exception {
        beginConfirmedRetry();a.onPause();
        assertEquals(List.of(CampaignMenuState.CANCEL),Bridge.actions);
        Bridge.state[3]=Bridge.next;Bridge.state[2]++;Bridge.state[4]=CampaignMenuState.READY;Bridge.state[5]=8;
        // Cancellation beat automatic activation: no confirmed word and no Load operation yet.
        set("resumed",true);call("pollCampaignMenu");
        assertFalse(field("confirmedRetryPending").getBoolean(a));assertFalse(field("retryPending").getBoolean(a));
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertTrue(Bridge.paused);assertEquals(0,Bridge.resets);
    }

    @Test public void emptyNewGameSlotsAreSimpleAndContinueNeverGuessesASave()throws Exception {
        open(CampaignMenuState.NEW_GAME);
        String text=menuText(a.findViewById(R.id.menu_scrim));
        assertEquals(3,text.split("(?m)^Empty$",-1).length-1);
        assertFalse(text.contains("Manual save: Empty"));assertFalse(text.contains("Autosave: Empty"));
        assertNull(find("Continue"));
        assertEquals("New game in Slot 1",find("New game").getContentDescription());
        for(String internal:new String[]{"MANUAL +", "record", "private", "RT resources", "rebase"})assertFalse(text.contains(internal));
        Bridge.state[12]=CampaignMenuState.VALID;Bridge.state[13]=1;Bridge.state[14]=1;
        Bridge.state[15]=CampaignMenuState.VALID;Bridge.state[16]=3;Bridge.state[17]=2;
        Bridge.state[2]++;call("pollCampaignMenu");
        Button detachedNewGame=find("New game");Button continueGame=find("Continue");assertNotNull(continueGame);
        assertTrue(continueGame.isEnabled());continueGame.performClick();
        assertEquals("Continue only refreshes existing chooser; it never submits Load/New Game",List.of(1,1),Bridge.actions);
        assertEquals(CampaignMenuState.LOAD,field("campaignMenuAction").getInt(a));
        detachedNewGame.performClick();assertEquals(List.of(1,1),Bridge.actions);
        Bridge.state[3]=Bridge.next;Bridge.state[2]++;call("pollCampaignMenu");
        assertTrue(find("Load manual save").isEnabled());assertTrue(find("Load autosave").isEnabled());
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Manual save: Lower chamber"));
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Autosave: Forest lookout"));
        find("Load manual save").performClick();assertEquals(List.of(1,1,3),Bridge.actions);
        assertEquals(0,Bridge.loadSlot);assertEquals(CampaignMenuState.MANUAL,Bridge.loadKind);assertTrue(Bridge.paused);
    }
    @Test public void checkingAndUnreadableSlotsAreNotPresentedAsEmptyAndOverwriteStaysExplicit()throws Exception {
        Bridge.state[12]=CampaignMenuState.CHECKING;Bridge.state[15]=CampaignMenuState.READ_ERROR;
        open(CampaignMenuState.NEW_GAME);
        String text=menuText(a.findViewById(R.id.menu_scrim));
        assertTrue(text.contains("Manual save: Checking"));assertTrue(text.contains("Autosave: Could not be opened"));
        assertFalse(find("New game").isEnabled());
        Bridge.state[12]=CampaignMenuState.VALID;Bridge.state[13]=1;Bridge.state[14]=1;
        Bridge.state[2]++;call("pollCampaignMenu");find("New game").performClick();
        AlertDialog dialog=(AlertDialog)field("campaignConfirmation").get(a);assertNotNull(dialog);
        TextView message=dialog.findViewById(android.R.id.message);assertNotNull(message);
        assertTrue(message.getText().toString().contains("Your manual save stays"));
        assertTrue(message.getText().toString().contains("autosave will be replaced by your new game"));
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).performClick();assertEquals(List.of(1),Bridge.actions);
    }

}
