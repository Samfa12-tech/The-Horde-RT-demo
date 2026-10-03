package com.samfa12.hordelanternrt;

import android.annotation.TargetApi;
import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

// OS focus gate only. PCM generation, buffering, and cue selection remain elsewhere.
final class MusicAudioFocus implements AutoCloseable {
    private static final String TAG = "HordeLanternMusic";

    private final AudioManager audioManager;
    private final Handler mainHandler;
    private volatile boolean granted;
    private boolean eligible;
    private boolean closed;
    private Request activeRequest;

    MusicAudioFocus(Context context, Handler mainHandler) {
        AudioManager manager = null;
        try {
            manager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        } catch (RuntimeException | LinkageError error) {
            Log.w(TAG, "Music audio focus service unavailable; playback remains silent", error);
        }
        this.audioManager = manager;
        this.mainHandler = mainHandler;
    }

    void setEligible(boolean value) {
        if (Looper.myLooper() != mainHandler.getLooper()) {
            mainHandler.post(() -> setEligible(value));
            return;
        }
        if (closed || eligible == value) return;
        eligible = value;
        if (value) request();
        else abandonActive();
    }

    boolean isGranted() { return granted; }

    private void request() {
        granted = false;
        if (audioManager == null) return;
        Request request = new Request();
        activeRequest = request;
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                request.platformRequest = Api26.request(request.listener, mainHandler);
                int result = Api26.requestFocus(audioManager, request.platformRequest);
                requestResult(request, result);
            } else {
                @SuppressWarnings("deprecation")
                int result = audioManager.requestAudioFocus(request.listener,
                        AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
                requestResult(request, result);
            }
        } catch (RuntimeException | LinkageError error) {
            Log.w(TAG, "Music audio focus unavailable; playback remains silent", error);
            abandonActive();
        }
    }

    private void requestResult(Request request, int result) {
        if (activeRequest != request || closed || !eligible) return;
        if (result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) granted = true;
        else if (result != AudioManager.AUDIOFOCUS_REQUEST_DELAYED) abandonActive();
        // A delayed request remains live for a later GAIN callback. A failed
        // request is intentionally not retried until eligibility falls and rises.
    }

    private void onFocusChange(Request request, int change) {
        if (Looper.myLooper() != mainHandler.getLooper()) {
            mainHandler.post(() -> onFocusChange(request, change));
            return;
        }
        if (closed || !eligible || activeRequest != request) return;
        switch (change) {
            case AudioManager.AUDIOFOCUS_GAIN:
                granted = true;
                break;
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) granted = false;
                // On API 26+, leave attenuation to Android's automatic ducking policy.
                break;
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                granted = false;
                break;
            case AudioManager.AUDIOFOCUS_LOSS:
                granted = false;
                abandonActive();
                break;
            default:
                break;
        }
    }

    private void abandonActive() {
        granted = false;
        Request request = activeRequest;
        activeRequest = null; // Invalidate before calling the platform.
        if (request == null || audioManager == null) return;
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && request.platformRequest != null) {
                Api26.abandon(audioManager, request.platformRequest);
            } else {
                @SuppressWarnings("deprecation")
                int ignored = audioManager.abandonAudioFocus(request.listener);
            }
        } catch (RuntimeException | LinkageError error) {
            Log.w(TAG, "Could not abandon music audio focus", error);
        }
    }

    @Override public void close() {
        if (Looper.myLooper() != mainHandler.getLooper()) {
            mainHandler.post(this::close);
            return;
        }
        if (closed) return;
        closed = true;
        eligible = false;
        abandonActive();
    }

    private final class Request {
        Object platformRequest;
        final AudioManager.OnAudioFocusChangeListener listener =
                change -> onFocusChange(this, change);
    }

    // Keep API 26 types out of the API 24 class verification path.
    @TargetApi(Build.VERSION_CODES.O)
    private static final class Api26 {
        static Object request(AudioManager.OnAudioFocusChangeListener listener, Handler handler) {
            AudioAttributes attributes = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build();
            return new android.media.AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                    .setAudioAttributes(attributes)
                    .setAcceptsDelayedFocusGain(true)
                    .setOnAudioFocusChangeListener(listener, handler)
                    .build();
        }

        static int requestFocus(AudioManager manager, Object request) {
            return manager.requestAudioFocus((android.media.AudioFocusRequest) request);
        }

        static int abandon(AudioManager manager, Object request) {
            return manager.abandonAudioFocusRequest((android.media.AudioFocusRequest) request);
        }
    }
}
