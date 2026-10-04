package com.samfa12.hordelanternrt;

import android.media.AudioTrack;

// Worker-owned device transitions. Reconcile while holding the publication
// lock so stale native controls cannot resume a newly suspended/reset session.
final class AmbienceOutputGate {
    private final AudioTrack output;
    private boolean playing;
    private float appliedLeft = -1, appliedRight = -1;
    AmbienceOutputGate(AudioTrack output) { this.output = output; }
    void reset() {
        output.pause(); output.flush();
        playing = false;
    }
    boolean reconcile(boolean nativeSuspended, boolean liveSuspended, boolean focusGranted,
                      long nativeEpoch, long liveEpoch, float left, float right) {
        HordeAmbiencePlayback.validateControl(left, right, liveEpoch);
        if (nativeEpoch < 0) throw new IllegalArgumentException("Invalid native ambience epoch");
        final boolean canPlay = !nativeSuspended && !liveSuspended && focusGranted && nativeEpoch == liveEpoch;
        final float leftGain = canPlay ? left : 0;
        final float rightGain = canPlay ? right : 0;
        if (leftGain != appliedLeft || rightGain != appliedRight) {
            HordeAmbiencePlayback.applyStereoGain(output, leftGain, rightGain);
            appliedLeft = leftGain; appliedRight = rightGain;
        }
        if (!canPlay) {
            if (playing) output.pause();
            playing = false;
        } else if (!playing) { output.play(); playing = true; }
        return playing;
    }
}
