package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;

import java.util.HashSet;
import java.util.Set;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class ContextualControlsLayoutTest {
    private static Rect bounds(final View view) {
        return new Rect(view.getLeft(), view.getTop(), view.getRight(), view.getBottom());
    }

    private static void requireRenderedVariation(final Bitmap bitmap, final Rect bounds) {
        final Set<Integer> colours = new HashSet<>();
        for (int y = bounds.top + 2; y < bounds.bottom - 2; y += 2) {
            for (int x = bounds.left + 2; x < bounds.right - 2; x += 2) {
                colours.add(bitmap.getPixel(x, y));
            }
        }
        assertTrue("button must produce rendered background/text pixels; colours=" +
                           colours.size(), colours.size() >= 4);
    }

    @Test
    public void debugCheckpointMappingIncludesEveryNewOwnerReviewCapture() {
        final String[] names = {
                "pbr-sword-closeup", "pbr-torch-fire", "player-body-grips",
                "lantern-chest-unlock", "lantern-held-high", "lantern-held-low",
                "lantern-glass-transmission", "lantern-motion-extreme"
        };
        final int[] ids = {100, 101, 102, 114, 116, 117, 118, 119};
        for (int index = 0; index < names.length; ++index) {
            assertEquals("debug capture name must reach its native checkpoint",
                         ids[index], MainActivity.checkpointId(names[index]));
        }
        assertEquals("unknown automation names must remain rejected", -1,
                     MainActivity.checkpointId("not-a-checkpoint"));
    }

    @Test
    public void updaterComparesDebugCandidatesUsingThePublishedBaseVersion() {
        assertEquals("1.5.2", MainActivity.installedUpdateVersion("1.5.2-debug"));
        assertEquals("1.5.2-alpha.1", MainActivity.installedUpdateVersion("1.5.2-alpha.1"));
        assertEquals("", MainActivity.installedUpdateVersion(null));
        assertEquals("CHECK FOR UPDATES",
                     RuntimeEnvironment.getApplication().getString(R.string.check_for_updates));
        assertEquals("the release check requires only ordinary network access",
                     PackageManager.PERMISSION_GRANTED,
                     RuntimeEnvironment.getApplication().getPackageManager().checkPermission(
                             android.Manifest.permission.INTERNET,
                             RuntimeEnvironment.getApplication().getPackageName()));
    }

    @Test
    public void maximumFontScaleContextualControlsRenderWithoutClippingOrOverlap() {
        final Configuration maximumFont = new Configuration(
                RuntimeEnvironment.getApplication().getResources().getConfiguration());
        maximumFont.fontScale = 2.0f;
        maximumFont.densityDpi = DisplayMetrics.DENSITY_MEDIUM;
        final Context context = RuntimeEnvironment.getApplication()
                .createConfigurationContext(maximumFont);
        final FrameLayout root = (FrameLayout) LayoutInflater.from(context)
                .inflate(R.layout.activity_main, null, false);

        root.findViewById(R.id.menu_scrim).setVisibility(View.GONE);
        root.findViewById(R.id.diagnostics_panel).setVisibility(View.GONE);
        root.findViewById(R.id.developer_overlay).setVisibility(View.GONE);
        final Button interact = root.findViewById(R.id.interact_button);
        final Button toggle = root.findViewById(R.id.toggle_held_light_pose_button);
        final Button attack = root.findViewById(R.id.attack_button);
        final Button parry = root.findViewById(R.id.parry_button);
        interact.setVisibility(View.VISIBLE);
        interact.setText(R.string.chest_locked_until_lich_defeated);
        toggle.setVisibility(View.VISIBLE);
        attack.setVisibility(View.VISIBLE);
        parry.setVisibility(View.VISIBLE);
        toggle.setText(R.string.lower_lantern);

        final int width = 320;
        final int height = 568;
        root.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                     View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
        root.layout(0, 0, width, height);

        final Button[] buttons = {interact, toggle, attack, parry};
        for (final Button button : buttons) {
            assertNotNull("real text layout must be produced", button.getLayout());
            final int expectedLines = button == interact ? 2 : 1;
            assertEquals("action text must use only its authored line count", expectedLines,
                         button.getLayout().getLineCount());
            for (int line = 0; line < button.getLayout().getLineCount(); ++line) {
                assertEquals("action text must not be ellipsized", 0,
                             button.getLayout().getEllipsisCount(line));
            }
            assertTrue("text must fit the rendered vertical content box",
                       button.getLayout().getHeight() <=
                               button.getHeight() - button.getPaddingTop() -
                                       button.getPaddingBottom());
            final Rect buttonBounds = bounds(button);
            assertTrue("button must remain inside the compact viewport",
                       buttonBounds.left >= 0 && buttonBounds.top >= 0 &&
                               buttonBounds.right <= width &&
                               buttonBounds.bottom <= height);
        }
        assertFalse("contextual controls must not overlap each other",
                    Rect.intersects(bounds(interact), bounds(toggle)));
        assertFalse("INTERACT must not overload or overlap SWING",
                    Rect.intersects(bounds(interact), bounds(attack)));
        assertFalse("RAISE/LOWER must not overload or overlap PARRY",
                    Rect.intersects(bounds(toggle), bounds(parry)));

        final Bitmap rendered = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        root.draw(new Canvas(rendered));
        for (final Button button : buttons) {
            requireRenderedVariation(rendered, bounds(button));
        }
    }
}
