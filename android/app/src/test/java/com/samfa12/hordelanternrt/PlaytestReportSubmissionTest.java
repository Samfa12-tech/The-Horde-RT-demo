package com.samfa12.hordelanternrt;

import org.junit.Test;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import static org.junit.Assert.*;

public final class PlaytestReportSubmissionTest {
    private static final String ID = "opaque-123";
    private static byte[] json() {
        return ("{\"schemaVersion\":1,\"product\":\"horde-lantern-rt\",\"report\":{" +
                "\"schemaVersion\":1,\"reportId\":\"" + ID + "\",\"note\":\"dragon 龍 \\\"\"}," +
                "\"consentToSubmit\":true,\"includeDiagnostics\":false,\"includeScreenshot\":false}")
                .getBytes(StandardCharsets.UTF_8);
    }
    private static byte[] prepared() {
        byte[] json = json(), result = new byte[json.length + 1];
        System.arraycopy(json, 0, result, 1, json.length);
        return result;
    }
    private static final class QueuedExecutor implements PlaytestReportSubmission.Executor {
        final List<Runnable> jobs = new ArrayList<>();
        @Override public void execute(Runnable work) { jobs.add(work); }
        void runNext() { jobs.remove(0).run(); }
    }
    private static final class FakeCall implements PlaytestReportSubmission.Call {
        final PlaytestReportSubmission.Response response;
        final CountDownLatch entered, release;
        volatile boolean cancelled;
        FakeCall(PlaytestReportSubmission.Response response) { this(response, null, null); }
        FakeCall(PlaytestReportSubmission.Response response, CountDownLatch entered, CountDownLatch release) {
            this.response = response; this.entered = entered; this.release = release;
        }
        @Override public PlaytestReportSubmission.Response execute() throws IOException {
            if (entered != null) {
                entered.countDown();
                try { release.await(5, TimeUnit.SECONDS); }
                catch (InterruptedException interrupted) { Thread.currentThread().interrupt(); throw new IOException(); }
            }
            if (cancelled) throw new IOException("cancelled");
            return response;
        }
        @Override public void cancel() { cancelled = true; if (release != null) release.countDown(); }
    }
    private static final class BlockingCancelCall implements PlaytestReportSubmission.Call {
        final CountDownLatch entered = new CountDownLatch(1), executeRelease = new CountDownLatch(1), cancelEntered = new CountDownLatch(1), cancelRelease = new CountDownLatch(1);
        volatile boolean cancelled;
        @Override public PlaytestReportSubmission.Response execute() throws IOException {
            entered.countDown();
            try { executeRelease.await(5, TimeUnit.SECONDS); }
            catch (InterruptedException interrupted) { Thread.currentThread().interrupt(); throw new IOException(); }
            if (cancelled) throw new IOException("cancelled");
            return ack(202, ID, "accepted");
        }
        @Override public void cancel() {
            cancelled = true;
            cancelEntered.countDown();
            try { cancelRelease.await(5, TimeUnit.SECONDS); }
            catch (InterruptedException interrupted) { Thread.currentThread().interrupt(); }
            executeRelease.countDown();
        }
    }
    private static PlaytestReportSubmission.Response ack(int status, String id, String acceptedStatus) {
        return new PlaytestReportSubmission.Response(status,
                ("{\"ok\":true,\"id\":\"" + id + "\",\"status\":\"" + acceptedStatus + "\"}")
                        .getBytes(StandardCharsets.UTF_8));
    }
    private static final class Fixture {
        final QueuedExecutor executor = new QueuedExecutor();
        final List<byte[]> requests = new ArrayList<>();
        final List<byte[]> ownedRequests = new ArrayList<>();
        final List<FakeCall> calls = new ArrayList<>();
        PlaytestReportSubmission.Response next = ack(202, ID, "accepted");
        final PlaytestReportSubmission owner = new PlaytestReportSubmission(executor, body -> {
            requests.add(body.clone());
            ownedRequests.add(body);
            FakeCall call = new FakeCall(next);
            calls.add(call);
            return call;
        });
        final List<PlaytestReportSubmission.Result> results = new ArrayList<>();
        final PlaytestReportSubmission.Callback callback = (attempt, result) -> results.add(result);
        final byte[] beginBuffer = prepared();
        Fixture() { assertTrue(owner.begin(beginBuffer, true)); }
    }

