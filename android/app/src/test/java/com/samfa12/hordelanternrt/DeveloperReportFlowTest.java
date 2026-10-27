package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.nio.charset.StandardCharsets;
import java.io.ByteArrayOutputStream;
import java.util.*;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class DeveloperReportFlowTest {
    static final String ID = BenchmarkSummaryReviewTest.REPORT, UTC = BenchmarkSummaryReviewTest.UTC;
    static byte[] prepared(String model) {
        // Native builder is tested separately. This bridge fixture intentionally
        // has no RT scene, GPU, device ID, logs or renderer extent.
        return ("\0{\"schemaVersion\":2, \"product\":\"horde-lantern-rt\",\"reportKind\":\"compatibility\","
                + "\"consentToPrepare\":true,\"report\":{\"schemaVersion\":1,\"reportId\":\"" + ID
                + "\",\"capturedAtUtc\":\"" + UTC + "\",\"model\":\"" + model + "\",\"rtPresented\":false}}")
                .getBytes(StandardCharsets.UTF_8);
    }
    static CompatibilityReportReview review(String model) {
        CompatibilityReportReview review = new CompatibilityReportReview(7);
        assertTrue(review.prepare(7, true, ID, UTC, (attempt,id,utc,consent) -> prepared(model)));
        return review;
    }
    static final class Executor implements PlaytestReportSubmission.Executor {
        final List<Runnable> work = new ArrayList<>();
        public void execute(Runnable value) { work.add(value); }
        void drain() { while (!work.isEmpty()) work.remove(0).run(); }
    }
    static final class LocalRelay implements PlaytestReportSubmission.Transport {
        final Map<String,String> accepted = new HashMap<>();
        final List<String> requests = new ArrayList<>();
        boolean loseFirstReceipt = true;
        String destination = "developer-private", kind = "compatibility";
        int deliveries;
        public boolean admitsPrivateDeveloperReports() { return true; }
        public PlaytestReportSubmission.Call open(byte[] body) {
            String request = new String(body, StandardCharsets.UTF_8); requests.add(request);
            return new PlaytestReportSubmission.Call() {
                public PlaytestReportSubmission.Response execute() {
                    String frozen = request.substring(0, request.lastIndexOf(",\"turnstileToken\"")) + "}";
                    String prior = accepted.putIfAbsent(ID, frozen);
                    if (prior != null && !prior.equals(frozen)) return response(409, "");
                    if (prior == null) deliveries++;
                    if (loseFirstReceipt) { loseFirstReceipt = false; return response(503, ""); }
                    return response(202, "{\"ok\":true,\"id\":\"" + ID + "\",\"status\":\"accepted\","
                            + "\"destination\":\"" + destination + "\",\"reportKind\":\"" + kind + "\"}");
                }
                public void cancel() { Arrays.fill(body, (byte)0); }
            };
        }
    }
    static PlaytestReportSubmission.Response response(int status, String body) {
        return new PlaytestReportSubmission.Response(status, body.getBytes(StandardCharsets.UTF_8));
    }
    static PlaytestReportSubmission owner(Executor executor, PlaytestReportSubmission.Transport relay) {
        return new PlaytestReportSubmission(executor, relay, (expired,ms) -> () -> {}, Runnable::run);
    }
    @Test public void compatibilityReviewNeedsConsentAndCurrentAttemptAndKeepsExactLocalExport() {
        CompatibilityReportReview review = new CompatibilityReportReview(7);
        final int[] calls = {0};
        CompatibilityReportReview.Preparer prepare = (attempt,id,utc,consent) -> { calls[0]++; return prepared("Example model"); };
        assertFalse(review.prepare(7,false,ID,UTC,prepare));
        assertFalse(review.prepare(8,true,ID,UTC,prepare));
        assertEquals(0,calls[0]);
        assertTrue(review.prepare(7,true,ID,UTC,prepare));
        byte[] original = review.exactJsonBytes(), copy = review.exactJsonBytes(); copy[0] = '!';
        assertArrayEquals(original,review.exactJsonBytes());
        assertTrue(review.beginSave());
        PlaytestReportExport export = review.exportOwner(); long token = export.token();
        byte[] bytes = export.startWrite(token); assertTrue(export.complete(token,false));
        assertTrue(review.beginSave()); token = export.token(); bytes = export.startWrite(token);
        ByteArrayOutputStream file = new ByteArrayOutputStream();
        assertTrue(PlaytestReportExport.writeApproved(export,token,bytes,() -> file));
        assertArrayEquals(original,file.toByteArray());
        assertFalse(review.json().contains("consentToSubmit"));
        review.close(); assertNull(review.exactJsonBytes());
    }
    @Test public void privateRetryDeduplicatesIdenticalPayloadAndRejectsChangedContent() {
        Executor executor = new Executor(); LocalRelay relay = new LocalRelay();
        PlaytestReportSubmission owner = owner(executor,relay);
        CompatibilityReportReview local = review("Example model");
        assertFalse(owner.beginPrivateCompatibility(local,false));
        assertTrue(owner.beginPrivateCompatibility(local,true)); local.close();
        assertTrue(owner.submit(owner.attempt(),"first-verification",(t,r) -> {})); executor.drain();
        assertEquals(PlaytestReportSubmission.State.RETRYABLE,owner.state());
        long old = owner.attempt(); long retry = owner.retry(); assertTrue(retry > old);
        assertFalse(owner.submit(old,"stale-verification",(t,r) -> fail()));
        assertTrue(owner.submit(retry,"second-verification",(t,r) -> {})); executor.drain();
        assertEquals(PlaytestReportSubmission.State.QUEUED,owner.state()); assertEquals(1,relay.deliveries);
        assertTrue(relay.requests.get(1).contains("\"publishToBoard\":false"));
        PlaytestReportSubmission conflict = owner(executor,relay);
        assertTrue(conflict.beginPrivateCompatibility(review("Different model"),true));
        assertTrue(conflict.submit(conflict.attempt(),"fresh-verification",(t,r) -> {})); executor.drain();
        assertEquals(PlaytestReportSubmission.State.CONFLICT,conflict.state()); assertEquals(1,relay.deliveries);
    }
    @Test public void productionAndCancellationNeverOpenNewReportTransport() {
        Executor executor = new Executor();
        PlaytestReportSubmission production = owner(executor,new PlaytestReportSubmission.HttpsTransport());
        assertTrue(production.beginPrivateCompatibility(review("Example model"),true));
        assertFalse(production.submit(production.attempt(),"fresh-verification",(t,r) -> fail())); assertTrue(executor.work.isEmpty());
        LocalRelay relay = new LocalRelay(); PlaytestReportSubmission cancelled = owner(executor,relay);
        assertTrue(cancelled.beginPrivateCompatibility(review("Example model"),true));
        assertTrue(cancelled.submit(cancelled.attempt(),"fresh-verification",(t,r) -> fail()));
        cancelled.cancel(); executor.drain(); assertTrue(relay.requests.isEmpty());
    }
    @Test public void wrongPrivateDestinationOrKindCannotClaimDelivery() {
        for (boolean wrongDestination : new boolean[]{true,false}) {
            Executor executor = new Executor(); LocalRelay relay = new LocalRelay(); relay.loseFirstReceipt = false;
            if (wrongDestination) relay.destination = "public-board"; else relay.kind = "benchmark-summary";
            PlaytestReportSubmission owner = owner(executor,relay);
            assertTrue(owner.beginPrivateCompatibility(review("Example model"),true));
            assertTrue(owner.submit(owner.attempt(),"fresh-verification",(t,r) -> {})); executor.drain();
            assertEquals(PlaytestReportSubmission.State.RETRYABLE,owner.state());
        }
    }
    @Test public void benchmarkUsesSamePrivateReceiptPathWithoutPublicConsent() {
        Executor executor = new Executor(); LocalRelay relay = new LocalRelay();
        relay.kind = "benchmark-summary"; relay.loseFirstReceipt = false;
        BenchmarkSummaryReview benchmark = new BenchmarkSummaryReview(BenchmarkSummaryReviewTest.RUN);
        assertTrue(benchmark.prepare(true,false,0,ID,UTC,(run,id,utc,c,h,k) -> BenchmarkSummaryReviewTest.prepared(run,id,h,k)));
        PlaytestReportSubmission owner = owner(executor,relay);
        assertTrue(owner.beginPrivateBenchmarkSummary(benchmark,true));
        assertTrue(owner.submit(owner.attempt(),"fresh-verification",(t,r) -> {})); executor.drain();
        assertEquals(PlaytestReportSubmission.State.QUEUED,owner.state());
    }
}
