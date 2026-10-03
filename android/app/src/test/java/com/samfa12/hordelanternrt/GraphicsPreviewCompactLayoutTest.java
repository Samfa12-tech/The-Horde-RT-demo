package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.content.res.Configuration;
import android.app.AlertDialog;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.ScrollView;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class GraphicsPreviewCompactLayoutTest {
    private static Object field(MainActivity activity, String name) throws Exception {
        Field field = MainActivity.class.getDeclaredField(name); field.setAccessible(true); return field.get(activity);
    }
    private static void collect(View view, List<Button> buttons, List<HorizontalScrollView> rows) {
        if (view instanceof Button) buttons.add((Button)view);
        if (view instanceof HorizontalScrollView) rows.add((HorizontalScrollView)view);
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup)view;
            for(int i=0;i<group.getChildCount();++i) collect(group.getChildAt(i),buttons,rows);
        }
        assertFalse("timing graph belongs only in requested Details", view instanceof PreviewTimingGraphView);
    }
    private static void layout(View view, int width, int height) {
        view.measure(View.MeasureSpec.makeMeasureSpec(width,View.MeasureSpec.EXACTLY),
                View.MeasureSpec.makeMeasureSpec(height,View.MeasureSpec.EXACTLY));
        view.layout(0,0,width,height);
    }

    @Test public void productionPreviewDefaultLeavesImageUncoveredAndKeepsNativeAccessibleControls() throws Exception {
        for (float font : new float[]{1,1.3f,1.7f,2}) for (int[] size : new int[][]{{320,568},{360,640},{412,915}}) {
            MainActivity activity = Robolectric.buildActivity(MainActivity.class).get();
            Configuration original = new Configuration(activity.getResources().getConfiguration());
            Configuration config = new Configuration(original); config.fontScale=font; config.densityDpi=160;
            activity.getResources().updateConfiguration(config,activity.getResources().getDisplayMetrics());
            try {
                FrameLayout scrim = new FrameLayout(activity); layout(scrim,size[0],size[1]);
                Field scrimField=MainActivity.class.getDeclaredField("menuScrim");
                scrimField.setAccessible(true); scrimField.set(activity,scrim);
                Method show=MainActivity.class.getDeclaredMethod("showGraphicsPreviewPage");
                show.setAccessible(true); show.invoke(activity);
                TextView status=(TextView)field(activity,"graphicsTelemetry");
                status.setText("Resolution / After: RT ready\nView: Materials");
                layout(scrim,size[0],size[1]);
                assertEquals(1,scrim.getChildCount());
                View strip=scrim.getChildAt(0); FrameLayout.LayoutParams lp=(FrameLayout.LayoutParams)strip.getLayoutParams();
                assertTrue("font " + font + " viewport " + size[1],strip.getHeight()+lp.bottomMargin <= size[1]*.35);
                assertTrue(strip.getTop() >= size[1]*.65);
                List<Button> buttons=new ArrayList<>(); List<HorizontalScrollView> rows=new ArrayList<>();
                collect(strip,buttons,rows);
                assertEquals(2,rows.size()); assertEquals(7,buttons.size());
                String[] labels={"Before","After","Back","View","Details","Apply","Revert"};
                int minimum=HordeUiTokens.dp(activity,48);
                for(int i=0;i<buttons.size();++i) {
                    Button button=buttons.get(i); assertEquals(labels[i],button.getText().toString());
                    assertTrue(button.getMeasuredWidth()>=minimum); assertTrue(button.getMeasuredHeight()>=minimum);
                    assertTrue(button.isFocusable()); assertEquals(0,button.getAutoSizeTextType());
                    assertEquals(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP,15,
                            activity.getResources().getDisplayMetrics()),button.getTextSize(),.5);
                }
                assertEquals(2,status.getMaxLines());
                assertNull(field(activity,"graphicsDetailsDialog")); assertNull(field(activity,"graphicsGraph"));
                assertFalse(((Button)field(activity,"graphicsConfirm")).isEnabled());
            } finally { activity.getResources().updateConfiguration(original,activity.getResources().getDisplayMetrics()); }
        }
    }

    @Test public void heightBudgetIncludesSafeAreaAndOversizeTextUsesNativeVerticalScrolling() {
        for(float font:new float[]{1,1.7f,2}) {
            Configuration config=new Configuration(RuntimeEnvironment.getApplication().getResources().getConfiguration());
            config.fontScale=font; config.densityDpi=160;
            Context context=RuntimeEnvironment.getApplication().createConfigurationContext(config);
            int height=568,gap=48;
            ScrollView strip=new GraphicsPreviewControlsScrollView(context,GraphicsPreviewComparison.maximumOverlayHeight(height,gap));
            TextView text=new TextView(context); text.setTextSize(24);
            text.setText("Before\nAfter\nView\nDetails\nApply\nKeep\nRevert\nBack"); strip.addView(text);
            strip.measure(View.MeasureSpec.makeMeasureSpec(320,View.MeasureSpec.EXACTLY),
                    View.MeasureSpec.makeMeasureSpec(height,View.MeasureSpec.AT_MOST));
            strip.layout(0,0,strip.getMeasuredWidth(),strip.getMeasuredHeight());
            assertTrue(strip.getHeight()+gap<=height*.35);
            assertTrue(text.getHeight()>strip.getHeight());
            assertEquals(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP,24,
                    context.getResources().getDisplayMetrics()),text.getTextSize(),.5);
        }
    }

    @Test public void detailsIsCreatedOnlyOnRequestAndDismissReleasesGraphAndDialogReferences() throws Exception {
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        FrameLayout scrim=new FrameLayout(activity); layout(scrim,360,640);
        Field scrimField=MainActivity.class.getDeclaredField("menuScrim");
        scrimField.setAccessible(true); scrimField.set(activity,scrim);
        Method show=MainActivity.class.getDeclaredMethod("showGraphicsPreviewPage");
        show.setAccessible(true); show.invoke(activity);
        assertNull(field(activity,"graphicsDetailsDialog")); assertNull(field(activity,"graphicsGraph"));
        ((Button)field(activity,"graphicsDetailsButton")).performClick();
        AlertDialog dialog=(AlertDialog)field(activity,"graphicsDetailsDialog");
        assertNotNull(dialog); assertTrue(dialog.isShowing());
        assertNotNull(field(activity,"graphicsGraph")); assertNotNull(field(activity,"graphicsDetailsTelemetry"));
        Method dismiss=MainActivity.class.getDeclaredMethod("dismissGraphicsPreviewDetails");
        dismiss.setAccessible(true); dismiss.invoke(activity);
        assertFalse(dialog.isShowing()); assertNull(field(activity,"graphicsDetailsDialog"));
        assertNull(field(activity,"graphicsDetailsTelemetry")); assertNull(field(activity,"graphicsDetailsPanel"));
        assertNull(field(activity,"graphicsGraph"));
    }
}