    @Test public void consentEnvelopeUtf8AndTotalBoundsAreRequired() {
        QueuedExecutor executor = new QueuedExecutor();
        PlaytestReportSubmission owner = new PlaytestReportSubmission(executor, bytes -> { fail("no request expected"); return null; });
        assertFalse(owner.begin(prepared(), false));
        assertFalse(owner.begin(null, true));
        assertFalse(owner.begin(new byte[] {1, '{', '}'}, true));
        assertFalse(owner.begin(new byte[] {0, '{', (byte) 0xff, '}'}, true));
        byte[] oversized = new byte[PlaytestReportSubmission.MAX_SUBMISSION_BYTES + 2];
        oversized[0] = 0;
        assertFalse(owner.begin(oversized, true));
        byte[] invalidProduct = ("{\"schemaVersion\":1,\"product\":\"other\",\"report\":{\"schemaVersion\":1,\"reportId\":\"" + ID + "\"}}")
                .getBytes(StandardCharsets.UTF_8);
        byte[] envelope = new byte[invalidProduct.length + 1];
        System.arraycopy(invalidProduct, 0, envelope, 1, invalidProduct.length);
        assertFalse(owner.begin(envelope, true));
        assertEquals(PlaytestReportSubmission.State.DRAFT, owner.state());
    }

    @Test public void explicitRetryFreezesNativeJsonButUsesFreshEscapedOneUseToken() {
        Fixture f = new Fixture();
        byte[] source = f.beginBuffer;
        String originalJson = new String(source, 1, source.length - 1, StandardCharsets.UTF_8);
        long first = f.owner.attempt();
        source[8] ^= 1;
        assertFalse(f.owner.submit(first, "bad\ntoken", f.callback));
        assertTrue(f.owner.submit(first, "first\\\"token", f.callback));
        assertFalse(f.owner.submit(first, "duplicate", f.callback));
        f.executor.runNext();
        assertEquals(PlaytestReportSubmission.ResultCode.QUEUED, f.results.get(0).code);
        assertEquals(PlaytestReportSubmission.State.QUEUED, f.owner.state());
        assertEquals(originalJson.substring(0, originalJson.length() - 1),
                new String(f.requests.get(0), 0, f.requests.get(0).length - ",\"turnstileToken\":\"first\\\\\\\"token\"}".length(), StandardCharsets.UTF_8));
        assertTrue(new String(f.requests.get(0), StandardCharsets.UTF_8).endsWith(",\"turnstileToken\":\"first\\\\\\\"token\"}"));
        assertEquals(-1L, f.owner.retry());
        assertTrue(allZero(f.ownedRequests.get(0)));
    }

    @Test public void maximumEscapedTurnstileTokenFitsAndOversizeTokenIsRejected() {
        Fixture f = new Fixture();
        String token = "\"\\".repeat(1024);
        assertEquals(2048, token.length());
        assertTrue(f.owner.submit(f.owner.attempt(), token, f.callback));
        f.executor.runNext();
        String request = new String(f.requests.get(0), StandardCharsets.UTF_8);
        StringBuilder escaped = new StringBuilder(",\"turnstileToken\":\"");
        for (int i = 0; i < token.length(); ++i) {
            char character = token.charAt(i);
            if (character == '"' || character == '\\') escaped.append('\\');
            escaped.append(character);
        }
        assertTrue(request.endsWith(escaped.append("\"}").toString()));
        assertTrue(f.requests.get(0).length <= PlaytestReportSubmission.MAX_SUBMISSION_BYTES);
        assertTrue(allZero(f.ownedRequests.get(0)));

        Fixture oversized = new Fixture();
        assertFalse(oversized.owner.submit(oversized.owner.attempt(), "x".repeat(2049), oversized.callback));
        assertEquals(PlaytestReportSubmission.State.READY, oversized.owner.state());
    }

