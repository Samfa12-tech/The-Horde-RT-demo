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
import android.view.WindowInsets;
import android.graphics.Insets;
import android.widget.FrameLayout;
import android.widget.ScrollView;
import android.view.Surface;
import android.graphics.SurfaceTexture;
import android.os.Looper;
import android.widget.ProgressBar;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import org.robolectric.Shadows;
import org.robolectric.RuntimeEnvironment;
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
        static int runtime,starts,textReportCalls;static long[] entry;
        static long startResult;static boolean failStart;
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
        @Implementation protected static int getSurfaceRuntimeState(long g){return runtime;}
        @Implementation protected static long[] getEntryMenuState(){return entry==null?null:entry.clone();}
        @Implementation protected static long startDiagnosticSurface(Surface surface,String directory){starts++;if(failStart)throw new IllegalStateException("fixture start failure");return startResult;}
        @Implementation protected static void stopDiagnosticSurface(long generation){}
        @Implementation protected static String getTextReport(){textReportCalls++;return "fixture report";}
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
    public static class HeldStartupActivity extends MainActivity {
        volatile CountDownLatch entered=new CountDownLatch(1),release=new CountDownLatch(1),finished=new CountDownLatch(1);
        volatile boolean prepared=true,ranOnUi;
        final AtomicInteger calls=new AtomicInteger();
        @Override StartupDiagnostics prepareInitialDiagnostics() {
            final CountDownLatch ownRelease=release,ownFinished=finished;
            final boolean result=prepared;
            calls.incrementAndGet();ranOnUi=Thread.currentThread()==Looper.getMainLooper().getThread();entered.countDown();
            try {
                if(!ownRelease.await(5,TimeUnit.SECONDS))return new StartupDiagnostics(false,"Fixture release did not arrive");
                return new StartupDiagnostics(result,result?"Fixture assets prepared":"Fixture asset failure");
            } catch(InterruptedException interrupted) {Thread.currentThread().interrupt();return new StartupDiagnostics(false,"Fixture interrupted");}
            finally {ownFinished.countDown();}
        }
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
        Bridge.confirmedRetryMode=false;Bridge.runtime=1;Bridge.starts=Bridge.textReportCalls=0;Bridge.entry=new long[]{31,2,0,0,1};
        Bridge.startResult=31;Bridge.failStart=false;
        Bridge.loadSlot=Bridge.loadKind=-1;
        Bridge.lifePhase=Bridge.retries=Bridge.resets=0;Bridge.vitality=(3L<<32)|3;
        a=Robolectric.buildActivity(MainActivity.class).get();
        wireActivityViews();
    }
    private void wireActivityViews()throws Exception {
        a.setContentView(R.layout.activity_main);
        set("preferences",a.getSharedPreferences("campaign-menu",Context.MODE_PRIVATE));
        a.getSharedPreferences("campaign-menu",Context.MODE_PRIVATE).edit().clear().apply();
        set("resumed",true);set("firstMenu",false);set("surfaceStarted",true);set("surfaceRequestGeneration",31L);
        set("campaignConfigured",true);set("startupAssetsPrepared",true);
        String[] names={"surfaceView","menuScrim","menuButton","attackButton","parryButton","dodgeButton","runButton","crouchButton",
                "interactButton","toggleHeldLightPoseButton","diagnosticsPanel","developerOverlay","vitalityStatus","rtStatus","controllerPrompt"};
        int[] ids={R.id.scene_surface,R.id.menu_scrim,R.id.menu_button,R.id.attack_button,R.id.parry_button,R.id.dodge_button,R.id.run_button,R.id.crouch_button,
                R.id.interact_button,R.id.toggle_held_light_pose_button,R.id.diagnostics_panel,R.id.developer_overlay,R.id.vitality_status,R.id.rt_status,R.id.controller_prompt};
        for(int i=0;i<names.length;i++)set(names[i],a.findViewById(ids[i]));
    }
    private FrameLayout entryCompositionFixture(int width,int height,float density,float fontScale,
                                                int direction,int insetLeft,int insetRight)throws Exception {
        Configuration config=new Configuration(a.getResources().getConfiguration());
        config.fontScale=fontScale;config.densityDpi=Math.round(density*160);
        config.screenWidthDp=Math.round(width/density);config.screenHeightDp=Math.round(height/density);
        config.orientation=width>height?Configuration.ORIENTATION_LANDSCAPE:Configuration.ORIENTATION_PORTRAIT;
        DisplayMetrics metrics=new DisplayMetrics();metrics.setTo(a.getResources().getDisplayMetrics());
        metrics.widthPixels=width;metrics.heightPixels=height;metrics.density=density;
        metrics.densityDpi=config.densityDpi;metrics.scaledDensity=density*fontScale;
        org.robolectric.shadows.ShadowDisplay display=Shadows.shadowOf(a.getWindowManager().getDefaultDisplay());
        display.setWidth(width);display.setHeight(height);
        display.setRealWidth(width);display.setRealHeight(height);display.setDensity(density);
        a.getResources().updateConfiguration(config,metrics);
        // Resource configuration can recalculate host display metrics. Keep the
        // app's physical viewport and the explicitly measured root coherent.
        a.getResources().getDisplayMetrics().setTo(metrics);
        assertEquals(width,a.getResources().getDisplayMetrics().widthPixels);
        assertEquals(height,a.getResources().getDisplayMetrics().heightPixels);
        final WindowInsets insets=new WindowInsets.Builder().setStableInsets(
            Insets.of(insetLeft,Math.round(24*density),insetRight,Math.round(24*density))).build();
        FrameLayout root=new FrameLayout(a) {
            @Override public WindowInsets getRootWindowInsets(){return insets;}
        };
        root.setId(R.id.menu_scrim);root.setLayoutDirection(direction);
        View old=a.findViewById(R.id.menu_scrim);ViewGroup parent=(ViewGroup)old.getParent();
        int index=parent.indexOfChild(old);ViewGroup.LayoutParams params=old.getLayoutParams();
        parent.removeView(old);parent.addView(root,index,params);set("menuScrim",root);
        set("entryMenuEnabled",true);call("showEntryMenu",new Class<?>[]{boolean.class},false);
        root.measure(View.MeasureSpec.makeMeasureSpec(width,View.MeasureSpec.EXACTLY),
                     View.MeasureSpec.makeMeasureSpec(height,View.MeasureSpec.EXACTLY));
        root.layout(0,0,width,height);
        root.getViewTreeObserver().dispatchOnPreDraw();
        return root;
    }
    private Rect entryBounds(FrameLayout root,String name)throws Exception {
        View button=(View)field(name).get(a);
        Rect rect=new Rect(0,0,button.getWidth(),button.getHeight());
        root.offsetDescendantRectToMyCoords(button,rect);return rect;
    }
    private void captureEntryGeometry(FrameLayout root,String scenario)throws Exception {
        final String directory=System.getenv("HORDE_ENTRY_LAYOUT_EVIDENCE_DIR");
        if(directory==null || directory.isEmpty())return;
        File output=new File(directory,"host-entry-widgets-"+scenario+".png");
        assertFalse("preserve prior layout evidence",output.exists());
        assertTrue(output.getParentFile().isDirectory() || output.getParentFile().mkdirs());
        Bitmap image=Bitmap.createBitmap(root.getWidth(),root.getHeight(),Bitmap.Config.ARGB_8888);
        root.draw(new Canvas(image));
        try(FileOutputStream stream=new FileOutputStream(output)){assertTrue(image.compress(Bitmap.CompressFormat.PNG,100,stream));}
        String text="ANDROID HOST WIDGETS ONLY; transparent world, synthetic insets, no phone or RT acceptance.\n"+
            "viewport="+root.getWidth()+"x"+root.getHeight()+" density="+a.getResources().getDisplayMetrics().density+
            " fontScale="+a.getResources().getConfiguration().fontScale+" direction="+root.getLayoutDirection()+
            " actualDisplay="+a.getResources().getDisplayMetrics().widthPixels+"x"+a.getResources().getDisplayMetrics().heightPixels+"\n";
        for(String name:new String[]{"entryPlayButton","entryMoreButton","entrySettingsButton"})
            text+=name+"="+entryBounds(root,name)+"\n";
        Files.write(new File(directory,output.getName()+".txt").toPath(),text.getBytes(java.nio.charset.StandardCharsets.UTF_8));
    }
    @Test @GraphicsMode(GraphicsMode.Mode.NATIVE)
    public void entrySecondaryPlaquesHaveEqualSizeAndBalancedAnchors()throws Exception {
        for(int[] size:new int[][]{{411,801},{804,381}})for(float scale:new float[]{1,2}) {
            FrameLayout root=entryCompositionFixture(size[0],size[1],1,scale,View.LAYOUT_DIRECTION_LTR,0,0);
            captureEntryGeometry(root,size[0]+"x"+size[1]+"-font"+scale);
            Rect more=entryBounds(root,"entryMoreButton"),settings=entryBounds(root,"entrySettingsButton");
            Rect play=entryBounds(root,"entryPlayButton");
            assertEquals("secondary actions need equal visual and touch width",more.width(),settings.width());
            assertEquals("secondary actions share a baseline",more.bottom,settings.bottom);
            assertEquals("outer spacing must balance",more.left,root.getWidth()-settings.right);
            assertTrue("secondary targets retain Android48dp minimum",more.height()>=48 && settings.height()>=48);
            assertFalse("primary and secondary targets remain separate",Rect.intersects(play,more)||Rect.intersects(play,settings));
            assertTrue("no unexpected campaign action",Bridge.actions.isEmpty());
        }
    }
    @Test @GraphicsMode(GraphicsMode.Mode.NATIVE)
    public void entryCompositionPreservesAsymmetricSafeInsetsAndRtl()throws Exception {
        for(int direction:new int[]{View.LAYOUT_DIRECTION_LTR,View.LAYOUT_DIRECTION_RTL}) {
            FrameLayout root=entryCompositionFixture(1080,2340,2.625f,1.3f,direction,40,100);
            captureEntryGeometry(root,"1080x2340-safe-"+direction);
            Rect more=entryBounds(root,"entryMoreButton"),settings=entryBounds(root,"entrySettingsButton");
            Rect play=entryBounds(root,"entryPlayButton");
            int safeLeft=Math.max(Math.round(16*2.625f),40+Math.round(8*2.625f));
            int safeRight=root.getWidth()-Math.max(Math.round(16*2.625f),100+Math.round(8*2.625f));
            for(Rect rect:new Rect[]{more,settings,play})
                assertTrue("all entry actions stay inside physical safe insets",rect.left>=safeLeft && rect.right<=safeRight);
            assertTrue("Play centres within the safe area rather than shifting by full inset delta",
                Math.abs(play.left+play.right-safeLeft-safeRight)<=1);
            assertEquals("secondary widths stay equal at physical phone density",more.width(),settings.width());
            assertTrue("secondary actions mirror within the same safe area",
                Math.abs(more.left+more.right+settings.left+settings.right-2*(safeLeft+safeRight))<=2);
        }
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
    @Test @GraphicsMode(GraphicsMode.Mode.NATIVE)
    public void threeSlotLoadShowsSelectedDateAndOnlyOneActionPerSlot()throws Exception {
        Bridge.state=java.util.Arrays.copyOf(Bridge.state,40);Bridge.state[0]=2;
        Bridge.state[12]=Bridge.state[15]=CampaignMenuState.VALID;
        Bridge.state[13]=1;Bridge.state[14]=1;Bridge.state[16]=2;Bridge.state[17]=2;
        Bridge.state[34]=1767225600L;Bridge.state[35]=1767225660L;
        open(CampaignMenuState.LOAD);
        String text=menuText(a.findViewById(R.id.menu_scrim));
        assertEquals(3,text.split("(?m)^Continue$",-1).length-1);
        assertTrue(text.contains("Safe exit"));assertFalse(text.contains("Lower chamber"));
        assertFalse(text.contains("Manual save"));assertFalse(text.contains("Autosave"));
        assertFalse(text.contains("Recheck"));assertFalse(text.contains("Resume loaded"));
        assertEquals("Continue Slot 1",find("Continue").getContentDescription());
        assertTrue(find("Continue").getMinimumHeight()>=48);
        final String evidence=System.getenv("HORDE_CAMPAIGN_UI_EVIDENCE_DIR");
        if(evidence!=null && !evidence.isEmpty())captureCampaignWidgets(evidence,"load-dated");
        Button first=find("Continue");first.performClick();
        assertEquals(CampaignMenuState.AUTO,Bridge.loadKind);assertEquals(0,Bridge.loadSlot);
        assertEquals(List.of(CampaignMenuState.REFRESH,CampaignMenuState.LOAD),Bridge.actions);
    }
    private void collectSlotActions(View view,List<Button> result) {
        if(view instanceof Button && view.getContentDescription()!=null &&
                view.getContentDescription().toString().matches("(Continue Slot |New game in Slot |Save in Slot )[123]"))result.add((Button)view);
        if(view instanceof ViewGroup)for(int i=0;i<((ViewGroup)view).getChildCount();i++)collectSlotActions(((ViewGroup)view).getChildAt(i),result);
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
                org.robolectric.shadows.ShadowDisplay display=Shadows.shadowOf(a.getWindowManager().getDefaultDisplay());
                display.setWidth(size[0]);display.setHeight(size[1]);display.setRealWidth(size[0]);display.setRealHeight(size[1]);display.setDensity(1);
                a.getResources().updateConfiguration(c,metrics);a.getResources().getDisplayMetrics().setTo(metrics);
                assertEquals(size[0],a.getResources().getDisplayMetrics().widthPixels);
                assertEquals(size[1],a.getResources().getDisplayMetrics().heightPixels);
                call("renderCampaignMenu");
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
                if(size[0]>size[1]) {
                    List<Button> slotActions=new ArrayList<>();collectSlotActions(root,slotActions);
                    assertEquals(3,slotActions.size());
                    Rect previous=null;
                    for(Button button:slotActions) {
                        Rect rect=new Rect(0,0,button.getWidth(),button.getHeight());((ViewGroup)root).offsetDescendantRectToMyCoords(button,rect);
                        assertTrue("landscape slot targets remain onscreen: "+rect,rect.left>=0 && rect.right<=size[0] && rect.top>=0 && rect.bottom<=size[1]);
                        assertTrue(button.getHeight()>=48);
                        if(previous!=null)assertFalse(Rect.intersects(previous,rect));previous=rect;
                    }
                }
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
        assertFalse(find("Continue").isEnabled());assertFalse(find("Continue").isEnabled());
        set("campaignMenuVisible",false);Bridge.state[7]=0;
        open(CampaignMenuState.SAVE_MANUAL);
        assertFalse(find("Save in Slot 1").isEnabled());
    }
    @Test public void forestAutosaveUsesExistingPausedLoadAndPresentationBarrier()throws Exception {
        Bridge.state[15]=CampaignMenuState.VALID;Bridge.state[16]=3;Bridge.state[17]=2;
        open(CampaignMenuState.LOAD);
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Forest lookout"));
        assertTrue(find("Continue").isEnabled());
        choose(CampaignMenuState.LOAD,0,CampaignMenuState.AUTO);
        assertEquals(List.of(CampaignMenuState.REFRESH,CampaignMenuState.LOAD),Bridge.actions);
        assertTrue(Bridge.paused);
        Bridge.state[3]=Bridge.next;Bridge.state[30]=CampaignMenuState.LOAD;
        Bridge.state[4]=CampaignMenuState.SUCCESS;Bridge.state[11]=0;Bridge.state[2]++;
        call("pollCampaignMenu");
        assertFalse(find("Continue").isEnabled());assertTrue(Bridge.paused);
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
        Bridge.state[2]++;Bridge.state[4]=6;call("pollCampaignMenu");assertFalse(find("Continue").isEnabled());
        Bridge.state[2]++;Bridge.state[4]=7;call("pollCampaignMenu");assertFalse(find("Continue").isEnabled());
        Bridge.state[2]++;Bridge.state[11]=1;call("pollCampaignMenu");
        assertFalse("exact presented Load enters directly",Bridge.paused);assertFalse(field("campaignMenuVisible").getBoolean(a));assertFalse(field("menuVisible").getBoolean(a));
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
        assertTrue(find("Continue").isEnabled());
        Bridge.state[0]=99;call("pollCampaignMenu");assertFalse(find("Continue").isEnabled());
        choose(3,0,0);assertEquals(List.of(1),Bridge.actions);
        Bridge.state[0]=1;call("pollCampaignMenu");assertTrue(find("Continue").isEnabled());
    }
    @Test public void cancelAfterCommitRequiresExactRebaseTokenBeforeOfferingResume()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;open(3);choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=71;Bridge.state[30]=3;
        call("pollCampaignMenu");a.onBackPressed();assertEquals(List.of(1,3,4),Bridge.actions);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=7;Bridge.state[11]=1;Bridge.state[8]=70;
        call("pollCampaignMenu");assertFalse(find("Continue").isEnabled());
        Bridge.state[2]++;Bridge.state[8]=71;call("pollCampaignMenu");assertNotNull(find("Continue"));
        a.onBackPressed();assertFalse(field("campaignMenuVisible").getBoolean(a));
        assertTrue(field("menuVisible").getBoolean(a));assertTrue(Bridge.paused);
    }
    @Test public void queuedSecondLoadCannotReusePriorSuccessBeforeCurrentRequestAcknowledgement()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;open(3);choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=71;Bridge.state[30]=3;
        call("pollCampaignMenu");Bridge.state[2]++;Bridge.state[4]=7;Bridge.state[11]=1;call("pollCampaignMenu");
        assertFalse(field("campaignMenuVisible").getBoolean(a));open(3);choose(3,0,0);
        call("pollCampaignMenu");assertFalse(field("campaignLoadedReady").getBoolean(a));
        call("resumeLoadedCampaign");assertTrue(Bridge.paused);assertTrue(field("campaignMenuVisible").getBoolean(a));
    }
    @Test public void invalidatedLoadedPublicationDoesNotTrapBackOrReuseResume()throws Exception {
        Bridge.state[12]=3;Bridge.state[13]=1;Bridge.state[14]=3;open(3);choose(3,0,0);
        Bridge.state[2]++;Bridge.state[3]=Bridge.next;Bridge.state[4]=5;Bridge.state[8]=71;Bridge.state[30]=3;
        call("pollCampaignMenu");a.onBackPressed();Bridge.state[3]=Bridge.next;Bridge.state[2]++;Bridge.state[4]=7;Bridge.state[11]=1;call("pollCampaignMenu");
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
        assertTrue(find("Continue").isEnabled());assertTrue(find("Continue").isEnabled());
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
        assertTrue(field("deathOverlayVisible").getBoolean(a));assertNotNull(find("Continue"));assertTrue(Bridge.paused);
        float[] controls=(float[])field("viewControls").get(a);controls[7]=.7f;controls[8]=.5f;
        find("Continue").performClick();
        assertFalse(field("deathOverlayVisible").getBoolean(a));assertFalse(field("campaignFromDeath").getBoolean(a));
        assertFalse(field("menuVisible").getBoolean(a));assertFalse(Bridge.paused);
        assertEquals(0,field("lastPlayerLifePhase").getInt(a));assertEquals(1,field("lastPlayerVitality").getInt(a));
        assertEquals(0,Bridge.x,0);assertEquals(0,Bridge.z,0);assertEquals(0,Bridge.retries);assertEquals(0,Bridge.resets);
    }
    @Test public void deathLoadCannotResumeIfNativeStillReportsDead()throws Exception {
        validLowerManual();openDeathLoad();choose(3,0,0);publishDeathLoadRebase();
        Bridge.lifePhase=2;Bridge.vitality=3L<<32;publishDeathLoadSuccess();find("Continue").performClick();
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
        assertTrue(find("Continue").isEnabled());assertTrue(find("Continue").isEnabled());
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Forest lookout"));
        assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Date unavailable"));
        find("Continue").performClick();assertEquals(List.of(1,1,3),Bridge.actions);
        assertEquals(0,Bridge.loadSlot);assertEquals(CampaignMenuState.AUTO,Bridge.loadKind);assertTrue(Bridge.paused);
    }
    @Test public void checkingAndUnreadableSlotsAreNotPresentedAsEmptyAndOverwriteStaysExplicit()throws Exception {
        Bridge.state[12]=CampaignMenuState.CHECKING;Bridge.state[15]=CampaignMenuState.READ_ERROR;
        open(CampaignMenuState.NEW_GAME);
        String text=menuText(a.findViewById(R.id.menu_scrim));
        assertTrue(text.contains("Checking saves"));assertFalse(text.contains("Empty\nSlot 1"));
        assertFalse(find("New game").isEnabled());
        Bridge.state[12]=CampaignMenuState.VALID;Bridge.state[13]=1;Bridge.state[14]=1;
        Bridge.state[2]++;call("pollCampaignMenu");find("New game").performClick();
        AlertDialog dialog=(AlertDialog)field("campaignConfirmation").get(a);assertNotNull(dialog);
        TextView message=dialog.findViewById(android.R.id.message);assertNotNull(message);
        assertTrue(message.getText().toString().contains("previous checkpoint is kept"));
        assertTrue(message.getText().toString().contains("New progress is saved at checkpoints"));
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).performClick();assertEquals(List.of(1),Bridge.actions);
    }

    @Test public void coldEntryPlayCannotOpenSaveChooserBeforeOwningPresentation()throws Exception {
        Bridge.runtime=0;Bridge.entry=new long[]{31,2,0,0,0};
        set("surfaceStarted",false);set("entryMenuEnabled",true);
        call("showEntryMenu",new Class<?>[]{boolean.class},false);
        Button play=(Button)field("entryPlayButton").get(a);
        if(play!=null)play.performClick();
        call("requestEntryPlay"); // A detached/programmatic callback cannot bypass hidden controls.
        assertFalse("accepted surface without RT Entry output cannot open slots",field("campaignMenuVisible").getBoolean(a));
        assertTrue("early tap cannot enqueue a save/load/new-game operation",Bridge.actions.isEmpty());
        assertTrue(Bridge.paused);
    }
    @Test public void stalePresentedEntryCannotUnlockReplacementSurface()throws Exception {
        set("entryMenuEnabled",true);set("surfaceStarted",false);Bridge.entry=new long[]{30,2,0,0,1};
        call("showEntryMenu",new Class<?>[]{boolean.class},false);
        call("applyEntryMenuAvailability",new Class<?>[]{EntryMenuState.class},EntryMenuState.decode(new long[]{30,2,0,0,1}));
        for(String name:new String[]{"entryPlayButton","entrySettingsButton","entryMoreButton"}) {
            Button button=(Button)field(name).get(a);assertTrue(button==null || !button.isEnabled());
        }
        call("requestEntryPlay");assertFalse(field("campaignMenuVisible").getBoolean(a));
    }
    @Test public void coldPauseResumeAndLoadCannotBypassPendingSurface()throws Exception {
        set("entryMenuEnabled",false);
        call("showMainMenu",new Class<?>[]{boolean.class},false);
        Button detachedResume=find(a.getString(R.string.resume_demo));
        Button detachedLoad=find("Load checkpoint");assertNotNull(detachedResume);assertNotNull(detachedLoad);
        Bridge.runtime=0;set("surfaceStarted",false);
        call("showMainMenu",new Class<?>[]{boolean.class},false);
        assertNull(find(a.getString(R.string.resume_demo)));assertNull(find("Load checkpoint"));
        detachedResume.performClick();
        assertTrue("Resume must retain pause before actual ready output",Bridge.paused);
        assertTrue(field("menuVisible").getBoolean(a));
        detachedLoad.performClick();
        assertFalse(field("campaignMenuVisible").getBoolean(a));assertTrue(Bridge.actions.isEmpty());
    }

    private HeldStartupActivity heldStartup(boolean surfaceAvailable)throws Exception {
        HeldStartupActivity held=Robolectric.buildActivity(HeldStartupActivity.class).get();a=held;wireActivityViews();
        set("startupAssetsPrepared",false);set("surfaceRequestGeneration",0L);set("surfaceStarted",false);
        set("entryMenuEnabled",true);set("startupUpdateCheckCompleted",true);
        set("surfaceAvailable",surfaceAvailable);
        if(surfaceAvailable)set("currentSurface",new Surface(new SurfaceTexture(0)));
        Bridge.runtime=0;Bridge.entry=new long[]{31,2,0,0,0};
        call("showMainMenu",new Class<?>[]{boolean.class},true);return held;
    }
    private void awaitStartupResult(MainActivity activity)throws Exception {
        long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(3);
        while(field("startupAssetsPending").getBoolean(activity) && System.nanoTime()<deadline) {
            Shadows.shadowOf(Looper.getMainLooper()).idle();Thread.sleep(1);
        }
        assertFalse("worker result must be handled within fixture deadline",field("startupAssetsPending").getBoolean(activity));
    }
    private int countProgress(View view) {
        int count=view instanceof ProgressBar?1:0;
        if(view instanceof ViewGroup)for(int i=0;i<((ViewGroup)view).getChildCount();++i)count+=countProgress(((ViewGroup)view).getChildAt(i));
        return count;
    }
    @Test public void heldStartupWriterLeavesLocalCancelResponsiveAndOnlyRealCurrentPresentationUnlocks()throws Exception {
        HeldStartupActivity held=heldStartup(true);
        try {
            call("collectInitialDiagnostics");assertTrue(held.entered.await(1,TimeUnit.SECONDS));assertFalse(held.ranOnUi);
            assertEquals(0,Bridge.starts);call("startSurfaceIfReady");assertEquals(0,Bridge.starts);
            AtomicInteger uiEvent=new AtomicInteger();new android.os.Handler(Looper.getMainLooper()).post(uiEvent::incrementAndGet);
            Shadows.shadowOf(Looper.getMainLooper()).idle();assertEquals(1,uiEvent.get());
            assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("Loading"));
            find("Cancel").performClick();AlertDialog confirmation=(AlertDialog)field("backExitDialog").get(a);assertNotNull(confirmation);
            a.onBackPressed();Shadows.shadowOf(Looper.getMainLooper()).idle();
            assertFalse("Back dismisses the actual loading exit dialog",confirmation.isShowing());
            assertNull("Back revokes loading exit confirmation ownership",field("backExitDialog").get(a));
            assertTrue(Bridge.actions.isEmpty());
            held.release.countDown();awaitStartupResult(held);assertTrue(field("startupAssetsPrepared").getBoolean(a));
            assertEquals(1,Bridge.starts);assertNull(field("entryPlayButton").get(a));
            Bridge.runtime=1;Bridge.entry=new long[]{30,2,0,0,1};
            call("showMainMenu",new Class<?>[]{boolean.class},false);assertTrue(field("startupUiVisible").getBoolean(a));
            Bridge.entry=new long[]{31,2,0,0,1};call("showMainMenu",new Class<?>[]{boolean.class},false);
            assertFalse(field("startupUiVisible").getBoolean(a));assertNotNull(field("entryPlayButton").get(a));
            ((Button)field("entryPlayButton").get(a)).performClick();assertTrue(field("campaignMenuVisible").getBoolean(a));
        } finally {held.release.countDown();}
    }
    @Test public void startupFailureRetryCannotOverlapWriterAndReducedMotionRemainsStatic()throws Exception {
        HeldStartupActivity held=heldStartup(false);held.prepared=false;
        try {
            call("collectInitialDiagnostics");assertTrue(held.entered.await(1,TimeUnit.SECONDS));held.release.countDown();awaitStartupResult(held);
            assertTrue(field("startupAssetsFailed").getBoolean(a));assertEquals(0,Bridge.starts);
            Button oldRetry=find("Retry");assertNotNull(oldRetry);
            held.prepared=true;held.entered=new CountDownLatch(1);held.release=new CountDownLatch(1);held.finished=new CountDownLatch(1);
            oldRetry.performClick();assertTrue(held.entered.await(1,TimeUnit.SECONDS));oldRetry.performClick();
            assertEquals(2,held.calls.get());assertEquals(0,Bridge.starts);
            a.getSharedPreferences("campaign-menu",Context.MODE_PRIVATE).edit().putBoolean(InterfacePreferences.REDUCED_MOTION,true).apply();
            call("showStartupLoading");assertEquals(0,countProgress(a.findViewById(R.id.menu_scrim)));
            held.release.countDown();awaitStartupResult(held);oldRetry.performClick();assertEquals(2,held.calls.get());
            assertTrue(field("startupAssetsPrepared").getBoolean(a));assertTrue(field("startupUiVisible").getBoolean(a));
        } finally {held.release.countDown();}
    }
    @Test public void replacementActivitiesSerializeStartupWritersAndRejectOldCompletion()throws Exception {
        HeldStartupActivity first=heldStartup(false),second=null;
        try {
            call("collectInitialDiagnostics");assertTrue(first.entered.await(1,TimeUnit.SECONDS));
            second=heldStartup(false);call("collectInitialDiagnostics");
            assertFalse("replacement waits behind running cache writer",second.entered.await(50,TimeUnit.MILLISECONDS));
            first.release.countDown();assertTrue(second.entered.await(1,TimeUnit.SECONDS));
            Shadows.shadowOf(Looper.getMainLooper()).idle();
            assertFalse("old Activity result cannot admit its startup",field("startupAssetsPrepared").getBoolean(first));
            second.release.countDown();awaitStartupResult(second);
            assertTrue(field("startupAssetsPrepared").getBoolean(second));assertEquals(0,Bridge.starts);
        } finally {first.release.countDown();if(second!=null)second.release.countDown();}
    }
    @Test public void backgroundStartupCompletionCannotStartSurfaceAndDestroyedResultCannotReopenUi()throws Exception {
        HeldStartupActivity held=heldStartup(true);
        try {
            call("collectInitialDiagnostics");assertTrue(held.entered.await(1,TimeUnit.SECONDS));
            a.onPause();held.release.countDown();awaitStartupResult(held);
            assertTrue(field("startupAssetsPrepared").getBoolean(a));assertEquals(0,Bridge.starts);
            a.onResume();assertEquals(1,Bridge.starts);assertTrue(field("startupUiVisible").getBoolean(a));
            long epoch=field("campaignConfigurationEpoch").getLong(a),attempt=field("startupAssetsAttempt").getLong(a);
            set("campaignDestroyed",true);set("startupAssetsPrepared",false);
            call("completeInitialDiagnostics",new Class<?>[]{long.class,long.class,MainActivity.StartupDiagnostics.class},
                epoch,attempt,new MainActivity.StartupDiagnostics(true,"late"));
            assertFalse(field("startupAssetsPrepared").getBoolean(a));assertEquals(1,Bridge.starts);
        } finally {held.release.countDown();}
    }

    @Test public void nativeReadyCannotUnlockMenusWhenRequiredAssetsFailedOrAreStillPreparing()throws Exception {
        set("startupAssetsPrepared",false);set("entryMenuEnabled",true);
        call("showEntryMenu",new Class<?>[]{boolean.class},false);call("requestEntryPlay");
        assertTrue(field("startupUiVisible").getBoolean(a));assertFalse(field("campaignMenuVisible").getBoolean(a));
        assertTrue(Bridge.actions.isEmpty());assertNull(field("entryPlayButton").get(a));
    }

    @Test @GraphicsMode(GraphicsMode.Mode.NATIVE)
    public void startupLoadingAndErrorCardsKeepLargeTextActionsReachableWithinSafeInsets()throws Exception {
        final String originalQualifiers=RuntimeEnvironment.getQualifiers();
        final String evidence=System.getenv("HORDE_STARTUP_UI_EVIDENCE_DIR");
        try {
            for(int[] size:new int[][]{{320,568},{640,320}}) {
                RuntimeEnvironment.setQualifiers("w"+size[0]+"dp-h"+size[1]+"dp-"+(size[0]>size[1]?"land":"port")+"-mdpi");
                a=Robolectric.buildActivity(MainActivity.class).get();wireActivityViews();
                Configuration c=new Configuration(a.getResources().getConfiguration());c.fontScale=2;
                a.getResources().updateConfiguration(c,a.getResources().getDisplayMetrics());
                final WindowInsets safe=new WindowInsets.Builder().setStableInsets(Insets.of(12,24,12,24)).build();
                // Exercise the existing attachPanel inset API with explicit synthetic host insets.
                FrameLayout scrim=new FrameLayout(a){@Override public WindowInsets getRootWindowInsets(){return safe;}};
                scrim.setId(R.id.menu_scrim);
                View old=a.findViewById(R.id.menu_scrim);ViewGroup parent=(ViewGroup)old.getParent();
                ViewGroup.LayoutParams params=old.getLayoutParams();parent.removeView(old);parent.addView(scrim,params);set("menuScrim",scrim);
                set("entryMenuEnabled",true);set("startupAssetsPrepared",false);
                for(boolean failed:new boolean[]{false,true}) {
                    Bridge.runtime=failed?3:0;call("showStartupLoading");
                    scrim.measure(View.MeasureSpec.makeMeasureSpec(size[0],View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(size[1],View.MeasureSpec.EXACTLY));
                    scrim.layout(0,0,size[0],size[1]);
                    ScrollView scroller=(ScrollView)scrim.getChildAt(0);
                    assertTrue(scroller.getLeft()>=20 && scroller.getRight()<=size[0]-20);
                    assertTrue(scroller.getTop()>=32 && scroller.getBottom()<=size[1]-32);
                    assertEquals(2,a.getResources().getConfiguration().fontScale,0);
                    Button cancel=find("Cancel"),retry=find("Retry");assertNotNull(cancel);
                    assertEquals(failed,retry!=null);
                    final String scenario=(failed?"error":"loading")+"-"+size[0]+"x"+size[1]+"-font200";
                    writeStartupWidgetEvidence(evidence,scenario+"-top",scrim,scroller);
                    // A short landscape card may scroll, but neither action may be clipped or unreachable.
                    scroller.scrollTo(0,scroller.getChildAt(0).getHeight());
                    for(Button action:new Button[]{cancel,retry})if(action!=null) {
                        Rect bounds=new Rect(0,0,action.getWidth(),action.getHeight());scrim.offsetDescendantRectToMyCoords(action,bounds);
                        assertTrue(scenario+" complete action target "+bounds,action.isEnabled() && bounds.width()>=48 && bounds.height()>=48 &&
                            bounds.left>=scroller.getLeft() && bounds.right<=scroller.getRight() &&
                            bounds.top>=scroller.getTop() && bounds.bottom<=scroller.getBottom());
                        assertNotNull(action.getLayout());assertEquals(0,action.getLayout().getEllipsisCount(0));
                    }
                    writeStartupWidgetEvidence(evidence,scenario+"-actions",scrim,scroller);
                }
            }
        } finally {RuntimeEnvironment.setQualifiers(originalQualifiers);}
    }
    private void writeStartupWidgetEvidence(String directory,String scenario,ViewGroup root,ScrollView scroller)throws Exception {
        if(directory==null || directory.isEmpty())return;
        Bitmap image=Bitmap.createBitmap(root.getWidth(),root.getHeight(),Bitmap.Config.ARGB_8888);root.draw(new Canvas(image));
        File file=new File(directory,"android-widget-fixture-startup-"+scenario+".png");
        assertTrue(file.getParentFile().isDirectory() || file.getParentFile().mkdirs());
        try(FileOutputStream stream=new FileOutputStream(file)){assertTrue(image.compress(Bitmap.CompressFormat.PNG,100,stream));}
        StringBuilder metrics=new StringBuilder("HOST ANDROID WIDGET FIXTURE ONLY; no phone, RT world, native readiness or touch evidence.\n");
        metrics.append("viewport=").append(root.getWidth()).append('x').append(root.getHeight()).append(" syntheticStableInsets=12,24,12,24 fontScale=")
            .append(a.getResources().getConfiguration().fontScale).append(" actualDisplay=")
            .append(a.getResources().getDisplayMetrics().widthPixels).append('x').append(a.getResources().getDisplayMetrics().heightPixels)
            .append(" scroller=").append(scroller.getLeft()).append(',').append(scroller.getTop()).append(',').append(scroller.getRight()).append(',').append(scroller.getBottom())
            .append(" scrollY=").append(scroller.getScrollY()).append('\n');
        for(String label:new String[]{"Cancel","Retry"}) {
            Button action=find(label);if(action==null)continue;
            Rect bounds=new Rect(0,0,action.getWidth(),action.getHeight());root.offsetDescendantRectToMyCoords(action,bounds);
            metrics.append(label).append('=').append(bounds).append('\n');
        }
        Files.write(new File(directory,file.getName()+".txt").toPath(),(metrics.toString()+menuText(root)).getBytes(java.nio.charset.StandardCharsets.UTF_8));
    }

    @Test public void rejectedStartupSurfaceKeepsLocalRecoveryWithoutAutomaticUiThreadReprobe()throws Exception {
        set("surfaceAvailable",true);set("currentSurface",new Surface(new SurfaceTexture(0)));
        set("entryMenuEnabled",true);Bridge.runtime=0;
        for(boolean thrown:new boolean[]{false,true}) {
            set("surfaceRequestGeneration",0L);set("startupEntryFailed",false);
            Bridge.startResult=0;Bridge.failStart=thrown;call("showStartupLoading");call("startSurfaceIfReady");
            assertTrue(field("startupEntryFailed").getBoolean(a));assertFalse(field("diagnosticsVisible").getBoolean(a));
            assertNotNull(find("Retry"));assertNotNull(find("Cancel"));assertEquals(0,Bridge.textReportCalls);
        }
        assertEquals(2,Bridge.starts);
    }

    @Test public void startupNativeFailureKeepsRetryAndCancelWithoutAutomaticUiThreadReprobe()throws Exception {
        set("entryMenuEnabled",true);Bridge.runtime=0;call("showStartupLoading");
        assertNull(find("Retry"));
        Runnable poll=(Runnable)field("runtimePoll").get(a);
        for(int error:new int[]{2,3}) {
            Bridge.runtime=error;poll.run();
            assertTrue(field("startupUiVisible").getBoolean(a));assertFalse(field("diagnosticsVisible").getBoolean(a));
            assertNotNull(find("Retry"));assertNotNull(find("Cancel"));
            assertEquals("startup polling never invokes synchronous diagnostic probe",0,Bridge.textReportCalls);
            if(error==2)assertTrue(menuText(a.findViewById(R.id.menu_scrim)).contains("hardware ray tracing"));
        }
    }

}
