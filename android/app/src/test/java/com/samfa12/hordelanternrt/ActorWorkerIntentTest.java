package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Intent;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class ActorWorkerIntentTest {
    @Test public void explicitModeRequiresBothPrivateBuildAndDebuggableApp() {
        Intent i=new Intent().putExtra(ActorWorkerIntent.EXTRA,"worker");
        assertEquals(1,ActorWorkerIntent.mode(i,true,true));
        assertEquals(-1,ActorWorkerIntent.mode(i,false,true));
        assertEquals(-1,ActorWorkerIntent.mode(i,true,false));
        assertEquals(-1,ActorWorkerIntent.mode(i,false,false));
        assertEquals(0,ActorWorkerIntent.mode(i.putExtra(ActorWorkerIntent.EXTRA,"serial"),true,true));
    }
    @Test public void missingMalformedAndWrongTypedRequestsDoNotChangeMode() {
        assertEquals(-1,ActorWorkerIntent.mode(null,true,true));
        assertEquals(-1,ActorWorkerIntent.mode(new Intent(),true,true));
        for(String s:new String[]{"","on","WORKER"," worker","worker ","serial\n"})
            assertEquals(-1,ActorWorkerIntent.mode(new Intent().putExtra(ActorWorkerIntent.EXTRA,s),true,true));
        assertEquals(-1,ActorWorkerIntent.mode(new Intent().putExtra(ActorWorkerIntent.EXTRA,true),true,true));
        assertEquals(-1,ActorWorkerIntent.mode(new Intent().putExtra(ActorWorkerIntent.EXTRA,1),true,true));
    }
}