    @Test public void networkUncertaintyRetriesSameIdAndNativeBytesWithFreshToken() {
        Fixture f = new Fixture();
        f.next = null;
        assertTrue(f.owner.submit(f.owner.attempt(), "token-one", f.callback));
        f.executor.runNext();
        assertEquals(PlaytestReportSubmission.ResultCode.UNCERTAIN, f.results.get(0).code);
        assertEquals(PlaytestReportSubmission.State.RETRYABLE, f.owner.state());
        long second = f.owner.retry();
        assertTrue(second > 0);
        f.next = ack(202, ID, "accepted");
        assertTrue(f.owner.submit(second, "token-two", f.callback));
        f.executor.runNext();
        assertEquals(PlaytestReportSubmission.ResultCode.QUEUED, f.results.get(1).code);
        assertEquals(2, f.requests.size());
        assertTrue(new String(f.requests.get(0), StandardCharsets.UTF_8).contains("token-one"));
        assertTrue(new String(f.requests.get(1), StandardCharsets.UTF_8).contains("token-two"));
        assertFalse(Arrays.equals(f.requests.get(0), f.requests.get(1)));
        assertTrue(new String(f.requests.get(0), StandardCharsets.UTF_8).replace(",\"turnstileToken\":\"token-one\"}", "}")
                .equals(new String(f.requests.get(1), StandardCharsets.UTF_8).replace(",\"turnstileToken\":\"token-two\"}", "}")));
        assertEquals(ID, f.owner.reportId());
    }

    @Test public void lifecycleInterruptionBeforeDispatchRetainsIdentityWithoutBackgroundRetry() {
        Fixture f = new Fixture();
        long first = f.owner.attempt();
        assertTrue(f.owner.submit(first, "old-token", f.callback));
        assertTrue(f.owner.interruptInFlight());
        assertEquals(PlaytestReportSubmission.State.RETRYABLE, f.owner.state());
        assertEquals(ID, f.owner.reportId());
        assertFalse(f.owner.interruptInFlight());
        f.executor.runNext();
        assertTrue(f.requests.isEmpty());
        assertTrue(f.results.isEmpty());
        assertTrue(f.executor.jobs.isEmpty()); // Interruption is not permission to send again.
        assertFalse(f.owner.submit(first, "stale-token", f.callback));
        long retry = f.owner.retry();
        assertTrue(retry > first);
        assertTrue(f.owner.submit(retry, "fresh-token", f.callback));
        f.executor.runNext();
        assertEquals(ID, f.owner.reportId());
        assertEquals(1, f.requests.size());
        assertEquals(new String(json(), StandardCharsets.UTF_8),
                new String(f.requests.get(0), StandardCharsets.UTF_8)
                        .replace(",\"turnstileToken\":\"fresh-token\"}", "}"));
        assertEquals(1, f.results.size());
        assertEquals(PlaytestReportSubmission.ResultCode.QUEUED, f.results.get(0).code);
        assertFalse(f.owner.interruptInFlight()); // Do not turn acknowledged acceptance into uncertainty.
    }

