package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;

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
public final class RenderScalePreferenceTest {
    private SharedPreferences preferences;

    @Before
    public void createIsolatedPreferences() {
        preferences = RuntimeEnvironment.getApplication().getSharedPreferences(
                "render_scale_preference_test", Context.MODE_PRIVATE);
        preferences.edit().clear().commit();
    }

    @Test
    public void unsetPreferenceUsesTheMobileDefault() {
        assertEquals(75, MainActivity.DEFAULT_ANDROID_RT_RENDER_SCALE_PERCENT);
        assertEquals(75, MainActivity.renderScalePercent(preferences));
    }

    @Test
    public void explicitSavedPreferenceIsPreserved() {
        preferences.edit().putInt(MainActivity.PREF_RENDER_SCALE, 92).commit();

        assertEquals(92, MainActivity.renderScalePercent(preferences));
    }

    @Test
    public void runtimeSliderSelectionPersistsForTheNextRead() {
        MainActivity.persistRenderScaleSelection(preferences, 68);

        assertEquals(68, preferences.getInt(MainActivity.PREF_RENDER_SCALE, -1));
        assertEquals(68, MainActivity.renderScalePercent(preferences));
    }
}
