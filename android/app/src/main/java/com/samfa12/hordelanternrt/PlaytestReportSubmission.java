package com.samfa12.hordelanternrt;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/** Foreground-only, memory-only remote report submission owner. */
final class PlaytestReportSubmission {
    static final String ENDPOINT = "https://briarhold-signal.samfa12.com/api/horde-reports";
    static final int MAX_SUBMISSION_BYTES = 768 * 1024;
    static final int MAX_TOKEN_CHARS = 2048;
    static final int MAX_RESPONSE_BYTES = 8 * 1024;
    static final int TIMEOUT_MILLIS = 10_000;
    static final int MAX_ATTEMPT_MILLIS = 30_000;
    static final int MAX_BENCHMARK_LOCAL_BYTES = 16 * 1024;
    static final int MAX_BENCHMARK_SUBMISSION_BYTES = 20 * 1024;
    // Exact compatible private service admission/deployment is unproven. A
    // prepared offline wrapper never enables any benchmark network attempt.
    private static final boolean BENCHMARK_SUMMARY_LIVE_SEND_ENABLED = false;

    enum State { DRAFT, READY, IN_FLIGHT, RETRYABLE, QUEUED, SENT, CONFLICT, REJECTED, CANCELLED }
    enum ResultCode {
        QUEUED, SENT, VERIFICATION_EXPIRED, RATE_LIMITED, CONTENT_CONFLICT,
        UNCERTAIN, REJECTED, CANCELLED
    }

    static final class Result {
        final ResultCode code;
        private Result(ResultCode code) { this.code = code; }
    }

    interface Executor { void execute(Runnable work); }
    interface Cleanup { void execute(Runnable cleanup); }
    /** May run on the worker or deadline thread; callers must post to UI and recheck attempt/state. */
    interface Callback { void completed(long attempt, Result result); }
    interface Transport {
        /** Takes ownership of requestBody and must wipe it on completion or cancellation. */
        Call open(byte[] requestBody);
    }
    interface Call {
        Response execute() throws IOException;
        void cancel();
    }

    static final class Response {
        final int status;
        final byte[] body;
        Response(int status, byte[] body) {
            this.status = status;
            this.body = body == null ? new byte[0] : body.clone();
        }
    }

    private static final Pattern REPORT_ID = Pattern.compile(
            "^\\{\\\"schemaVersion\\\":1,\\\"product\\\":\\\"horde-lantern-rt\\\",\\\"report\\\":\\{\\\"schemaVersion\\\":1,\\\"reportId\\\":\\\"([A-Za-z0-9_-]{8,96})\\\"");

    private final Executor executor;
    private final Transport transport;
    private final TimeoutScheduler attemptTimeouts;
    private final Cleanup cleanup;
    private State state = State.DRAFT;
    private byte[] frozenJson;
    private String reportId;
    private long generation;
    private ActiveCall active;
    private boolean benchmarkSummary;

    PlaytestReportSubmission(Executor executor, Transport transport) {
        this(executor, transport, REAL_TIMEOUTS, CLEANUP_EXECUTOR::execute);
    }

    PlaytestReportSubmission(Executor executor, Transport transport, TimeoutScheduler attemptTimeouts) {
        this(executor, transport, attemptTimeouts, CLEANUP_EXECUTOR::execute);
    }

    PlaytestReportSubmission(Executor executor, Transport transport, TimeoutScheduler attemptTimeouts, Cleanup cleanup) {
        if (executor == null || transport == null || attemptTimeouts == null || cleanup == null)
            throw new IllegalArgumentException("dependencies required");
        this.executor = executor;
        this.transport = transport;
        this.attemptTimeouts = attemptTimeouts;
        this.cleanup = cleanup;
    }

    synchronized State state() { return state; }
    synchronized long attempt() { return generation; }
    synchronized String reportId() { return reportId; }
    static boolean benchmarkSummarySendAvailable() { return BENCHMARK_SUMMARY_LIVE_SEND_ENABLED; }