    @Test public void lifecycleInterruptionDuringHttpSuppressesOldCallbackAndPreservesFrozenRetry() throws Exception {
        QueuedExecutor executor = new QueuedExecutor();
        QueuedExecutor cleanup = new QueuedExecutor();
        CountDownLatch entered = new CountDownLatch(1), release = new CountDownLatch(1);
        List<byte[]> requests = new ArrayList<>();
        List<PlaytestReportSubmission.Result> results = new ArrayList<>();
        FakeCall blocking = new FakeCall(ack(202, ID, "accepted"), entered, release);
        PlaytestReportSubmission owner = new PlaytestReportSubmission(executor, body -> {
            requests.add(body.clone());
            return requests.size() == 1 ? blocking : new FakeCall(ack(200, ID, "sent"));
        }, (work, delay) -> () -> {}, cleanup::execute);
        assertTrue(owner.begin(prepared(), true));
        long first = owner.attempt();
        assertTrue(owner.submit(first, "old-token", (attempt, result) -> results.add(result)));
        Runnable request = executor.jobs.remove(0);
        Thread worker = new Thread(request, "report-lifecycle-fixture");
        worker.start();
        try {
            assertTrue(entered.await(2, TimeUnit.SECONDS));
            assertTrue(owner.interruptInFlight());
            assertEquals(PlaytestReportSubmission.State.RETRYABLE, owner.state());
            assertEquals(ID, owner.reportId());
            assertFalse(blocking.cancelled); // Disconnect is off the calling/UI thread.
            assertEquals(1, cleanup.jobs.size());
            cleanup.runNext();
            worker.join(2000);
            assertFalse(worker.isAlive());
            assertTrue(results.isEmpty());
            assertTrue(executor.jobs.isEmpty());
            assertTrue(owner.submit(owner.retry(), "fresh-token", (attempt, result) -> results.add(result)));
            executor.runNext();
            assertEquals(2, requests.size());
            assertEquals(new String(requests.get(0), StandardCharsets.UTF_8)
                            .replace(",\"turnstileToken\":\"old-token\"}", "}"),
                    new String(requests.get(1), StandardCharsets.UTF_8)
                            .replace(",\"turnstileToken\":\"fresh-token\"}", "}"));
            assertEquals(1, results.size());
            assertEquals(PlaytestReportSubmission.ResultCode.SENT, results.get(0).code);
        } finally {
            release.countDown();
            worker.join(2000);
            owner.cancel();
        }
    }

    @Test public void typedAcknowledgmentsAndRetryStatusesAreDistinct() {
        Fixture f = new Fixture();
        f.next = ack(200, ID, "sent");
        assertTrue(f.owner.submit(f.owner.attempt(), "token", f.callback)); f.executor.runNext();
        assertEquals(PlaytestReportSubmission.ResultCode.SENT, f.results.get(0).code);
        assertEquals(PlaytestReportSubmission.State.SENT, f.owner.state());

        f = new Fixture();
        f.next = new PlaytestReportSubmission.Response(403, new byte[0]);
        assertTrue(f.owner.submit(f.owner.attempt(), "token", f.callback)); f.executor.runNext();
        assertEquals(PlaytestReportSubmission.ResultCode.VERIFICATION_EXPIRED, f.results.get(0).code);
        assertTrue(f.owner.retry() > 0);

        f = new Fixture();
        f.next = new PlaytestReportSubmission.Response(429, new byte[0]);
        assertTrue(f.owner.submit(f.owner.attempt(), "token", f.callback)); f.executor.runNext();
        assertEquals(PlaytestReportSubmission.ResultCode.RATE_LIMITED, f.results.get(0).code);

        f = new Fixture();
        f.next = new PlaytestReportSubmission.Response(409, new byte[0]);
        assertTrue(f.owner.submit(f.owner.attempt(), "token", f.callback)); f.executor.runNext();
        assertEquals(PlaytestReportSubmission.ResultCode.CONTENT_CONFLICT, f.results.get(0).code);
        assertEquals(PlaytestReportSubmission.State.CONFLICT, f.owner.state());
        assertEquals(-1L, f.owner.retry());
    }

    @Test public void malformedWrongIdDuplicateFieldsRedirectAndOversizeAreNeverAcknowledged() {
        byte[][] bad = {
                "not json".getBytes(StandardCharsets.UTF_8),
                ack(202, "different-123", "accepted").body,
                ("{\"ok\":true,\"ok\":true,\"id\":\"" + ID + "\",\"status\":\"accepted\"}").getBytes(StandardCharsets.UTF_8),
                ("{\"ok\":true,\"id\":\"" + ID + "\",\"status\":\"accepted\",\"extra\":1}").getBytes(StandardCharsets.UTF_8),
                ("{\"ok\":true,\"id\":\"" + ID + "\",\"status\":\"accepted\",}").getBytes(StandardCharsets.UTF_8),
                new byte[PlaytestReportSubmission.MAX_RESPONSE_BYTES + 1]
        };
        for (byte[] response : bad) {
            Fixture f = new Fixture();
            f.next = new PlaytestReportSubmission.Response(202, response);
            assertTrue(f.owner.submit(f.owner.attempt(), "token", f.callback)); f.executor.runNext();
            assertEquals(PlaytestReportSubmission.ResultCode.UNCERTAIN, f.results.get(0).code);
            assertEquals(PlaytestReportSubmission.State.RETRYABLE, f.owner.state());
        }
        Fixture redirect = new Fixture();
        redirect.next = new PlaytestReportSubmission.Response(302, new byte[0]);
        assertTrue(redirect.owner.submit(redirect.owner.attempt(), "token", redirect.callback)); redirect.executor.runNext();
        assertEquals(PlaytestReportSubmission.ResultCode.REJECTED, redirect.results.get(0).code);
    }

