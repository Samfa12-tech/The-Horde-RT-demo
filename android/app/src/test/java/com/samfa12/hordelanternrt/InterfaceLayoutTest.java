package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.view.View;
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
    @Test public void disabledLabelsRemainOpaqueAndFocusHasDistinctDrawableState() {
        Context c=RuntimeEnvironment.getApplication();
        android.graphics.drawable.StateListDrawable background=HordeUiTokens.button(c,HordeUiTokens.SLATE);
        background.setState(new int[]{android.R.attr.state_enabled}); android.graphics.drawable.Drawable normal=background.getCurrent();
        background.setState(new int[]{android.R.attr.state_enabled,android.R.attr.state_focused}); assertNotSame(normal,background.getCurrent());
        int disabled=HordeUiTokens.label(HordeUiTokens.PARCHMENT).getColorForState(new int[]{-android.R.attr.state_enabled},0);
        assertEquals(255,android.graphics.Color.alpha(disabled));
    }
}
