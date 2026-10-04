package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.content.SharedPreferences;
import java.lang.reflect.Proxy;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class GraphicsPreferencesTest {
    private SharedPreferences prefs;
    @Before public void setup() {
        prefs = RuntimeEnvironment.getApplication().getSharedPreferences("graphics-only-fixture", Context.MODE_PRIVATE);
        prefs.edit().clear().commit();
    }
    @Test public void migratesLegacyAppearanceAndKeepsExplicitIndependentFire() {
        prefs.edit().putInt("render_scale", 92).putInt("water_quality", 2).commit();
        GraphicsPreferences.Values a = GraphicsPreferences.confirmed(prefs);
        assertEquals(92, a.scale); assertEquals(2, a.water); assertEquals(1, a.fire);
        prefs.edit().putInt(GraphicsPreferences.FIRE, 0).commit();
        assertEquals(0, GraphicsPreferences.confirmed(prefs).fire);
    }
    @Test public void pendingCrashRetainsIntentAndConfirmedStartup() {
        GraphicsPreferences.confirm(prefs, new GraphicsPreferences.Values(68, 0, 1, 30));
        GraphicsPreferences.Values candidate = new GraphicsPreferences.Values(100, 2, 1, 60);
        assertTrue(GraphicsPreferences.markPending(prefs, candidate));
        assertTrue(GraphicsPreferences.hasPending(prefs));
        assertEquals(68, GraphicsPreferences.confirmed(prefs).scale);
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(candidate));
        GraphicsPreferences.clearAfterRestore(prefs);
        assertFalse(GraphicsPreferences.hasPending(prefs));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(candidate));
    }
    @Test public void confirmationIsScopedAndInvalidSettingsCannotPersist() {
        prefs.edit().putInt("music_volume", 43).putBoolean("rt_lab_unlocked", true)
                .putString("report_consent_fixture", "private-choice").commit();
        assertFalse(GraphicsPreferences.markPending(prefs, new GraphicsPreferences.Values(10, 1, 0, 30)));
        assertTrue(GraphicsPreferences.confirm(prefs, GraphicsPreferences.baseline()));
        assertEquals(43, prefs.getInt("music_volume", 0));
        assertTrue(prefs.getBoolean("rt_lab_unlocked", false));
        assertEquals("private-choice", prefs.getString("report_consent_fixture", ""));
    }
    @Test public void malformedOrUnknownStoredGraphicsRecoverToUsableBaseline() {
        prefs.edit().putInt("render_scale", 150).commit();
        assertTrue(GraphicsPreferences.confirmed(prefs).same(GraphicsPreferences.baseline()));
        prefs.edit().putInt("render_scale", 80).putInt("graphics_schema", 99).commit();
        assertTrue(GraphicsPreferences.confirmed(prefs).same(GraphicsPreferences.baseline()));
        prefs.edit().remove("graphics_schema").putString("render_scale", "invalid").commit();
        assertEquals(75, GraphicsPreferences.confirmed(prefs).scale);
    }
    @Test public void snapshotRequiresCurrentGenerationAndRealPresentedDimensions() {
        long[] a = new long[24];
        a[1]=7; a[7]=800; a[8]=600; a[9]=1200; a[10]=900; a[12]=1; a[13]=1;
        assertTrue(GraphicsPreferences.presented(a, 7));
        assertFalse(GraphicsPreferences.presented(a, 8));
        a[13]=0; assertFalse(GraphicsPreferences.presented(a, 7));
        a[13]=1; a[7]=0; assertFalse(GraphicsPreferences.presented(a, 7));
        assertFalse(GraphicsPreferences.presented(new long[19], 7));
        assertFalse(GraphicsPreferences.presented(new long[20], 7));
        assertFalse(GraphicsPreferences.presented(new long[22], 7));
        assertFalse(GraphicsPreferences.presented(null, 7));
    }
    @Test public void schemaOneMigrationDefaultsOnlyGlassAndKeepsPendingTuple() {
        prefs.edit().putInt("graphics_schema",1).putInt("render_scale",68).putInt("water_quality",0)
                .putInt(GraphicsPreferences.FIRE,1).putInt(GraphicsPreferences.CAP,42)
                .putBoolean(GraphicsPreferences.GLASS,false).putBoolean(GraphicsPreferences.PENDING,true)
                .putInt("graphics_pending_scale",92).putInt("graphics_pending_water",2)
                .putInt("graphics_pending_fire",0).putInt("graphics_pending_cap",15)
                .putBoolean(GraphicsPreferences.PENDING_GLASS,false).commit();
        assertTrue(GraphicsPreferences.confirmed(prefs).same(new GraphicsPreferences.Values(68,0,1,42,true)));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(new GraphicsPreferences.Values(92,2,0,15,true)));
        assertTrue(GraphicsPreferences.hasPending(prefs));
        // First new transient Off request can coexist with a schema1 confirmed tuple.
        GraphicsPreferences.Values off=new GraphicsPreferences.Values(63,1,0,30,false);
        assertTrue(GraphicsPreferences.markPending(prefs,off));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(off));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(new GraphicsPreferences.Values(68,0,1,42,true)));
        assertTrue(GraphicsPreferences.confirm(prefs,off));
        assertEquals(3,prefs.getInt("graphics_schema",0));
        assertFalse(GraphicsPreferences.hasPending(prefs));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(off));
        prefs.edit().putBoolean(GraphicsPreferences.PENDING,true).commit();
        assertTrue(GraphicsPreferences.confirmed(prefs).same(off));
        assertTrue(GraphicsPreferences.clearAfterRestore(prefs));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(off));
    }
    @Test public void malformedGlassOrSnapshotBooleanCannotBecomePresentedOrPersistedIntent() {
        prefs.edit().putInt("graphics_schema",2).putInt("render_scale",92)
                .putString(GraphicsPreferences.GLASS,"Off").commit();
        assertTrue(GraphicsPreferences.confirmed(prefs).same(GraphicsPreferences.baseline()));
        long[] a=new long[24]; a[1]=7; a[7]=800; a[8]=600; a[9]=1200; a[10]=900; a[12]=2; a[13]=1;
        assertTrue(GraphicsPreferences.presented(a,7));
        a[20]=2; assertFalse(GraphicsPreferences.presented(a,7));
        a[20]=1; a[21]=-1; assertFalse(GraphicsPreferences.presented(a,7));
    }
    @Test public void benchmarkAdmissionPreservesBaselineAndOrdinaryStoredRecovery() {
        assertEquals(75, GraphicsPreferences.baseline().scale);
        for (int scale : new int[]{33,40}) {
            GraphicsPreferences.Values candidate=new GraphicsPreferences.Values(scale,1,0,30);
            assertTrue(candidate.valid(33)); assertFalse(candidate.valid(50));
            prefs.edit().putInt("render_scale",scale).putBoolean(GraphicsPreferences.PENDING,true)
                    .putInt("graphics_pending_scale",scale).commit();
            assertEquals(scale,GraphicsPreferences.confirmed(prefs,33).scale);
            assertEquals(75,GraphicsPreferences.confirmed(prefs,50).scale);
            assertEquals(scale,GraphicsPreferences.retainedCandidate(prefs).scale);
            assertTrue(GraphicsPreferences.hasPending(prefs));
        }
        for (int scale : new int[]{-1,0,32,34,39,41,49,101}) {
            assertFalse(GraphicsPreferences.validScale(scale,33));
            assertFalse(GraphicsPreferences.validScale(scale,50));
            assertFalse(GraphicsPreferences.markPending(prefs,new GraphicsPreferences.Values(scale,1,0,30)));
        }
        assertFalse(GraphicsPreferences.validScale(75,34));
    }
    @Test public void schemaTwoMigrationPreservesGlassAndIndependentPendingButDefaultsOnlyShadows() {
        prefs.edit().putInt("graphics_schema",2).putInt("render_scale",68).putInt("water_quality",0)
                .putInt(GraphicsPreferences.FIRE,1).putInt(GraphicsPreferences.CAP,42)
                .putBoolean(GraphicsPreferences.GLASS,false).putInt(GraphicsPreferences.SHADOW,2)
                .putBoolean(GraphicsPreferences.PENDING,true).putInt("graphics_pending_schema",2)
                .putInt("graphics_pending_scale",92).putInt("graphics_pending_water",2)
                .putInt("graphics_pending_fire",0).putInt("graphics_pending_cap",15)
                .putBoolean(GraphicsPreferences.PENDING_GLASS,false)
                .putString(GraphicsPreferences.PENDING_SHADOW,"stale-schema-three-field").commit();
        GraphicsPreferences.Values oldConfirmed=new GraphicsPreferences.Values(68,0,1,42,false,1);
        GraphicsPreferences.Values oldPending=new GraphicsPreferences.Values(92,2,0,15,false,1);
        assertTrue(GraphicsPreferences.confirmed(prefs).same(oldConfirmed));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(oldPending));
        assertTrue(GraphicsPreferences.hasPending(prefs));
        GraphicsPreferences.Values newest=new GraphicsPreferences.Values(63,1,2,30,false,0);
        assertTrue(GraphicsPreferences.markPending(prefs,newest));
        assertEquals(2,prefs.getInt("graphics_schema",0));
        assertEquals(3,prefs.getInt("graphics_pending_schema",0));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(oldConfirmed));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(newest));
        assertTrue(GraphicsPreferences.confirm(prefs,newest));
        // A retained older independently-versioned pending record does not inherit
        // the confirmed schema3 shadow value during interrupted recovery.
        prefs.edit().putBoolean(GraphicsPreferences.PENDING,true).putInt("graphics_pending_schema",2)
                .putInt("graphics_pending_fire",0).putInt(GraphicsPreferences.PENDING_SHADOW,2).commit();
        assertEquals(0,GraphicsPreferences.confirmed(prefs).shadow);
        assertEquals(1,GraphicsPreferences.retainedCandidate(prefs).shadow);
        assertTrue(GraphicsPreferences.hasPending(prefs));
    }
    @Test public void schemaThreeKeepsLowFireAndEveryShadowTierRejectsUnknownOrHistoricalEnums() {
        for(int shadow=0;shadow<=2;++shadow) {
            GraphicsPreferences.Values choice=new GraphicsPreferences.Values(75,2,2,42,false,shadow);
            assertTrue(GraphicsPreferences.markPending(prefs,choice));
            assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(choice));
            assertTrue(GraphicsPreferences.confirm(prefs,choice));
            assertTrue(GraphicsPreferences.confirmed(prefs).same(choice));
            assertFalse(GraphicsPreferences.hasPending(prefs));
        }
        for(int schema:new int[]{1,2}) {
            prefs.edit().putInt("graphics_schema",schema).putInt(GraphicsPreferences.FIRE,2)
                    .putInt("graphics_pending_schema",schema).putInt("graphics_pending_fire",2).commit();
            assertTrue(GraphicsPreferences.confirmed(prefs).same(GraphicsPreferences.baseline()));
            assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(GraphicsPreferences.baseline()));
        }
        for(int invalid:new int[]{-1,3,99}) {
            assertFalse(GraphicsPreferences.markPending(prefs,new GraphicsPreferences.Values(75,1,invalid,30,true,1)));
            assertFalse(GraphicsPreferences.confirm(prefs,new GraphicsPreferences.Values(75,1,0,30,true,invalid)));
            prefs.edit().putInt("graphics_schema",3).putInt(GraphicsPreferences.FIRE,0)
                    .putInt(GraphicsPreferences.SHADOW,invalid).putInt("graphics_pending_schema",3)
                    .putInt("graphics_pending_fire",0).putInt(GraphicsPreferences.PENDING_SHADOW,invalid).commit();
            assertTrue(GraphicsPreferences.confirmed(prefs).same(GraphicsPreferences.baseline()));
            assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(GraphicsPreferences.baseline()));
        }
        prefs.edit().putString(GraphicsPreferences.SHADOW,"Higher")
                .putString(GraphicsPreferences.PENDING_SHADOW,"Lower").commit();
        assertTrue(GraphicsPreferences.confirmed(prefs).same(GraphicsPreferences.baseline()));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(GraphicsPreferences.baseline()));
    }
    @Test public void failedConfirmationRetainsCompletePendingSixTupleAndSavedRecord() {
        GraphicsPreferences.Values saved=new GraphicsPreferences.Values(68,0,1,15,false,2);
        GraphicsPreferences.Values pending=new GraphicsPreferences.Values(100,2,2,60,true,0);
        assertTrue(GraphicsPreferences.confirm(prefs,saved));
        assertTrue(GraphicsPreferences.markPending(prefs,pending));
        prefs.edit().putInt("music_volume",43).putBoolean("rt_lab_unlocked",true).commit();
        SharedPreferences failing=(SharedPreferences)Proxy.newProxyInstance(SharedPreferences.class.getClassLoader(),
                new Class<?>[]{SharedPreferences.class},(proxy,method,args)->{
                    if(!method.getName().equals("edit")) return method.invoke(prefs,args);
                    SharedPreferences.Editor editor=prefs.edit();
                    return Proxy.newProxyInstance(SharedPreferences.Editor.class.getClassLoader(),
                            new Class<?>[]{SharedPreferences.Editor.class},(editorProxy,editMethod,editArgs)->{
                                if(editMethod.getName().equals("commit")) return false;
                                Object result=editMethod.invoke(editor,editArgs);
                                return result instanceof SharedPreferences.Editor?editorProxy:result;
                            });
                });
        assertFalse(GraphicsPreferences.confirm(failing,pending));
        assertTrue(GraphicsPreferences.hasPending(prefs));
        assertTrue(GraphicsPreferences.confirmed(prefs).same(saved));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(pending));
        assertTrue(GraphicsPreferences.clearAfterRestore(prefs));
        assertFalse(GraphicsPreferences.hasPending(prefs));
        assertTrue(GraphicsPreferences.retainedCandidate(prefs).same(pending));
        assertEquals(43,prefs.getInt("music_volume",0)); assertTrue(prefs.getBoolean("rt_lab_unlocked",false));
    }
}
