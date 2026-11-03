package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.app.AlertDialog;
import android.widget.Button;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.concurrent.ExecutorService;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=EndGameBenchmarkAvailabilityTest.Bridge.class)
public final class EndGameBenchmarkAvailabilityTest {
    @Implements(value=ProbeBridge.class,isInAndroidSdk=false)
    public static class Bridge {
        static long[] campaign,entry;
        static int requests;
        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static long[] getCampaignState(){return campaign.clone();}
        @Implementation protected static long[] getEntryMenuState(){return entry.clone();}
        @Implementation protected static int getSurfaceRuntimeState(long generation){return 1;}
        @Implementation protected static long requestCampaign(int action,int slot,int kind,long generation){requests++;return 100;}
    }
    private MainActivity activity;
    private CampaignMenuState active;
    private Button custom,provisional;
    private static long[] publication(long generation,long revision,int slot,int phase) {
        long[] v=CampaignMenuStateTest.ready();v[1]=generation;v[2]=revision;v[3]=42;
        v[4]=phase;v[6]=slot;v[30]=CampaignMenuState.END_GAME;return v;
    }
    private void set(String name,Object value)throws Exception {
        Field f=MainActivity.class.getDeclaredField(name);f.setAccessible(true);f.set(activity,value);
    }
    private Object get(String name)throws Exception {
        Field f=MainActivity.class.getDeclaredField(name);f.setAccessible(true);return f.get(activity);
    }
    private Object call(String name,Class<?>[] types,Object... values)throws Exception {
        Method m=MainActivity.class.getDeclaredMethod(name,types);m.setAccessible(true);return m.invoke(activity,values);
    }
    private boolean poll()throws Exception {return (Boolean)call("pollCampaignEndForEntry",new Class<?>[]{});}
    private void refresh()throws Exception {
        call("applyEntryMenuAvailability",new Class<?>[]{EntryMenuState.class},EntryMenuState.decode(Bridge.entry));
    }
    @Before public void setUp()throws Exception {
        activity=Robolectric.buildActivity(MainActivity.class).get();
        active=CampaignMenuState.decode(publication(31,10,2,CampaignMenuState.READY));
        set("campaignState",active);set("campaignPublicationValid",true);
        set("campaignEndPending",true);set("campaignEndRequest",42L);set("campaignEndRequestGeneration",31L);
        set("surfaceRequestGeneration",31L);set("resumed",true);set("startupAssetsPrepared",true);set("entryMenuEnabled",true);
        custom=new Button(activity);provisional=new Button(activity);
        set("openingBenchmarkButton",custom);set("openingBenchmarkProvisionalButton",provisional);
        Bridge.entry=new long[]{31,2,0,0,1};Bridge.campaign=publication(31,11,-1,CampaignMenuState.READY);Bridge.requests=0;
        refresh();assertFalse(custom.isEnabled());assertFalse(provisional.isEnabled());
    }
    @After public void tearDown()throws Exception {
        AlertDialog dialog=(AlertDialog)get("campaignEndFailureDialog");if(dialog!=null)dialog.dismiss();
        ((ExecutorService)get("reportExecutor")).shutdownNow();((ExecutorService)get("updateExecutor")).shutdownNow();
    }
    @Test public void acknowledgedEndReplacesActiveCacheAndEnablesBothActualButtons()throws Exception {
        assertFalse(poll());refresh();
        CampaignMenuState ended=(CampaignMenuState)get("campaignState");
        assertEquals(-1,ended.activeSlot);assertEquals(11,ended.revision);
        assertEquals(Boolean.TRUE,get("campaignPublicationValid"));assertEquals(Boolean.FALSE,get("campaignEndPending"));
        assertTrue(custom.isEnabled());assertTrue(provisional.isEnabled());assertEquals(0,Bridge.requests);
        // The END acknowledgement alone never bypasses scene readiness or measuring guards.
        Bridge.entry[4]=0;refresh();assertFalse(custom.isEnabled());assertFalse(provisional.isEnabled());
        Bridge.entry[4]=1;set("benchmarkRunning",true);refresh();assertFalse(custom.isEnabled());assertFalse(provisional.isEnabled());
    }
    @Test public void pendingAndFailedEndKeepActiveCacheAndNeverAutoAcknowledge()throws Exception {
        Bridge.campaign=publication(31,11,2,CampaignMenuState.SAVING);
        assertTrue(poll());refresh();assertSame(active,get("campaignState"));assertFalse(custom.isEnabled());
        Bridge.campaign=publication(31,12,2,CampaignMenuState.FAILED);Bridge.campaign[5]=6;
        assertTrue(poll());refresh();assertSame(active,get("campaignState"));assertFalse(custom.isEnabled());
        assertFalse(provisional.isEnabled());assertEquals(0,Bridge.requests);
        AlertDialog dialog=(AlertDialog)get("campaignEndFailureDialog");assertNotNull(dialog);assertTrue(dialog.isShowing());
    }
    @Test public void staleGenerationRevisionAndRequestCannotReleaseActiveCache()throws Exception {
        Bridge.campaign=publication(30,11,-1,CampaignMenuState.READY);
        assertTrue(poll());assertSame(active,get("campaignState"));
        Bridge.campaign=publication(31,9,-1,CampaignMenuState.READY);
        assertTrue(poll());assertSame(active,get("campaignState"));
        Bridge.campaign=publication(31,11,-1,CampaignMenuState.READY);Bridge.campaign[3]=41;
        assertTrue(poll());refresh();assertSame(active,get("campaignState"));
        assertFalse(custom.isEnabled());assertFalse(provisional.isEnabled());assertEquals(0,Bridge.requests);
    }
    @Test public void currentGenerationCanReplaceHigherRevisionFromRetiredSurface()throws Exception {
        set("campaignState",CampaignMenuState.decode(publication(30,1000,2,CampaignMenuState.READY)));
        assertFalse(poll());refresh();assertTrue(custom.isEnabled());assertTrue(provisional.isEnabled());
    }
}
