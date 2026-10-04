package com.samfa12.hordelanternrt;

import android.content.Context;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.util.Log;
import java.util.function.BooleanSupplier;

// Continuous Core PCM sink for the admitted Android waterfall derivative.
// No MediaPlayer per-wrap scheduler, independent focus requester or Java mixer.
final class HordeAmbiencePlayback implements AutoCloseable {
    private static final String TAG = "HordeAmbiencePCM";
    private final Object lock = new Object();
    private final Context context;
    private final BooleanSupplier focusGranted;
    private final Thread worker;
    private boolean stopped, suspended = true;
    private float left, right;
    private long generation;

    HordeAmbiencePlayback(Context context, BooleanSupplier focusGranted) {
        this.context = context.getApplicationContext() != null ? context.getApplicationContext() : context;
        this.focusGranted = focusGranted;
        Thread candidate = null;
        try {
            candidate = new Thread(this::run, "HordeAmbiencePCM");
            candidate.start();
        } catch (RuntimeException | LinkageError | OutOfMemoryError error) {
            synchronized (lock) { stopped = true; }
            Log.e(TAG, "Ambience worker unavailable", error);
        }
        worker = candidate;
    }
    void setControl(boolean suspended, float left, float right, long generation) {
        validateControl(left, right, generation);
        synchronized (lock) {
            if (stopped || generation < this.generation) return;
            this.suspended = suspended;
            this.left = left; this.right = right; this.generation = generation;
            lock.notifyAll();
        }
    }
    static void validateControl(float left, float right, long generation) {
        if (!Float.isFinite(left) || !Float.isFinite(right) || left < 0 || left > 1 ||
                right < 0 || right > 1 || generation < 0)
            throw new IllegalArgumentException("Invalid ambience controls");
    }
    @Override public void close() {
        synchronized (lock) { stopped = true; lock.notifyAll(); }
        if (worker == null) return;
        try { worker.join(1000L); }
        catch (InterruptedException error) { Thread.currentThread().interrupt(); }
        if (worker.isAlive()) Log.w(TAG, "Stop requested; worker retains cleanup ownership");
    }
    private void waitForControl() throws InterruptedException {
        synchronized (lock) { if (!stopped) lock.wait(5L); }
    }
    static AudioTrack createOutput(int minimumBytes) {
        if (minimumBytes <= 0 || minimumBytes > 12000 * 8)
            throw new IllegalArgumentException("Ambience minimum unavailable or exceeds250ms");
        return new AudioTrack.Builder()
                .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME)
                        .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
                .setAudioFormat(new AudioFormat.Builder().setSampleRate(48000)
                        .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                        .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
                .setTransferMode(AudioTrack.MODE_STREAM)
                .setBufferSizeInBytes(Math.max(minimumBytes, 1440 * 8)).build();
    }
    static int queueCapacityFrames(AudioTrack output, int minimumBytes) {
        final int capacity = output.getBufferSizeInFrames();
        if (capacity <= 0 || capacity > 12000 || (long)capacity * 8 < minimumBytes ||
                output.getBufferCapacityInFrames() > 12000)
            throw new IllegalStateException("Ambience allocation outside platform minimum/250ms bound");
        return capacity;
    }
    // Stereo-only physical source needs independent L/R gain. Android retains
    // this API for stereo output; setVolume would remove the source pan.
    @SuppressWarnings("deprecation")
    static void applyStereoGain(AudioTrack output, float left, float right) {
        validateControl(left, right, 0L);
        if (output.setStereoVolume(left, right) != AudioTrack.SUCCESS)
            throw new IllegalStateException("Ambience output gain failed");
    }
    private void run() {
        AudioTrack output = null;
        long handle = 0L;
        try {
            handle = nativeCreate(context.getAssets());
            if (handle == 0L) throw new IllegalStateException("Canonical Core ambience asset unavailable");
            final int minimum = AudioTrack.getMinBufferSize(48000,
                    AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_FLOAT);
            output = createOutput(minimum);
            if (output.getState() != AudioTrack.STATE_INITIALIZED)
                throw new IllegalStateException("Ambience output not initialized");
            final AmbienceOutputQueue queue = new AmbienceOutputQueue(queueCapacityFrames(output, minimum));
            final AmbienceOutputGate outputGate = new AmbienceOutputGate(output);
            final float[] pcm = new float[AmbienceOutputQueue.CHUNK_SAMPLES];
            final long[] control = new long[2];
            long epoch = -1L, reportedWraps = 0L;
            boolean playing = false;
            Log.i(TAG, "Core stereo48k; queueFrames=" + queue.capacity +
                    " allocatedFrames=" + output.getBufferCapacityInFrames() + "; chunkFrames=480");
            while (true) {
                final boolean pause;
                final long requestedEpoch;
                synchronized (lock) {
                    if (stopped) break;
                    pause = suspended || !focusGranted.getAsBoolean();
                    requestedEpoch = generation;
                }
                if (!nativeControl(handle, pause, requestedEpoch, control))
                    throw new IllegalStateException("Core ambience control failed");
                if (control[0] != requestedEpoch) throw new IllegalStateException("Ambience epoch mismatch");
                if (requestedEpoch != epoch) {
                    outputGate.reset();
                    queue.reset(); playing = false; reportedWraps = 0;
                    epoch = requestedEpoch;
                }
                // Native polling can race with a newer lifecycle publication.
                // Reconcile device transitions under the producer's lock: no
                // old accepted queue may be restarted after suspend returns.
                synchronized (lock) {
                    if (stopped) break;
                    playing = outputGate.reconcile(pause, suspended, focusGranted.getAsBoolean(),
                            epoch, generation, left, right);
                }
                if (!playing) { waitForControl(); continue; }
                queue.observeHead(output.getPlaybackHeadPosition());
                if (control[1] != queue.generated())
                    throw new IllegalStateException("Ambience Core/device content mismatch");
                if (queue.canRender()) {
                    if (!nativeRender(handle, pcm)) throw new IllegalStateException("Core ambience render failed");
                    queue.rendered(control[1]);
                }
                if (queue.remaining() == 0) { waitForControl(); continue; }
                // Recheck cancellation while owning the publication lock. A
                // lifecycle change cannot accept one more old block afterward.
                int written = 0;
                synchronized (lock) {
                    if (stopped) break;
                    if (!suspended && focusGranted.getAsBoolean() && generation == epoch) {
                        final int writable = (int)Math.min(queue.remaining(), (queue.capacity - queue.queued()) * 2);
                        if (writable > 0)
                            written = output.write(pcm, queue.offset(), writable, AudioTrack.WRITE_NON_BLOCKING);
                        queue.accepted(written);
                    }
                }
                if (BuildConfig.DEBUG && queue.consumed() / 552000L > reportedWraps) {
                    reportedWraps = queue.consumed() / 552000L;
                    Log.i(TAG, "wraps=" + reportedWraps + " epoch=" + epoch +
                            " generated=" + queue.generated() + " accepted=" + queue.submitted() +
                            " deviceHead=" + queue.consumed() + " underruns=" + output.getUnderrunCount());
                }
                if (written == 0) waitForControl(); // Keep the partial block, no regenerated samples.
            }
            Log.i(TAG, "Ambience stopped; epoch=" + epoch + " accepted=" + queue.submitted() +
                    " deviceHead=" + queue.consumed() + " underruns=" + output.getUnderrunCount());
        } catch (Exception | LinkageError | OutOfMemoryError error) {
            Log.e(TAG, "Waterfall ambience unavailable; renderer and one-shots unchanged", error);
        } finally {
            synchronized (lock) { stopped = true; lock.notifyAll(); }
            if (output != null) {
                try { output.pause(); } catch (Throwable ignored) { }
                try { output.flush(); } catch (Throwable ignored) { }
                try { output.release(); } catch (Throwable ignored) { }
            }
            if (handle != 0L) nativeDestroy(handle);
        }
    }
    private static native long nativeCreate(AssetManager assets);
    private static native boolean nativeControl(long handle, boolean suspended, long epoch, long[] control);
    private static native boolean nativeRender(long handle, float[] output);
    private static native void nativeDestroy(long handle);
}
