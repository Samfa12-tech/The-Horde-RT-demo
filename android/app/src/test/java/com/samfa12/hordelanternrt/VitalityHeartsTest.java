package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class VitalityHeartsTest {
    private static final int FULL_RED = 0xFFD64747;
    private Bitmap render(Context c, int health, int maximum, int width) {
        VitalityHeartsDrawable d = new VitalityHeartsDrawable(c, health, maximum, width);
        Rect bounds = d.getBounds();
        Bitmap b = Bitmap.createBitmap(bounds.width(), bounds.height(), Bitmap.Config.ARGB_8888);
        d.draw(new Canvas(b));
        return b;
    }
    @Test public void eachHealthPointFillsOneHeartAndLostPointsHaveTransparentInteriors() {
        Context c = RuntimeEnvironment.getApplication();
        int size = HordeUiTokens.dp(c, 22), step = size + HordeUiTokens.dp(c, 4);
        for (int health = 0; health <= 3; ++health) {
            Bitmap b = render(c, health, 3, HordeUiTokens.dp(c, 200));
            for (int i = 0; i < 3; ++i) {
                int x = i * step + size / 2, y = size / 2;
                assertEquals("full red or transparent empty interior", i < health ? FULL_RED : 0, b.getPixel(x, y));
                boolean outline = false;
                for (int py = 0; py < size; ++py) for (int px = i * step; px < i * step + size; ++px)
                    outline |= b.getPixel(px, py) == HordeUiTokens.PARCHMENT;
                assertTrue("every heart must keep a visible outline", outline);
            }
        }
    }
    @Test public void largerMaximumWrapsWithoutShrinkingOrDroppingHearts() {
        Context c = RuntimeEnvironment.getApplication();
        int size = HordeUiTokens.dp(c, 22), step = size + HordeUiTokens.dp(c, 4);
        int available = 3 * step - HordeUiTokens.dp(c, 4);
        Bitmap b = render(c, 4, 7, available);
        assertTrue(b.getWidth() <= available);
        assertTrue("future hearts wrap to additional rows", b.getHeight() > size * 2);
        for (int i = 0; i < 7; ++i)
            assertEquals(i < 4 ? FULL_RED : 0, b.getPixel((i % 3) * step + size / 2, (i / 3) * step + size / 2));
        assertEquals("unused cells are transparent", 0, b.getPixel(step + size / 2, 2 * step + size / 2));
    }
    @Test public void heartOnlyHudRetainsAccessibleCurrentAndChangingMaximum() throws Exception {
        MainActivity a = Robolectric.buildActivity(MainActivity.class).get();
        a.setContentView(R.layout.activity_main);
        TextView hud = a.findViewById(R.id.vitality_status);
        Field field = MainActivity.class.getDeclaredField("vitalityStatus");
        field.setAccessible(true); field.set(a, hud);
        Method update = MainActivity.class.getDeclaredMethod("updateVitalityHud", int.class, int.class);
        update.setAccessible(true);
        update.invoke(a, 3, 3);
        int originalWidth = hud.getCompoundDrawables()[0].getBounds().width();
        update.invoke(a, 3, 5);
        assertEquals("", hud.getText().toString());
        assertEquals("Vitality 3 of 5", hud.getContentDescription().toString());
        assertTrue(hud.getCompoundDrawables()[0].getBounds().width() > originalWidth);
        update.invoke(a, 99, 5);
        assertEquals("Vitality 5 of 5", hud.getContentDescription().toString());
        update.invoke(a, -1, 5);
        assertEquals("Vitality 0 of 5", hud.getContentDescription().toString());
    }
}
