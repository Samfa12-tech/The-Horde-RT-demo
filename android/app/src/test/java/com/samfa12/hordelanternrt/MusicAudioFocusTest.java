package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.ContextWrapper;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Looper;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowAudioManager;
import org.robolectric.Shadows;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class MusicAudioFocusTest {
    @Test
    public void grantUsesGameMusicAttributesAndRepeatedEligibilityIsIdempotent() {
        MusicAudioFocus focus = newFocus();
        ShadowAudioManager shadow = shadowAudioManager();
        focus.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest recorded = shadow.getLastAudioFocusRequest();

        assertTrue(focus.isGranted());
        assertEquals(AudioManager.AUDIOFOCUS_GAIN, recorded.audioFocusRequest.getFocusGain());
        AudioAttributes attributes = recorded.audioFocusRequest.getAudioAttributes();
        assertEquals(AudioAttributes.USAGE_GAME, attributes.getUsage());
        assertEquals(AudioAttributes.CONTENT_TYPE_MUSIC, attributes.getContentType());
        assertTrue(recorded.audioFocusRequest.acceptsDelayedFocusGain());
        assertFalse(recorded.audioFocusRequest.willPauseWhenDucked());

        focus.setEligible(true);
        focus.setEligible(true);
        drainMain();
        assertSame(recorded, shadow.getLastAudioFocusRequest());
        focus.close();
        drainMain();
    }

    @Test
    public void delayedRequestStaysSilentUntilGainAndFailedRequestDoesNotRetry() {
        ShadowAudioManager shadow = shadowAudioManager();
        shadow.setNextFocusRequestResponse(AudioManager.AUDIOFOCUS_REQUEST_DELAYED);
        MusicAudioFocus delayed = newFocus();
        delayed.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest pending = shadow.getLastAudioFocusRequest();
        assertFalse(delayed.isGranted());
        pending.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        drainMain();
        assertTrue(delayed.isGranted());
        delayed.close();
        drainMain();

        shadow.setNextFocusRequestResponse(AudioManager.AUDIOFOCUS_REQUEST_FAILED);
        MusicAudioFocus denied = newFocus();
        denied.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest failed = shadow.getLastAudioFocusRequest();
        assertFalse(denied.isGranted());
        assertSame(failed.audioFocusRequest, shadow.getLastAbandonedAudioFocusRequest());
        failed.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        drainMain();
        assertFalse(denied.isGranted());
        denied.setEligible(true);
        drainMain();
        assertSame(failed, shadow.getLastAudioFocusRequest());
        denied.setEligible(false);
        drainMain();
        shadow.setNextFocusRequestResponse(AudioManager.AUDIOFOCUS_REQUEST_GRANTED);
        denied.setEligible(true);
        drainMain();
        assertTrue(denied.isGranted());
        denied.close();
        drainMain();
    }

    @Test
    public void delayedRequestCancelledByLifecycleEdgeRejectsLateGainAndUsesFreshRequest() {
        ShadowAudioManager shadow = shadowAudioManager();
        shadow.setNextFocusRequestResponse(AudioManager.AUDIOFOCUS_REQUEST_DELAYED);
        MusicAudioFocus focus = newFocus();
        focus.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest old = shadow.getLastAudioFocusRequest();
        assertFalse(focus.isGranted());

        focus.setEligible(false);
        drainMain();
        assertSame(old.audioFocusRequest, shadow.getLastAbandonedAudioFocusRequest());
        old.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        drainMain();
        assertFalse(focus.isGranted());

        shadow.setNextFocusRequestResponse(AudioManager.AUDIOFOCUS_REQUEST_GRANTED);
        focus.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest replacement = shadow.getLastAudioFocusRequest();
        assertNotSame(old, replacement);
        assertTrue(focus.isGranted());
        focus.close();
        drainMain();
    }

    @Test
    public void missingAudioFocusServiceFailsClosed() {
        Context base = RuntimeEnvironment.getApplication();
        Context unavailable = new ContextWrapper(base) {
            @Override public Object getSystemService(String name) {
                return Context.AUDIO_SERVICE.equals(name) ? null : super.getSystemService(name);
            }
        };
        MusicAudioFocus focus = new MusicAudioFocus(unavailable, new Handler(Looper.getMainLooper()));
        focus.setEligible(true);
        drainMain();
        assertFalse(focus.isGranted());
        focus.setEligible(false);
        drainMain();
        focus.close();
        drainMain();
    }

    @Test
    public void transientLossPausesAndGainResumesWhileAutomaticDuckIsLeftToPlatform() {
        MusicAudioFocus focus = newFocus();
        focus.setEligible(true);
        drainMain();
        AudioManager.OnAudioFocusChangeListener listener = shadowAudioManager().getLastAudioFocusRequest().listener;

        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS_TRANSIENT);
        drainMain();
        assertFalse(focus.isGranted());
        assertTrue(HordeMusicPlayback.isSuspended(false, focus.isGranted()));
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        drainMain();
        assertTrue(focus.isGranted());
        assertFalse(HordeMusicPlayback.isSuspended(false, focus.isGranted()));
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK);
        drainMain();
        assertTrue(focus.isGranted());
        assertTrue(HordeMusicPlayback.isSuspended(true, focus.isGranted()));
        focus.close();
        drainMain();
    }

    @Test
    public void permanentLossAbandonsAndRequiresNewEligibleEdgeAndRejectsStaleGain() {
        MusicAudioFocus focus = newFocus();
        ShadowAudioManager shadow = shadowAudioManager();
        focus.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest old = shadow.getLastAudioFocusRequest();
        old.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS);
        drainMain();
        assertFalse(focus.isGranted());
        assertSame(old.audioFocusRequest, shadow.getLastAbandonedAudioFocusRequest());

        focus.setEligible(true);
        drainMain();
        assertSame(old, shadow.getLastAudioFocusRequest());
        shadow.setNextFocusRequestResponse(AudioManager.AUDIOFOCUS_REQUEST_GRANTED);
        focus.setEligible(false);
        drainMain();
        focus.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest replacement = shadow.getLastAudioFocusRequest();
        assertNotSame(old, replacement);
        replacement.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS_TRANSIENT);
        drainMain();
        assertFalse(focus.isGranted());
        old.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        drainMain();
        assertFalse(focus.isGranted());
        focus.close();
        drainMain();
    }

    @Test
    public void closeAbandonsOnceAndLateCallbackCannotGrant() {
        MusicAudioFocus focus = newFocus();
        ShadowAudioManager shadow = shadowAudioManager();
        focus.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest request = shadow.getLastAudioFocusRequest();
        focus.close();
        drainMain();
        assertFalse(focus.isGranted());
        assertSame(request.audioFocusRequest, shadow.getLastAbandonedAudioFocusRequest());
        request.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        drainMain();
        focus.close();
        drainMain();
        assertFalse(focus.isGranted());
    }

    @Test
    @Config(sdk = 24)
    public void api24UsesLegacyFocusListenerAndAbandonment() {
        MusicAudioFocus focus = newFocus();
        ShadowAudioManager shadow = shadowAudioManager();
        focus.setEligible(true);
        drainMain();
        ShadowAudioManager.AudioFocusRequest recorded = shadow.getLastAudioFocusRequest();
        assertEquals(AudioManager.STREAM_MUSIC, recorded.streamType);
        assertEquals(AudioManager.AUDIOFOCUS_GAIN, recorded.durationHint);
        assertTrue(focus.isGranted());
        recorded.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS_TRANSIENT);
        drainMain();
        assertFalse(focus.isGranted());
        recorded.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        drainMain();
        assertTrue(focus.isGranted());
        recorded.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK);
        drainMain();
        assertFalse(focus.isGranted());
        assertTrue(HordeMusicPlayback.isSuspended(false, focus.isGranted()));
        recorded.listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        drainMain();
        assertTrue(focus.isGranted());
        focus.close();
        drainMain();
        assertSame(recorded.listener, shadow.getLastAbandonedAudioFocusListener());
    }

    @org.junit.Test
    public void menuSfxGateIsNotifiedImmediatelyOnFocusLossAndReturn() {
        MusicAudioFocus focus = newFocus();
        java.util.ArrayList<Boolean> changes = new java.util.ArrayList<>();
        focus.setOnFocusChanged(() -> changes.add(focus.isGranted()));
        focus.setEligible(true);
        assertEquals(java.util.Arrays.asList(true), changes);
        AudioManager.OnAudioFocusChangeListener listener = shadowAudioManager().getLastAudioFocusRequest().listener;
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS_TRANSIENT);
        assertEquals(java.util.Arrays.asList(true, false), changes);
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        assertEquals(java.util.Arrays.asList(true, false, true), changes);
        focus.setEligible(false);
        assertEquals(java.util.Arrays.asList(true, false, true, false), changes);
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        assertEquals(4, changes.size()); // Abandoned callback cannot restart ambience.
        focus.close();
    }

    private static MusicAudioFocus newFocus() {
        Context context = RuntimeEnvironment.getApplication();
        return new MusicAudioFocus(context, new Handler(Looper.getMainLooper()));
    }

    private static ShadowAudioManager shadowAudioManager() {
        AudioManager manager = (AudioManager) RuntimeEnvironment.getApplication()
                .getSystemService(Context.AUDIO_SERVICE);
        return Shadows.shadowOf(manager);
    }

    private static void drainMain() {
        Shadows.shadowOf(Looper.getMainLooper()).idle();
    }
}
