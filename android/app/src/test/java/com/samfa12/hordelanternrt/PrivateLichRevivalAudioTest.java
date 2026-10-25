package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.io.File;
import java.io.RandomAccessFile;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public final class PrivateLichRevivalAudioTest {
    @Rule public TemporaryFolder temporary=new TemporaryFolder();
    private static final class Sink implements PrivateLichRevivalAudio.Sink {
        int starts,stops,updates;float left,right;
        public int start(float l,float r){starts++;left=l;right=r;return starts;}
        public void gain(int stream,float l,float r){updates++;left=l;right=r;}
        public void stop(int stream){stops++;}
    }
    private static long event(long sequence,int source,int target){return 36L|((long)source<<8)|((long)target<<16)|(sequence<<32);}
    private static long pan(float l,float r){return (Float.floatToRawIntBits(l)&0xffffffffL)|((long)Float.floatToRawIntBits(r)<<32);}
    @Test public void explicitDebugOptInAndExactPrivateIdentityAreRequired()throws Exception {
        assertTrue(PrivateLichRevivalAudio.enabled(true,true));
        assertFalse(PrivateLichRevivalAudio.enabled(false,true));assertFalse(PrivateLichRevivalAudio.enabled(true,false));
        assertTrue(PrivateLichRevivalAudio.matchesPinnedIdentity(350252,PrivateLichRevivalAudio.SHA256));
        assertFalse(PrivateLichRevivalAudio.matchesPinnedIdentity(350253,PrivateLichRevivalAudio.SHA256));
        assertFalse(PrivateLichRevivalAudio.matchesPinnedIdentity(350252,"different"));
        File app=temporary.newFolder("app");assertEquals(new File(app,"debug_audio/lich_revival.wav"),PrivateLichRevivalAudio.file(app));
        assertTrue(PrivateLichRevivalAudio.validate(app).contains("Missing"));
        File wave=PrivateLichRevivalAudio.file(app);assertTrue(wave.getParentFile().mkdirs());
        try(RandomAccessFile data=new RandomAccessFile(wave,"rw")){data.setLength(7);}
        assertTrue(PrivateLichRevivalAudio.validate(app).contains("size differs"));
        try(RandomAccessFile data=new RandomAccessFile(wave,"rw")){data.setLength(350252);data.seek(100);data.write(1);}
        assertTrue(PrivateLichRevivalAudio.validate(app).contains("checksum differs"));
    }
    @Test public void oneSourceSequenceRetainsOriginalPanWhileSavedVolumeAndSharedDuckChange() {
        Sink sink=new Sink();PrivateLichRevivalAudio cue=new PrivateLichRevivalAudio(sink);
        assertFalse(cue.start(event(1,4,1),pan(.8f,.2f),1,true,.7f,1));
        assertFalse(cue.start(event(1,3,4),pan(.8f,.2f),1,true,.7f,1));
        assertTrue(cue.start(event(1,3,1),pan(.8f,.2f),1,true,.7f,1));
        assertEquals(.8f*.55f*.7f,sink.left,.00001f);assertEquals(.2f*.55f*.7f,sink.right,.00001f);
        assertFalse(cue.start(event(1,3,1),pan(.2f,.8f),1,true,.7f,1));assertEquals(1,sink.starts);
        cue.update(1,true,.2f,.65f);
        assertEquals(.8f*.55f*.2f*.65f,sink.left,.00001f);assertEquals(.2f*.55f*.2f*.65f,sink.right,.00001f);
        assertEquals(0,sink.stops);
    }
    @Test public void pauseResetAndOldCallbacksCannotRestartOrCancelTheNewOwningLifetime() {
        Sink sink=new Sink();PrivateLichRevivalAudio cue=new PrivateLichRevivalAudio(sink);
        assertTrue(cue.start(event(7,3,1),pan(.5f,.4f),2,true,1,1));
        cue.update(2,false,1,1);assertEquals(1,sink.stops);
        assertFalse(cue.start(event(7,3,1),pan(.5f,.4f),2,true,1,1));
        cue.update(3,true,1,1);assertTrue(cue.start(event(1,3,1),pan(.3f,.6f),3,true,1,1));
        cue.update(2,false,1,1);assertEquals(1,sink.stops);
        assertFalse(cue.start(event(8,3,1),pan(.5f,.4f),2,true,1,1));assertEquals(2,sink.starts);
        cue.stop();assertEquals(2,sink.stops);cue.stop();assertEquals(2,sink.stops);
    }
    @Test public void invalidOrMutedGainFailsClosedWithoutANewDuckClock() {
        Sink sink=new Sink();PrivateLichRevivalAudio cue=new PrivateLichRevivalAudio(sink);
        assertEquals(0,PrivateLichRevivalAudio.volume(Float.NaN,1),0);
        assertEquals(0,PrivateLichRevivalAudio.volume(1,Float.POSITIVE_INFINITY),0);
        assertEquals(1,PrivateLichRevivalAudio.volume(2,2),0);
        assertFalse(cue.start(event(1,3,1),pan(Float.NaN,1),1,true,1,1));
        assertFalse(cue.start(event(2,3,1),pan(1,1),1,true,0,1));
        assertTrue(cue.start(event(3,3,1),pan(1,1),1,true,1,1));
        cue.update(1,true,1,Float.NaN);assertEquals(1,sink.stops);
        assertFalse(cue.start(event(3,3,1),pan(1,1),1,true,1,1));
    }
}
