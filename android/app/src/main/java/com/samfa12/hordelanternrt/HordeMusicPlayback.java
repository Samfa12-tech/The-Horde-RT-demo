package com.samfa12.hordelanternrt;

import android.content.Context;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

// Thin PCM sink. Cue decisions and Core mixing stay native; no Java music engine.
// One worker owns the native bank/session, writes and device-head accounting.
final class HordeMusicPlayback implements AutoCloseable {
    private static final String TAG = "HordeLanternMusic";
    private static final int FRAMES = 480;
    private static final int MAX_BUFFER_FRAMES = 12000; // 250ms maximum, music only; SFX independent.
    private static final int CONTROL_FIELDS = 5;
    private final Object controlLock = new Object();
    private final Context context;
    private final MusicAudioFocus audioFocus;
    private final Thread worker;
    private boolean stopped = false;
    private boolean suspended = true;
    private int volume = 70;

    HordeMusicPlayback(Context context, int volume) {
        this.context = context.getApplicationContext() != null ? context.getApplicationContext() : context;
        this.audioFocus = new MusicAudioFocus(this.context, new Handler(Looper.getMainLooper()));
        this.volume = Math.max(0, Math.min(100, volume));
        Thread candidate = null;
        try {
            candidate = new Thread(this::run, "HordeMusicPCM");
            candidate.start();
        } catch (RuntimeException | LinkageError | OutOfMemoryError error) {
            synchronized (controlLock) { stopped = true; }
            Log.e(TAG, "Music worker unavailable; RT and SFX unchanged", error);
            audioFocus.close();
        }
        worker = candidate;
    }
    void setSuspended(boolean value) {
        boolean focusEligible;
        synchronized (controlLock) {
            if (stopped) return;
            suspended = value;
            focusEligible = !suspended;
            controlLock.notifyAll(); // Worker pauses without flushing accepted PCM.
        }
        audioFocus.setEligible(focusEligible);
    }
    void setVolumePercent(int value) {
        synchronized (controlLock) {
            if (stopped) return;
            volume = Math.max(0, Math.min(100, value));
            controlLock.notifyAll(); // Worker applies backend volume on its next control pass.
        }
    }
    // Ambience shares the existing volatile OS-focus gate; it never requests focus.
    boolean isAudioFocusGranted() { return audioFocus.isGranted(); }
    @Override public void close() {
        synchronized (controlLock) {
            stopped = true;
            controlLock.notifyAll();
        }
        audioFocus.close();
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
    // A single nativePoll tuple carries the presentation envelope alongside the
    // owning session epoch/cursor. Saved user volume remains an independent int.
    static float applyOutputGain(AudioTrack output, int userVolume, long[] control, float appliedGain) {
        if (control == null || control.length != CONTROL_FIELDS ||
                (control[0] != 0L && control[0] != 1L) ||
                (control[2] != 0L && control[2] != 1L) ||
                (control[4] & ~0xffffffffL) != 0L || userVolume < 0 || userVolume > 100)
            throw new IllegalStateException("Invalid native music output control");
        final float presentationGain = Float.intBitsToFloat((int) control[4]);
        if (!Float.isFinite(presentationGain) || presentationGain < 0.0f || presentationGain > 1.0f)
            throw new IllegalStateException("Invalid native music presentation gain");
        final float effectiveGain = (userVolume / 100.0f) * presentationGain;
        if (Float.compare(effectiveGain, appliedGain) != 0) {
            final int result = output.setVolume(effectiveGain);
            if (result != AudioTrack.SUCCESS)
                throw new IllegalStateException("PCM volume update failed: " + result);
        }
        return effectiveGain;
    }
    private void run() {
        long nativeHandle = 0L;
        AudioTrack output = null;
        try {
            nativeHandle = nativeCreate(context.getAssets());
            if (nativeHandle == 0L) throw new IllegalStateException("Native music bank unavailable");
            final int minimumBytes = AudioTrack.getMinBufferSize(48000,
                    AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_FLOAT);
            if (minimumBytes <= 0) throw new IllegalStateException("PCM48k stereo float unsupported: " + minimumBytes);
            output = createOutput(minimumBytes);
            if (output.getState() != AudioTrack.STATE_INITIALIZED) throw new IllegalStateException("PCM output not initialized");
            final int capacity = queueCapacityFrames(output, minimumBytes);
            float appliedGain = -1.0f;
            Log.i(TAG, "PCM ready48k stereo; queue capacity=" + capacity +
                    "frames minimumBytes=" + minimumBytes +
                    " allocatedFrames=" + output.getBufferCapacityInFrames() +
                    " sampleRate=" + output.getSampleRate());
            final float[] pcm = new float[FRAMES * 2];
            // available, restartEpoch, suspended, generatedFrames, float32 gain bits
            final long[] control = new long[CONTROL_FIELDS];
            long epoch = -1L, submitted = 0L, rawPrevious = 0L, wraps = 0L, consumed = 0L;
            long generated = 0L, reportedPeriods = 0L;
            int offset = pcm.length; // No pending block.
            boolean playing = false;
            while (true) {
                final boolean externallySuspended;
                final int requestedVolume;
                synchronized (controlLock) {
                    if (stopped) break;
                    externallySuspended = isSuspended(suspended, audioFocus.isGranted());
                    requestedVolume = volume;
                }
                if (!nativePoll(nativeHandle, externallySuspended, control))
                    throw new IllegalStateException("Music input/cursor contract failed (including copied-event overflow)");
                appliedGain = applyOutputGain(output, requestedVolume, control, appliedGain);
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
                    if (isSuspended(suspended, audioFocus.isGranted()) || control[2] != 0L) {
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
            // Audio-focus APIs stay on the main thread, including worker failure.
            audioFocus.close();
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

    static boolean isSuspended(boolean lifecycleSuspended, boolean focusGranted) {
        return lifecycleSuspended || !focusGranted;
    }

    private static native long nativeCreate(AssetManager assets);
    private static native boolean nativePoll(long handle, boolean suspended, long[] control);
    private static native boolean nativeRender(long handle, float[] output);
    private static native void nativeDestroy(long handle);
}
