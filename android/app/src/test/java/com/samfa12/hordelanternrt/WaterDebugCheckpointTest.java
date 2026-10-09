package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import android.content.Intent;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
@RunWith(RobolectricTestRunner.class) @Config(sdk = 34)
public final class WaterDebugCheckpointTest {
    @Test public void frozenWaterViewsMatchSharedAngles() {
        String[] names = {"water-torch-near", "water-torch-far", "water-torch-oblique", "water-torch-catchment"};
        float[][] poses = {{-1.5707963f,-0.16f},{-1.5707963f,-0.10f},{-1.18055f,-0.20f},{-1.94055f,-0.32f}};
        for (int i=0;i<4;i++) {
            assertEquals(180+i,MainActivity.checkpointId(names[i]));
            assertArrayEquals(poses[i],MainActivity.developmentCheckpointViewPose(180+i),0.000001f);
        }
        assertEquals(-1,MainActivity.checkpointId("water-torch-unknown"));
        assertNull(MainActivity.developmentCheckpointViewPose(184));
    }
    @Test public void waterOverrideRequiresOnlyAnExplicitDebugWaterCapture() {
        Intent intent=new Intent().putExtra("horde.debug.water_quality",1);
        assertEquals(1,MainActivity.admittedDebugWaterQuality(intent,true,180,true,false,false,false));
        intent.putExtra("horde.debug.water_quality",2);
        assertEquals(2,MainActivity.admittedDebugWaterQuality(intent,true,183,true,false,false,false));
        assertEquals(-1,MainActivity.admittedDebugWaterQuality(intent,false,180,true,false,false,false));
        assertEquals(-1,MainActivity.admittedDebugWaterQuality(intent,true,2,true,false,false,false));
        assertEquals(-1,MainActivity.admittedDebugWaterQuality(intent,true,180,false,false,false,false));
        assertEquals(-1,MainActivity.admittedDebugWaterQuality(intent,true,180,true,true,false,false));
        assertEquals(-1,MainActivity.admittedDebugWaterQuality(intent,true,180,true,false,true,false));
        assertEquals(-1,MainActivity.admittedDebugWaterQuality(intent,true,180,true,false,false,true));
        for(int invalid:new int[]{-1,0,3,99}) {
            intent.putExtra("horde.debug.water_quality",invalid);
            assertEquals(-1,MainActivity.admittedDebugWaterQuality(intent,true,180,true,false,false,false));
        }
    }
    @Test public void waterCapturePreservesFireAndOtherSavedChoices() {
        GraphicsPreferences.Values current=new GraphicsPreferences.Values(33,1,0,30,true,1,true,1);
        GraphicsPreferences.Values high=MainActivity.withDebugWaterQuality(current,2,50);
        assertEquals(50,high.scale); assertEquals(2,high.water); assertEquals(current.fire,high.fire);
        assertEquals(current.cap,high.cap); assertEquals(current.glassEnabled,high.glassEnabled);
        assertEquals(current.shadow,high.shadow); assertEquals(current.mistEnabled,high.mistEnabled);
        assertEquals(current.dust,high.dust); assertEquals(33,current.scale); assertEquals(1,current.water);
        assertEquals(33,MainActivity.withDebugWaterQuality(current,1,-1).scale);
        assertNull(MainActivity.withDebugWaterQuality(current,0,50));
    }
}
