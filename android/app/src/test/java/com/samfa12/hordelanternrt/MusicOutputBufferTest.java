package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

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
import org.robolectric.util.ReflectionHelpers;

// Host Android API contracts only; actual sink cadence/underruns need the phone.
@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, shadows = MusicOutputBufferTest.BufferTrackShadow.class)
public final class MusicOutputBufferTest {
    // Robolectric 4.14 does not implement these native buffer-size queries.
    // Model only their documented capacity/effective-size distinction, not
    // audio hardware, timing, writing or underruns.
    @Implements(AudioTrack.class)
    public static final class BufferTrackShadow extends ShadowAudioTrack {
        @RealObject private AudioTrack track;
        private int effectiveFrames = -1;

        @Implementation
        protected int getBufferCapacityInFrames() {
            return ReflectionHelpers.getField(track, "mNativeBufferSizeInFrames");
        }
        @Implementation
        protected int getBufferSizeInFrames() {
            return effectiveFrames < 0 ? getBufferCapacityInFrames() : effectiveFrames;
        }
        @Implementation
        protected int setBufferSizeInFrames(int requested) {
            effectiveFrames = Math.min(Math.max(1, requested), getBufferCapacityInFrames());
            return effectiveFrames;
        }
    }
    @Test
    public void largerPlatformMinimumIsNotShrunkToThirtyMilliseconds() {
        int minimumFrames = 3840;
        AudioTrack output = HordeMusicPlayback.createOutput(minimumFrames * 8);
        try {
            assertTrue(output.getBufferSizeInFrames() >= minimumFrames);
            assertEquals(output.getBufferSizeInFrames(), HordeMusicPlayback.queueCapacityFrames(output, minimumFrames * 8));
            assertEquals(output.getBufferCapacityInFrames(), output.getBufferSizeInFrames());
            assertEquals(48000, output.getSampleRate());
            assertEquals(2, output.getChannelCount());
            assertEquals(AudioFormat.ENCODING_PCM_FLOAT, output.getAudioFormat());
        } finally { output.release(); }
    }

    @Test
    public void smallerMinimumRetainsTheExistingBoundedRequest() {
        AudioTrack output = HordeMusicPlayback.createOutput(480 * 8);
        try {
            assertEquals(1440, output.getBufferSizeInFrames());
            assertEquals(output.getBufferCapacityInFrames(), output.getBufferSizeInFrames());
        } finally { output.release(); }
    }

    @Test(expected = IllegalArgumentException.class)
    public void unavailableMinimumFailsWithoutCreatingOutput() {
        HordeMusicPlayback.createOutput(-1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void excessivePlatformMinimumFailsInsteadOfAllocatingOrShrinking() {
        HordeMusicPlayback.createOutput(12001 * 8);
    }

    @Test(expected = IllegalStateException.class)
    public void insufficientEffectiveQueueFailsInsteadOfIgnoringTheMinimum() {
        AudioTrack output = HordeMusicPlayback.createOutput(3840 * 8);
        try {
            output.setBufferSizeInFrames(1440); // Explicitly injected defective sink state.
            HordeMusicPlayback.queueCapacityFrames(output, 3840 * 8);
        } finally { output.release(); }
    }
}
