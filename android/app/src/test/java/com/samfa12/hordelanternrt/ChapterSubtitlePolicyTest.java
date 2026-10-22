package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.graphics.Rect;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Button;
import android.widget.TextView;
import android.widget.ScrollView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class ChapterSubtitlePolicyTest {
    private MainActivity activity;
    private FrameLayout root;

    private static Field field(String name) throws Exception {
        Field value = MainActivity.class.getDeclaredField(name);
        value.setAccessible(true);
        return value;
    }

    private Object get(String name) throws Exception { return field(name).get(activity); }
    private void set(String name, Object value) throws Exception { field(name).set(activity, value); }

    private Object call(String name, Class<?>[] types, Object... args) throws Exception {
        Method method = MainActivity.class.getDeclaredMethod(name, types);
        method.setAccessible(true);
        return method.invoke(activity, args);
    }

    @Before public void setUp() throws Exception {
        activity = Robolectric.buildActivity(MainActivity.class).get();
        root = new FrameLayout(activity);
        activity.setContentView(root);
        set("appRoot", root);
        set("preferences", activity.getSharedPreferences("subtitle-policy", Context.MODE_PRIVATE));
        activity.getSharedPreferences("subtitle-policy", Context.MODE_PRIVATE).edit().clear().apply();
        call("createChapterDialogueOverlay", new Class<?>[]{});
    }

    private void layout(int width, int height) throws Exception {
        root.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
        root.layout(0, 0, width, height);
    }

    private int reflow(int width, int height, int sizePercent, int preference,
            int inputMode, long generation, boolean bottomControl) throws Exception {
        layout(width, height);
        TextView text = (TextView) get("chapterSubtitleText");
        text.setText("The keeper waits beyond the lantern light.");
        text.setTextSize(20.0f * sizePercent / 100.0f);
        ((LinearLayout) get("chapterSubtitlePanel")).setVisibility(View.VISIBLE);
        activity.getSharedPreferences("subtitle-policy", Context.MODE_PRIVATE).edit()
                .putInt("chapter_subtitle_position", preference).apply();
        MainActivity.ChapterSubtitlePlacement placement =
                (MainActivity.ChapterSubtitlePlacement) get("chapterSubtitlePlacement");
        placement.beginLine(generation, inputMode);
        if (bottomControl) {
            View control = new Button(activity);
            FrameLayout.LayoutParams controlParams = new FrameLayout.LayoutParams(160, 140,
                    Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
            root.addView(control, controlParams);
            set("attackButton", control);
            layout(width, height);
            assertNotNull("the visible lower control hit region is measured",
                    call("visibleRectInRoot", new Class<?>[]{View.class}, control));
        } else set("attackButton", null);
        call("reflowChapterDialoguePlacement", new Class<?>[]{});
        FrameLayout.LayoutParams params =
                (FrameLayout.LayoutParams) ((View) get("chapterSubtitlePanel")).getLayoutParams();
        return params.gravity;
    }

    @Test public void automaticPlacementFollowsInputAndMeasuredLandscapeControlRoom() throws Exception {
        assertEquals(Gravity.TOP, reflow(400, 800, 100, MainActivity.SUBTITLE_POSITION_AUTO,
                MainActivity.SUBTITLE_INPUT_TOUCH, 1, false));
        assertEquals(Gravity.BOTTOM, reflow(800, 400, 100, MainActivity.SUBTITLE_POSITION_AUTO,
                MainActivity.SUBTITLE_INPUT_TOUCH, 2, false));
        // A visible lower hit region is included in the measured room before choosing bottom.
        assertEquals(Gravity.TOP, reflow(800, 400, 100, MainActivity.SUBTITLE_POSITION_AUTO,
                MainActivity.SUBTITLE_INPUT_TOUCH, 3, true));
        assertEquals(Gravity.BOTTOM, reflow(400, 800, 100, MainActivity.SUBTITLE_POSITION_AUTO,
                MainActivity.SUBTITLE_INPUT_KEYBOARD_MOUSE_CONTROLLER, 4, false));
    }

    @Test public void largeTextAndExplicitChoicesUseActualMeasuredLayout() throws Exception {
        assertEquals(Gravity.BOTTOM, reflow(400, 800, 160, MainActivity.SUBTITLE_POSITION_BOTTOM,
                MainActivity.SUBTITLE_INPUT_TOUCH, 5, false));
        assertEquals(Gravity.TOP, reflow(800, 400, 160, MainActivity.SUBTITLE_POSITION_TOP,
                MainActivity.SUBTITLE_INPUT_KEYBOARD_MOUSE_CONTROLLER, 6, false));
        TextView subtitle = (TextView) get("chapterSubtitleText");
        assertTrue(subtitle.getTextSize() > 20.0f);
        assertTrue("skip line retains the 44dp minimum touch target",
                ((View) get("chapterDialogueSkipButton")).getMinimumHeight() >= activity.getResources()
                        .getDisplayMetrics().density * 44.0f);
    }

    @Test public void realInputSourcesAreLatchedOnlyAtTheNextLineGeneration() throws Exception {
        MainActivity.ChapterSubtitlePlacement placement =
                (MainActivity.ChapterSubtitlePlacement) get("chapterSubtitlePlacement");
        KeyEvent keyboard = new KeyEvent(1, 2, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_A,
                0, 0, 9, 0, 0, InputDevice.SOURCE_KEYBOARD);
        activity.dispatchKeyEvent(keyboard);
        placement.beginLine(10, (int) get("latestSubtitleInputMode"));
        assertEquals(MainActivity.SUBTITLE_INPUT_KEYBOARD_MOUSE_CONTROLLER,
                placement.inputModeForCurrentLine());

        MotionEvent mouse = MotionEvent.obtain(1, 2, MotionEvent.ACTION_MOVE, 20, 20, 0);
        mouse.setSource(InputDevice.SOURCE_MOUSE);
        activity.dispatchTouchEvent(mouse);
        mouse.recycle();
        MotionEvent touch = MotionEvent.obtain(2, 3, MotionEvent.ACTION_DOWN, 20, 20, 0);
        touch.setSource(InputDevice.SOURCE_TOUCHSCREEN);
        activity.dispatchTouchEvent(touch);
        touch.recycle();
        assertEquals("a physical touch is observed for the next line",
                MainActivity.SUBTITLE_INPUT_TOUCH, (int) get("latestSubtitleInputMode"));
        assertEquals("same line keeps its original placement input", 10L, placement.generation());
        assertEquals(MainActivity.SUBTITLE_INPUT_KEYBOARD_MOUSE_CONTROLLER,
                placement.inputModeForCurrentLine());

        placement.beginLine(11, (int) get("latestSubtitleInputMode"));
        assertEquals(MainActivity.SUBTITLE_INPUT_TOUCH, placement.inputModeForCurrentLine());
        assertEquals(11L, placement.generation());
    }

    @Test public void rotationReflowsSameLineAndOnlyChangesItsPositionForNewOrientation() throws Exception {
        MainActivity.ChapterSubtitlePlacement placement = new MainActivity.ChapterSubtitlePlacement();
        placement.beginLine(77, MainActivity.SUBTITLE_INPUT_TOUCH);
        assertEquals(MainActivity.SUBTITLE_POSITION_TOP,
                placement.positionFor(MainActivity.SUBTITLE_POSITION_AUTO, false, 500, 400, 100));
        assertEquals(MainActivity.SUBTITLE_POSITION_BOTTOM,
                placement.positionFor(MainActivity.SUBTITLE_POSITION_AUTO, true, 250, 250, 100));
        assertEquals(MainActivity.SUBTITLE_POSITION_BOTTOM,
                placement.positionFor(MainActivity.SUBTITLE_POSITION_AUTO, true, 250, 250, 300));
        assertEquals(77L, placement.generation());
    }

    @Test public void panelUsesCompactTextTreatmentAndKeepsAccessibleFullText() throws Exception {
        TextView subtitle = (TextView) get("chapterSubtitleText");
        LinearLayout panel = (LinearLayout) get("chapterSubtitlePanel");
        assertEquals(null, panel.getBackground());
        activity.getSharedPreferences("subtitle-policy", Context.MODE_PRIVATE).edit()
                .putBoolean("chapter_subtitle_backing", true).apply();
        call("applyChapterSubtitleBacking", new Class<?>[]{});
        assertNotNull("the optional accessibility backing is available", panel.getBackground());
        assertNotNull(subtitle.getTypeface());
        assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_YES, subtitle.getImportantForAccessibility());
        assertEquals(View.ACCESSIBILITY_LIVE_REGION_POLITE, subtitle.getAccessibilityLiveRegion());
        subtitle.setText("An entire line remains available to screen readers.");
        subtitle.setContentDescription(subtitle.getText());
        assertEquals(subtitle.getText().toString(), subtitle.getContentDescription().toString());
    }

    @Test public void landscapeCaptionUsesTheMeasuredOpenLaneAtLargeSystemFont() throws Exception {
        layout(800, 360);
        View left = new Button(activity), right = new Button(activity);
        FrameLayout.LayoutParams leftParams = new FrameLayout.LayoutParams(120, 150,
                Gravity.LEFT | Gravity.BOTTOM);
        FrameLayout.LayoutParams rightParams = new FrameLayout.LayoutParams(220, 150,
                Gravity.RIGHT | Gravity.BOTTOM);
        root.addView(left, leftParams);root.addView(right, rightParams);
        set("runButton", left);set("attackButton", right);
        View hearts = new TextView(activity), menu = new Button(activity);
        root.addView(hearts, new FrameLayout.LayoutParams(120, 70, Gravity.LEFT | Gravity.TOP));
        root.addView(menu, new FrameLayout.LayoutParams(120, 70, Gravity.RIGHT | Gravity.TOP));
        set("vitalityStatus", hearts);set("menuButton", menu);
        TextView subtitle = (TextView) get("chapterSubtitleText");
        subtitle.setText("Kit\nA road to the treasury, perhaps. Someone in the village might read it.");
        // 100% subtitle size with the owner's 1.7 system font multiplier.
        subtitle.setTextSize(android.util.TypedValue.COMPLEX_UNIT_PX, 34);
        subtitle.setContentDescription(subtitle.getText());
        ((View) get("chapterSubtitlePanel")).setVisibility(View.VISIBLE);
        ((MainActivity.ChapterSubtitlePlacement) get("chapterSubtitlePlacement"))
                .beginLine(80, MainActivity.SUBTITLE_INPUT_TOUCH);
        layout(800, 360);
        call("reflowChapterDialoguePlacement", new Class<?>[]{});
        layout(800, 360);
        ScrollView scroll = (ScrollView) get("chapterSubtitleScroll");
        assertEquals(View.VISIBLE, subtitle.getVisibility());
        assertTrue("the entire actual Kit line fits without scrolling at this normal landscape size",
                subtitle.getMeasuredHeight() <= scroll.getHeight());
        Rect panel = (Rect) call("visibleRectInRoot", new Class<?>[]{View.class},
                (View) get("chapterSubtitlePanel"));
        assertNotNull(panel);
        assertFalse("caption avoids the left touch hit region", Rect.intersects(panel,
                (Rect) call("visibleRectInRoot", new Class<?>[]{View.class}, left)));
        assertFalse("caption avoids the right touch hit region", Rect.intersects(panel,
                (Rect) call("visibleRectInRoot", new Class<?>[]{View.class}, right)));
        assertTrue("full caption stays available to accessibility",
                subtitle.getContentDescription().toString().contains("treasury"));
    }

    @Test public void extremeCaptionKeepsTextAndSkipReachableInsideOneScrollOwner() throws Exception {
        layout(320, 180);
        TextView subtitle = (TextView) get("chapterSubtitleText");
        subtitle.setText("Kit\nA road to the treasury, perhaps. Someone in the village might read it.");
        subtitle.setTextSize(54);
        ((View) get("chapterSubtitlePanel")).setVisibility(View.VISIBLE);
        ((MainActivity.ChapterSubtitlePlacement) get("chapterSubtitlePlacement"))
                .beginLine(81, MainActivity.SUBTITLE_INPUT_TOUCH);
        call("reflowChapterDialoguePlacement", new Class<?>[]{});
        layout(320, 180);
        ScrollView scroll = (ScrollView) get("chapterSubtitleScroll");
        View skip = (View) get("chapterDialogueSkipButton");
        assertEquals(View.VISIBLE, subtitle.getVisibility());
        assertEquals(View.VISIBLE, scroll.getVisibility());
        assertEquals(View.VISIBLE, skip.getVisibility());
        assertTrue("a caption viewport remains available", scroll.getHeight() > 0);
        assertTrue("all text and the skip touch target can be reached by scrolling",
                scroll.getChildAt(0) instanceof LinearLayout &&
                skip.getParent() == scroll.getChildAt(0));
        assertTrue(skip.getMinimumHeight() >= activity.getResources().getDisplayMetrics().density * 44);
    }

    @Test public void explicitPositionAndAccessibilityBoundsStayStable() {
        assertEquals(MainActivity.SUBTITLE_POSITION_TOP,
                MainActivity.resolveChapterSubtitlePosition(MainActivity.SUBTITLE_POSITION_TOP,
                        MainActivity.SUBTITLE_INPUT_TOUCH, true, 10, 200, 500));
        assertEquals(MainActivity.SUBTITLE_POSITION_BOTTOM,
                MainActivity.resolveChapterSubtitlePosition(MainActivity.SUBTITLE_POSITION_BOTTOM,
                        MainActivity.SUBTITLE_INPUT_KEYBOARD_MOUSE_CONTROLLER, false, 200, 10, 500));
        assertEquals(80, MainActivity.clampChapterSubtitleSize(0));
        assertEquals(121, MainActivity.clampChapterSubtitleSize(121));
        assertEquals(160, MainActivity.clampChapterSubtitleSize(500));
    }
}
