package com.samfa12.hordelanternrt;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;

/** Explicit Debug audition only; no asset is supplied by or copied into the APK. */
final class PrivateLichRevivalAudio {
    static final int EVENT_TYPE=36;
    static final String INTENT_OPT_IN="horde_debug_lich_revival_audio";
    static final String SHA256="1e1891010f4c89e6f20ec5fba90191aa0f0999c0bd95a4df3fe36659beb9e521";
    static final long BYTES=350252;
    static final float MIX_GAIN=.55f;
    interface Sink { int start(float left,float right); void gain(int stream,float left,float right); void stop(int stream); }
    private final Sink sink;
    private long lifetime,lastSequence;
    private boolean seenLifetime;
    private int stream;
    private float left,right;
    PrivateLichRevivalAudio(Sink sink){this.sink=sink;}
    static boolean enabled(boolean debug,boolean requested){return debug && requested;}
    static File file(File appDirectory){return new File(appDirectory,"debug_audio/lich_revival.wav");}
    static boolean matchesPinnedIdentity(long size,String hash){return size==BYTES && SHA256.equals(hash);}
    static String validate(File appDirectory) {
        File wave=file(appDirectory);
        if(!wave.isFile())return "Missing private debug_audio/lich_revival.wav; no substitute is used.";
        if(wave.length()!=BYTES)return "Private revival WAV size differs from the approved draft.";
        try(FileInputStream in=new FileInputStream(wave)) {
            MessageDigest hash=MessageDigest.getInstance("SHA-256");byte[] bytes=new byte[16384];int read;
            while((read=in.read(bytes))!=-1)hash.update(bytes,0,read);
            StringBuilder hex=new StringBuilder();for(byte value:hash.digest())hex.append(String.format(java.util.Locale.ROOT,"%02x",value&255));
            return matchesPinnedIdentity(wave.length(),hex.toString())?null:"Private revival WAV checksum differs from the approved mono 48 kHz PCM16 draft.";
        } catch(IOException | java.security.NoSuchAlgorithmException unavailable) {return "Private revival WAV could not be verified: "+unavailable.getClass().getSimpleName();}
    }
    private boolean observeLifetime(long next) {
        if(next<0 || seenLifetime && next<lifetime)return false;
        if(seenLifetime && lifetime==next)return true;
        stop();seenLifetime=true;lifetime=next;lastSequence=0;return true;
    }
    static float volume(float user,float duck) {
        return Float.isFinite(user) && Float.isFinite(duck)?Math.max(0,Math.min(1,user))*Math.max(0,Math.min(1,duck)):0;
    }
    boolean start(long metadata,long stereo,long generation,boolean eligible,float user,float duck) {
        if(!observeLifetime(generation))return false;
        if((metadata&255)!=EVENT_TYPE || ((metadata>>>8)&255)!=3 || ((metadata>>>16)&255)!=1)return false;
        long sequence=(metadata>>>32)&0xffffffffL;
        if(sequence==0 || sequence<=lastSequence)return false;
        lastSequence=sequence;stop();
        float l=Float.intBitsToFloat((int)stereo),r=Float.intBitsToFloat((int)(stereo>>>32));
        if(!eligible || !Float.isFinite(l) || !Float.isFinite(r) || volume(user,duck)<=0)return false;
        left=Math.max(0,Math.min(1,l));right=Math.max(0,Math.min(1,r));
        if(left==0 && right==0)return false;
        float gain=MIX_GAIN*volume(user,duck);stream=sink.start(left*gain,right*gain);return stream!=0;
    }
    void update(long generation,boolean eligible,float user,float duck) {
        if(!observeLifetime(generation))return;
        float gain=MIX_GAIN*volume(user,duck);
        if(!eligible || gain<=0){stop();return;}
        if(stream!=0)sink.gain(stream,left*gain,right*gain);
    }
    void stop(){if(stream!=0)sink.stop(stream);stream=0;}
}
