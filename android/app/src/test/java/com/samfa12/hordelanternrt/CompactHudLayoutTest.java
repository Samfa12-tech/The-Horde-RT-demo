package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import java.lang.reflect.Method;
import java.io.File;
import java.io.FileOutputStream;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class CompactHudLayoutTest {
    @Test public void combatHierarchyPreservesReservedMovementLookAndSafeTargets() throws Exception {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        Configuration original=new Configuration(a.getResources().getConfiguration());
        Method style=MainActivity.class.getDeclaredMethod("styleGameplayActionButton",Button.class,int.class,int.class,boolean.class);
        style.setAccessible(true);
        try {
            for(float font:new float[]{1,1.7f,2})for(boolean compact:new boolean[]{false,true})
                for(int scale:new int[]{85,100,110})for(int[] size:new int[][]{{320,568},{411,801},{804,381}}) {
                Configuration config=new Configuration(original);config.densityDpi=160;config.fontScale=font;
                a.getResources().updateConfiguration(config,a.getResources().getDisplayMetrics());
                FrameLayout root=(FrameLayout)LayoutInflater.from(a).inflate(R.layout.activity_main,null,false);
                Button[] b={root.findViewById(R.id.attack_button),root.findViewById(R.id.parry_button),
                    root.findViewById(R.id.dodge_button),root.findViewById(R.id.interact_button),root.findViewById(R.id.toggle_held_light_pose_button)};
                UiControlLayout.apply(a,new InterfacePreferences.Values(compact,scale,70,false,false),b[0],b[1],b[2],b[3],b[4],0,0,size[0],size[1]);
                LanternPoseControl.apply(b[4],true,"Lower lantern");
                for(int i=0;i<b.length;i++) {b[i].setVisibility(View.VISIBLE);style.invoke(a,b[i],0xB324272A,HordeUiTokens.PARCHMENT,i==4);}
                root.measure(View.MeasureSpec.makeMeasureSpec(size[0],View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(size[1],View.MeasureSpec.EXACTLY));
                root.layout(0,0,size[0],size[1]);
                for(int i=0;i<b.length;i++) {
                    Button target=b[i];String context="font="+font+" width="+size[0]+" compact="+compact+" scale="+scale+" label="+target.getText();
                    assertTrue(context+" preserves left movement",target.getLeft()>=size[0]/2);
                    assertTrue(context,target.getWidth()>=48 && target.getHeight()>=48);
                    assertTrue(context+" stays inside parent",target.getTop()>=0 && target.getRight()<=size[0] && target.getBottom()<=size[1]);
                    if(i<2) {
                        assertEquals(context,"",target.getText().toString());
                        assertEquals(i==0?a.getString(R.string.swing):a.getString(R.string.parry),target.getContentDescription());
                        assertNotNull(context,target.getCompoundDrawables()[0]);
                        assertEquals(context,target.getWidth(),target.getHeight());
                        assertTrue(context+" primary larger than utility",target.getWidth()>b[4].getWidth());
                    }
                    if(i==2) {
                        assertEquals(context,1,target.getLayout().getLineCount());
                        assertEquals(context,0,target.getLayout().getEllipsisCount(0));
                        assertTrue(context+" fits complete label",target.getLayout().getLineWidth(0)<=target.getWidth()-target.getCompoundPaddingLeft()-target.getCompoundPaddingRight()+1);
                    }
                }
                for(int i=0;i<b.length;i++)for(int j=0;j<i;j++)
                    assertFalse("actual targets cannot overlap",android.graphics.Rect.intersects(
                        new android.graphics.Rect(b[i].getLeft(),b[i].getTop(),b[i].getRight(),b[i].getBottom()),
                        new android.graphics.Rect(b[j].getLeft(),b[j].getTop(),b[j].getRight(),b[j].getBottom())));
                assertTrue("Parry remains nearly equal to Swing",b[1].getWidth()>=b[0].getWidth()*.9);
                for(int i:new int[]{0,1,2,4})
                    assertTrue("broad right-middle look lane stays open",b[i].getTop()>=size[1]*.5);
                String evidence=System.getenv("HORDE_COMPACT_HUD_EVIDENCE_DIR");
                if(evidence!=null && font==1.7f && !compact && scale==100 && size[0]!=320) {
                    root.setBackgroundColor(Color.TRANSPARENT);
                    for(int id:new int[]{R.id.scene_surface,R.id.menu_scrim,R.id.diagnostics_panel,
                            R.id.rt_status,R.id.vitality_status,R.id.menu_button})root.findViewById(id).setVisibility(View.GONE);
                    // Component evidence only. An absent context prompt leaves
                    // the deliberate action cluster unchanged in normal play.
                    b[3].setVisibility(View.GONE);
                    Bitmap image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);
                    Canvas canvas=new Canvas(image);Paint background=new Paint();
                    for(int y=0;y<size[1];y+=40) {
                        background.setColor((y/40)%2==0?0xFF6F7775:0xFFACA38B);
                        canvas.drawRect(0,y,size[0],y+40,background);
                    }
                    root.draw(canvas);
                    File file=new File(evidence,"component-"+size[0]+"x"+size[1]+".png");
                    assertTrue(file.getParentFile().isDirectory() || file.getParentFile().mkdirs());
                    try(FileOutputStream stream=new FileOutputStream(file)) {assertTrue(image.compress(Bitmap.CompressFormat.PNG,100,stream));}
                    for(boolean bright:new boolean[]{false,true}) {
                        canvas.drawColor(bright?0xFFEDE1BA:0xFF101A17);
                        root.draw(canvas);
                        File contrast=new File(evidence,"component-"+(bright?"bright-":"dark-")+size[0]+"x"+size[1]+".png");
                        try(FileOutputStream stream=new FileOutputStream(contrast)) {assertTrue(image.compress(Bitmap.CompressFormat.PNG,100,stream));}
                    }
                }
            }
        } finally {a.getResources().updateConfiguration(original,a.getResources().getDisplayMetrics());}
    }
    @Test public void shortResizableWindowsKeepEveryContextAndActionTargetReachable() throws Exception {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        Configuration original=new Configuration(a.getResources().getConfiguration());
        try {
            for(float font:new float[]{1,2})for(int inset:new int[]{0,16})for(int top:new int[]{0,16})for(int scale:new int[]{100,110}) {
                Configuration config=new Configuration(original);config.densityDpi=160;config.fontScale=font;
                a.getResources().updateConfiguration(config,a.getResources().getDisplayMetrics());
                FrameLayout root=(FrameLayout)LayoutInflater.from(a).inflate(R.layout.activity_main,null,false);
                Button[] b={root.findViewById(R.id.attack_button),root.findViewById(R.id.parry_button),root.findViewById(R.id.dodge_button),
                    root.findViewById(R.id.interact_button),root.findViewById(R.id.toggle_held_light_pose_button)};
                UiControlLayout.apply(a,new InterfacePreferences.Values(false,scale,70,false,false),b[0],b[1],b[2],b[3],b[4],inset,24,320,320-top);
                b[3].setText(R.string.chest_locked_until_lich_defeated);
                for(Button target:b)target.setVisibility(View.VISIBLE);
                root.measure(View.MeasureSpec.makeMeasureSpec(320,View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(320,View.MeasureSpec.EXACTLY));root.layout(0,0,320,320);
                for(int j=0;j<b.length;j++) {
                    Button target=b[j];String message="font="+font+" inset="+inset+" scale="+scale+" id="+target.getId()+" top="+target.getTop();
                    assertTrue(message,target.getTop()>=top && target.getBottom()<=296 && target.getLeft()>=160 && target.getRight()<=320-inset);
                    assertTrue(message,target.getWidth()>=48 && target.getHeight()>=48);
                    if(j==3) {
                        assertTrue(message,target.getLayout().getHeight()<=target.getHeight()-target.getCompoundPaddingTop()-target.getCompoundPaddingBottom());
                        for(int line=0;line<target.getLayout().getLineCount();line++)assertEquals(message,0,target.getLayout().getEllipsisCount(line));
                    }
                    for(int k=0;k<j;k++)assertFalse(message,android.graphics.Rect.intersects(
                        new android.graphics.Rect(target.getLeft(),target.getTop(),target.getRight(),target.getBottom()),
                        new android.graphics.Rect(b[k].getLeft(),b[k].getTop(),b[k].getRight(),b[k].getBottom())));
                }
            }
        } finally {a.getResources().updateConfiguration(original,a.getResources().getDisplayMetrics());}
    }
}