    synchronized boolean begin(byte[] prepared, boolean explicitConsent) {
        if (!explicitConsent || state != State.DRAFT || prepared == null || prepared.length < 3 ||
                prepared.length > MAX_SUBMISSION_BYTES + 1 || prepared[0] != 0) return false;
        byte[] json = Arrays.copyOfRange(prepared, 1, prepared.length);
        final String text;
        try {
            text = StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(json)).toString();
        } catch (Exception malformed) { return false; }
        if (text.getBytes(StandardCharsets.UTF_8).length != json.length ||
                json.length > MAX_SUBMISSION_BYTES || !text.endsWith("}")) return false;
        Matcher match = REPORT_ID.matcher(text);
        if (!match.find()) return false;
        frozenJson = json;
        reportId = match.group(1);
        ++generation;
        state = State.READY;
        return true;
    }

    /**
     * Offline typed admission from the native-approved local owner. Remote
     * consent is separate from preparation; no parsing/reserialization, token,
     * screenshot or current renderer data enters the wrapper. The same UI thread
     * that owns review must call this method before closing/editing its owner.
     */
    synchronized boolean beginBenchmarkSummary(BenchmarkSummaryReview review, boolean explicitRemoteConsent) {
        if (!explicitRemoteConsent || state != State.DRAFT || review == null || !review.isPrepared()) return false;
        final String id = review.reportId();
        final byte[] local = review.exactJsonBytes();
        if (!BenchmarkSummaryReview.validUuid(id) || local == null || local.length < 2 ||
                local.length > MAX_BENCHMARK_LOCAL_BYTES || local[0] != '{' || local[local.length - 1] != '}') return false;
        byte[] consent = ",\"consentToSubmit\":true}".getBytes(StandardCharsets.UTF_8);
        long length = (long)local.length - 1 + consent.length;
        if (length > MAX_BENCHMARK_SUBMISSION_BYTES) return false;
        byte[] wrapper = Arrays.copyOf(local, (int)length);
        System.arraycopy(consent, 0, wrapper, local.length - 1, consent.length);
        frozenJson = wrapper;
        reportId = id;
        benchmarkSummary = true;
        ++generation;
        state = State.READY;
        return true;
    }

    boolean beginBenchmarkSummary(BenchmarkSummaryReview review) { return beginBenchmarkSummary(review, false); }

    /** Memory-only clone for offline review. This is not a transport capability. */
    synchronized byte[] benchmarkSummaryJsonForOfflineReview() {
        return benchmarkSummary && frozenJson != null ? frozenJson.clone() : null;
    }

    synchronized long retry() {
        if (state != State.RETRYABLE || frozenJson == null) return -1L;
        ++generation;
        state = State.READY;
        return generation;
    }

    /** Call only from an explicit foreground action; token is never retained after request construction. */
    boolean submit(long requestedAttempt, String freshToken, Callback callback) {
        if (callback == null) return false;
        final byte[] body;
        final ActiveCall activeCall;
        synchronized (this) {
            if (benchmarkSummary && !BENCHMARK_SUMMARY_LIVE_SEND_ENABLED) return false;
            if (state != State.READY || requestedAttempt != generation || !validToken(freshToken)) return false;
            body = buildRequest(frozenJson, freshToken,
                    benchmarkSummary ? MAX_BENCHMARK_SUBMISSION_BYTES : MAX_SUBMISSION_BYTES);
            if (body == null) return false;
            state = State.IN_FLIGHT;
            activeCall = new ActiveCall(requestedAttempt, body);
            active = activeCall;
        }
        try {
            Timeout attemptDeadline = attemptTimeouts.schedule(
                    () -> timeoutAttempt(requestedAttempt, activeCall, callback), MAX_ATTEMPT_MILLIS);
            activeCall.setDeadline(attemptDeadline);
        } catch (RuntimeException unavailable) {
            activeCall.clear();
            finish(requestedAttempt, activeCall, new Result(ResultCode.UNCERTAIN), callback);
            return true;
        }
        try {
            executor.execute(() -> runAttempt(requestedAttempt, activeCall, callback));
        } catch (RuntimeException rejected) {
            activeCall.clear();
            finish(requestedAttempt, activeCall, new Result(ResultCode.UNCERTAIN), callback);
        }
        return true;
    }

    private void runAttempt(long requestedAttempt, ActiveCall activeCall, Callback callback) {
        try {
            if (!activeCall.isCurrent()) return;
            byte[] requestBody = activeCall.requestBody();
            if (requestBody == null) return;
            Call call = transport.open(requestBody);
            activeCall.register(call);
            if (!activeCall.isCurrent()) return;
            Response response = call.execute();
            Result result = classify(response, reportId);
            finish(requestedAttempt, activeCall, result, callback);
        } catch (IOException | RuntimeException failure) {
            finish(requestedAttempt, activeCall, new Result(ResultCode.UNCERTAIN), callback);
        } finally {
            activeCall.clear();
        }
    }

    private void timeoutAttempt(long requestedAttempt, ActiveCall call, Callback callback) {
        Call toCancel;
        synchronized (this) {
            if (state != State.IN_FLIGHT || generation != requestedAttempt || active != call) return;
            active = null;
            state = State.RETRYABLE;
            toCancel = call.markCancelled();
            try { callback.completed(requestedAttempt, new Result(ResultCode.UNCERTAIN)); }
            catch (RuntimeException ignored) { /* No server or payload details cross the UI boundary. */ }
        }
        if (toCancel != null) call.queueCancel(toCancel);
    }

    private void finish(long requestedAttempt, ActiveCall call, Result result, Callback callback) {
        synchronized (this) {
            if (state != State.IN_FLIGHT || generation != requestedAttempt || active != call || call.cancelled) return;
            active = null;
            switch (result.code) {
                case QUEUED: state = State.QUEUED; break;
                case SENT: state = State.SENT; break;
                case CONTENT_CONFLICT: state = State.CONFLICT; break;
                case REJECTED: state = State.REJECTED; break;
                case CANCELLED: state = State.CANCELLED; break;
                default: state = State.RETRYABLE; break;
            }
            if (state == State.QUEUED || state == State.SENT || state == State.CONFLICT || state == State.REJECTED) {
                frozenJson = null;
            }
            // Serialize callback delivery with cancel(): once cancellation wins the
            // state lock, no completion from this attempt can reach the UI.
            try { callback.completed(requestedAttempt, result); }
            catch (RuntimeException ignored) { /* UI callback failures are not transport diagnostics. */ }
        }
    }

    /**
     * Stops only the current foreground attempt on lifecycle interruption. HTTP
     * cancellation cannot establish non-acceptance: retain the exact ID/body for
     * an explicit same-report retry with fresh verification after returning.
     * Invalidates old callbacks and never enqueues a retry by itself.
     */
    boolean interruptInFlight() {
        ActiveCall old;
        synchronized (this) {
            if (state != State.IN_FLIGHT) return false;
            ++generation;
            state = State.RETRYABLE;
            old = active;
            active = null;
        }
        if (old != null) old.cancel(); // Bounded asynchronous disconnect, never on the UI thread.
        return true;
    }

    void cancel() {
        ActiveCall old;
        synchronized (this) {
            if (state == State.CANCELLED) return;
            ++generation;
            state = State.CANCELLED;
            frozenJson = null;
            reportId = null;
            old = active;
            active = null;
        }
        if (old != null) old.cancel();
    }

    private final class ActiveCall {
        final long attempt;
        private Call call;
        private byte[] requestBody;
        private Timeout deadline;
        volatile boolean cancelled;
        ActiveCall(long attempt, byte[] requestBody) { this.attempt = attempt; this.requestBody = requestBody; }
        synchronized byte[] requestBody() { return cancelled ? null : requestBody; }
        void register(Call value) {
            boolean cancelNow;
            synchronized (this) { call = value; cancelNow = cancelled; }
            if (cancelNow) queueCancel(value);
        }
        synchronized void setDeadline(Timeout value) {
            deadline = value;
            if (cancelled) value.cancel();
        }
        synchronized void clear() {
            call = null;
            if (deadline != null) deadline.cancel();
            deadline = null;
            if (requestBody != null) Arrays.fill(requestBody, (byte) 0);
            requestBody = null;
        }
        void cancel() {
            Call toCancel = markCancelled();
            if (toCancel != null) queueCancel(toCancel);
        }
        synchronized Call markCancelled() {
            if (cancelled) return null;
            cancelled = true;
            if (deadline != null) deadline.cancel();
            deadline = null;
            if (requestBody != null) Arrays.fill(requestBody, (byte) 0);
            requestBody = null;
            return call;
        }
        private void queueCancel(Call value) {
            try { cleanup.execute(value::cancel); }
            catch (RuntimeException ignored) { /* Bounded cleanup saturation preserves uncertainty and UI responsiveness. */ }
        }
        boolean isCurrent() {
            synchronized (PlaytestReportSubmission.this) {
                return !cancelled && state == State.IN_FLIGHT && generation == attempt && active == this;
            }
        }
    }

    private static boolean validToken(String token) {
        if (token == null || token.isEmpty() || token.length() > MAX_TOKEN_CHARS) return false;
        for (int i = 0; i < token.length(); ++i) {
            char c = token.charAt(i);
            if (c < 0x20 || c > 0x7e) return false;
        }
        return true;
    }

    private static byte[] buildRequest(byte[] frozen, String token, int maximumBytes) {
        if (frozen == null || frozen.length < 2 || frozen[frozen.length - 1] != '}' || !validToken(token)) return null;
        StringBuilder escaped = new StringBuilder(token.length() + 2);
        for (int i = 0; i < token.length(); ++i) {
            char c = token.charAt(i);
            if (c == '"' || c == '\\') escaped.append('\\');
            escaped.append(c);
        }
        byte[] suffix = (",\"turnstileToken\":\"" + escaped + "\"}").getBytes(StandardCharsets.UTF_8);
        long length = (long) frozen.length - 1 + suffix.length;
        if (length > maximumBytes) return null;
        byte[] request = Arrays.copyOf(frozen, (int) length);
        System.arraycopy(suffix, 0, request, frozen.length - 1, suffix.length);
        return request;
    }

    private static Result classify(Response response, String reportId) {
        if (response == null || response.body.length > MAX_RESPONSE_BYTES) return new Result(ResultCode.UNCERTAIN);
        if (response.status == 200 || response.status == 202) {
            Ack ack = parseAck(response.body);
            if (ack == null || !reportId.equals(ack.id)) return new Result(ResultCode.UNCERTAIN);
            if (response.status == 202 && "accepted".equals(ack.status)) return new Result(ResultCode.QUEUED);
            if (response.status == 200 && "sent".equals(ack.status)) return new Result(ResultCode.SENT);
            return new Result(ResultCode.UNCERTAIN);
        }
        if (response.status == 403) return new Result(ResultCode.VERIFICATION_EXPIRED);
        if (response.status == 429) return new Result(ResultCode.RATE_LIMITED);
        if (response.status == 409) return new Result(ResultCode.CONTENT_CONFLICT);
        if (response.status >= 300 && response.status < 400) return new Result(ResultCode.REJECTED);
        if (response.status >= 500) return new Result(ResultCode.UNCERTAIN);
        return new Result(ResultCode.REJECTED);
    }

    private static final class Ack {
        String id;
        String status;
    }

    // The relay acknowledgment has exactly three flat fields. A small strict reader
    // avoids Android-only JSON classes and rejects duplicate/unknown keys.
    private static Ack parseAck(byte[] bytes) {
        if (bytes == null || bytes.length == 0 || bytes.length > MAX_RESPONSE_BYTES) return null;
        final String text;
        try {
            text = StandardCharsets.UTF_8.newDecoder().onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT).decode(ByteBuffer.wrap(bytes)).toString();
        } catch (Exception malformed) { return null; }
        AckParser parser = new AckParser(text);
        return parser.parse();
    }

    private static final class AckParser {
        final String text;
        int at;
        final Set<String> keys = new HashSet<>();
        Ack ack = new Ack();
        boolean ok;
        AckParser(String text) { this.text = text; }
        Ack parse() {
            ws(); if (!take('{')) return null; ws();
            if (take('}')) return null;
            while (true) {
                String key = string(); if (key == null || !keys.add(key)) return null;
                ws(); if (!take(':')) return null; ws();
                if ("ok".equals(key)) { if (!literal("true")) return null; ok = true; }
                else if ("id".equals(key)) { ack.id = string(); if (ack.id == null) return null; }
                else if ("status".equals(key)) { ack.status = string(); if (ack.status == null) return null; }
                else return null;
                ws();
                if (take('}')) break;
                if (!take(',')) return null;
                ws();
                if (at >= text.length() || text.charAt(at) == '}') return null;
            }
            ws();
            if (at != text.length() || !ok || keys.size() != 3 || ack.id == null || ack.status == null ||
                    !ack.id.matches("[A-Za-z0-9_-]{8,96}") ||
                    !("accepted".equals(ack.status) || "sent".equals(ack.status))) return null;
            return ack;
        }
        void ws() { while (at < text.length() && (text.charAt(at) == ' ' || text.charAt(at) == '\n' || text.charAt(at) == '\r' || text.charAt(at) == '\t')) ++at; }
        boolean take(char c) { if (at < text.length() && text.charAt(at) == c) { ++at; return true; } return false; }
        boolean literal(String value) { if (!text.startsWith(value, at)) return false; at += value.length(); return true; }
        String string() {
            if (!take('"')) return null;
            StringBuilder out = new StringBuilder();
            while (at < text.length()) {
                char c = text.charAt(at++);
                if (c == '"') return out.toString();
                if (c < 0x20) return null;
                if (c == '\\') {
                    if (at >= text.length()) return null;
                    char escaped = text.charAt(at++);
                    if (escaped == '"' || escaped == '\\' || escaped == '/') out.append(escaped);
                    else if (escaped == 'b') out.append('\b'); else if (escaped == 'f') out.append('\f');
                    else if (escaped == 'n') out.append('\n'); else if (escaped == 'r') out.append('\r');
                    else if (escaped == 't') out.append('\t');
                    else if (escaped == 'u') {
                        if (at + 4 > text.length()) return null;
                        int value = 0;
                        for (int i = 0; i < 4; ++i) {
                            int digit = Character.digit(text.charAt(at++), 16);
                            if (digit < 0) return null;
                            value = (value << 4) | digit;
                        }
                        out.append((char) value);
                    } else return null;
                } else out.append(c);
            }
            return null;
        }
    }

    interface Connection {
        void setInstanceFollowRedirects(boolean value);
        void setConnectTimeout(int value);
        void setReadTimeout(int value);
        void setUseCaches(boolean value);
        void setRequestMethod(String value) throws IOException;
        void setDoOutput(boolean value);
        void setRequestProperty(String name, String value);
        void setFixedLengthStreamingMode(int length);
        OutputStream getOutputStream() throws IOException;
        int getResponseCode() throws IOException;
        InputStream getInputStream() throws IOException;
        InputStream getErrorStream();
        void disconnect();
    }
    interface ConnectionFactory { Connection open(URL endpoint) throws IOException; }
    interface Timeout { void cancel(); }
    interface TimeoutScheduler { Timeout schedule(Runnable expired, long delayMillis); }

    private static final ScheduledExecutorService DEADLINE_EXECUTOR = Executors.newSingleThreadScheduledExecutor(task -> {
        Thread thread = new Thread(task, "HordeReportDeadline");
        thread.setDaemon(true);
        return thread;
    });
    private static final TimeoutScheduler REAL_TIMEOUTS = (expired, delay) -> {
        ScheduledFuture<?> future = DEADLINE_EXECUTOR.schedule(expired, delay, TimeUnit.MILLISECONDS);
        return () -> future.cancel(false);
    };
    private static final ThreadPoolExecutor CLEANUP_EXECUTOR = new ThreadPoolExecutor(1, 1, 0L,
            TimeUnit.MILLISECONDS, new ArrayBlockingQueue<>(8), task -> {
                Thread thread = new Thread(task, "HordeReportCleanup");
                thread.setDaemon(true);
                return thread;
            }, new ThreadPoolExecutor.DiscardPolicy());

    /** Fixed endpoint production transport. It never follows redirects or carries app credentials. */
    static final class HttpsTransport implements Transport {
        private final ConnectionFactory connections;

        HttpsTransport() { this(HttpsTransport::openHttpConnection); }
        HttpsTransport(ConnectionFactory connections) {
            if (connections == null) throw new IllegalArgumentException("transport dependencies required");
            this.connections = connections;
        }

        private static Connection openHttpConnection(URL endpoint) throws IOException {
            HttpURLConnection delegate = (HttpURLConnection) endpoint.openConnection();
            return new Connection() {
                @Override public void setInstanceFollowRedirects(boolean value) { delegate.setInstanceFollowRedirects(value); }
                @Override public void setConnectTimeout(int value) { delegate.setConnectTimeout(value); }
                @Override public void setReadTimeout(int value) { delegate.setReadTimeout(value); }
                @Override public void setUseCaches(boolean value) { delegate.setUseCaches(value); }
                @Override public void setRequestMethod(String value) throws IOException { delegate.setRequestMethod(value); }
                @Override public void setDoOutput(boolean value) { delegate.setDoOutput(value); }
                @Override public void setRequestProperty(String name, String value) { delegate.setRequestProperty(name, value); }
                @Override public void setFixedLengthStreamingMode(int length) { delegate.setFixedLengthStreamingMode(length); }
                @Override public OutputStream getOutputStream() throws IOException { return delegate.getOutputStream(); }
                @Override public int getResponseCode() throws IOException { return delegate.getResponseCode(); }
                @Override public InputStream getInputStream() throws IOException { return delegate.getInputStream(); }
                @Override public InputStream getErrorStream() { return delegate.getErrorStream(); }
                @Override public void disconnect() { delegate.disconnect(); }
            };
        }

        @Override public Call open(byte[] body) {
            final byte[] owned = body;
            return new Call() {
                private volatile Connection connection;
                private volatile boolean cancelled;
                private final AtomicBoolean cancelledSignal = new AtomicBoolean();
                @Override public Response execute() throws IOException {
                    if (cancelled) throw new IOException("cancelled");
                    final long deadlineNanos = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(MAX_ATTEMPT_MILLIS);
                    Connection opened = null;
                    try {
                        opened = connections.open(new URL(ENDPOINT));
                        connection = opened;
                        checkDeadline(cancelledSignal, deadlineNanos);
                        opened.setInstanceFollowRedirects(false);
                        opened.setConnectTimeout(remainingTimeoutMillis(deadlineNanos));
                        opened.setReadTimeout(remainingTimeoutMillis(deadlineNanos));
                        opened.setUseCaches(false);
                        opened.setRequestMethod("POST");
                        opened.setDoOutput(true);
                        opened.setRequestProperty("Content-Type", "application/json; charset=utf-8");
                        opened.setRequestProperty("Accept", "application/json");
                        opened.setRequestProperty("Cookie", "");
                        opened.setFixedLengthStreamingMode(owned.length);
                        try (OutputStream output = opened.getOutputStream()) { output.write(owned); }
                        checkDeadline(cancelledSignal, deadlineNanos);
                        opened.setReadTimeout(remainingTimeoutMillis(deadlineNanos));
                        int status = opened.getResponseCode();
                        checkDeadline(cancelledSignal, deadlineNanos);
                        InputStream input = status >= 400 ? opened.getErrorStream() : opened.getInputStream();
                        byte[] response = input == null ? new byte[0] :
                                readBounded(input, cancelledSignal, opened, deadlineNanos);
                        checkDeadline(cancelledSignal, deadlineNanos);
                        return new Response(status, response);
                    } finally {
                        if (opened != null) opened.disconnect();
                        connection = null;
                        Arrays.fill(owned, (byte) 0);
                    }
                }
                @Override public void cancel() {
                    cancelled = true;
                    cancelledSignal.set(true);
                    Connection opened = connection;
                    if (opened != null) opened.disconnect();
                    Arrays.fill(owned, (byte) 0);
                }
            };
        }
    }

    private static void checkDeadline(AtomicBoolean cancelled, long deadlineNanos) throws IOException {
        if (cancelled.get() || System.nanoTime() >= deadlineNanos) throw new IOException("cancelled or timed out");
    }

    private static int remainingTimeoutMillis(long deadlineNanos) throws IOException {
        long remainingNanos = deadlineNanos - System.nanoTime();
        if (remainingNanos <= 0) throw new IOException("attempt deadline exceeded");
        long remainingMillis = (remainingNanos + 999_999L) / 1_000_000L;
        return (int) Math.min(TIMEOUT_MILLIS, Math.max(1L, remainingMillis));
    }

    private static byte[] readBounded(InputStream input, AtomicBoolean cancelled,
            Connection connection, long deadlineNanos) throws IOException {
        try (InputStream source = input; ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[1024];
            int count;
            while (true) {
                checkDeadline(cancelled, deadlineNanos);
                connection.setReadTimeout(remainingTimeoutMillis(deadlineNanos));
                count = source.read(buffer);
                checkDeadline(cancelled, deadlineNanos);
                if (count == -1) break;
                if (output.size() + count > MAX_RESPONSE_BYTES) throw new IOException("response too large");
                output.write(buffer, 0, count);
            }
            return output.toByteArray();
        }
    }
}
