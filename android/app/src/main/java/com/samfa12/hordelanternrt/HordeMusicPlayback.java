package com.samfa12.hordelanternrt;

import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.util.Log;

// Thin PCM sink. Cue decisions and Core mixing stay native; no Java music engine.
// One worker owns the native bank/session, writes and device-head accounting.
final class HordeMusicPlayback implements AutoCloseable {
    private static final String TAG = "HordeLanternMusic";
    private static final int FRAMES = 480;
    private static final int MAX_BUFFER_FRAMES = 12000; // 250ms maximum, music only; SFX independent.
    private final Object controlLock = new Object();
    private final AssetManager assets;
    private final Thread worker;
    private boolean stopped = false;
    private boolean suspended = true;
    private int volume = 70;

    HordeMusicPlayback(AssetManager assets, int volume) {
        this.assets = assets; // Strong reference through native asset loading.
        this.volume = Math.max(0, Math.min(100, volume));
        Thread candidate = null;
        try {
            candidate = new Thread(this::run, "HordeMusicPCM");
            candidate.start();
        } catch (RuntimeException | LinkageError | OutOfMemoryError error) {
            synchronized (controlLock) { stopped = true; }
            Log.e(TAG, "Music worker unavailable; RT and SFX unchanged", error);
        }
        worker = candidate;
    }
    void setSuspended(boolean value) {
        synchronized (controlLock) {
            if (stopped) return;
            suspended = value;
            controlLock.notifyAll(); // Worker pauses without flushing accepted PCM.
        }
    }
    void setVolumePercent(int value) {
        synchronized (controlLock) {
            if (stopped) return;
            volume = Math.max(0, Math.min(100, value));
            controlLock.notifyAll(); // Worker applies backend volume on its next control pass.
        }
    }
    @Override public void close() {
        synchronized (controlLock) {
            stopped = true;
            controlLock.notifyAll();
        }
        if (worker == null) return; // Thread construction failed; no worker owns resources.
        // Only worker finally destroys native PCM/storage. A slow startup cannot
        // make the UI free a bank still in use; don't hang the Activity forever.
        try { worker.join(1000L); }
        catch (InterruptedException error) { Thread.currentThread().interrupt(); }
        catch (RuntimeException | LinkageError error) {
            Log.w(TAG, "Could not join music worker; cleanup remains worker-owned", error);
        }
        if (worker.isAlive()) Log.w(TAG, "Music stop requested; worker still owns cleanup");
    }
    private void waitForControl() throws InterruptedException {
        synchronized (controlLock) { if (!stopped) controlLock.wait(5L); }
    }
    // Package-visible construction seam: Android's PCM sink, not Core mixing.
    static AudioTrack createOutput(int minimumBytes) {
        if (minimumBytes <= 0 || minimumBytes > MAX_BUFFER_FRAMES * 8)
            throw new IllegalArgumentException("PCM minimum buffer unavailable or exceeds250ms bound");
        // getMinBufferSize is a creation estimate, not permission to shrink the
        // constructed sink. Its actual allocation may be larger on the device.
        return new AudioTrack.Builder()
                .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME)
                        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
                .setAudioFormat(new AudioFormat.Builder().setSampleRate(48000)
                        .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                        .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
                .setTransferMode(AudioTrack.MODE_STREAM)
                .setBufferSizeInBytes(Math.max(minimumBytes, FRAMES * 3 * 8)).build();
    }
    static int queueCapacityFrames(AudioTrack output, int minimumBytes) {
        final int frames = output.getBufferSizeInFrames();
        if (frames <= 0 || frames > MAX_BUFFER_FRAMES || (long) frames * 8 < minimumBytes)
            throw new IllegalStateException("PCM queue outside platform minimum/250ms bound: " + frames);
        return frames;
    }
    private void run() {
        long nativeHandle = 0L;
        AudioTrack output = null;
        try {
            nativeHandle = nativeCreate(assets);
            if (nativeHandle == 0L) throw new IllegalStateException("Native music bank unavailable");
            final int minimumBytes = AudioTrack.getMinBufferSize(48000,
                    AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_FLOAT);
            if (minimumBytes <= 0) throw new IllegalStateException("PCM48k stereo float unsupported: " + minimumBytes);
            output = createOutput(minimumBytes);
            if (output.getState() != AudioTrack.STATE_INITIALIZED) throw new IllegalStateException("PCM output not initialized");
            final int capacity = queueCapacityFrames(output, minimumBytes);
            int appliedVolume = -1;
            Log.i(TAG, "PCM ready48k stereo; queue capacity=" + capacity +
                    "frames minimumBytes=" + minimumBytes +
                    " allocatedFrames=" + output.getBufferCapacityInFrames() +
                    " sampleRate=" + output.getSampleRate());
            final float[] pcm = new float[FRAMES * 2];
            final long[] control = new long[4]; // available, restartEpoch, suspended, generatedFrames
            long epoch = -1L, submitted = 0L, rawPrevious = 0L, wraps = 0L, consumed = 0L;
            long generated = 0L, reportedPeriods = 0L;
            int offset = pcm.length; // No pending block.
            boolean playing = false;
            while (true) {
                final boolean externallySuspended;
                synchronized (controlLock) {
                    if (stopped) break;
                    externallySuspended = suspended;
                }
                final int requestedVolume;
                synchronized (controlLock) { requestedVolume = volume; }
                if (requestedVolume != appliedVolume) {
                    final int volumeResult = output.setVolume(requestedVolume / 100.0f);
                    if (volumeResult != 0)
                        throw new IllegalStateException("PCM volume update failed: " + volumeResult);
                    appliedVolume = requestedVolume;
                }
                if (!nativePoll(nativeHandle, externallySuspended, control))
                    throw new IllegalStateException("Music input/cursor contract failed (including copied-event overflow)");
                if (control[0] == 0L) { waitForControl(); continue; }
                if (control[1] != epoch) {
                    output.pause(); output.flush(); // Only explicit session restart discards old queue.
                    playing = false; offset = pcm.length; submitted = 0L;
                    rawPrevious = 0L; wraps = 0L; consumed = 0L; epoch = control[1];
                    reportedPeriods = 0L;
                    Log.i(TAG, "Music epoch=" + epoch + "; discarded prior-session PCM");
                }
                generated = control[3]; // Native content clock, not accepted/device-played frames.
                synchronized (controlLock) {
                    if (stopped) break;
                    if (suspended || control[2] != 0L) {
                        if (playing) output.pause();
                        playing = false;
                    } else if (!playing) { output.play(); playing = true; }
                }
                if (!playing) { waitForControl(); continue; }
                final long raw = ((long) output.getPlaybackHeadPosition()) & 0xffffffffL;
                if (raw < rawPrevious) {
                    if (rawPrevious - raw < 0x80000000L) throw new IllegalStateException("Unexpected device-head reset");
                    wraps += 0x100000000L;
                }
                rawPrevious = raw; consumed = wraps + raw;
                if (consumed > submitted) throw new IllegalStateException("Device consumed beyond accepted PCM");
                if (offset == pcm.length) {
                    if (submitted - consumed >= capacity) { waitForControl(); continue; }
                    if (control[3] != submitted) throw new IllegalStateException("Generated/submitted PCM mismatch");
                    if (!nativeRender(nativeHandle, pcm)) throw new IllegalStateException("Core PCM render failed");
                    generated += FRAMES;
                    offset = 0;
                }
                final int written = output.write(pcm, offset, pcm.length - offset, AudioTrack.WRITE_NON_BLOCKING);
                if (written < 0 || (written & 1) != 0 || written > pcm.length - offset)
                    throw new IllegalStateException("PCM write failed/invalid count: " + written);
                offset += written; submitted += written / 2;
                if (BuildConfig.DEBUG && consumed / 576000L > reportedPeriods) {
                    reportedPeriods = consumed / 576000L;
                    Log.i(TAG, "Music progress epoch=" + epoch + " periods=" + reportedPeriods +
                            " generated=" + generated + " accepted=" + submitted +
                            " deviceHeadObserved=" + consumed + " capacity=" + capacity +
                            " underruns=" + output.getUnderrunCount());
                }
                if (written == 0) waitForControl(); // Retain partial block; never regenerate/skip it.
            }
            Log.i(TAG, "Music stop requested; epoch=" + epoch + " generated=" + generated +
                    " accepted=" + submitted + " deviceHeadObserved=" + consumed +
                    " capacity=" + capacity + " underruns=" + output.getUnderrunCount());
        } catch (Exception | LinkageError | OutOfMemoryError error) {
            Log.e(TAG, "Music unavailable; RT and SFX unchanged", error);
        } finally {
            synchronized (controlLock) {
                stopped = true;
                controlLock.notifyAll();
            }
            try {
                releaseOutput(output);
            } finally {
                if (nativeHandle != 0L) {
                    try { nativeDestroy(nativeHandle); }
                    catch (RuntimeException | LinkageError error) {
                        Log.e(TAG, "Native music storage cleanup failed", error);
                    }
                }
            }
        }
    }

    private static void releaseOutput(AudioTrack output) {
        if (output == null) return;
        try { output.pause(); }
        catch (Throwable error) { logCleanupFailure("Could not pause music output during cleanup", error); }
        try { output.flush(); }
        catch (Throwable error) { logCleanupFailure("Could not flush music output during cleanup", error); }
        try { output.release(); }
        catch (Throwable error) { logCleanupFailure("Could not release music output", error); }
    }

    private static void logCleanupFailure(String message, Throwable error) {
        try { Log.w(TAG, message, error); }
        catch (Throwable ignored) { /* Cleanup must continue even if logging is unavailable. */ }
    }

    private static native long nativeCreate(AssetManager assets);
    private static native boolean nativePoll(long handle, boolean suspended, long[] control);
    private static native boolean nativeRender(long handle, float[] output);
    private static native void nativeDestroy(long handle);
}
