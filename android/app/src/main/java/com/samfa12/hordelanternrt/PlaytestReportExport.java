package com.samfa12.hordelanternrt;

import java.nio.ByteBuffer;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.io.IOException;
import java.io.OutputStream;

/** Foreground, memory-only export owner. No transport, files, timers or automatic retries. */
final class PlaytestReportExport {
    enum State { DRAFT, CHOOSING, WRITING, RETRYABLE, SAVED, CANCELLED }
    static final int MAX_JSON_BYTES = 16 * 1024;
    private State state = State.DRAFT;
    private byte[] json;
    private long token;

    synchronized State state() { return state; }
    synchronized long token() { return token; }

    synchronized boolean begin(final byte[] prepared, final boolean explicitConsent) {
        if (!explicitConsent || state != State.DRAFT || prepared == null || prepared.length < 3 ||
                prepared.length > MAX_JSON_BYTES + 1 || prepared[0] != 0) return false;
        final byte[] candidate = Arrays.copyOfRange(prepared, 1, prepared.length);
        // Native builder is the content/schema authority. Validate the bridge envelope only;
        // never reserialize its exact bytes or accept JNI modified UTF-8 replacement.
        try {
            final String text = StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(candidate)).toString();
            if (!text.startsWith("{") || !text.endsWith("}")) return false;
        } catch (final Exception invalid) { return false; }
        json = candidate;
        ++token;
        state = State.CHOOSING;
        return true;
    }

    synchronized boolean retry() {
        if (state != State.RETRYABLE || json == null) return false;
        ++token;
        state = State.CHOOSING;
        return true;
    }

    synchronized byte[] startWrite(final long attempt) {
        if (state != State.CHOOSING || attempt != token) return null;
        state = State.WRITING;
        return json.clone();
    }

    synchronized boolean canWrite(final long attempt) {
        return state == State.WRITING && attempt == token;
    }

    synchronized boolean complete(final long attempt, final boolean saved) {
        if (state != State.WRITING || attempt != token) return false;
        state = saved ? State.SAVED : State.RETRYABLE;
        if (saved) json = null;
        return true;
    }

    synchronized boolean pickerCancelled(final long attempt) {
        if (state != State.CHOOSING || attempt != token) return false;
        state = State.RETRYABLE;
        return true;
    }

    synchronized void cancel() {
        ++token;
        state = State.CANCELLED;
        json = null;
    }

    interface Destination { OutputStream open() throws IOException; }

    static boolean writeApproved(final PlaytestReportExport owner, final long attempt,
            final byte[] bytes, final Destination destination) {
        if (bytes == null || bytes.length == 0 || bytes.length > MAX_JSON_BYTES ||
                !owner.canWrite(attempt)) return false;
        // Destination has been explicitly approved by the picker. A write already
        // started cannot be undone by Back/destroy; late UI completion is discarded.
        try (OutputStream output = destination.open()) {
            if (output == null) return false;
            output.write(bytes);
            output.flush();
            return true;
        } catch (final IOException | RuntimeException failure) { return false; }
    }

    static String preparationError(final byte[] prepared) {
        if (prepared == null || prepared.length < 2) return "Report preparation is unavailable. Your note is still here.";
        if (prepared[0] == 0) return "Prepared report is invalid. Your note is still here.";
        return new String(prepared, 1, prepared.length - 1, StandardCharsets.UTF_8);
    }
}
