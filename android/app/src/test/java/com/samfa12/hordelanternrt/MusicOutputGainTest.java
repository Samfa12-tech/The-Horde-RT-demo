package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.media.AudioTrack;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadow.api.Shadow;
import org.robolectric.shadows.ShadowAudioTrack;

// Native-control-to-platform-output boundary only. These checks do not model
// the Android audio device, perceived gain ramps, cadence or owner listening.
@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, shadows = MusicOutputGainTest.GainTrackShadow.class)
public final class MusicOutputGainTest {
    @Implements(AudioTrack.class)
    public static final class GainTrackShadow extends ShadowAudioTrack {
        int updates = 0;
        int result = AudioTrack.SUCCESS;
        float gain = -1.0f;

        @Implementation protected int setVolume(float requested) {
            ++updates;
            if (result == AudioTrack.SUCCESS) gain = requested;
            return result;
        }
    }

    private static long[] packet(long epoch, long suspended, long frames, float gain) {
        return new long[] {1L, epoch, suspended, frames,
                Integer.toUnsignedLong(Float.floatToRawIntBits(gain))};
    }

    @Test public void coherentEnvelopeChangesPreserveUserVolumeEpochAndCursor() {
        AudioTrack output = HordeMusicPlayback.createOutput(480 * 8);
        GainTrackShadow sink = Shadow.extract(output);
        try {
            int savedUserVolume = 70;
            long[] ducked = packet(19L, 0L, 480L, 0.72f);
            long[] original = ducked.clone();
            float applied = HordeMusicPlayback.applyOutputGain(output, savedUserVolume, ducked, -1.0f);
            assertEquals(0.504f, sink.gain, 0.000001f);
            assertArrayEquals(original, ducked);
            assertEquals(70, savedUserVolume);
            // A repeated worker poll is stable; it neither re-applies volume nor
            // mutates/advances the owning native packet's content clock.
            applied = HordeMusicPlayback.applyOutputGain(output, savedUserVolume, ducked, applied);
            assertEquals(1, sink.updates);
            assertArrayEquals(original, ducked);
            applied = HordeMusicPlayback.applyOutputGain(output, 20, ducked, applied);
            assertEquals(0.144f, sink.gain, 0.000001f);
            assertEquals(2, sink.updates);
            // Retry/reward may restore unity in a new native epoch. Restore the
            // newly chosen user volume, never a cached pre-reveal setting.
            long[] restored = packet(20L, 0L, 0L, 1.0f);
            HordeMusicPlayback.applyOutputGain(output, 20, restored, applied);
            assertEquals(0.20f, sink.gain, 0.000001f);
            assertEquals(20L, restored[1]);
            assertEquals(0L, restored[3]);
        } finally { output.release(); }
    }

    @Test public void pausedSinkAcceptsGainWithoutRestartingPlaybackOrAdvancingControl() {
        AudioTrack output = HordeMusicPlayback.createOutput(480 * 8);
        GainTrackShadow sink = Shadow.extract(output);
        try {
            output.play();
            output.pause();
            long[] suspended = packet(7L, 1L, 960L, 0.84f);
            long[] copy = suspended.clone();
            float applied = HordeMusicPlayback.applyOutputGain(output, 50, suspended, -1.0f);
            assertEquals(0.42f, sink.gain, 0.000001f);
            assertEquals(AudioTrack.PLAYSTATE_PAUSED, output.getPlayState());
            assertArrayEquals(copy, suspended);
            HordeMusicPlayback.applyOutputGain(output, 50, suspended, applied);
            assertEquals(1, sink.updates);
        } finally { output.release(); }
    }

    @Test public void mutedUserVolumeStaysMutedAcrossEnvelopeUpdates() {
        AudioTrack output = HordeMusicPlayback.createOutput(480 * 8);
        GainTrackShadow sink = Shadow.extract(output);
        try {
            float applied = HordeMusicPlayback.applyOutputGain(output, 0, packet(1L, 0L, 0L, 0.72f), -1.0f);
            HordeMusicPlayback.applyOutputGain(output, 0, packet(1L, 0L, 480L, 1.0f), applied);
            assertEquals(0.0f, sink.gain, 0.0f);
            assertEquals(1, sink.updates);
        } finally { output.release(); }
    }

    private static void rejectsBeforeOutputMutation(AudioTrack output, GainTrackShadow sink,
                                                    int volume, long[] control) {
        int before = sink.updates;
        try {
            HordeMusicPlayback.applyOutputGain(output, volume, control, -1.0f);
            fail("invalid control must disable output instead of reaching platform gain");
        } catch (IllegalStateException expected) {
            assertEquals(before, sink.updates);
        }
    }

    @Test public void malformedTupleOrGainFailsBeforeChangingDeviceVolume() {
        AudioTrack output = HordeMusicPlayback.createOutput(480 * 8);
        GainTrackShadow sink = Shadow.extract(output);
        try {
            rejectsBeforeOutputMutation(output, sink, 70, null);
            rejectsBeforeOutputMutation(output, sink, 70, new long[4]);
            rejectsBeforeOutputMutation(output, sink, 70, new long[6]);
            for (float gain : new float[] {Float.NaN, Float.POSITIVE_INFINITY,
                    Float.NEGATIVE_INFINITY, -0.01f, 1.01f})
                rejectsBeforeOutputMutation(output, sink, 70, packet(1L, 0L, 0L, gain));
            long[] wrongBits = packet(1L, 0L, 0L, 1.0f);
            wrongBits[4] |= 1L << 40;
            rejectsBeforeOutputMutation(output, sink, 70, wrongBits);
            long[] wrongFlag = packet(1L, 0L, 0L, 1.0f);
            wrongFlag[0] = 2L;
            rejectsBeforeOutputMutation(output, sink, 70, wrongFlag);
            wrongFlag[0] = 1L;
            wrongFlag[2] = 2L;
            rejectsBeforeOutputMutation(output, sink, 70, wrongFlag);
            rejectsBeforeOutputMutation(output, sink, -1, packet(1L, 0L, 0L, 1.0f));
            rejectsBeforeOutputMutation(output, sink, 101, packet(1L, 0L, 0L, 1.0f));
        } finally { output.release(); }
    }

    @Test public void platformVolumeFailureIsExplicitAndCannotBeAcceptedAsAppliedGain() {
        AudioTrack output = HordeMusicPlayback.createOutput(480 * 8);
        GainTrackShadow sink = Shadow.extract(output);
        try {
            sink.result = AudioTrack.ERROR;
            try {
                HordeMusicPlayback.applyOutputGain(output, 70, packet(1L, 0L, 0L, 0.72f), -1.0f);
                fail("failed sink gain update must terminate worker output");
            } catch (IllegalStateException expected) {
                assertEquals(1, sink.updates);
                assertEquals(-1.0f, sink.gain, 0.0f);
            }
        } finally { output.release(); }
    }
}
