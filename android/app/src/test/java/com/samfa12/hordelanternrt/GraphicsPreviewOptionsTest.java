package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.content.SharedPreferences;
import android.widget.FrameLayout;
import android.widget.PopupMenu;
import android.widget.Button;
import android.widget.TextView;
import java.time.Duration;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowSystemClock;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=GraphicsPreviewOptionsTest.GraphicsBridgeShadow.class)
public final class GraphicsPreviewOptionsTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class GraphicsBridgeShadow {
        static GraphicsPreferences.Values rebased;
        static int resetCount;
        static long[] applied;
        static double epoch=9, confirmationSeconds;
        static int confirmationCalls;
        static int compareCalls, applyCalls;
        static GraphicsPreferences.Values requested;
        @Implementation protected static void __staticInitializer__() { }
        @Implementation protected static boolean confirmGraphicsSettings(long serial,long generation) { return (serial==42 || serial==45) && generation==7; }
        @Implementation protected static void beginGraphicsEdit(int scale,int water,int fire,int cap,boolean glassEnabled,int shadowQuality,boolean mistEnabled) {
            rebased=new GraphicsPreferences.Values(scale,water,fire,cap,glassEnabled,shadowQuality,mistEnabled);
        }
        @Implementation protected static long revertGraphicsSettings(long generation) { return generation==7?43:0; }
        @Implementation protected static long compareGraphicsPreview(int scale,int water,int fire,int cap,boolean glassEnabled,int shadowQuality,boolean mistEnabled,long generation) {
            if(generation!=7) return 0;
            ++compareCalls; requested=new GraphicsPreferences.Values(scale,water,fire,cap,glassEnabled,shadowQuality,mistEnabled); return 44;
        }
        @Implementation protected static long applyGraphicsSettings(int scale,int water,int fire,int cap,boolean glassEnabled,int shadowQuality,boolean mistEnabled,long generation) {
            if(generation!=7) return 0;
            ++applyCalls; requested=new GraphicsPreferences.Values(scale,water,fire,cap,glassEnabled,shadowQuality,mistEnabled); return 45;
        }
        @Implementation protected static double[] getGraphicsPreviewPerformance() { return new double[]{epoch,13,74,2,-1,0,0,0,1,74,0}; }
        @Implementation protected static long[] getGraphicsSnapshot() { return applied.clone(); }
        @Implementation protected static int getSurfaceRuntimeState(long generation) { return generation==7?1:0; }
        @Implementation protected static long advanceGraphicsConfirmation(double seconds,boolean foreground,long generation) {
            if(foreground && generation==7) { ++confirmationCalls; confirmationSeconds+=seconds; }
            return confirmationSeconds>=15?43:0;
        }
        @Implementation protected static void setGraphicsPreview(boolean enabled,boolean paused,boolean motion,int camera,boolean reset,long generation) {
            if(enabled && reset && generation==7) ++resetCount;
        }
    }

    private static void set(MainActivity activity,String name,Object value) throws Exception {
        Field field=MainActivity.class.getDeclaredField(name); field.setAccessible(true); field.set(activity,value);
    }
    private static Object get(MainActivity activity,String name) throws Exception {
        Field field=MainActivity.class.getDeclaredField(name); field.setAccessible(true); return field.get(activity);
    }
    private final GraphicsPreferences.Values confirmed = new GraphicsPreferences.Values(68, 0, 1, 15);
    private final GraphicsPreferences.Values draft = new GraphicsPreferences.Values(100, 2, 0, 60, false);
    @Test public void experimentalChoicesAreExplicitAndAbsentFromOrdinaryAdmission() {
        assertArrayEquals(new int[]{33,40,50,63,68,75,100},
                GraphicsPreviewOptions.choices(confirmed,GraphicsPreviewOptions.RESOLUTION,33));
        assertArrayEquals(new int[]{50,63,68,75,100},
                GraphicsPreviewOptions.choices(confirmed,GraphicsPreviewOptions.RESOLUTION,50));
        assertArrayEquals(new int[]{50,63,75,100},GraphicsPreviewOptions.choices(
                new GraphicsPreferences.Values(33,1,0,30),GraphicsPreviewOptions.RESOLUTION,50));
        assertEquals("33% (Experimental)",GraphicsPreviewOptions.resolutionLabel(33,33));
        assertEquals("40% (Experimental)",GraphicsPreviewOptions.resolutionLabel(40,33));
        assertEquals("50%",GraphicsPreviewOptions.resolutionLabel(50,33));
        assertEquals("75%",GraphicsPreviewOptions.resolutionLabel(75,50));
    }

    @Test public void choicesRetainOtherCurrentFieldsAndNeverMutateConfirmedOrOriginalDraft() {
        int[][] expected = {{100,0,1,15}, {68,2,1,15}, {68,0,0,15}, {68,0,1,60}};
        for (int choice = 0; choice < 4; ++choice) {
            GraphicsPreferences.Values initial = GraphicsPreviewOptions.withChoice(confirmed,choice,
                    GraphicsPreviewOptions.value(draft,choice));
            assertTrue(initial.same(new GraphicsPreferences.Values(expected[choice][0],expected[choice][1],
                    expected[choice][2],expected[choice][3])));
        }
        GraphicsPreferences.Values live = GraphicsPreviewOptions.withChoice(confirmed,GraphicsPreviewOptions.WATER,2);
        live = GraphicsPreviewOptions.withChoice(live,GraphicsPreviewOptions.RESOLUTION,63);
        live = GraphicsPreviewOptions.withChoice(live,GraphicsPreviewOptions.CAP,30);
        assertTrue(live.same(new GraphicsPreferences.Values(63,2,1,30)));
        live = GraphicsPreviewOptions.withChoice(live,GraphicsPreviewOptions.FIRE,0);
        assertTrue(live.same(new GraphicsPreferences.Values(63,2,0,30)));
        live = GraphicsPreviewOptions.withChoice(live,GraphicsPreviewOptions.GLASS,0);
        assertTrue(live.same(new GraphicsPreferences.Values(63,2,0,30,false)));
        live = GraphicsPreviewOptions.withChoice(live,GraphicsPreviewOptions.WATER,0);
        live = GraphicsPreviewOptions.withChoice(live,GraphicsPreviewOptions.FIRE,1);
        live = GraphicsPreviewOptions.withChoice(live,GraphicsPreviewOptions.CAP,15);
        live = GraphicsPreviewOptions.withChoice(live,GraphicsPreviewOptions.RESOLUTION,75);
        assertTrue("every other live edit preserves explicit glass Off",live.same(new GraphicsPreferences.Values(75,0,1,15,false)));
        assertTrue(confirmed.same(new GraphicsPreferences.Values(68,0,1,15)));
        assertTrue(draft.same(new GraphicsPreferences.Values(100,2,0,60,false)));
        GraphicsPreferences.Values mistOff=GraphicsPreviewOptions.withChoice(confirmed,GraphicsPreviewOptions.MIST,0);
        assertFalse(mistOff.mistEnabled); assertTrue(confirmed.mistEnabled);
        for(int choice=0;choice<6;++choice) {
            mistOff=GraphicsPreviewOptions.withChoice(mistOff,choice,GraphicsPreviewOptions.value(draft,choice));
            assertFalse("every other live edit preserves explicit Mist Off: "+choice,mistOff.mistEnabled);
        }
        assertTrue(GraphicsPreviewOptions.withChoice(mistOff,GraphicsPreviewOptions.MIST,1).mistEnabled);
        assertArrayEquals(new int[]{0,1},GraphicsPreviewOptions.choices(mistOff,GraphicsPreviewOptions.MIST));
        for(int invalid:new int[]{-1,2,99}) {
            try { GraphicsPreviewOptions.withChoice(mistOff,GraphicsPreviewOptions.MIST,invalid); fail("invalid mist"); }
            catch(IllegalArgumentException rejectedMist) { }
        }
    }

    @Test public void boundedMenusIncludeCurrentCustomValuesAndUseProductionCameras() {
        assertArrayEquals(new int[]{50,63,68,75,100},GraphicsPreviewOptions.choices(confirmed,0));
        assertArrayEquals(new int[]{50,63,75,100},GraphicsPreviewOptions.choices(draft,0));
        assertArrayEquals(new int[]{0,1,2},GraphicsPreviewOptions.choices(confirmed,1));
        assertArrayEquals(new int[]{2,0,1},GraphicsPreviewOptions.choices(confirmed,2));
        assertArrayEquals(new int[]{15,30,42,60},GraphicsPreviewOptions.choices(new GraphicsPreferences.Values(75,1,0,42),3));
        assertArrayEquals(new int[]{15,30,60},GraphicsPreviewOptions.choices(confirmed,3));
        assertEquals(1,GraphicsPreviewOptions.camera(0)); assertEquals(3,GraphicsPreviewOptions.camera(1));
        assertEquals(0,GraphicsPreviewOptions.camera(2)); assertEquals(4,GraphicsPreviewOptions.camera(3));
        assertEquals(2,GraphicsPreviewOptions.camera(GraphicsPreviewOptions.GLASS));
        assertArrayEquals(new int[]{0,1},GraphicsPreviewOptions.choices(confirmed,GraphicsPreviewOptions.GLASS));
        assertEquals(0,GraphicsPreviewOptions.value(draft,GraphicsPreviewOptions.GLASS));
    }

    @Test public void invalidChoicesAndValuesCannotProduceLiveTuple() {
        for (int[] input : new int[][]{{7,0},{6,-1},{6,2},{0,49},{0,101},{1,-1},{1,3},{2,-1},{2,3},{3,14},{3,61},{4,-1},{4,2},{5,-1},{5,3}}) {
            try { GraphicsPreviewOptions.withChoice(confirmed,input[0],input[1]); fail("invalid choice/value accepted"); }
            catch (IllegalArgumentException expected) { }
        }
    }

    @Test public void lowFireAndShadowEditsPreserveEveryOtherLiveFieldAndStoredMeaning() {
        GraphicsPreferences.Values initial=new GraphicsPreferences.Values(68,0,2,42,false,2);
        assertArrayEquals(new int[]{2,0,1},GraphicsPreviewOptions.choices(initial,GraphicsPreviewOptions.FIRE));
        assertArrayEquals(new int[]{0,1,2},GraphicsPreviewOptions.choices(initial,GraphicsPreviewOptions.SHADOW));
        assertEquals("Low",GraphicsPreviewOptions.fireLabel(2));
        assertEquals("Mobile",GraphicsPreviewOptions.fireLabel(0));
        assertEquals("High",GraphicsPreviewOptions.fireLabel(1));
        assertEquals("Lower",GraphicsPreviewOptions.shadowLabel(0));
        assertEquals("Current",GraphicsPreviewOptions.shadowLabel(1));
        assertEquals("Higher",GraphicsPreviewOptions.shadowLabel(2));
        assertEquals("Shadows",GraphicsPreviewOptions.name(GraphicsPreviewOptions.SHADOW));
        assertEquals(0,GraphicsPreviewOptions.camera(GraphicsPreviewOptions.SHADOW));
        GraphicsPreferences.Values[] expected={
            new GraphicsPreferences.Values(75,0,2,42,false,2),new GraphicsPreferences.Values(68,1,2,42,false,2),
            new GraphicsPreferences.Values(68,0,1,42,false,2),new GraphicsPreferences.Values(68,0,2,30,false,2),
            new GraphicsPreferences.Values(68,0,2,42,true,2),new GraphicsPreferences.Values(68,0,2,42,false,0)};
        int[] selected={75,1,1,30,1,0};
        for(int choice=0;choice<expected.length;++choice)
            assertTrue("single choice preserves five other fields: "+choice,
                    GraphicsPreviewOptions.withChoice(initial,choice,selected[choice]).same(expected[choice]));
        assertTrue(initial.same(new GraphicsPreferences.Values(68,0,2,42,false,2)));
        assertEquals(1,GraphicsPreferences.baseline().shadow); assertEquals(0,GraphicsPreferences.baseline().fire);
        for(int unknown:new int[]{-1,3,99}) {
            try { GraphicsPreviewOptions.fireLabel(unknown); fail("unknown fire label admitted"); }
            catch(IllegalArgumentException expectedFailure) { }
            try { GraphicsPreviewOptions.shadowLabel(unknown); fail("unknown shadow label admitted"); }
            catch(IllegalArgumentException expectedFailure) { }
        }
    }

    @Test public void lowAndShadowReadinessRequiresBothExactUploadedAndRequestedPolicy() {
        GraphicsPreferences.Values selected=new GraphicsPreferences.Values(68,0,2,15,true,0);
        long[] ack=snapshot(); ack[5]=2; ack[17]=2; ack[22]=0; ack[23]=0;
        assertTrue(GraphicsPreviewOptions.presented(ack,7,42,selected));
        for(int index:new int[]{5,17,22,23}) {
            long[] stale=ack.clone(); stale[index]=1;
            assertFalse("current serial/present cannot hide stale fire or shadows: "+index,
                    GraphicsPreviewOptions.presented(stale,7,42,selected));
        }
        for(int shadow=0;shadow<=2;++shadow) {
            ack[22]=shadow; ack[23]=shadow;
            assertTrue(GraphicsPreviewOptions.presented(ack,7,42,
                    new GraphicsPreferences.Values(68,0,2,15,true,shadow)));
        }
        for(int index:new int[]{22,23}) for(int invalid:new int[]{-1,3,99}) {
            long[] malformed=ack.clone(); malformed[index]=invalid;
            assertFalse(GraphicsPreferences.presented(malformed,7));
        }
        for(int index:new int[]{5,17}) for(int invalid:new int[]{-1,3,99}) {
            long[] malformed=ack.clone(); malformed[index]=invalid;
            assertFalse("unknown fire cannot reach exhaustive presentation labels",GraphicsPreferences.presented(malformed,7));
        }
        assertFalse(GraphicsPreviewOptions.presented(java.util.Arrays.copyOf(ack,22),7,42,selected));
        assertFalse(GraphicsPreviewOptions.presented(ack,8,42,selected));
        assertFalse(GraphicsPreviewOptions.presented(ack,7,43,selected));
    }

    private long[] snapshot() {
        return new long[]{42,7,0,68,0,1,15,245,435,360,640,0,1,1,0,68,0,1,15,1,1,1,1,1,1,1};
    }

    private MainActivity livePreviewFixture() throws Exception {
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        FrameLayout scrim=new FrameLayout(activity); activity.setContentView(scrim);
        set(activity,"menuScrim",scrim);
        SharedPreferences prefs=RuntimeEnvironment.getApplication().getSharedPreferences("modal-preview",Context.MODE_PRIVATE);
        prefs.edit().clear().commit(); GraphicsPreferences.confirm(prefs,confirmed);
        set(activity,"preferences",prefs); set(activity,"graphicsConfirmed",confirmed); set(activity,"graphicsDraft",confirmed);
        set(activity,"graphicsSubmitted",confirmed); set(activity,"graphicsOriginalPreviewDraft",draft);
        set(activity,"graphicsVisible",true); set(activity,"graphicsPreviewWanted",true); set(activity,"resumed",true);
        set(activity,"surfaceRequestGeneration",7L); set(activity,"graphicsRequestSerial",42L);
        set(activity,"graphicsPreviewPerformanceGeneration",7L); set(activity,"graphicsPreviewPerformanceEpochFloor",9.0);
        GraphicsBridgeShadow.applied=snapshot(); GraphicsBridgeShadow.epoch=9;
        GraphicsBridgeShadow.confirmationSeconds=0; GraphicsBridgeShadow.confirmationCalls=0;
        GraphicsBridgeShadow.compareCalls=0; GraphicsBridgeShadow.applyCalls=0; GraphicsBridgeShadow.requested=null;
        Method show=MainActivity.class.getDeclaredMethod("showGraphicsPreviewPage"); show.setAccessible(true); show.invoke(activity);
        return activity;
    }

    @Test public void nativeGlassPopupStagesWholeLiveTupleAndOnlyCurrentOffAckAllowsApplyKeep() throws Exception {
        MainActivity activity=livePreviewFixture();
        GraphicsBridgeShadow.epoch=10;
        Runnable poll=(Runnable)get(activity,"refreshGraphics"); poll.run();
        Button glass=((Button[])get(activity,"graphicsOptionButtons"))[GraphicsPreviewOptions.GLASS];
        assertTrue(glass.isEnabled()); assertEquals("Glass On",glass.getText().toString());
        Method menu=MainActivity.class.getDeclaredMethod("showGraphicsPreviewOptionMenu",Button.class,int.class);
        menu.setAccessible(true); menu.invoke(activity,glass,GraphicsPreviewOptions.GLASS);
        PopupMenu popup=(PopupMenu)get(activity,"graphicsOptionsPopup");
        assertEquals("Off",popup.getMenu().findItem(0).getTitle());
        assertEquals("On",popup.getMenu().findItem(1).getTitle());
        assertTrue(popup.getMenu().findItem(1).isChecked());
        assertTrue(popup.getMenu().performIdentifierAction(0,0));
        GraphicsPreferences.Values off=new GraphicsPreferences.Values(68,0,1,15,false);
        SharedPreferences prefs=(SharedPreferences)get(activity,"preferences");
        assertEquals(1,GraphicsBridgeShadow.compareCalls); assertTrue(GraphicsBridgeShadow.requested.same(off));
        assertTrue(((GraphicsPreferences.Values)get(activity,"graphicsDraft")).same(off));
        assertSame(draft,get(activity,"graphicsOriginalPreviewDraft"));
        assertTrue(GraphicsPreferences.hasPending(prefs)); assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(off));
        assertTrue("transient choice cannot save confirmed glass",GraphicsPreferences.confirmed(prefs).same(confirmed));
        GraphicsBridgeShadow.applied=snapshot(); GraphicsBridgeShadow.applied[0]=44; GraphicsBridgeShadow.applied[21]=0;
        GraphicsBridgeShadow.epoch=11; poll.run(); // Prior On resources with current request/serial/extents.
        assertFalse(((Button)get(activity,"graphicsConfirm")).isEnabled());
        ((Button)get(activity,"graphicsConfirm")).performClick();
        assertEquals("previous On frame cannot Apply an Off candidate",0,GraphicsBridgeShadow.applyCalls);
        GraphicsBridgeShadow.applied[20]=0; poll.run();
        assertTrue(((Button)get(activity,"graphicsConfirm")).isEnabled());
        ((Button)get(activity,"graphicsConfirm")).performClick();
        assertEquals(1,GraphicsBridgeShadow.applyCalls); assertTrue(GraphicsBridgeShadow.requested.same(off));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(confirmed));
        GraphicsBridgeShadow.applied[0]=45; GraphicsBridgeShadow.applied[2]=2; GraphicsBridgeShadow.epoch=12;
        poll.run(); assertEquals("Keep and save",((Button)get(activity,"graphicsConfirm")).getText().toString());
        ((Button)get(activity,"graphicsConfirm")).performClick();
        assertTrue(GraphicsPreferences.confirmed(prefs).same(off)); assertFalse(GraphicsPreferences.hasPending(prefs));
    }

    @Test public void failedGlassRebuildClosesModalPreservesConfirmedAndRequiresRestoredAckToClearPending() throws Exception {
        MainActivity activity=livePreviewFixture();
        GraphicsPreferences.Values off=new GraphicsPreferences.Values(68,0,1,15,false);
        SharedPreferences prefs=(SharedPreferences)get(activity,"preferences");
        assertTrue(GraphicsPreferences.markPending(prefs,off));
        set(activity,"graphicsDraft",off); set(activity,"graphicsSubmitted",off);
        Method menu=MainActivity.class.getDeclaredMethod("showGraphicsPreviewOptionMenu",Button.class,int.class);
        menu.setAccessible(true); menu.invoke(activity,((Button[])get(activity,"graphicsOptionButtons"))[4],4);
        GraphicsBridgeShadow.applied[21]=0; GraphicsBridgeShadow.applied[14]=64; // Actual prior On scene restored.
        Runnable poll=(Runnable)get(activity,"refreshGraphics"); poll.run();
        assertNull(get(activity,"graphicsOptionsPopup"));
        assertTrue(((TextView)get(activity,"graphicsTelemetry")).getText().toString().contains("Choice failed; Restore saved"));
        assertFalse(((Button)get(activity,"graphicsConfirm")).isEnabled());
        assertTrue(((Button)get(activity,"graphicsRevert")).isEnabled());
        assertTrue(GraphicsPreferences.hasPending(prefs)); assertTrue(GraphicsPreferences.confirmed(prefs).same(confirmed));
        ((Button)get(activity,"graphicsRevert")).performClick();
        assertSame(confirmed,get(activity,"graphicsDraft")); assertNull(get(activity,"graphicsLiveChoiceError"));
        assertTrue(GraphicsPreferences.hasPending(prefs));
        poll.run(); assertTrue("old failed frame cannot clear restore marker",GraphicsPreferences.hasPending(prefs));
        GraphicsBridgeShadow.applied=snapshot(); GraphicsBridgeShadow.applied[0]=43;
        poll.run(); assertFalse(GraphicsPreferences.hasPending(prefs));
        assertSame(draft,get(activity,"graphicsOriginalPreviewDraft"));
    }

    @Test public void detailsSnapshotDoesNotSkipVisibleConfirmationTimeoutOrRestoredAck() throws Exception {
        MainActivity activity=livePreviewFixture();
        GraphicsPreferences.markPending((SharedPreferences)get(activity,"preferences"),confirmed);
        GraphicsBridgeShadow.applied[2]=2;
        Method details=MainActivity.class.getDeclaredMethod("showGraphicsPreviewDetails"); details.setAccessible(true); details.invoke(activity);
        Runnable poll=(Runnable)get(activity,"refreshGraphics");
        ShadowSystemClock.advanceBy(Duration.ofMillis(1)); poll.run();
        ShadowSystemClock.advanceBy(Duration.ofSeconds(15)); poll.run();
        assertEquals(2,GraphicsBridgeShadow.confirmationCalls); assertTrue(GraphicsBridgeShadow.confirmationSeconds>=15);
        assertEquals(43L,get(activity,"graphicsRequestSerial")); assertEquals(true,get(activity,"graphicsAwaitingRestore"));
        assertNull("changed restore serial invalidates the modal snapshot",get(activity,"graphicsDetailsDialog"));
        assertTrue(((TextView)get(activity,"graphicsTelemetry")).getText().toString().contains("waiting for RT"));
        GraphicsBridgeShadow.applied=snapshot(); GraphicsBridgeShadow.applied[0]=43; poll.run();
        assertEquals(false,get(activity,"graphicsAwaitingRestore"));
        assertFalse(GraphicsPreferences.hasPending((SharedPreferences)get(activity,"preferences")));
        assertNull(get(activity,"graphicsDetailsDialog")); assertSame(draft,get(activity,"graphicsOriginalPreviewDraft"));
    }

    @Test public void popupDismissRejectsCachedOldScopeAndRevalidatesCurrentAckBeforeCounter() throws Exception {
        MainActivity activity=livePreviewFixture();
        set(activity,"graphicsDisplayedFps",99.0);
        Method menu=MainActivity.class.getDeclaredMethod("showGraphicsPreviewOptionMenu",Button.class,int.class);
        menu.setAccessible(true); menu.invoke(activity,((Button[])get(activity,"graphicsOptionButtons"))[0],0);
        PopupMenu popup=(PopupMenu)get(activity,"graphicsOptionsPopup"); popup.dismiss();
        assertNull(get(activity,"graphicsOptionsPopup")); assertTrue(Double.isNaN((double)get(activity,"graphicsDisplayedFps")));
        Runnable poll=(Runnable)get(activity,"refreshGraphics");
        GraphicsBridgeShadow.applied[1]=8; GraphicsBridgeShadow.epoch=10; poll.run();
        assertTrue(((TextView)get(activity,"graphicsTelemetry")).getText().toString().contains("waiting for RT"));
        GraphicsBridgeShadow.applied=snapshot(); GraphicsBridgeShadow.epoch=9; poll.run();
        assertTrue(((TextView)get(activity,"graphicsTelemetry")).getText().toString().contains("waiting for RT"));
        GraphicsBridgeShadow.epoch=10; poll.run();
        assertTrue(((TextView)get(activity,"graphicsTelemetry")).getText().toString().contains("Preview FPS: 13.0"));
        assertEquals(10,(double)get(activity,"graphicsDisplayedFpsEpoch"),0);
        menu.invoke(activity,((Button[])get(activity,"graphicsOptionButtons"))[0],0);
        GraphicsBridgeShadow.epoch=11; poll.run();
        assertNull("actual changed epoch invalidates an open native popup",get(activity,"graphicsOptionsPopup"));
        assertEquals(11,(double)get(activity,"graphicsDisplayedFpsEpoch"),0);
        GraphicsBridgeShadow.applied[15]=75; poll.run();
        assertTrue(((TextView)get(activity,"graphicsTelemetry")).getText().toString().contains("waiting for RT"));
        GraphicsBridgeShadow.applied=snapshot(); poll.run();
        menu.invoke(activity,((Button[])get(activity,"graphicsOptionButtons"))[0],0);
        GraphicsBridgeShadow.applied[14]=64; GraphicsBridgeShadow.applied[19]=0; poll.run();
        assertNull("restored Showcase failure must close the popup",get(activity,"graphicsOptionsPopup"));
        assertEquals(false,get(activity,"graphicsPreviewWanted"));
        assertNotNull(get(activity,"graphicsRecoveryNotice"));
        assertEquals(0,((double[])get(activity,"graphicsPreviewDetailsSamples"))[8],0);
    }

    @Test public void readinessRequiresCurrentGenerationSerialProfileRequestedAndEffectiveTuple() throws Exception {
        assertTrue(GraphicsPreviewOptions.presented(snapshot(),7,42,confirmed));
        assertTrue(GraphicsPreviewOptions.presented(snapshot(),7,0,confirmed));
        assertFalse(GraphicsPreviewOptions.presented(snapshot(),8,42,confirmed));
        assertFalse(GraphicsPreviewOptions.presented(snapshot(),7,43,confirmed));
        assertFalse(GraphicsPreviewOptions.presented(snapshot(),7,42,draft));
        for (int index : new int[]{3,4,5,6,15,16,17,18,22,23}) {
            long[] changed=snapshot(); ++changed[index];
            assertFalse("tuple index " + index,GraphicsPreviewOptions.presented(changed,7,42,confirmed));
        }
        for (int index : new int[]{20,21,24,25}) {
            long[] changed=snapshot(); changed[index]=0;
            assertFalse("current resources/requested glass differs despite matching extent, index " + index,
                    GraphicsPreviewOptions.presented(changed,7,42,confirmed));
        }
        for (int index : new int[]{1,7,8,9,10,12,13,19}) {
            long[] changed=snapshot(); changed[index]=0;
            assertFalse("presentation index " + index,GraphicsPreviewOptions.presented(changed,7,42,confirmed));
        }
        long[] compute=snapshot(); compute[12]=2;
        assertTrue(GraphicsPreviewOptions.presented(compute,7,42,confirmed));
        assertFalse(GraphicsPreviewOptions.presented(null,7,42,confirmed));
        assertFalse(GraphicsPreviewOptions.presented(new long[19],7,42,confirmed));

        // Invoke the production save-failure branch, with only its JNI boundary replaced.
        GraphicsPreferences.Values savedHigher=new GraphicsPreferences.Values(68,0,1,15,true,2);
        GraphicsPreferences.Values pendingLowLower=new GraphicsPreferences.Values(100,2,2,60,false,0,false);
        SharedPreferences stored=RuntimeEnvironment.getApplication().getSharedPreferences("preview-save-failure",Context.MODE_PRIVATE);
        stored.edit().clear().commit(); assertTrue(GraphicsPreferences.confirm(stored,savedHigher));
        SharedPreferences failing=(SharedPreferences)Proxy.newProxyInstance(SharedPreferences.class.getClassLoader(),
                new Class<?>[]{SharedPreferences.class},(proxy,method,args)->{
                    if (!method.getName().equals("edit")) return method.invoke(stored,args);
                    SharedPreferences.Editor editor=stored.edit();
                    return Proxy.newProxyInstance(SharedPreferences.Editor.class.getClassLoader(),
                            new Class<?>[]{SharedPreferences.Editor.class},(editorProxy,editMethod,editArgs)->{
                                if (editMethod.getName().equals("commit")) return false;
                                Object result=editMethod.invoke(editor,editArgs);
                                return result instanceof SharedPreferences.Editor?editorProxy:result;
                            });
                });
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        set(activity,"preferences",failing); set(activity,"graphicsConfirmed",savedHigher);
        set(activity,"graphicsSubmitted",pendingLowLower); set(activity,"graphicsDraft",pendingLowLower);
        set(activity,"graphicsOriginalPreviewDraft",draft); set(activity,"graphicsPreviewWanted",true);
        set(activity,"graphicsRequestSerial",42L); set(activity,"surfaceRequestGeneration",7L);
        set(activity,"graphicsPreviewPerformanceGeneration",7L);
        GraphicsBridgeShadow.rebased=null; GraphicsBridgeShadow.resetCount=0; GraphicsBridgeShadow.epoch=9;
        Method confirm=MainActivity.class.getDeclaredMethod("confirmGraphicsSelection");
        confirm.setAccessible(true); confirm.invoke(activity);
        assertTrue(GraphicsBridgeShadow.rebased.same(savedHigher));
        assertSame(savedHigher,get(activity,"graphicsDraft")); assertSame(draft,get(activity,"graphicsOriginalPreviewDraft"));
        assertEquals(43L,get(activity,"graphicsRequestSerial")); assertEquals(true,get(activity,"graphicsAwaitingRestore"));
        assertEquals(1,GraphicsBridgeShadow.resetCount);
        assertEquals(9,(double)get(activity,"graphicsPreviewPerformanceEpochFloor"),0);
        long[] restored=snapshot(); restored[0]=43; restored[22]=2; restored[23]=2;
        assertTrue(GraphicsPreviewOptions.presented(restored,7,43,(GraphicsPreferences.Values)get(activity,"graphicsDraft")));
        assertFalse(GraphicsPreviewOptions.presented(restored,7,43,draft));
        assertTrue(GraphicsPreferences.confirmed(stored).same(savedHigher));
    }

    @Test public void counterRejectsOldOrMalformedScopesAndOverlayBudgetIncludesSafeGap() {
        double[] current={9,13.5,74,2,-1,27635392,1024,1,1,74,0};
        assertTrue(GraphicsPreviewOptions.currentPerformance(current,8));
        assertFalse(GraphicsPreviewOptions.currentPerformance(current,9));
        assertFalse(GraphicsPreviewOptions.currentPerformance(current,10));
        assertEquals(9,GraphicsPreviewOptions.performanceEpochFloor(7,7,current),0);
        assertEquals(0,GraphicsPreviewOptions.performanceEpochFloor(7,8,current),0);
        assertEquals(0,GraphicsPreviewOptions.performanceEpochFloor(7,7,null),0);
        assertTrue(GraphicsPreviewOptions.currentPerformance(new double[]{1,12,80,2,-1,0,0,0,1,80,0},
                GraphicsPreviewOptions.performanceEpochFloor(7,8,current)));
        assertFalse(GraphicsPreviewOptions.currentPerformance(new double[9],0));
        assertFalse(GraphicsPreviewOptions.currentPerformance(null,0));
        for (double fps : new double[]{-1,Double.NaN,Double.POSITIVE_INFINITY}) {
            double[] malformed=current.clone(); malformed[1]=fps;
            assertFalse(GraphicsPreviewOptions.currentPerformance(malformed,8));
        }
        for (double count : new double[]{0,1.5,129,Double.NaN}) {
            double[] malformed=current.clone(); malformed[8]=count;
            assertFalse(GraphicsPreviewOptions.currentPerformance(malformed,8));
        }
        assertFalse(GraphicsPreviewOptions.currentPerformance(new double[]{9,13,74,2,-1,0,0,0,1},8));
        for (int height : new int[]{568,640,915,100000}) for (int gap : new int[]{8,24,48})
            assertTrue(GraphicsPreviewOptions.maximumOverlayHeight(height,gap)+gap <= height*.35);
        assertEquals(1,GraphicsPreviewOptions.maximumOverlayHeight(0,0));
    }
    @Test public void nativeMistPopupRequiresOffResourceAckAndExplicitUseThenKeepBeforeSaving() throws Exception {
        MainActivity activity=livePreviewFixture();
        GraphicsBridgeShadow.epoch=10;
        Runnable poll=(Runnable)get(activity,"refreshGraphics"); poll.run();
        Button mist=((Button[])get(activity,"graphicsOptionButtons"))[GraphicsPreviewOptions.MIST];
        assertTrue(mist.isEnabled()); assertEquals("Mist On",mist.getText().toString());
        Method menu=MainActivity.class.getDeclaredMethod("showGraphicsPreviewOptionMenu",Button.class,int.class);
        menu.setAccessible(true); menu.invoke(activity,mist,GraphicsPreviewOptions.MIST);
        PopupMenu popup=(PopupMenu)get(activity,"graphicsOptionsPopup");
        assertEquals("Off",popup.getMenu().findItem(0).getTitle()); assertTrue(popup.getMenu().findItem(1).isChecked());
        assertTrue(popup.getMenu().performIdentifierAction(0,0));
        GraphicsPreferences.Values off=new GraphicsPreferences.Values(68,0,1,15,true,1,false);
        SharedPreferences prefs=(SharedPreferences)get(activity,"preferences");
        assertTrue(GraphicsBridgeShadow.requested.same(off)); assertSame(draft,get(activity,"graphicsOriginalPreviewDraft"));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(off));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(confirmed));
        GraphicsBridgeShadow.applied=snapshot(); GraphicsBridgeShadow.applied[0]=44; GraphicsBridgeShadow.applied[25]=0;
        GraphicsBridgeShadow.epoch=11; poll.run();
        assertFalse("old Mist On frame cannot trial Off",((Button)get(activity,"graphicsConfirm")).isEnabled());
        GraphicsBridgeShadow.applied[24]=0; poll.run();
        assertTrue(((Button)get(activity,"graphicsConfirm")).isEnabled());
        ((Button)get(activity,"graphicsConfirm")).performClick();
        assertEquals(1,GraphicsBridgeShadow.applyCalls); assertTrue(GraphicsBridgeShadow.requested.same(off));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(confirmed));
        GraphicsBridgeShadow.applied[0]=45; GraphicsBridgeShadow.applied[2]=2; GraphicsBridgeShadow.epoch=12; poll.run();
        assertEquals("Keep and save",((Button)get(activity,"graphicsConfirm")).getText().toString());
        ((Button)get(activity,"graphicsConfirm")).performClick();
        assertTrue(GraphicsPreferences.confirmed(prefs).same(off)); assertFalse(GraphicsPreferences.hasPending(prefs));
    }
    private static Button findButton(android.view.View view,String text) {
        if(view instanceof Button && text.contentEquals(((Button)view).getText())) return (Button)view;
        if(view instanceof android.view.ViewGroup) {
            android.view.ViewGroup group=(android.view.ViewGroup)view;
            for(int i=0;i<group.getChildCount();++i) { Button found=findButton(group.getChildAt(i),text); if(found!=null) return found; }
        }
        return null;
    }
    @Test public void productionResetStagesMobileDefaultsWithoutSavingOrChangingUnrelatedPrefs() throws Exception {
        MainActivity activity=livePreviewFixture();
        set(activity,"graphicsPreviewWanted",false);
        SharedPreferences prefs=(SharedPreferences)get(activity,"preferences"); prefs.edit().putInt("music_volume",43).commit();
        Method show=MainActivity.class.getDeclaredMethod("showGraphicsPage"); show.setAccessible(true); show.invoke(activity);
        Button reset=findButton((android.view.View)get(activity,"menuScrim"),"Reset draft to mobile defaults");
        assertNotNull(reset); reset.performClick();
        assertTrue(((GraphicsPreferences.Values)get(activity,"graphicsDraft")).same(GraphicsPreferences.mobileDefaults()));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(confirmed)); assertFalse(GraphicsPreferences.hasPending(prefs));
        assertEquals(43,prefs.getInt("music_volume",0)); assertEquals(0,GraphicsBridgeShadow.applyCalls);
        ((Button)get(activity,"graphicsApply")).performClick();
        assertEquals(1,GraphicsBridgeShadow.applyCalls); assertTrue(GraphicsBridgeShadow.requested.same(GraphicsPreferences.mobileDefaults()));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(GraphicsPreferences.mobileDefaults()));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(confirmed));
    }

}
