package com.samfa12.hordelanternrt;

import java.util.Objects;

/**
 * UI-thread owner for the entry-menu room loop plus its occasional chain cue.
 * Playback remains delegated to the platform SFX sink; this class does not decode or mix PCM.
 */
final class MenuAmbiencePlayback implements AutoCloseable {
    static final String MENU_ROOM = "menu_room";
    static final String MENU_CHAIN = "menu_chain";

    private static final float ROOM_BASE_GAIN = 0.07f;
    private static final float CHAIN_BASE_GAIN = 0.21f;
    private static final int PACKET_LENGTH = 6;

    interface Sink {
        int start(String key, float gain, boolean looping);
        void gain(int stream, float gain);
        void stop(int stream);
    }

    private float roomGain = ROOM_BASE_GAIN;
    private float chainGain = CHAIN_BASE_GAIN;

    private final Sink sink;
    private final Thread ownerThread;
    private int roomStream;
    private int chainStream;
    private boolean hasOwner;
    private boolean creakBaselinePending = true;
    private boolean closed;
    private long ownerGeneration;
    private long resetSerial;
    private long lastTick = -1L;
    private long lastCreakSerial = -1L;

    MenuAmbiencePlayback(Sink sink) {
        this.sink = Objects.requireNonNull(sink, "sink");
        this.ownerThread = Thread.currentThread();
    }

    /** Development mix tuning; the caller explicitly owns saving the two values. */
    boolean setMixPercent(int room, int chain) {
        requireOwnerThread();
        if (closed || room < 0 || room > 100 || chain < 0 || chain > 100)
            return false;
        roomGain = room / 100.0f;
        chainGain = chain / 100.0f;
        return true;
    }

    /**
     * Applies one coherent fixed-step publication. Returns false for malformed or stale
     * publications; accepted but inaudible publications return true after stopping owned streams.
     */
    boolean update(long[] packet, long currentGeneration, boolean eligible,
                   boolean focusGranted, float sfxGain) {
        requireOwnerThread();
        if (closed) return false;
        if (!isValid(packet, currentGeneration, sfxGain)) {
            interruptAndStop();
            return false;
        }

        final long generation = packet[0];
        final long newResetSerial = packet[1];
        final long tick = packet[2];
        final long creakSerial = packet[3];
        final int fadePermille = (int) packet[4];
        final boolean entryPresented = packet[5] == 1L;

        if (generation != currentGeneration) {
            interruptAndStop();
            return false;
        }

        final boolean newOwner = !hasOwner || generation > ownerGeneration;
        final boolean newReset = hasOwner && generation == ownerGeneration &&
                newResetSerial > resetSerial;
        if (newOwner || newReset) {
            stopOwnedStreams();
            hasOwner = true;
            ownerGeneration = generation;
            resetSerial = newResetSerial;
            lastTick = -1L;
            lastCreakSerial = creakSerial;
            creakBaselinePending = false;
        } else if (generation < ownerGeneration || newResetSerial < resetSerial ||
                tick < lastTick || creakSerial < lastCreakSerial) {
            interruptAndStop();
            return false;
        }

        lastTick = tick;
        final boolean audible = eligible && entryPresented && focusGranted && sfxGain > 0.0f &&
                fadePermille < 1000;
        if (!audible) {
            stopOwnedStreams();
            lastCreakSerial = creakSerial;
            // The first audible publication after any interruption only establishes
            // a fresh event baseline; it cannot replay a creak from while inaudible.
            creakBaselinePending = true;
            return true;
        }

        final float gain = sfxGain * (1.0f - fadePermille / 1000.0f);
        roomStream = ensureLoop(roomStream, MENU_ROOM, roomGain * gain);
        if (chainStream != 0) chainStream = applyGain(chainStream, chainGain * gain);

        if (creakBaselinePending) {
            lastCreakSerial = creakSerial;
            creakBaselinePending = false;
        } else if (creakSerial > lastCreakSerial) {
            // Advance before starting: a late load or sink failure drops this cue without replay.
            lastCreakSerial = creakSerial;
            stopStream(chainStream);
            chainStream = tryStart(MENU_CHAIN, chainGain * gain, false);
        } else {
            lastCreakSerial = creakSerial;
        }
        return true;
    }

    /** Stops only streams owned by this controller and establishes a no-replay baseline. */
    void stop() {
        requireOwnerThread();
        if (closed) return;
        stopOwnedStreams();
        creakBaselinePending = true;
    }

    @Override public void close() {
        requireOwnerThread();
        if (closed) return;
        stopOwnedStreams();
        creakBaselinePending = true;
        closed = true;
    }

    private int ensureLoop(int stream, String key, float gain) {
        if (gain <= 0.0f) { stopStream(stream); return 0; }
        if (stream == 0) return tryStart(key, gain, true);
        return applyGain(stream, gain);
    }

    private int tryStart(String key, float gain, boolean looping) {
        if (gain <= 0.0f) return 0;
        try {
            final int stream = sink.start(key, gain, looping);
            return stream > 0 ? stream : 0;
        } catch (RuntimeException ignored) {
            return 0;
        }
    }

    private int applyGain(int stream, float gain) {
        if (gain <= 0.0f) { stopStream(stream); return 0; }
        try {
            sink.gain(stream, gain);
            return stream;
        } catch (RuntimeException ignored) {
            stopStream(stream);
            return 0;
        }
    }

    private void stopOwnedStreams() {
        stopStream(roomStream);
        stopStream(chainStream);
        roomStream = 0;
        chainStream = 0;
    }

    private void stopStream(int stream) {
        if (stream == 0) return;
        try {
            sink.stop(stream);
        } catch (RuntimeException ignored) {
            // Clear the local handle even if the platform has already retired the stream.
        }
    }

    private void interruptAndStop() {
        stopOwnedStreams();
        creakBaselinePending = true;
    }

    private static boolean isValid(long[] packet, long currentGeneration, float sfxGain) {
        return packet != null && packet.length == PACKET_LENGTH && currentGeneration > 0L &&
                packet[0] > 0L && packet[1] >= 0L && packet[2] >= 0L && packet[3] >= 0L &&
                packet[4] >= 0L && packet[4] <= 1000L &&
                (packet[5] == 0L || packet[5] == 1L) &&
                Float.isFinite(sfxGain) && sfxGain >= 0.0f && sfxGain <= 1.0f;
    }

    private void requireOwnerThread() {
        if (Thread.currentThread() != ownerThread)
            throw new IllegalStateException("Menu ambience must be controlled on its owner thread");
    }
}