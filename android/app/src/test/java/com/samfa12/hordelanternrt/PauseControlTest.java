package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;
import org.robolectric.shadows.ShadowLooper;
import org.robolectric.android.controller.ActivityController;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class PauseControlTest {
    private static void event(FrameLayout root,int action,float x,float y) {
        MotionEvent e=MotionEvent.obtain(10,20,action,x,y,0);
        try { assertTrue(root.dispatchTouchEvent(e)); } finally { e.recycle(); }
    }
    @Test public void existingUpperRightAnchorKeepsComfortableTargetAndStartTargetOwnership() {
        final String originalQualifiers=RuntimeEnvironment.getQualifiers();
        Configuration original=new Configuration(RuntimeEnvironment.getApplication().getResources().getConfiguration());
        try {
            for(float font:new float[]{1f,1.7f,2f})for(int[] size:new int[][]{{320,568},{411,801},{804,381}}) {
                // Configure the real test display before attaching; queued traversal must not
                // replace a manually forced viewport with Robolectric's default dimensions.
                RuntimeEnvironment.setQualifiers("w"+size[0]+"dp-h"+size[1]+"dp-"+(size[0]>size[1]?"land":"port")+"-mdpi");
                ActivityController<Activity> controller=Robolectric.buildActivity(Activity.class);
                Activity a=controller.get();
                a.requestWindowFeature(Window.FEATURE_NO_TITLE);
                a.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
                controller.setup();
                try {
                    Configuration c=new Configuration(a.getResources().getConfiguration());c.fontScale=font;
                    a.getResources().updateConfiguration(c,a.getResources().getDisplayMetrics());
                    FrameLayout root=new FrameLayout(a);
                    AtomicInteger worldDown=new AtomicInteger(),worldMove=new AtomicInteger(),pauses=new AtomicInteger();
                    View world=new View(a);world.setOnTouchListener((v,e)->{
                        if(e.getActionMasked()==MotionEvent.ACTION_DOWN)worldDown.incrementAndGet();
                        if(e.getActionMasked()==MotionEvent.ACTION_MOVE)worldMove.incrementAndGet();return true;
                    });root.addView(world,new FrameLayout.LayoutParams(-1,-1));
                    Button pause=new Button(a);pause.setId(R.id.menu_button);
                    FrameLayout.LayoutParams old=new FrameLayout.LayoutParams(72,48,Gravity.TOP|Gravity.END);
                    old.topMargin=16+11;old.setMarginEnd(16+19);pause.setLayoutParams(old);
                    pause.setOnClickListener(v->pauses.incrementAndGet());root.addView(pause);
                    PauseControl.apply(pause,19,11);
                    a.setContentView(root);
                    ShadowLooper.idleMainLooper();
                    assertTrue("dispatch fixture must be attached to a real Activity window",pause.isAttachedToWindow());
                    assertEquals("window matches selected viewport width",size[0],root.getWidth());
                    assertEquals("window matches selected viewport height",size[1],root.getHeight());
                    assertEquals(48,pause.getWidth());assertEquals(48,pause.getHeight());
                    assertEquals(size[0]-16-19,pause.getRight());assertEquals(16+11,pause.getTop());
                    assertEquals("same previous height clears compact RT status at 64 dp plus inset",64+11,pause.getBottom());
                    assertEquals("",pause.getText().toString());assertEquals("Pause / menu",pause.getContentDescription().toString());
                    assertNotNull(pause.getCompoundDrawables()[0]);assertEquals(24,pause.getCompoundDrawables()[0].getBounds().width());
                    Rect before=new Rect(pause.getLeft(),pause.getTop(),pause.getRight(),pause.getBottom());
                    Rect look=new Rect(size[0]/2,size[1]/3,size[0]-88-19,size[1]*3/4);
                    assertFalse("top-right pause cannot occupy the right-middle look rectangle",Rect.intersects(look,before));
                    float x=before.exactCenterX(),y=before.exactCenterY();
                    event(root,MotionEvent.ACTION_DOWN,size[0]*.6f,size[1]*.4f);
                    event(root,MotionEvent.ACTION_MOVE,x,y);event(root,MotionEvent.ACTION_UP,x,y);
                    ShadowLooper.idleMainLooper(); // View.onTouchEvent posts PerformClick on release.
                    assertEquals("queued window traversal preserves viewport width",size[0],root.getWidth());
                    assertEquals("queued window traversal preserves viewport height",size[1],root.getHeight());
                    assertEquals("button does not move under a cached touch",before,new Rect(pause.getLeft(),pause.getTop(),pause.getRight(),pause.getBottom()));
                    assertEquals(0,pauses.get());assertEquals(1,worldDown.get());assertEquals(1,worldMove.get());
                    event(root,MotionEvent.ACTION_DOWN,x,y);event(root,MotionEvent.ACTION_UP,x,y);
                    ShadowLooper.idleMainLooper();
                    assertEquals("original callback survives the presentation replacement",1,pauses.get());
                } finally {controller.pause().stop().destroy();}
            }
        } finally {
            RuntimeEnvironment.setQualifiers(originalQualifiers);
            RuntimeEnvironment.getApplication().getResources().updateConfiguration(original,RuntimeEnvironment.getApplication().getResources().getDisplayMetrics());
        }
    }
    @Test public void circularBackingUsesSavedOpacityAndDoesNotRewritePreferences() throws Exception {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        android.content.SharedPreferences p=a.getSharedPreferences("pause-presentation",Context.MODE_PRIVATE);
        p.edit().putInt(InterfacePreferences.OPACITY,55).putInt(InterfacePreferences.SCALE,110).apply();
        Map<String,?> before=p.getAll();InterfacePreferences.Values v=InterfacePreferences.read(p);
        Method backing=MainActivity.class.getDeclaredMethod("interfaceBacking",InterfacePreferences.Values.class,int.class);backing.setAccessible(true);
        Method style=MainActivity.class.getDeclaredMethod("styleGameplayActionButton",Button.class,int.class,int.class,boolean.class);style.setAccessible(true);
        Button pause=new Button(a);pause.setId(R.id.menu_button);pause.setLayoutParams(new FrameLayout.LayoutParams(72,48));
        int fill=(Integer)backing.invoke(a,v,HordeUiTokens.SLATE);
        style.invoke(a,pause,fill,HordeUiTokens.PARCHMENT,true);PauseControl.apply(pause,0,0);
        assertTrue(pause.getBackground() instanceof TouchButtonDrawable);
        int side=pause.getLayoutParams().width;Bitmap image=Bitmap.createBitmap(side,side,Bitmap.Config.ARGB_8888);
        pause.getBackground().setBounds(0,0,side,side);pause.getBackground().draw(new Canvas(image));
        assertEquals(0,image.getPixel(0,0));assertEquals(Math.round(.55f*255),image.getPixel(side/2,side/2)>>>24);
        assertEquals(before,p.getAll());
    }
}
