package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.media.AudioFormat;
import android.media.AudioTrack;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.RealObject;
import org.robolectric.shadows.ShadowAudioTrack;
import org.robolectric.shadow.api.Shadow;
import org.robolectric.util.ReflectionHelpers;

// Android API admission/gain tests; this shadow supplies no device timing.
@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, shadows = AmbienceOutputBufferTest.BufferTrackShadow.class)
public final class AmbienceOutputBufferTest {
    @Implements(AudioTrack.class)
    public static final class BufferTrackShadow extends ShadowAudioTrack {
        @RealObject private AudioTrack track;
        private int effectiveFrames = -1;
        float left = -1, right = -1;
        int gainCalls, gainResult = AudioTrack.SUCCESS, flushCalls;
        @Implementation protected int getBufferCapacityInFrames() {
            return ReflectionHelpers.getField(track, "mNativeBufferSizeInFrames");
        }
        @Implementation protected int getBufferSizeInFrames() {
            return effectiveFrames < 0 ? getBufferCapacityInFrames() : effectiveFrames;
        }
        @Implementation protected int setBufferSizeInFrames(int requested) {
            effectiveFrames = Math.min(Math.max(1, requested), getBufferCapacityInFrames());
            return effectiveFrames;
        }
        @Implementation protected int setStereoVolume(float left, float right) {
            ++gainCalls;
            this.left = left; this.right = right;
            return gainResult;
        }
        @Implementation protected void flush() { ++flushCalls; }
    }
    @Test public void platformMinimumAndStereoFloatFormatArePreserved() {
        AudioTrack output = HordeAmbiencePlayback.createOutput(3840 * 8);
        try {
            assertEquals(3840, HordeAmbiencePlayback.queueCapacityFrames(output, 3840 * 8));
            assertEquals(3840, output.getBufferCapacityInFrames());
            assertEquals(48000, output.getSampleRate());
            assertEquals(2, output.getChannelCount());
            assertEquals(AudioFormat.ENCODING_PCM_FLOAT, output.getAudioFormat());
        } finally { output.release(); }
    }
    @Test public void smallMinimumKeepsThirtyMillisecondAllocation() {
        AudioTrack output = HordeAmbiencePlayback.createOutput(480 * 8);
        try { assertEquals(1440, HordeAmbiencePlayback.queueCapacityFrames(output, 480 * 8)); }
        finally { output.release(); }
    }
    @Test(expected=IllegalArgumentException.class) public void unavailableMinimumIsRejected() {
        HordeAmbiencePlayback.createOutput(-1);
    }
    @Test(expected=IllegalArgumentException.class) public void excessiveMinimumIsRejectedWithoutShrinking() {
        HordeAmbiencePlayback.createOutput(12001 * 8);
    }
    @Test(expected=IllegalStateException.class) public void insufficientEffectiveCapacityFails() {
        AudioTrack output = HordeAmbiencePlayback.createOutput(3840 * 8);
        try {
            output.setBufferSizeInFrames(1440);
            HordeAmbiencePlayback.queueCapacityFrames(output, 3840 * 8);
        } finally { output.release(); }
    }
    @Test public void spatialGainAndSuspendRestoreKeepIndependentChannels() {
        AudioTrack output = HordeAmbiencePlayback.createOutput(480 * 8);
        BufferTrackShadow shadow = Shadow.extract(output);
        try {
            HordeAmbiencePlayback.applyStereoGain(output, .08f, .32f);
            assertEquals(.08f, shadow.left, 0); assertEquals(.32f, shadow.right, 0);
            HordeAmbiencePlayback.applyStereoGain(output, 0, 0);
            assertEquals(0, shadow.left, 0); assertEquals(0, shadow.right, 0);
            HordeAmbiencePlayback.applyStereoGain(output, .16f, .64f);
            assertEquals(.16f, shadow.left, 0); assertEquals(.64f, shadow.right, 0);
            final int calls = shadow.gainCalls;
            float[] invalid = {Float.NaN, Float.POSITIVE_INFINITY, -.01f, 1.01f};
            for (float bad : invalid) {
                assertThrows(IllegalArgumentException.class,
                        () -> HordeAmbiencePlayback.applyStereoGain(output, bad, .2f));
                assertThrows(IllegalArgumentException.class,
                        () -> HordeAmbiencePlayback.applyStereoGain(output, .2f, bad));
            }
            assertEquals(calls, shadow.gainCalls);
            assertThrows(IllegalArgumentException.class,
                    () -> HordeAmbiencePlayback.validateControl(.2f, .3f, -1));
            shadow.gainResult = AudioTrack.ERROR;
            assertThrows(IllegalStateException.class,
                    () -> HordeAmbiencePlayback.applyStereoGain(output, .2f, .3f));
        } finally { output.release(); }
    }
    @Test public void staleNativeControlCannotRestartSuspendedOrResetOutput() {
        AudioTrack output = HordeAmbiencePlayback.createOutput(480 * 8);
        BufferTrackShadow shadow = Shadow.extract(output);
        AmbienceOutputGate gate = new AmbienceOutputGate(output);
        try {
            assertTrue(gate.reconcile(false, false, true, 7, 7, .1f, .4f));
            assertEquals(AudioTrack.PLAYSTATE_PLAYING, output.getPlayState());
            // The UI publishes pause after native polling, before sink transition.
            assertFalse(gate.reconcile(false, true, true, 7, 7, .1f, .4f));
            assertEquals(AudioTrack.PLAYSTATE_PAUSED, output.getPlayState());
            assertEquals(0, shadow.left, 0); assertEquals(0, shadow.right, 0);
            assertEquals(0, shadow.flushCalls); // Ordinary pause preserves accepted PCM.
            assertFalse(gate.reconcile(false, false, true, 7, 8, .1f, .4f));
            assertEquals(AudioTrack.PLAYSTATE_PAUSED, output.getPlayState());
            gate.reset(); // New native epoch is reconciled only after owned flush.
            assertEquals(1, shadow.flushCalls);
            assertFalse(gate.reconcile(true, false, true, 8, 8, .2f, .8f));
            assertTrue(gate.reconcile(false, false, true, 8, 8, .2f, .8f));
            assertEquals(.2f, shadow.left, 0); assertEquals(.8f, shadow.right, 0);
            assertFalse(gate.reconcile(false, false, false, 8, 8, .2f, .8f));
            assertEquals(AudioTrack.PLAYSTATE_PAUSED, output.getPlayState());
            assertEquals(1, shadow.flushCalls); // Focus loss also retains accepted content.
            assertTrue(gate.reconcile(false, false, true, 8, 8, .2f, .8f));
            assertEquals(1, shadow.flushCalls);
        } finally { output.release(); }
    }
}
