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
                int overlayBottom=strip.getHeight()+lp.bottomMargin;
                assertTrue("preserve a useful RT image",overlayBottom<=size[1]*.60);
                if(overlayBottom<=size[1]*.35) assertTrue(strip.getTop()>=size[1]*.65);
                else assertTrue("expanded overlay still leaves at least 40% for the RT image",strip.getTop()>=size[1]*.40);
                View dock=strip.findViewWithTag("graphics-action-dock");
                ScrollView controlViewport=(ScrollView)strip.findViewWithTag("graphics-preview-scroll");
                assertNotNull(dock); assertNotNull(controlViewport);
                assertTrue("sticky actions stay within the overlay",dock.getBottom()<=strip.getHeight());
                List<Button> buttons=new ArrayList<>(); List<HorizontalScrollView> rows=new ArrayList<>();
                collect(strip,buttons,rows);
                assertEquals(2,rows.size()); assertEquals(15,buttons.size());
                for(HorizontalScrollView row:rows) {
                    assertTrue(row.isHorizontalScrollBarEnabled()); assertFalse(row.isVerticalScrollBarEnabled());
                    assertFalse("scroll affordance must stay visible",row.isScrollbarFadingEnabled());
                    assertEquals(View.SCROLLBARS_INSIDE_INSET,row.getScrollBarStyle());
                    assertTrue(row.getScrollBarSize()>=HordeUiTokens.dp(activity,4));
                    assertTrue("bar occupies its own space below button targets",
                            row.getHeight()-row.getChildAt(0).getBottom()>=HordeUiTokens.dp(activity,4));
                    if("graphics-preview-row-1".equals(row.getTag()))
                        assertTrue("this portrait row really overflows",row.getChildAt(0).getWidth()>row.getWidth()-row.getPaddingLeft()-row.getPaddingRight());
                    assertEquals(HordeUiTokens.BRASS,((ColorDrawable)row.getHorizontalScrollbarThumbDrawable()).getColor());
                    assertEquals(HordeUiTokens.IRON,((ColorDrawable)row.getHorizontalScrollbarTrackDrawable()).getColor());
                }
                String[] labels={"Resolution: 75%","Water: Mobile","Fire: Mobile","Cap: 30 Hz","Glass: On","Shadows: Current","Mist: On","Indoor dust: Off","View","Image","Details","Use","Keep","Restore","Back"};
                int minimum=HordeUiTokens.dp(activity,48);
                for(int i=0;i<buttons.size();++i) {
                    Button button=buttons.get(i); assertEquals(labels[i],button.getText().toString());
                    assertTrue(button.getMeasuredWidth()>=minimum); assertTrue(button.getMeasuredHeight()>=minimum);
                    assertTrue(button.isFocusable()); assertEquals(0,button.getAutoSizeTextType());
                    assertEquals(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP,15,
                            activity.getResources().getDisplayMetrics()),button.getTextSize(),.5);
                }
                HorizontalScrollView optionStrip=(HorizontalScrollView)buttons.get(0).getParent().getParent();
                int firstOptionBottom=optionStrip.getBottom();
                assertTrue("the whole option row, including its scrollbar, remains visible above the sticky dock; " +
                                "font="+font+" viewport="+size[0]+"x"+size[1]+" overlay="+strip.getHeight()+
                                " dock="+dock.getHeight()+" scroller="+controlViewport.getHeight()+
                                " targetBottom="+firstOptionBottom,
                        controlViewport.getHeight()>=firstOptionBottom);
                assertEquals(2,status.getMaxLines());
                assertNull(field(activity,"graphicsDetailsDialog")); assertNull(field(activity,"graphicsGraph"));
                assertFalse(((Button)field(activity,"graphicsConfirm")).isEnabled());
                Method menu=MainActivity.class.getDeclaredMethod("createGraphicsPreviewOptionMenu",Button.class,int.class);
                menu.setAccessible(true);
                int[][] values={BuildConfig.MIN_RENDER_SCALE_PERCENT == 33 ? new int[]{33,40,50,63,75,100} : new int[]{50,63,75,100},{0,1,2},{2,0,1},{15,30,60},{0,1},{0,1,2},{0,1},{0,1,2}};
                int[] selected={75,1,0,30,1,1,1,0};
                for(int choice=0;choice<8;++choice) {
                    PopupMenu popup=(PopupMenu)menu.invoke(activity,buttons.get(choice),choice);
                    assertEquals(values[choice].length,popup.getMenu().size());
                    for(int i=0;i<values[choice].length;++i) {
                        assertEquals(values[choice][i],popup.getMenu().getItem(i).getItemId());
                        assertTrue(popup.getMenu().getItem(i).isCheckable());
                        if(choice==GraphicsPreviewOptions.RESOLUTION && values[choice][i]<50) {
                            assertTrue("isolated sub-50 choices must be explicit experiments",
                                    popup.getMenu().getItem(i).getTitle().toString().contains("Experimental"));
                        }
                        assertEquals(values[choice][i]==selected[choice],popup.getMenu().getItem(i).isChecked());
                        if(choice==GraphicsPreviewOptions.FIRE) assertEquals(new String[]{"Low","Mobile","High"}[i],popup.getMenu().getItem(i).getTitle().toString());
                        if(choice==GraphicsPreviewOptions.SHADOW) assertEquals(new String[]{"Lower","Current","Higher"}[i],popup.getMenu().getItem(i).getTitle().toString());
                    }
                    popup.dismiss();
                }
            } finally { activity.getResources().updateConfiguration(original,activity.getResources().getDisplayMetrics()); }
        }
    }

    @Test public void previewUseAndKeepStaySeparateAndRetainAllReadinessGates() throws Exception {
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        TransitionButton use=new TransitionButton(activity), keep=new TransitionButton(activity);
        use.setText("Use"); keep.setText("Keep");
        use.setContentDescription("Use these acknowledged preview settings for a temporary trial. This does not save.");
        setField(activity,"graphicsApply",use); setField(activity,"graphicsConfirm",keep);
        setField(activity,"graphicsPreviewWanted",true);
        GraphicsPreferences.Values saved=GraphicsPreferences.baseline();
        GraphicsPreferences.Values candidate=new GraphicsPreferences.Values(50,1,GraphicsPreferences.FIRE_LOW,30,false,GraphicsPreferences.SHADOW_HIGHER);
        setField(activity,"graphicsConfirmed",saved); setField(activity,"graphicsDraft",candidate);
        Method optionLabel=MainActivity.class.getDeclaredMethod("graphicsOptionLabel",int.class);
        optionLabel.setAccessible(true);
        assertEquals("Fire: Low",optionLabel.invoke(activity,GraphicsPreviewOptions.FIRE));
        assertEquals("Shadows: Higher",optionLabel.invoke(activity,GraphicsPreviewOptions.SHADOW));
        assertEquals("Indoor dust: Off",optionLabel.invoke(activity,GraphicsPreviewOptions.DUST));
        TextView summary=new TextView(activity); setField(activity,"graphicsSelectionSummary",summary);
        Method selection=MainActivity.class.getDeclaredMethod("updateGraphicsSelectionSummary",boolean.class);
        selection.setAccessible(true);
        selection.invoke(activity,false);
        assertEquals("Requested 50% / Saved 75%",summary.getText().toString());
        selection.invoke(activity,true);
        assertEquals("Preview 50% / Saved 75%",summary.getText().toString());
        Method update=MainActivity.class.getDeclaredMethod("updateGraphicsActionButtons",
                boolean.class,int.class,boolean.class,boolean.class,boolean.class);
        update.setAccessible(true);
        use.setEnabled(false); keep.setEnabled(true); use.enabledTransitions=0; keep.enabledTransitions=0;
        for(int poll=0;poll<12;++poll) update.invoke(activity,false,2,true,true,true);
        assertFalse(use.isEnabled()); assertTrue(keep.isEnabled());
        assertEquals("Keep",keep.getText().toString());
        assertTrue("a presented trial must not change the saved label",summary.getText().toString().contains("Saved 75%"));
        assertSame(saved,field(activity,"graphicsConfirmed"));
        assertEquals(GraphicsPreferences.FIRE_MOBILE,saved.fire);
        assertEquals(GraphicsPreferences.SHADOW_CURRENT,saved.shadow);
        assertEquals("an acknowledged Keep must not churn enabled state",0,keep.enabledTransitions);
        update.invoke(activity,false,2,true,true,false); // Stale preview/presentation.
        assertFalse(keep.isEnabled()); assertEquals(1,keep.enabledTransitions);
        update.invoke(activity,false,2,true,false,true); // Current preview, wrong request ACK.
        assertFalse(keep.isEnabled()); assertEquals(1,keep.enabledTransitions);
        update.invoke(activity,false,2,true,true,true);
        assertTrue(keep.isEnabled()); assertEquals(2,keep.enabledTransitions);
        update.invoke(activity,false,2,false,false,true); // ACKed state alone is insufficient when readiness drops.
        assertFalse(keep.isEnabled());
        update.invoke(activity,false,2,true,true,true); // A ready presentation remains keepable even while busy.
        assertTrue(keep.isEnabled());
        setField(activity,"graphicsLiveChoiceError","Choice failed");
        update.invoke(activity,false,2,true,true,true); assertFalse(keep.isEnabled());
        setField(activity,"graphicsLiveChoiceError",null);
        update.invoke(activity,false,1,true,true,true); assertFalse(use.isEnabled());
        update.invoke(activity,false,4,true,true,true); assertFalse(use.isEnabled());
        update.invoke(activity,false,0,false,true,true); assertTrue(use.isEnabled());

        TransitionButton draft=new TransitionButton(activity);
        setField(activity,"graphicsApply",draft); setField(activity,"graphicsPreviewWanted",false);
        update.invoke(activity,false,0,false,false,false); // Draft surface not ready.
        assertFalse(draft.isEnabled()); assertFalse(keep.isEnabled());
        update.invoke(activity,true,0,false,false,false); // Actual ready draft surface.
        assertTrue(draft.isEnabled()); assertFalse(keep.isEnabled());
        update.invoke(activity,false,2,true,true,false); // Distinct ordinary Apply/Keep.
        assertFalse(draft.isEnabled()); assertTrue(keep.isEnabled());
        update.invoke(activity,false,2,true,false,false); assertFalse(keep.isEnabled());
        setField(activity,"graphicsPreviewWanted",true); setField(activity,"graphicsApply",use);
        update.invoke(activity,false,0,false,false,true);
        assertEquals("Use",use.getText().toString());
        assertTrue(use.getContentDescription().toString().contains("does not save"));
        assertTrue(use.isEnabled()); assertFalse(keep.isEnabled());
        setField(activity,"graphicsDraft",saved); selection.invoke(activity,true);
        assertEquals("restored labels match saved values","Preview 75% / Saved 75%",summary.getText().toString());
        setField(activity,"graphicsConfirmed",candidate); setField(activity,"graphicsDraft",candidate); selection.invoke(activity,true);
        assertEquals("only a confirmed saved tuple moves Saved","Preview 50% / Saved 50%",summary.getText().toString());
    }

    @Test public void heightBudgetIncludesSafeAreaAndOversizeTextUsesNativeVerticalScrolling() {
        for(float font:new float[]{1,1.7f,2}) {
            Configuration config=new Configuration(RuntimeEnvironment.getApplication().getResources().getConfiguration());
            config.fontScale=font; config.densityDpi=160;
            Context context=RuntimeEnvironment.getApplication().createConfigurationContext(config);
            int height=568,gap=48;
            ScrollView strip=new GraphicsPreviewControlsScrollView(context,GraphicsPreviewOptions.maximumOverlayHeight(height,gap));
            TextView text=new TextView(context); text.setTextSize(24);
            text.setText("Resolution\nWater\nFire\nCap\nGlass\nShadows\nMist\nIndoor dust\nView\nDetails\nUse these settings\nKeep and save\nRestore saved\nBack"); strip.addView(text);
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
                GraphicsPreferences.Values draft=GraphicsPreferences.mobileDefaults();
                setField(activity,"graphicsDraft",draft); setField(activity,"graphicsConfirmed",draft);
                setField(activity,"graphicsPreviewWanted",true);
                Method show=MainActivity.class.getDeclaredMethod("showGraphicsPreviewPage");
                show.setAccessible(true); show.invoke(activity); layout(root,360,640);
                ((Button)field(activity,"graphicsImageButton")).performClick(); layout(root,360,640);
                assertTrue((boolean)field(activity,"graphicsPreviewImageOnly"));
                assertTrue(scrim.isClickable()); assertEquals(1,scrim.getChildCount());
                List<Button> buttons=new ArrayList<>(); List<HorizontalScrollView> rows=new ArrayList<>();
                collect(scrim,buttons,rows); assertEquals(5,buttons.size());
                Button restore=buttons.get(0); assertEquals("Controls",restore.getText().toString());
                assertTrue(restore.isEnabled()); assertTrue(restore.isFocusable());
                assertTrue(restore.getMeasuredWidth()>=HordeUiTokens.dp(activity,48));
                assertTrue(restore.getMeasuredHeight()>=HordeUiTokens.dp(activity,48));
                String[] imageActions={"Controls","Use","Keep","Restore","Back"};
                for(int i=0;i<imageActions.length;++i) {
                    Button action=buttons.get(i); assertEquals(imageActions[i],action.getText().toString());
                    assertNotNull(action.getContentDescription()); assertTrue(action.isFocusable());
                    assertTrue(action.getMeasuredWidth()>=HordeUiTokens.dp(activity,48));
                    assertTrue(action.getMeasuredHeight()>=HordeUiTokens.dp(activity,48));
                }
                assertFalse(buttons.get(1).isEnabled()); assertFalse(buttons.get(2).isEnabled());
                assertTrue(buttons.get(3).isEnabled()); assertTrue(buttons.get(4).isEnabled());
                assertTrue(buttons.get(1).getContentDescription().toString().contains("does not save"));
                assertTrue(buttons.get(2).getContentDescription().toString().contains("Save only"));
                assertTrue(buttons.get(3).getContentDescription().toString().contains("confirmed"));
                assertTrue(buttons.get(4).getContentDescription().toString().contains("return to Graphics"));
                buttons.get(1).performClick(); // A not-ready Use is inert; it cannot stage or save a draft.
                assertSame(draft,field(activity,"graphicsDraft"));
                assertFalse((boolean)field(activity,"graphicsBusy"));
                assertEquals(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP,15,
                        activity.getResources().getDisplayMetrics()),restore.getTextSize(),.5);
                View imageHost=scrim.getChildAt(0);
                assertEquals("image inspection owns a full-screen transparent touch layer",640,imageHost.getHeight());
                View imageDock=imageHost.findViewWithTag("graphics-action-dock");
                assertNotNull(imageDock);
                assertTrue("the complete dock stays inside image inspection",
                        imageDock.getTop()>=0 && imageDock.getBottom()<=imageHost.getHeight());
                assertTrue("Restore and Back remain visible inside the bottom dock",
                        buttons.get(3).getParent().getParent()==imageDock &&
                                buttons.get(4).getParent().getParent()==imageDock);
                MotionEvent down=MotionEvent.obtain(0,0,MotionEvent.ACTION_DOWN,180,500,0);
                MotionEvent up=MotionEvent.obtain(0,10,MotionEvent.ACTION_UP,180,500,0);
                try { assertTrue(root.dispatchTouchEvent(down)); assertTrue(root.dispatchTouchEvent(up)); }
                finally { down.recycle(); up.recycle(); }
                assertEquals(0,gameplayTouches[0]); assertEquals(42,serial.getLong(activity));
                // No native renderer or preferences are installed: Image/Controls must be pure UI actions.
                restore.performClick(); layout(root,360,640);
                assertFalse((boolean)field(activity,"graphicsPreviewImageOnly"));
                assertTrue((boolean)field(activity,"graphicsPreviewWanted"));
                assertSame(draft,field(activity,"graphicsDraft"));
                buttons.clear(); rows.clear(); collect(scrim,buttons,rows);
                assertEquals(15,buttons.size()); assertEquals(2,rows.size()); assertEquals(42,serial.getLong(activity));
                assertNull(field(activity,"graphicsDetailsDialog")); assertNull(field(activity,"graphicsGraph"));
            } finally { activity.getResources().updateConfiguration(original,activity.getResources().getDisplayMetrics()); }
        }
    }
}
