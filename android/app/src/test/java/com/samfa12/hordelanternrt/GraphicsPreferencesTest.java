package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.content.SharedPreferences;
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
        long[] a = new long[20];
        a[1]=7; a[7]=800; a[8]=600; a[9]=1200; a[10]=900; a[12]=1; a[13]=1;
        assertTrue(GraphicsPreferences.presented(a, 7));
        assertFalse(GraphicsPreferences.presented(a, 8));
        a[13]=0; assertFalse(GraphicsPreferences.presented(a, 7));
        a[13]=1; a[7]=0; assertFalse(GraphicsPreferences.presented(a, 7));
        assertFalse(GraphicsPreferences.presented(new long[19], 7));
        assertFalse(GraphicsPreferences.presented(null, 7));
    }
}
