package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class TouchHudPresentationTest {
    private static Bitmap draw(Drawable drawable,int width,int height) {
        Bitmap image=Bitmap.createBitmap(width,height,Bitmap.Config.ARGB_8888);
        drawable.setBounds(0,0,width,height); drawable.draw(new Canvas(image)); return image;
    }
    private static int[] pixels(Bitmap image) {
        int[] result=new int[image.getWidth()*image.getHeight()];
        image.getPixels(result,0,image.getWidth(),0,0,image.getWidth(),image.getHeight());return result;
    }
    @Test public void savedBackingOpacityActuallyReachesTheWorldAndCornersStayClear() {
        Context c=RuntimeEnvironment.getApplication();
        for(int percent:new int[]{55,70,95}) for(boolean circle:new boolean[]{false,true}) {
            int alpha=Math.round(percent*255/100f),width=circle?56:104,height=circle?56:72;
            Drawable next=HordeUiTokens.touchButton(c,(alpha<<24)|0x00151719,circle);
            next.setState(new int[]{android.R.attr.state_enabled});
            Bitmap image=draw(next,width,height);
            assertEquals("no hidden opaque underlay",alpha,Color.alpha(image.getPixel(width/2,height/2)));
            assertEquals("rounded corners reveal the world",0,Color.alpha(image.getPixel(0,0)));
            next.setAlpha(0);
            assertEquals("drawable alpha includes rim",0,Color.alpha(draw(next,width,height).getPixel(width/2,2)));
        }
        Drawable old=HordeUiTokens.button(c,0xB3151719);
        old.setState(new int[]{android.R.attr.state_enabled});
        assertEquals("retained old plate is an opaque control",255,Color.alpha(draw(old,104,72).getPixel(52,36)));
    }
    @Test public void pressedFocusDisabledAndRaisedStatesStayDistinctWithoutFadingLabels() {
        Context c=RuntimeEnvironment.getApplication();
        TouchButtonDrawable next=HordeUiTokens.touchButton(c,0xB324272A,true);
        next.setState(new int[]{android.R.attr.state_enabled});
        int[] normal=pixels(draw(next,56,56));
        for(int[] state:new int[][]{{android.R.attr.state_enabled,android.R.attr.state_pressed},
                {android.R.attr.state_enabled,android.R.attr.state_focused},
                {android.R.attr.state_enabled,android.R.attr.state_activated},{}}) {
            assertTrue(next.setState(state));
            assertFalse("state must have visible feedback",Arrays.equals(normal,pixels(draw(next,56,56))));
        }
        assertEquals(255,Color.alpha(HordeUiTokens.label(HordeUiTokens.PARCHMENT)
                .getColorForState(new int[]{-android.R.attr.state_enabled},0)));
    }
    @Test public void actualNativeLayoutsKeepTargetsLabelsActionsAndProduceMatchedComponentViews() throws Exception {
        MainActivity activity=Robolectric.buildActivity(MainActivity.class).get();
        Configuration original=new Configuration(activity.getResources().getConfiguration());
        Method oldStyle=MainActivity.class.getDeclaredMethod("styleActionButton",Button.class,int.class,int.class);
        Method newStyle=MainActivity.class.getDeclaredMethod("styleGameplayActionButton",Button.class,int.class,int.class,boolean.class);
        oldStyle.setAccessible(true);newStyle.setAccessible(true);
        String directory=System.getenv("HORDE_HUD_EVIDENCE_DIR");
        try {
            Configuration config=new Configuration(original);config.densityDpi=160;config.fontScale=1.7f;
            activity.getResources().updateConfiguration(config,activity.getResources().getDisplayMetrics());
            for(int[] size:new int[][]{{360,640},{800,360}}) {
                Rect[] before=null;
                for(boolean candidate:new boolean[]{false,true}) {
                    FrameLayout root=(FrameLayout)LayoutInflater.from(activity).inflate(R.layout.activity_main,null,false);
                    root.setBackgroundColor(Color.TRANSPARENT);
                    root.findViewById(R.id.scene_surface).setVisibility(View.GONE);
                    root.findViewById(R.id.menu_scrim).setVisibility(View.GONE);
                    root.findViewById(R.id.diagnostics_panel).setVisibility(View.GONE);
                    // This is a five-control component comparison, not a running
                    // Activity. Do not display unconfigured startup/menu chrome.
                    root.findViewById(R.id.rt_status).setVisibility(View.GONE);
                    root.findViewById(R.id.vitality_status).setVisibility(View.GONE);
                    root.findViewById(R.id.menu_button).setVisibility(View.GONE);
                    Button[] controls={root.findViewById(R.id.attack_button),root.findViewById(R.id.parry_button),
                            root.findViewById(R.id.dodge_button),root.findViewById(R.id.interact_button),
                            root.findViewById(R.id.toggle_held_light_pose_button)};
                    UiControlLayout.apply(activity,InterfacePreferences.defaults(),controls[0],controls[1],controls[2],controls[3],controls[4],20,24,size[0]-8);
                    LanternPoseControl.apply(controls[4],true,"Lower lantern");
                    AtomicInteger calls=new AtomicInteger();
                    for(int i=0;i<controls.length;i++) {
                        Button b=controls[i];b.setVisibility(View.VISIBLE);
                        if(candidate)newStyle.invoke(activity,b,0xB324272A,HordeUiTokens.PARCHMENT,i==4);
                        else oldStyle.invoke(activity,b,0xB324272A,HordeUiTokens.PARCHMENT);
                        b.setOnClickListener(view->calls.incrementAndGet());
                        if(i<4)b.setContentDescription(b.getText());
                    }
                    root.measure(View.MeasureSpec.makeMeasureSpec(size[0],View.MeasureSpec.EXACTLY),
                            View.MeasureSpec.makeMeasureSpec(size[1],View.MeasureSpec.EXACTLY));
                    root.layout(0,0,size[0],size[1]);
                    Rect[] bounds=new Rect[controls.length];
                    for(int i=0;i<controls.length;i++) {
                        Button b=controls[i];bounds[i]=new Rect(b.getLeft(),b.getTop(),b.getRight(),b.getBottom());
                        assertTrue(bounds[i].width()>=48 && bounds[i].height()>=48);
                        assertTrue(bounds[i].left>=8 && bounds[i].right<=size[0]-20 && bounds[i].top>=0 && bounds[i].bottom<=size[1]-24);
                        assertNotNull(b.getContentDescription());assertTrue(b.getContentDescription().length()>0);
                        if(candidate && i<4) {
                            assertEquals("label="+b.getText()+" width="+b.getWidth()+" textSize="+b.getTextSize(),1,b.getLayout().getLineCount());
                            assertEquals(0,b.getLayout().getEllipsisCount(0));
                            assertTrue("full action label must fit inside rounded face: "+b.getText(),
                                    b.getLayout().getLineWidth(0)<=b.getWidth()-b.getCompoundPaddingLeft()-b.getCompoundPaddingRight()+1);
                        }
                        b.performClick();
                        for(int j=0;j<i;j++)assertFalse(Rect.intersects(bounds[i],bounds[j]));
                        if(candidate)assertEquals("presentation must preserve target bounds",before[i],bounds[i]);
                    }
                    assertEquals(5,calls.get());
                    assertEquals("",controls[4].getText().toString());assertTrue(controls[4].isActivated());
                    if(!candidate)before=bounds;
                    if(directory!=null) {
                        Bitmap image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);
                        Canvas canvas=new Canvas(image);Paint paint=new Paint();
                        for(int y=0;y<size[1];y+=40) {
                            paint.setColor((y/40)%2==0?0xFF6F7775:0xFFACA38B);
                            canvas.drawRect(0,y,size[0],y+40,paint);
                        }
                        root.draw(canvas);
                        File output=new File(directory,(candidate?"after-":"before-")+size[0]+"x"+size[1]+".png");
                        assertTrue(output.getParentFile().isDirectory() || output.getParentFile().mkdirs());
                        try(FileOutputStream stream=new FileOutputStream(output)) {assertTrue(image.compress(Bitmap.CompressFormat.PNG,100,stream));}
                    }
                }
            }
        } finally {activity.getResources().updateConfiguration(original,activity.getResources().getDisplayMetrics());}
    }
}