    @Test public void cancelBeforeTransportAndDuringRequestSuppressesCompletionAndDisconnects() throws Exception {
        Fixture before = new Fixture();
        assertTrue(before.owner.submit(before.owner.attempt(), "token", before.callback));
        before.owner.cancel();
        before.executor.runNext();
        assertTrue(before.calls.isEmpty());
        assertTrue(before.results.isEmpty());
        assertNull(before.owner.reportId());

        CountDownLatch entered = new CountDownLatch(1), release = new CountDownLatch(1);
        FakeCall blocking = new FakeCall(ack(202, ID, "accepted"), entered, release);
        QueuedExecutor notUsed = new QueuedExecutor();
        PlaytestReportSubmission owner = new PlaytestReportSubmission(Runnable::run, body -> blocking);
        assertTrue(owner.begin(prepared(), true));
        List<PlaytestReportSubmission.Result> results = new ArrayList<>();
        Thread worker = new Thread(() -> owner.submit(owner.attempt(), "token", (attempt, result) -> results.add(result)));
        worker.start();
        assertTrue(entered.await(2, TimeUnit.SECONDS));
        owner.cancel();
        worker.join(3000);
        assertTrue(blocking.cancelled);
        assertTrue(results.isEmpty());
        assertEquals(PlaytestReportSubmission.State.CANCELLED, owner.state());
    }

    private static boolean allZero(byte[] bytes) {
        for (byte value : bytes) if (value != 0) return false;
        return true;
    }

    private static final class FakeTimeouts implements PlaytestReportSubmission.TimeoutScheduler {
        Runnable task;
        long delay;
        boolean cancelled;
        @Override public PlaytestReportSubmission.Timeout schedule(Runnable expired, long delayMillis) {
            task = expired; delay = delayMillis;
            return () -> cancelled = true;
        }
        void expire() { task.run(); }
    }

    private static final class FakeConnection implements PlaytestReportSubmission.Connection {
        final ByteArrayOutputStream request = new ByteArrayOutputStream();
        final Map<String, String> headers = new HashMap<>();
        InputStream response = new ByteArrayInputStream(ack(202, ID, "accepted").body);
        OutputStream requestStream;
        int status = 202, connectTimeout, readTimeout, contentLength;
        boolean followRedirects = true, useCaches = true, doOutput, disconnected;
        String method;
        Runnable onDisconnect;
        @Override public void setInstanceFollowRedirects(boolean value) { followRedirects = value; }
        @Override public void setConnectTimeout(int value) { connectTimeout = value; }
        @Override public void setReadTimeout(int value) { readTimeout = value; }
        @Override public void setUseCaches(boolean value) { useCaches = value; }
        @Override public void setRequestMethod(String value) { method = value; }
        @Override public void setDoOutput(boolean value) { doOutput = value; }
        @Override public void setRequestProperty(String name, String value) { headers.put(name.toLowerCase(), value); }
        @Override public void setFixedLengthStreamingMode(int length) { contentLength = length; }
        @Override public OutputStream getOutputStream() { return requestStream == null ? request : requestStream; }
        @Override public int getResponseCode() { return status; }
        @Override public InputStream getInputStream() { return response; }
        @Override public InputStream getErrorStream() { return response; }
        @Override public void disconnect() { disconnected = true; if (onDisconnect != null) onDisconnect.run(); }
    }

