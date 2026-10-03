package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.content.res.Configuration;
import android.app.AlertDialog;
import android.graphics.drawable.ColorDrawable;
import android.util.TypedValue;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.ViewGroup;
import android.view.MotionEvent;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.PopupMenu;
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
    private static void setField(MainActivity activity, String name, Object value) throws Exception {
        Field field = MainActivity.class.getDeclaredField(name); field.setAccessible(true); field.set(activity,value);
    }
    private static final class TransitionButton extends Button {
        int enabledTransitions;
        TransitionButton(Context context) { super(context); }
        @Override public void setEnabled(boolean enabled) {
            if (isEnabled() != enabled) ++enabledTransitions;
            super.setEnabled(enabled);
        }
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
                Field draft=MainActivity.class.getDeclaredField("graphicsDraft");
                draft.setAccessible(true); draft.set(activity,GraphicsPreferences.baseline());
                Method show=MainActivity.class.getDeclaredMethod("showGraphicsPreviewPage");
                show.setAccessible(true); show.invoke(activity);
                TextView status=(TextView)field(activity,"graphicsTelemetry");
                status.setText("Preview FPS: 13.0\nWater · RT ready");
                final int[] changes={0};
                status.addTextChangedListener(new TextWatcher() {
                    @Override public void beforeTextChanged(CharSequence s,int start,int count,int after) { }
                    @Override public void onTextChanged(CharSequence s,int start,int before,int count) { ++changes[0]; }
                    @Override public void afterTextChanged(Editable s) { }
                });
                Method update=MainActivity.class.getDeclaredMethod("setGraphicsPreviewStatus",String.class);
                update.setAccessible(true);
                update.invoke(activity,"Preview FPS: 14.0\nWater · RT ready");
                update.invoke(activity,"Preview FPS: 14.0\nWater · RT ready");
                assertEquals("unchanged status must not emit repeated text/accessibility events",1,changes[0]);
                layout(scrim,size[0],size[1]);
                assertEquals(1,scrim.getChildCount());
                View strip=scrim.getChildAt(0); FrameLayout.LayoutParams lp=(FrameLayout.LayoutParams)strip.getLayoutParams();
                assertTrue("font " + font + " viewport " + size[1],strip.getHeight()+lp.bottomMargin <= size[1]*.35);
                assertTrue(strip.getTop() >= size[1]*.65);
                List<Button> buttons=new ArrayList<>(); List<HorizontalScrollView> rows=new ArrayList<>();
                collect(strip,buttons,rows);
                assertEquals(2,rows.size()); assertEquals(11,buttons.size());
                for(HorizontalScrollView row:rows) {
                    assertTrue(row.isHorizontalScrollBarEnabled()); assertFalse(row.isVerticalScrollBarEnabled());
                    assertFalse("scroll affordance must stay visible",row.isScrollbarFadingEnabled());
                    assertEquals(View.SCROLLBARS_OUTSIDE_INSET,row.getScrollBarStyle());
                    assertTrue(row.getScrollBarSize()>=HordeUiTokens.dp(activity,4));
                    assertTrue("bar occupies its own space below button targets",
                            row.getHeight()-row.getChildAt(0).getBottom()>=HordeUiTokens.dp(activity,4));
                    assertTrue("this portrait row really overflows",row.getChildAt(0).getWidth()>row.getWidth()-row.getPaddingLeft()-row.getPaddingRight());
                    assertEquals(HordeUiTokens.BRASS,((ColorDrawable)row.getHorizontalScrollbarThumbDrawable()).getColor());
                    assertEquals(HordeUiTokens.IRON,((ColorDrawable)row.getHorizontalScrollbarTrackDrawable()).getColor());
                }
                String[] labels={"Resolution 75%","Water Mobile","Fire Mobile","Cap 30 Hz","Glass On","View","Image","Details","Apply","Revert","Back"};
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
                Method menu=MainActivity.class.getDeclaredMethod("createGraphicsPreviewOptionMenu",Button.class,int.class);
                menu.setAccessible(true);
                int[][] values={{50,63,75,100},{0,1,2},{0,1},{15,30,60},{0,1}};
                int[] selected={75,1,0,30,1};
                for(int choice=0;choice<5;++choice) {
                    PopupMenu popup=(PopupMenu)menu.invoke(activity,buttons.get(choice),choice);
                    assertEquals(values[choice].length,popup.getMenu().size());
                    for(int i=0;i<values[choice].length;++i) {
                        assertEquals(values[choice][i],popup.getMenu().getItem(i).getItemId());
                        assertTrue(popup.getMenu().getItem(i).isCheckable());
                        assertEquals(values[choice][i]==selected[choice],popup.getMenu().getItem(i).isChecked());
                    }
                    popup.dismiss();
                }
            } finally { activity.getResources().updateConfiguration(original,activity.getResources().getDisplayMetrics()); }
        }
    }

    @Test public void freshPreviewKeepDoesNotChurnEnabledStateAndRetainsAllReadinessGates() throws Exception {
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        TransitionButton action=new TransitionButton(activity);
        setField(activity,"graphicsApply",action); setField(activity,"graphicsConfirm",action);
        setField(activity,"graphicsPreviewWanted",true);
        Method update=MainActivity.class.getDeclaredMethod("updateGraphicsActionButtons",
                boolean.class,int.class,boolean.class,boolean.class,boolean.class);
        update.setAccessible(true);
        action.setEnabled(true); action.enabledTransitions=0;
        for(int poll=0;poll<12;++poll) update.invoke(activity,false,2,true,true,true);
        assertTrue(action.isEnabled());
        assertEquals("one shared current Keep must not disable/re-enable each poll",0,action.enabledTransitions);
        update.invoke(activity,false,2,true,true,false); // Stale preview/presentation.
        assertFalse(action.isEnabled()); assertEquals(1,action.enabledTransitions);
        update.invoke(activity,false,2,true,false,true); // Current preview, wrong request ACK.
        assertFalse(action.isEnabled()); assertEquals(1,action.enabledTransitions);
        update.invoke(activity,false,2,true,true,true);
        assertTrue(action.isEnabled()); assertEquals(2,action.enabledTransitions);
        setField(activity,"graphicsLiveChoiceError","Choice failed");
        update.invoke(activity,false,2,true,true,true); assertFalse(action.isEnabled());
        setField(activity,"graphicsLiveChoiceError",null);
        update.invoke(activity,false,1,true,true,true); assertFalse(action.isEnabled());
        update.invoke(activity,false,4,true,true,true); assertFalse(action.isEnabled());
        update.invoke(activity,false,0,false,true,true); assertTrue(action.isEnabled());

        TransitionButton draft=new TransitionButton(activity);
        setField(activity,"graphicsApply",draft); setField(activity,"graphicsPreviewWanted",false);
        update.invoke(activity,false,0,false,false,false); // Draft surface not ready.
        assertFalse(draft.isEnabled()); assertFalse(action.isEnabled());
        update.invoke(activity,true,0,false,false,false); // Actual ready draft surface.
        assertTrue(draft.isEnabled()); assertFalse(action.isEnabled());
        update.invoke(activity,false,2,true,true,false); // Distinct ordinary Apply/Keep.
        assertFalse(draft.isEnabled()); assertTrue(action.isEnabled());
        update.invoke(activity,false,2,true,false,false); assertFalse(action.isEnabled());
    }

    @Test public void heightBudgetIncludesSafeAreaAndOversizeTextUsesNativeVerticalScrolling() {
        for(float font:new float[]{1,1.7f,2}) {
            Configuration config=new Configuration(RuntimeEnvironment.getApplication().getResources().getConfiguration());
            config.fontScale=font; config.densityDpi=160;
            Context context=RuntimeEnvironment.getApplication().createConfigurationContext(config);
            int height=568,gap=48;
            ScrollView strip=new GraphicsPreviewControlsScrollView(context,GraphicsPreviewOptions.maximumOverlayHeight(height,gap));
            TextView text=new TextView(context); text.setTextSize(24);
            text.setText("Resolution\nWater\nFire\nCap\nView\nDetails\nApply\nKeep\nRevert\nBack"); strip.addView(text);
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
        activity.setContentView(scrim);
        Field scrimField=MainActivity.class.getDeclaredField("menuScrim");
        scrimField.setAccessible(true); scrimField.set(activity,scrim);
        Method show=MainActivity.class.getDeclaredMethod("showGraphicsPreviewPage");
        show.setAccessible(true); show.invoke(activity);
        assertNull(field(activity,"graphicsDetailsDialog")); assertNull(field(activity,"graphicsGraph"));
        Field draft=MainActivity.class.getDeclaredField("graphicsDraft");
        draft.setAccessible(true); draft.set(activity,GraphicsPreferences.baseline());
        Method menu=MainActivity.class.getDeclaredMethod("showGraphicsPreviewOptionMenu",Button.class,int.class);
        menu.setAccessible(true);
        menu.invoke(activity,((Button[])field(activity,"graphicsOptionButtons"))[0],0);
        PopupMenu popup=(PopupMenu)field(activity,"graphicsOptionsPopup"); assertNotNull(popup);
        TextView status=(TextView)field(activity,"graphicsTelemetry");
        String frozen=status.getText().toString();
        Method update=MainActivity.class.getDeclaredMethod("setGraphicsPreviewStatus",String.class);
        update.setAccessible(true); update.invoke(activity,"Preview FPS: 99.0");
        assertEquals("native popup must freeze background status events",frozen,status.getText().toString());
        popup.dismiss(); assertNull(field(activity,"graphicsOptionsPopup"));
        assertTrue(Double.isNaN((double)field(activity,"graphicsDisplayedFps")));
        update.invoke(activity,"Preview FPS: waiting for current RT");
        assertEquals("Preview FPS: waiting for current RT",status.getText().toString());
        // Opening another modal freezes its snapshot too; close restores guarded publication.
        menu.invoke(activity,((Button[])field(activity,"graphicsOptionButtons"))[0],0);
        ((Button)field(activity,"graphicsDetailsButton")).performClick();
        AlertDialog dialog=(AlertDialog)field(activity,"graphicsDetailsDialog");
        assertNotNull(dialog); assertTrue(dialog.isShowing());
        assertNull(field(activity,"graphicsOptionsPopup"));
        assertNotNull(field(activity,"graphicsGraph")); assertNotNull(field(activity,"graphicsDetailsTelemetry"));
        frozen=status.getText().toString(); update.invoke(activity,"Preview FPS: 100.0");
        assertEquals(frozen,status.getText().toString());
        Method dismiss=MainActivity.class.getDeclaredMethod("dismissGraphicsPreviewDetails");
        dismiss.setAccessible(true); dismiss.invoke(activity);
        assertFalse(dialog.isShowing()); assertNull(field(activity,"graphicsDetailsDialog"));
        assertNull(field(activity,"graphicsDetailsTelemetry")); assertNull(field(activity,"graphicsDetailsPanel"));
        assertNull(field(activity,"graphicsGraph"));
        update.invoke(activity,"Preview FPS: waiting for current RT");
        assertEquals("Preview FPS: waiting for current RT",status.getText().toString());
    }

    @Test public void imageInspectionKeepsAccessibleRestoreTargetOwnsTouchesAndPreservesGraphicsState() throws Exception {
        for(float font:new float[]{1,1.7f,2}) {
            MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
            Configuration original=new Configuration(activity.getResources().getConfiguration());
            Configuration config=new Configuration(original); config.fontScale=font; config.densityDpi=160;
            activity.getResources().updateConfiguration(config,activity.getResources().getDisplayMetrics());
            try {
                FrameLayout root=new FrameLayout(activity), scrim=new FrameLayout(activity);
                View surface=new View(activity); final int[] gameplayTouches={0};
                surface.setOnTouchListener((v,event)->{++gameplayTouches[0];return true;});
                root.addView(surface,new FrameLayout.LayoutParams(-1,-1));
                root.addView(scrim,new FrameLayout.LayoutParams(-1,-1)); layout(root,360,640);
                Field scrimField=MainActivity.class.getDeclaredField("menuScrim");
                scrimField.setAccessible(true); scrimField.set(activity,scrim);
                Field serial=MainActivity.class.getDeclaredField("graphicsRequestSerial"); serial.setAccessible(true); serial.setLong(activity,42);
                Method show=MainActivity.class.getDeclaredMethod("showGraphicsPreviewPage");
                show.setAccessible(true); show.invoke(activity); layout(root,360,640);
                ((Button)field(activity,"graphicsImageButton")).performClick(); layout(root,360,640);
                assertTrue((boolean)field(activity,"graphicsPreviewImageOnly"));
                assertTrue(scrim.isClickable()); assertEquals(1,scrim.getChildCount());
                List<Button> buttons=new ArrayList<>(); List<HorizontalScrollView> rows=new ArrayList<>();
                collect(scrim,buttons,rows); assertEquals(1,buttons.size());
                Button restore=buttons.get(0); assertEquals("Controls",restore.getText().toString());
                assertTrue(restore.isEnabled()); assertTrue(restore.isFocusable());
                assertTrue(restore.getMeasuredWidth()>=HordeUiTokens.dp(activity,48));
                assertTrue(restore.getMeasuredHeight()>=HordeUiTokens.dp(activity,48));
                assertEquals(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP,15,
                        activity.getResources().getDisplayMetrics()),restore.getTextSize(),.5);
                assertTrue(scrim.getChildAt(0).getTop()<640*.20);
                assertTrue(scrim.getChildAt(0).getBottom()<640*.35);
                MotionEvent down=MotionEvent.obtain(0,0,MotionEvent.ACTION_DOWN,180,500,0);
                MotionEvent up=MotionEvent.obtain(0,10,MotionEvent.ACTION_UP,180,500,0);
                try { assertTrue(root.dispatchTouchEvent(down)); assertTrue(root.dispatchTouchEvent(up)); }
                finally { down.recycle(); up.recycle(); }
                assertEquals(0,gameplayTouches[0]); assertEquals(42,serial.getLong(activity));
                // No native renderer or preferences are installed: Image/Controls must be pure UI actions.
                restore.performClick(); layout(root,360,640);
                assertFalse((boolean)field(activity,"graphicsPreviewImageOnly"));
                buttons.clear(); rows.clear(); collect(scrim,buttons,rows);
                assertEquals(11,buttons.size()); assertEquals(2,rows.size()); assertEquals(42,serial.getLong(activity));
                assertNull(field(activity,"graphicsDetailsDialog")); assertNull(field(activity,"graphicsGraph"));
            } finally { activity.getResources().updateConfiguration(original,activity.getResources().getDisplayMetrics()); }
        }
    }
}
