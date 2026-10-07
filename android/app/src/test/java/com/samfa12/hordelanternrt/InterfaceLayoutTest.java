package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
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
public final class InterfaceLayoutTest {
    @Test public void boundedPresetsRemainSeparatedInsideSafeAreaAndKeepContextSlots() {
        for(float font:new float[]{1,1.3f,1.7f,2}) for(boolean compact:new boolean[]{false,true}) for(int scale:new int[]{85,100,110}) {
            Configuration config=new Configuration(RuntimeEnvironment.getApplication().getResources().getConfiguration());
            config.fontScale=font; config.densityDpi=160;
            Context c=RuntimeEnvironment.getApplication().createConfigurationContext(config);
            FrameLayout root=new FrameLayout(c); Button[] b={new Button(c),new Button(c),new Button(c),new Button(c)};
            for(Button button:b) root.addView(button);
            UiControlLayout.apply(c,new InterfacePreferences.Values(compact,scale,70,false,false),b[0],b[1],b[2],b[3],16,24,344);
            root.measure(View.MeasureSpec.makeMeasureSpec(360,View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(640,View.MeasureSpec.EXACTLY));
            root.layout(0,0,360,640);
            Rect[] r=new Rect[4];
            for(int i=0;i<4;i++) {
                r[i]=new Rect(b[i].getLeft(),b[i].getTop(),b[i].getRight(),b[i].getBottom());
                assertTrue(r[i].width()>=48 && r[i].height()>=48);
                assertTrue(r[i].left>=16 && r[i].right<=344 && r[i].bottom<=616);
            }
            assertTrue(r[0].left-r[1].right>=8); assertTrue(r[2].left-r[3].right>=8);
            assertTrue(r[0].top-r[2].bottom>=8);
            b[2].setText("LOCKED\nSLAY LICH"); b[3].setText("LOWER");
            root.measure(View.MeasureSpec.makeMeasureSpec(360,View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(640,View.MeasureSpec.EXACTLY)); root.layout(0,0,360,640);
            assertEquals(r[2],new Rect(b[2].getLeft(),b[2].getTop(),b[2].getRight(),b[2].getBottom()));
            assertEquals(r[3],new Rect(b[3].getLeft(),b[3].getTop(),b[3].getRight(),b[3].getBottom()));
        }
    }
    @Test public void longMenuLabelsReflowInsteadOfFixedHeightClipping() throws Exception {
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        Method create=MainActivity.class.getDeclaredMethod("createMenuButton",String.class,Runnable.class);
        create.setAccessible(true);
        Button b=(Button)create.invoke(activity,"A long menu label that must wrap onto multiple readable lines",(Runnable)() -> {});
        b.setTextSize(30); b.measure(View.MeasureSpec.makeMeasureSpec(190,View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(0,View.MeasureSpec.UNSPECIFIED));
        b.layout(0,0,190,b.getMeasuredHeight());
        assertTrue("wrapped label must grow beyond minimum target: "+b.getMeasuredHeight(),b.getMeasuredHeight()>48);
        assertTrue("native text layout must wrap: "+b.getLayout().getLineCount(),b.getLayout().getLineCount()>1);
        assertTrue(b.isFocusable()); assertFalse(b.getText().toString().isEmpty());
    }
    @Test public void actualPausedPreviewKeepsInteractReadableWithoutShrinkingAcrossFontsAndWidths() throws Exception {
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        Configuration original=new Configuration(activity.getResources().getConfiguration());
        Method create=MainActivity.class.getDeclaredMethod("createInterfaceControlPreview",InterfacePreferences.Values.class);
        create.setAccessible(true);
        boolean paired=false,stacked=false;
        try {
            for(float font:new float[]{1,1.3f,1.7f,2}) for(boolean compact:new boolean[]{false,true})
                for(int scale:new int[]{85,100,110}) for(int width:new int[]{208,272,360}) {
                    Configuration config=new Configuration(original); config.fontScale=font; config.densityDpi=160;
                    activity.getResources().updateConfiguration(config,activity.getResources().getDisplayMetrics());
                    LinearLayout preview=(LinearLayout)create.invoke(activity,new InterfacePreferences.Values(compact,scale,70,false,false));
                    preview.measure(View.MeasureSpec.makeMeasureSpec(width,View.MeasureSpec.EXACTLY),
                            View.MeasureSpec.makeMeasureSpec(0,View.MeasureSpec.UNSPECIFIED));
                    preview.layout(0,0,width,preview.getMeasuredHeight());
                    final String configuration="font="+font+" compact="+compact+" scale="+scale+" width="+width;
                    assertEquals(configuration,3,preview.getChildCount());
                    assertEquals("middle reserved action row",activity.getString(R.string.dodge),
                            ((Button)((LinearLayout)preview.getChildAt(1)).getChildAt(0)).getText().toString());
                    for(int rowIndex=0;rowIndex<3;++rowIndex) {
                        LinearLayout row=(LinearLayout)preview.getChildAt(rowIndex);
                        paired|=row.getOrientation()==LinearLayout.HORIZONTAL;
                        stacked|=row.getOrientation()==LinearLayout.VERTICAL;
                        for(int i=0;i<row.getChildCount();++i) {
                            Button button=(Button)row.getChildAt(i);
                            final String context=configuration+" row="+rowIndex+" label="+button.getText()+
                                    " bounds="+button.getLeft()+","+button.getTop()+","+button.getRight()+","+button.getBottom();
                            assertTrue(context+" native target width",button.getWidth()>=48);
                            assertTrue(context+" native target height",button.getHeight()>=48);
                            assertTrue(context+" responsive label stays inside row",button.getLeft()>=0 && button.getRight()<=width);
                            assertNotNull(context,button.getLayout());
                            assertEquals(context+" each complete action label uses one line",1,button.getLayout().getLineCount());
                            assertEquals(context+" no hidden ellipsis",0,button.getLayout().getEllipsisCount(0));
                            assertTrue(context+" entire label fits its native text area",button.getLayout().getLineWidth(0)<=
                                    button.getWidth()-button.getCompoundPaddingLeft()-button.getCompoundPaddingRight()+1);
                            assertEquals(context+" system-sized 17sp text is not auto-shrunk",android.widget.TextView.AUTO_SIZE_TEXT_TYPE_NONE,button.getAutoSizeTextType());
                            assertEquals(context+" system 17sp size remains intact",android.util.TypedValue.applyDimension(
                                    android.util.TypedValue.COMPLEX_UNIT_SP,17,activity.getResources().getDisplayMetrics()),button.getTextSize(),.5);
                            assertFalse(button.isClickable()); assertFalse(button.isFocusable());
                        }
                    }
                    Button interact=(Button)((ViewGroup)preview.getChildAt(0)).getChildAt(1);
                    assertEquals(activity.getString(R.string.interact),interact.getText().toString());
                    assertEquals(font,activity.getResources().getConfiguration().fontScale,0);
                }
            assertTrue("roomy widths retain paired controls",paired);
            assertTrue("narrow large-font previews reflow vertically",stacked);
        } finally { activity.getResources().updateConfiguration(original,activity.getResources().getDisplayMetrics()); }
    }
    @Test public void disabledLabelsRemainOpaqueAndFocusHasDistinctDrawableState() {
        Context c=RuntimeEnvironment.getApplication();
        android.graphics.drawable.StateListDrawable background=HordeUiTokens.button(c,HordeUiTokens.SLATE);
        background.setState(new int[]{android.R.attr.state_enabled}); android.graphics.drawable.Drawable normal=background.getCurrent();
        background.setState(new int[]{android.R.attr.state_enabled,android.R.attr.state_focused}); assertNotSame(normal,background.getCurrent());
        int disabled=HordeUiTokens.label(HordeUiTokens.PARCHMENT).getColorForState(new int[]{-android.R.attr.state_enabled},0);
        assertEquals(255,android.graphics.Color.alpha(disabled));
    }
}
