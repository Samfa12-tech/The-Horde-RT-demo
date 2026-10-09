package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import android.view.LayoutInflater;
import android.view.View;
import android.graphics.drawable.ColorDrawable;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class GraphicsMenuLifecycleTest {
    @Test public void previewLeavesNativeSceneUndimmedAndRegularPanelRestoresScrim() throws Exception {
        MainActivity activity = Robolectric.buildActivity(MainActivity.class).get();
        FrameLayout scrim = new FrameLayout(activity);
        Field field = MainActivity.class.getDeclaredField("menuScrim");
        field.setAccessible(true); field.set(activity, scrim);
        Method preview = MainActivity.class.getDeclaredMethod("showGraphicsPreviewPage");
        preview.setAccessible(true); preview.invoke(activity);
        assertEquals(0, ((ColorDrawable)scrim.getBackground()).getColor());
        Method regular = MainActivity.class.getDeclaredMethod("attachPanel", LinearLayout.class);
        regular.setAccessible(true); regular.invoke(activity, new LinearLayout(activity));
        assertEquals(0xC7080706, ((ColorDrawable)scrim.getBackground()).getColor());
    }
    @Test public void restoringSceneCannotResumeGameplayThroughHideMenu() throws Exception {
        // Deliberately no onCreate/native renderer. Guard precedes input/audio calls.
        MainActivity activity = Robolectric.buildActivity(MainActivity.class).get();
        Field restoring = MainActivity.class.getDeclaredField("graphicsSceneRestoring");
        restoring.setAccessible(true); restoring.setBoolean(activity, true);
        Field visible = MainActivity.class.getDeclaredField("menuVisible");
        visible.setAccessible(true); visible.setBoolean(activity, true);
        Method hide = MainActivity.class.getDeclaredMethod("hideMenu");
        hide.setAccessible(true); hide.invoke(activity);
        assertTrue(visible.getBoolean(activity));
    }
    @Test public void revealTitleIsHiddenAndDoesNotConsumeGameplayTouchesByDefault() {
        View root = LayoutInflater.from(RuntimeEnvironment.getApplication()).inflate(R.layout.activity_main, null);
        View title = root.findViewById(R.id.keeper_reveal_title);
        assertNotNull(title); assertEquals(View.GONE, title.getVisibility());
        assertFalse(title.isClickable()); assertFalse(title.isFocusable());
    }
}