    @Test public void fixedHttpsRequestDoesNotFollowRedirectsOrSendCredentialsAndClosesStreams() throws Exception {
        FakeConnection connection = new FakeConnection();
        connection.status = 302;
        connection.response = new ByteArrayInputStream(new byte[0]);
        final URL[] openedUrl = new URL[1];
        PlaytestReportSubmission.HttpsTransport transport = new PlaytestReportSubmission.HttpsTransport(url -> {
            openedUrl[0] = url;
            return connection;
        });
        byte[] body = "request bytes".getBytes(StandardCharsets.UTF_8);
        PlaytestReportSubmission.Response result = transport.open(body).execute();
        assertEquals(302, result.status);
        assertEquals(PlaytestReportSubmission.ENDPOINT, openedUrl[0].toString());
        assertFalse(connection.followRedirects);
        assertFalse(connection.useCaches);
        assertEquals("POST", connection.method);
        assertTrue(connection.doOutput);
        assertEquals(PlaytestReportSubmission.TIMEOUT_MILLIS, connection.connectTimeout);
        assertTrue(connection.readTimeout > 0 && connection.readTimeout <= PlaytestReportSubmission.TIMEOUT_MILLIS);
        assertEquals("", connection.headers.get("cookie"));
        assertFalse(connection.headers.containsKey("authorization"));
        assertEquals("application/json; charset=utf-8", connection.headers.get("content-type"));
        assertEquals(body.length, connection.contentLength);
        assertTrue(connection.disconnected);
        assertTrue(allZero(body));
    }

    @Test public void actualResponseStreamCapClosesStreamDisconnectsAndWipesRequest() throws Exception {
        FakeConnection connection = new FakeConnection();
        final boolean[] closed = {false};
        connection.response = new ByteArrayInputStream(new byte[PlaytestReportSubmission.MAX_RESPONSE_BYTES + 1]) {
            @Override public void close() throws IOException { closed[0] = true; super.close(); }
        };
        byte[] body = "bounded request".getBytes(StandardCharsets.UTF_8);
        PlaytestReportSubmission.HttpsTransport transport = new PlaytestReportSubmission.HttpsTransport(url -> connection);
        try { transport.open(body).execute(); fail("oversized stream must fail"); }
        catch (IOException expected) { }
        assertTrue(closed[0]);
        assertTrue(connection.disconnected);
        assertTrue(allZero(body));
    }

    @Test public void cancelBeforeWorkerStartsWipesOwnedRequestWithoutOpeningTransport() throws Exception {
        Fixture f = new Fixture();
        assertTrue(f.owner.submit(f.owner.attempt(), "token", f.callback));
        Field activeField = PlaytestReportSubmission.class.getDeclaredField("active");
        activeField.setAccessible(true);
        Object active = activeField.get(f.owner);
        Field bodyField = active.getClass().getDeclaredField("requestBody");
        bodyField.setAccessible(true);
        byte[] ownedBody = (byte[]) bodyField.get(active);
        assertNotNull(ownedBody);
        f.owner.cancel();
        assertTrue(allZero(ownedBody));
        f.executor.runNext();
        assertTrue(f.calls.isEmpty());
        assertTrue(f.results.isEmpty());
    }

    @Test public void foregroundAttemptDeadlineCompletesAsUncertainEvenIfTransportIsStillBlocked() throws Exception {
        CountDownLatch entered = new CountDownLatch(1), release = new CountDownLatch(1);
        FakeTimeouts deadlines = new FakeTimeouts();
        FakeCall blocked = new FakeCall(ack(202, ID, "accepted"), entered, release);
        List<Runnable> cleanup = new ArrayList<>();
        PlaytestReportSubmission owner = new PlaytestReportSubmission(Runnable::run, body -> blocked, deadlines, cleanup::add);
        assertTrue(owner.begin(prepared(), true));
        List<PlaytestReportSubmission.Result> results = new ArrayList<>();
        Thread worker = new Thread(() -> owner.submit(owner.attempt(), "token", (attempt, result) -> results.add(result)));
        worker.start();
        assertTrue(entered.await(2, TimeUnit.SECONDS));
        assertEquals(PlaytestReportSubmission.MAX_ATTEMPT_MILLIS, deadlines.delay);
        deadlines.expire();
        assertEquals(PlaytestReportSubmission.ResultCode.UNCERTAIN, results.get(0).code);
        assertEquals(PlaytestReportSubmission.State.RETRYABLE, owner.state());
        assertFalse(blocked.cancelled);
        assertEquals(1, cleanup.size());
        cleanup.remove(0).run();
        worker.join(3000);
        assertFalse(worker.isAlive());
        assertTrue(blocked.cancelled);
    }

