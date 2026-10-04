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

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
public final class InterfacePreferencesTest {
    private SharedPreferences p;
    @Before public void setUp() { p=RuntimeEnvironment.getApplication().getSharedPreferences("interface-test",Context.MODE_PRIVATE); p.edit().clear().commit(); }
    @Test public void missingUiKeysPreserveExistingGameplayAndSavedSettings() {
        p.edit().putInt("render_scale",85).putInt("water_quality",2).putInt("music_volume",40)
                .putBoolean("rt_lab_unlocked",true).putBoolean("report_consent",true).commit();
        java.util.Map<String,?> original=p.getAll();
        InterfacePreferences.Values v=InterfacePreferences.read(p);
        assertFalse(v.compact); assertEquals(100,v.scale); assertEquals(70,v.opacity);
        assertFalse(v.routineStatus); assertFalse(v.strongerBacking); assertEquals(original,p.getAll());
    }
    @Test public void corruptOrOutOfRangePresentationRecoversUsableDefaults() {
        p.edit().putString(InterfacePreferences.SCALE,"large").putInt(InterfacePreferences.OPACITY,999).commit();
        InterfacePreferences.Values v=InterfacePreferences.read(p);
        assertEquals(100,v.scale); assertEquals(70,v.opacity); assertTrue(v.valid());
        assertFalse(InterfacePreferences.save(p,new InterfacePreferences.Values(true,84,70,false,true)));
    }
    @Test public void interfaceResetIsScopedAndDoesNotResetGraphicsAudioConsentOrLab() {
        p.edit().putInt("render_scale",90).putInt("water_quality",0).putInt("music_volume",20)
                .putBoolean("rt_lab_unlocked",true).putBoolean("report_consent",true).putBoolean("show_hud",false).commit();
        assertTrue(InterfacePreferences.save(p,new InterfacePreferences.Values(true,110,55,true,true)));
        assertTrue(InterfacePreferences.reset(p));
        assertEquals(90,p.getInt("render_scale",0)); assertEquals(0,p.getInt("water_quality",-1));
        assertEquals(20,p.getInt("music_volume",0)); assertTrue(p.getBoolean("rt_lab_unlocked",false));
        assertTrue(p.getBoolean("report_consent",false)); assertTrue(p.getBoolean("show_hud",false));
        assertEquals(100,InterfacePreferences.read(p).scale); assertFalse(InterfacePreferences.read(p).compact);
    }
    @Test public void liveScaleAndBackingEditsPersistIndependentlyOfRendererState() {
        InterfacePreferences.saveLive(p,new InterfacePreferences.Values(true,85,95,true,false));
        InterfacePreferences.Values v=InterfacePreferences.read(p);
        assertTrue(v.compact); assertEquals(85,v.scale); assertEquals(95,v.opacity); assertTrue(v.strongerBacking);
        assertFalse(p.contains("graphics_pending")); assertFalse(p.contains("render_scale"));
    }
}
