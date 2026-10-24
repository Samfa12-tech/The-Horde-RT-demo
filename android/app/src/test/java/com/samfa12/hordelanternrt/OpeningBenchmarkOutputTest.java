package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.os.Bundle;
import android.view.Gravity;
import android.view.SurfaceView;
import android.view.View;
import android.widget.FrameLayout;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

/** Native View layout and protocol checks; supplied holder observations are not device evidence. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class OpeningBenchmarkOutputTest {
    private FrameLayout parent;
    private SurfaceView surface;
    private View hud;
    private FrameLayout.LayoutParams original;
    private OpeningBenchmarkOutput transaction;
    private static OpeningBenchmarkOutput.Command command(long generation,long sequence,int operation) {
        return OpeningBenchmarkOutput.Command.decode(new long[]{7,31,generation,sequence,operation,
            operation==1?1280:0,operation==1?720:0,2});
    }
    private void layout(int width,int height) {
        parent.measure(View.MeasureSpec.makeMeasureSpec(width,View.MeasureSpec.EXACTLY),
            View.MeasureSpec.makeMeasureSpec(height,View.MeasureSpec.EXACTLY));
        parent.layout(0,0,width,height);
        if(transaction!=null)transaction.observedLayoutOrPost(transaction.current());
    }
    @Before public void setup() {
        parent=new FrameLayout(RuntimeEnvironment.getApplication());surface=new SurfaceView(parent.getContext());
        FrameLayout.LayoutParams params=new FrameLayout.LayoutParams(-1,-1,Gravity.TOP|Gravity.START);
        params.setMargins(7,11,13,17);params.setMarginStart(19);params.setMarginEnd(23);
        parent.addView(surface,params);hud=new View(parent.getContext());parent.addView(hud,new FrameLayout.LayoutParams(-1,-1));
        layout(411,891);original=OpeningBenchmarkOutput.snapshot(surface);
        transaction=new OpeningBenchmarkOutput(7,31,original);
    }
    @Test public void fixedRectangleFitsAndCentersInBothOrientationsWithinPixelRounding() {
        for(int[] size:new int[][]{{411,891},{804,381},{1280,720},{333,333}}) {
            int[] fit=OpeningBenchmarkOutput.fit(size[0],size[1]);assertNotNull(fit);
            assertTrue(fit[0]<=size[0] && fit[1]<=size[1]);
            assertTrue(Math.abs((long)fit[0]*720-(long)fit[1]*1280)<=1280);
        }
        assertNull(OpeningBenchmarkOutput.fit(0,700));
        OpeningBenchmarkOutput.Command apply=command(31,1,1);
        assertTrue(transaction.begin(apply,31,false,surface));layout(411,891);
        assertTrue(OpeningBenchmarkOutput.aspectPreserved(surface));
        assertEquals(411,hud.getWidth());assertEquals(891,hud.getHeight());
    }
    @Test public void savedClientKeepsExactIndependentLayoutPolicyIncludingUndefinedRelativeEdges() {
        for(int relative=0;relative<4;relative++)for(int direction=0;direction<2;direction++) {
            FrameLayout.LayoutParams policy=new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM|Gravity.START);
            policy.setMargins(7,-11,13,17);policy.setLayoutDirection(direction);
            if((relative&1)!=0)policy.setMarginStart(-19);
            if((relative&2)!=0)policy.setMarginEnd(23);
            OpeningBenchmarkClient client=new OpeningBenchmarkClient(7,31,true,policy);
            Bundle saved=new Bundle();client.save(saved);
            policy.width=999;policy.leftMargin=999; // Neither caller nor getter can mutate retained policy.
            FrameLayout.LayoutParams retained=client.originalLayout();retained.height=999;
            OpeningBenchmarkClient restored=OpeningBenchmarkClient.restore(saved);
            assertNotNull(restored);assertEquals(7,restored.request);assertEquals(31,restored.origin);assertTrue(restored.provisional);
            FrameLayout.LayoutParams before=client.originalLayout(),after=restored.originalLayout();
            String context="relativeMask="+relative+" direction="+direction+" before="+describeLayout(before)+" after="+describeLayout(after);
            assertTrue(context,OpeningBenchmarkOutput.sameLayout(after,before));
            assertEquals(context,before.isMarginRelative(),after.isMarginRelative());
            // A currently equal getter value alone would hide lost undefined
            // relative edges. Change absolutes and require the same future policy.
            before.leftMargin=101;after.leftMargin=101;before.rightMargin=103;after.rightMargin=103;
            assertEquals(context+" future start",before.getMarginStart(),after.getMarginStart());
            assertEquals(context+" future end",before.getMarginEnd(),after.getMarginEnd());
        }
    }
    private static String describeLayout(FrameLayout.LayoutParams value) {
        FrameLayout.LayoutParams p=new FrameLayout.LayoutParams(value);
        return "size="+p.width+"x"+p.height+" gravity="+p.gravity+" absolute="+p.leftMargin+","+p.topMargin+","+p.rightMargin+","+p.bottomMargin+
            " relative="+p.getMarginStart()+","+p.getMarginEnd()+" isRelative="+p.isMarginRelative()+" direction="+p.getLayoutDirection();
    }
    @Test public void customSavedClientNeedsNoLayoutAndRecoveredOutputRequiresRealRestoreAcknowledgement() {
        Bundle saved=new Bundle();new OpeningBenchmarkClient(7,31,false,null).save(saved);
        OpeningBenchmarkClient custom=OpeningBenchmarkClient.restore(saved);
        assertNotNull(custom);assertFalse(custom.provisional);assertNull(custom.originalLayout());
        assertTrue(custom.matches(OpeningBenchmarkState.decode(new long[]{7,42,4,3,0,31})));
        assertFalse(custom.matches(OpeningBenchmarkState.decode(new long[]{8,42,4,3,0,31})));
        transaction=new OpeningBenchmarkOutput(7,31,original,true);assertFalse(transaction.canRelease());
        OpeningBenchmarkOutput.Command restore=command(42,2,2);
        assertTrue(transaction.begin(restore,42,true,surface));layout(411,891);
        assertFalse(transaction.canRelease());
        OpeningBenchmarkOutput.Ack ack=transaction.observe(restore,42,true,surface,true,surface.getWidth(),surface.getHeight());
        assertNotNull(ack);transaction.acknowledged(ack);assertTrue(transaction.canRelease());
    }
    @Test public void applyAcknowledgesObservedBufferOnlyAfterActualLayoutAndValidSurface() {
        OpeningBenchmarkOutput.Command apply=command(31,1,1);
        assertTrue(transaction.begin(apply,31,false,surface));
        assertNull(transaction.observe(apply,31,false,surface,true,1280,720));
        layout(411,891);
        assertNull(transaction.observe(apply,31,false,surface,true,1024,576));
        assertNull(transaction.observe(apply,31,false,surface,false,1280,720));
        OpeningBenchmarkOutput.Ack ack=transaction.observe(apply,31,false,surface,true,1280,720);
        assertNotNull(ack);assertTrue(ack.success && ack.layoutRestoredOrAspectPreserved);
        assertEquals(1280,ack.width);assertEquals(720,ack.height);
        transaction.acknowledged(ack);assertFalse(transaction.canRelease());
        assertNull(transaction.observe(apply,31,false,surface,true,1280,720));
    }
    @Test public void restorationClonesEveryOriginalLayoutFieldAndUsesNewOrientationPixels() {
        OpeningBenchmarkOutput.Command apply=command(31,1,1);
        transaction.begin(apply,31,false,surface);layout(411,891);
        transaction.acknowledged(transaction.observe(apply,31,false,surface,true,1280,720));
        OpeningBenchmarkOutput.Command restore=command(42,2,2);
        assertTrue(transaction.begin(restore,42,true,surface));layout(804,381);
        assertTrue(OpeningBenchmarkOutput.sameLayout(surface.getLayoutParams(),original));
        assertNotSame(original,surface.getLayoutParams());
        assertNull(transaction.observe(restore,42,true,surface,true,1280,720));
        OpeningBenchmarkOutput.Ack ack=transaction.observe(restore,42,true,surface,true,surface.getWidth(),surface.getHeight());
        assertNotNull(ack);assertTrue(ack.success && ack.layoutRestoredOrAspectPreserved);
        transaction.acknowledged(ack);assertTrue(transaction.canRelease());
        assertEquals(804,hud.getWidth());assertEquals(381,hud.getHeight());
    }
    @Test public void staleCallbacksAndSequencesCannotAcknowledgeOrReplaceSuccessor() {
        OpeningBenchmarkOutput.Command apply=command(31,1,1),restore=command(42,2,2);
        transaction.begin(apply,31,false,surface);layout(411,891);
        assertTrue(transaction.begin(restore,42,true,surface));layout(804,381);
        assertNull(transaction.observe(apply,42,false,surface,true,1280,720));
        assertNull(transaction.observe(restore,31,false,surface,true,surface.getWidth(),surface.getHeight()));
        assertFalse(transaction.begin(command(42,1,1),42,false,surface));
        assertFalse(transaction.begin(restore,42,false,surface));
        OpeningBenchmarkOutput.Command wrong=OpeningBenchmarkOutput.Command.decode(new long[]{8,31,42,3,1,1280,720,2});
        assertFalse(transaction.begin(wrong,42,false,surface));
        assertTrue(OpeningBenchmarkOutput.sameLayout(surface.getLayoutParams(),original));
    }
    @Test public void interruptedApplyFailsWithoutLosingOriginalAndLateApplyCannotResizeAfterCancel() {
        OpeningBenchmarkOutput.Command apply=command(31,1,1);
        transaction.begin(apply,31,false,surface);layout(411,891);
        OpeningBenchmarkOutput.Ack failed=transaction.observe(apply,31,true,surface,true,1280,720);
        assertNotNull(failed);assertFalse(failed.success);transaction.acknowledged(failed);
        assertFalse(transaction.canRelease());
        OpeningBenchmarkOutput.Command restore=command(31,2,2);
        transaction.begin(restore,31,true,surface);layout(411,891);
        OpeningBenchmarkOutput.Ack restored=transaction.observe(restore,31,true,surface,true,surface.getWidth(),surface.getHeight());
        transaction.acknowledged(restored);assertTrue(transaction.canRelease());
        OpeningBenchmarkOutput.Command lateApply=command(31,3,1);
        transaction.begin(lateApply,31,true,surface);
        OpeningBenchmarkOutput.Ack rejected=transaction.observe(lateApply,31,true,surface,true,surface.getWidth(),surface.getHeight());
        assertNotNull(rejected);assertFalse(rejected.success);
        assertTrue(OpeningBenchmarkOutput.sameLayout(surface.getLayoutParams(),original));
    }
    @Test public void noOpRestoreHasAnExplicitObservedAcknowledgementWithoutResizeCallback() {
        OpeningBenchmarkOutput.Command restore=command(31,1,2);
        assertTrue(transaction.begin(restore,31,true,surface));
        assertNull(transaction.observe(restore,31,true,surface,true,surface.getWidth(),surface.getHeight()));
        transaction.observedLayoutOrPost(restore); // Explicit posted check even when no resize callback occurs.
        OpeningBenchmarkOutput.Ack ack=transaction.observe(restore,31,true,surface,true,surface.getWidth(),surface.getHeight());
        assertNotNull(ack);assertTrue(ack.success);transaction.acknowledged(ack);assertTrue(transaction.canRelease());
    }
    @Test public void malformedOutputCommandCannotReachView() {
        assertNull(OpeningBenchmarkOutput.Command.decode(new long[7]));
        for(long[] invalid:new long[][]{{7,31,31,0,1,1280,720,0},{7,31,0,1,1,1280,720,0},
                {7,0,31,1,1,1280,720,0},{7,31,31,1,0,0,0,0},{7,31,31,1,3,1280,720,0},
                {7,31,31,1,1,-1,720,0}})
            assertNull(OpeningBenchmarkOutput.Command.decode(invalid));
    }
}