    @Test public void deadlineStartsBeforeExecutorQueueAndLateQueuedWorkIsIgnored() {
        QueuedExecutor executor = new QueuedExecutor();
        FakeTimeouts deadlines = new FakeTimeouts();
        int[] opens = {0};
        PlaytestReportSubmission owner = new PlaytestReportSubmission(executor, body -> {
            ++opens[0]; return new FakeCall(ack(202, ID, "accepted"));
        }, deadlines);
        assertTrue(owner.begin(prepared(), true));
        List<PlaytestReportSubmission.Result> results = new ArrayList<>();
        assertTrue(owner.submit(owner.attempt(), "token", (attempt, result) -> results.add(result)));
        assertEquals(1, executor.jobs.size());
        assertEquals(PlaytestReportSubmission.MAX_ATTEMPT_MILLIS, deadlines.delay);
        deadlines.expire();
        assertEquals(PlaytestReportSubmission.ResultCode.UNCERTAIN, results.get(0).code);
        assertEquals(PlaytestReportSubmission.State.RETRYABLE, owner.state());
        executor.runNext();
        assertEquals(0, opens[0]);
        assertEquals(1, results.size());
    }

    @Test public void cancellationReturnsBeforePotentiallyBlockingDisconnectCleanup() throws Exception {
        BlockingCancelCall blocked = new BlockingCancelCall();
        List<Runnable> cleanup = new ArrayList<>();
        PlaytestReportSubmission owner = new PlaytestReportSubmission(Runnable::run, body -> blocked,
                (task, delay) -> () -> {}, cleanup::add);
        assertTrue(owner.begin(prepared(), true));
        List<PlaytestReportSubmission.Result> results = new ArrayList<>();
        Thread worker = new Thread(() -> owner.submit(owner.attempt(), "token", (attempt, result) -> results.add(result)));
        worker.start();
        assertTrue(blocked.entered.await(2, TimeUnit.SECONDS));
        owner.cancel();
        assertEquals(PlaytestReportSubmission.State.CANCELLED, owner.state());
        assertTrue(results.isEmpty());
        assertEquals(1, cleanup.size());
        Thread cleanupWorker = new Thread(cleanup.remove(0));
        cleanupWorker.start();
        assertTrue(blocked.cancelEntered.await(2, TimeUnit.SECONDS));
        assertEquals(PlaytestReportSubmission.State.CANCELLED, owner.state());
        assertTrue(results.isEmpty());
        blocked.cancelRelease.countDown();
        cleanupWorker.join(2000);
        worker.join(2000);
        assertFalse(cleanupWorker.isAlive());
        assertFalse(worker.isAlive());
    }

    @Test public void noAutomaticRetryAndResponseDoesNotLeakServerText() {
        Fixture f = new Fixture();
        f.next = new PlaytestReportSubmission.Response(503, "private response detail".getBytes(StandardCharsets.UTF_8));
        assertTrue(f.owner.submit(f.owner.attempt(), "token", f.callback)); f.executor.runNext();
        assertEquals(1, f.requests.size());
        assertEquals(PlaytestReportSubmission.ResultCode.UNCERTAIN, f.results.get(0).code);
        assertEquals(PlaytestReportSubmission.State.RETRYABLE, f.owner.state());
        assertEquals("UNCERTAIN", f.results.get(0).code.name());
    }
}
