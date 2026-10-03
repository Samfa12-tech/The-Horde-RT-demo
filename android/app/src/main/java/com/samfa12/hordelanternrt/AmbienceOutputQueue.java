package com.samfa12.hordelanternrt;

// Device sink accounting only; the native Core owns the content cursor.
// One worker retains exactly one partial block until every accepted write.
final class AmbienceOutputQueue {
    static final int CHUNK_FRAMES = 480;
    static final int CHUNK_SAMPLES = CHUNK_FRAMES * 2;
    final int capacity;
    private int offset = CHUNK_SAMPLES;
    private long submitted, consumed, generated, rawPrevious, headWraps;
    AmbienceOutputQueue(int capacity) {
        if (capacity <= 0 || capacity > 12000) throw new IllegalArgumentException("Unbounded ambience queue");
        this.capacity = capacity;
    }
    void reset() {
        offset = CHUNK_SAMPLES;
        submitted = consumed = generated = rawPrevious = headWraps = 0L;
    }
    void observeHead(int rawHead) {
        final long raw = ((long)rawHead) & 0xffffffffL;
        if (raw < rawPrevious) {
            if (rawPrevious - raw < 0x80000000L) throw new IllegalStateException("Unexpected ambience device-head reset");
            headWraps += 0x100000000L;
        }
        final long observed = headWraps + raw;
        if (observed > submitted) throw new IllegalStateException("Device consumed unaccepted ambience PCM");
        rawPrevious = raw;
        consumed = observed;
    }
    boolean canRender() { return offset == CHUNK_SAMPLES && submitted - consumed < capacity; }
    void rendered(long nativeGeneratedBefore) {
        if (!canRender() || nativeGeneratedBefore != generated || generated != submitted)
            throw new IllegalStateException("Ambience generated/accepted ownership mismatch");
        generated += CHUNK_FRAMES;
        offset = 0;
    }
    void accepted(int samples) {
        if (offset == CHUNK_SAMPLES || samples < 0 || (samples & 1) != 0 || samples > remaining())
            throw new IllegalStateException("Invalid ambience partial write");
        if (submitted + samples / 2 - consumed > capacity)
            throw new IllegalStateException("Ambience accepted queue exceeds device bound");
        offset += samples;
        submitted += samples / 2;
    }
    int offset() { return offset; }
    int remaining() { return CHUNK_SAMPLES - offset; }
    long submitted() { return submitted; }
    long consumed() { return consumed; }
    long generated() { return generated; }
    long queued() { return submitted - consumed; }
}
