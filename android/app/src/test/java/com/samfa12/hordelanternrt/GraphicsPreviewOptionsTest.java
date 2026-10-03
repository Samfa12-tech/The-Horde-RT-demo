package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.content.SharedPreferences;
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

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=GraphicsPreviewOptionsTest.GraphicsBridgeShadow.class)
public final class GraphicsPreviewOptionsTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static final class GraphicsBridgeShadow {
        static GraphicsPreferences.Values rebased;
        static int resetCount;
        @Implementation protected static void __staticInitializer__() { }
        @Implementation protected static boolean confirmGraphicsSettings(long serial,long generation) { return serial==42 && generation==7; }
        @Implementation protected static void beginGraphicsEdit(int scale,int water,int fire,int cap) {
            rebased=new GraphicsPreferences.Values(scale,water,fire,cap);
        }
        @Implementation protected static long revertGraphicsSettings(long generation) { return generation==7?43:0; }
        @Implementation protected static double[] getGraphicsPreviewPerformance() { return new double[]{9,13,74,2,-1,0,0,0,1,74,0}; }
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
    private final GraphicsPreferences.Values draft = new GraphicsPreferences.Values(100, 2, 0, 60);

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
        assertTrue(confirmed.same(new GraphicsPreferences.Values(68,0,1,15)));
        assertTrue(draft.same(new GraphicsPreferences.Values(100,2,0,60)));
    }

    @Test public void boundedMenusIncludeCurrentCustomValuesAndUseProductionCameras() {
        assertArrayEquals(new int[]{50,63,68,75,100},GraphicsPreviewOptions.choices(confirmed,0));
        assertArrayEquals(new int[]{50,63,75,100},GraphicsPreviewOptions.choices(draft,0));
        assertArrayEquals(new int[]{0,1,2},GraphicsPreviewOptions.choices(confirmed,1));
        assertArrayEquals(new int[]{0,1},GraphicsPreviewOptions.choices(confirmed,2));
        assertArrayEquals(new int[]{15,30,42,60},GraphicsPreviewOptions.choices(new GraphicsPreferences.Values(75,1,0,42),3));
        assertArrayEquals(new int[]{15,30,60},GraphicsPreviewOptions.choices(confirmed,3));
        assertEquals(1,GraphicsPreviewOptions.camera(0)); assertEquals(3,GraphicsPreviewOptions.camera(1));
        assertEquals(0,GraphicsPreviewOptions.camera(2)); assertEquals(4,GraphicsPreviewOptions.camera(3));
    }

    @Test public void invalidChoicesAndValuesCannotProduceLiveTuple() {
        for (int[] input : new int[][]{{4,0},{0,49},{0,101},{1,-1},{1,3},{2,2},{3,14},{3,61}}) {
            try { GraphicsPreviewOptions.withChoice(confirmed,input[0],input[1]); fail("invalid choice/value accepted"); }
            catch (IllegalArgumentException expected) { }
        }
    }

    private long[] snapshot() {
        return new long[]{42,7,0,68,0,1,15,245,435,360,640,0,1,1,0,68,0,1,15,1};
    }

    @Test public void readinessRequiresCurrentGenerationSerialProfileRequestedAndEffectiveTuple() throws Exception {
        assertTrue(GraphicsPreviewOptions.presented(snapshot(),7,42,confirmed));
        assertTrue(GraphicsPreviewOptions.presented(snapshot(),7,0,confirmed));
        assertFalse(GraphicsPreviewOptions.presented(snapshot(),8,42,confirmed));
        assertFalse(GraphicsPreviewOptions.presented(snapshot(),7,43,confirmed));
        assertFalse(GraphicsPreviewOptions.presented(snapshot(),7,42,draft));
        for (int index : new int[]{3,4,5,6,15,16,17,18}) {
            long[] changed=snapshot(); ++changed[index];
            assertFalse("tuple index " + index,GraphicsPreviewOptions.presented(changed,7,42,confirmed));
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
        SharedPreferences stored=RuntimeEnvironment.getApplication().getSharedPreferences("preview-save-failure",Context.MODE_PRIVATE);
        stored.edit().clear().commit(); assertTrue(GraphicsPreferences.confirm(stored,confirmed));
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
        set(activity,"preferences",failing); set(activity,"graphicsConfirmed",confirmed);
        set(activity,"graphicsSubmitted",draft); set(activity,"graphicsDraft",draft);
        set(activity,"graphicsOriginalPreviewDraft",draft); set(activity,"graphicsPreviewWanted",true);
        set(activity,"graphicsRequestSerial",42L); set(activity,"surfaceRequestGeneration",7L);
        set(activity,"graphicsPreviewPerformanceGeneration",7L);
        GraphicsBridgeShadow.rebased=null; GraphicsBridgeShadow.resetCount=0;
        Method confirm=MainActivity.class.getDeclaredMethod("confirmGraphicsSelection");
        confirm.setAccessible(true); confirm.invoke(activity);
        assertTrue(GraphicsBridgeShadow.rebased.same(confirmed));
        assertSame(confirmed,get(activity,"graphicsDraft")); assertSame(draft,get(activity,"graphicsOriginalPreviewDraft"));
        assertEquals(43L,get(activity,"graphicsRequestSerial")); assertEquals(true,get(activity,"graphicsAwaitingRestore"));
        assertEquals(1,GraphicsBridgeShadow.resetCount);
        assertEquals(9,(double)get(activity,"graphicsPreviewPerformanceEpochFloor"),0);
        long[] restored=snapshot(); restored[0]=43;
        assertTrue(GraphicsPreviewOptions.presented(restored,7,43,(GraphicsPreferences.Values)get(activity,"graphicsDraft")));
        assertFalse(GraphicsPreviewOptions.presented(restored,7,43,draft));
        assertTrue(GraphicsPreferences.confirmed(stored).same(confirmed));
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
}
