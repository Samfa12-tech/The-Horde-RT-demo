package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class PublicBenchmarkReportFlowTest {
    static final String ID=BenchmarkSummaryReviewTest.REPORT, RUN=BenchmarkSummaryReviewTest.RUN, UTC=BenchmarkSummaryReviewTest.UTC;
    static BenchmarkSummaryReview review() {
        BenchmarkSummaryReview result=new BenchmarkSummaryReview(RUN,true);
        assertTrue(result.prepare(true,false,0,ID,UTC,(run,id,utc,c,h,k)->BenchmarkSummaryReviewTest.publicPrepared(id,h)));
        return result;
    }
    static final class Worker implements PlaytestReportSubmission.Executor {
        final List<Runnable> jobs=new ArrayList<>();
        public void execute(Runnable job){jobs.add(job);}
        void drain(){while(!jobs.isEmpty())jobs.remove(0).run();}
    }
    static final class PublicRelay implements PlaytestReportSubmission.Transport {
        final List<String> requests=new ArrayList<>();
        PlaytestReportSubmission.Response next=ack(202,"accepted","public-board","public-benchmark-result",ID);
        int privateOpens, publicOpens;
        public boolean admitsPublicBenchmarkReports(){return true;}
        public PlaytestReportSubmission.Call open(byte[] body){privateOpens++;throw new AssertionError("Public body reached legacy/private route");}
        public PlaytestReportSubmission.Call openPublicBenchmark(byte[] body){
            publicOpens++;requests.add(new String(body,StandardCharsets.UTF_8));
            return new PlaytestReportSubmission.Call(){
                public PlaytestReportSubmission.Response execute(){return next;}
                public void cancel(){Arrays.fill(body,(byte)0);}
            };
        }
    }
    static PlaytestReportSubmission owner(Worker worker,PlaytestReportSubmission.Transport transport){
        return new PlaytestReportSubmission(worker,transport,(expired,ms)->()->{},Runnable::run);
    }
    static PlaytestReportSubmission.Response ack(int code,String status,String destination,String kind,String id){
        return response(code,"{\"ok\":true,\"id\":\""+id+"\",\"status\":\""+status+
            "\",\"destination\":\""+destination+"\",\"reportKind\":\""+kind+"\"}");
    }
    static PlaytestReportSubmission.Response response(int code,String body){return new PlaytestReportSubmission.Response(code,body.getBytes(StandardCharsets.UTF_8));}
    static String frozen(String request){return request.substring(0,request.lastIndexOf(",\"turnstileToken\""))+"}";}

    @Test public void publicationRequiresSeparateConsentAndPublicTypedReviewAndPreservesLocalBytes(){
        Worker worker=new Worker();PublicRelay relay=new PublicRelay();PlaytestReportSubmission remote=owner(worker,relay);
        BenchmarkSummaryReview local=review();byte[] original=local.exactJsonBytes();
        assertFalse(remote.beginPublicBenchmarkSummary(local,false));assertFalse(remote.beginPrivateBenchmarkSummary(local,true));
        assertTrue(remote.beginPublicBenchmarkSummary(local,true));
        String submission=new String(remote.statisticsJsonForReview(),StandardCharsets.UTF_8);
        assertEquals(new String(original,StandardCharsets.UTF_8).substring(0,original.length-1)+",\"consentToPublish\":true}",submission);
        assertFalse(submission.contains("consentToSubmit"));assertFalse(submission.contains("turnstileToken"));
        assertArrayEquals(original,local.exactJsonBytes());
        byte[] copy=remote.statisticsJsonForReview();copy[0]='!';assertEquals('{',remote.statisticsJsonForReview()[0]);
        assertTrue(local.beginSave());assertArrayEquals(original,local.exportOwner().startWrite(local.exportOwner().token()));
        local.close();assertTrue(remote.submit(remote.attempt(),"public-fresh-token",(t,r)->{}));worker.drain();
        assertEquals(1,relay.publicOpens);assertEquals(0,relay.privateOpens);assertEquals(PlaytestReportSubmission.State.QUEUED,remote.state());
        BenchmarkSummaryReview privateReview=new BenchmarkSummaryReview(RUN);
        assertTrue(privateReview.prepare(true,false,0,ID,UTC,(r,id,u,c,h,k)->BenchmarkSummaryReviewTest.prepared(r,id,h,k)));
        assertFalse(owner(worker,relay).beginPublicBenchmarkSummary(privateReview,true));
    }
    @Test public void queuedPublicResultNeedsExplicitFreshVerificationAndSameBytesToCheckPublication(){
        Worker worker=new Worker();PublicRelay relay=new PublicRelay();PlaytestReportSubmission remote=owner(worker,relay);
        assertTrue(remote.beginPublicBenchmarkSummary(review(),true));
        assertTrue(remote.submit(remote.attempt(),"first-public-token",(t,r)->{}));worker.drain();
        assertEquals(PlaytestReportSubmission.State.QUEUED,remote.state());assertEquals(1,relay.requests.size());
        long stale=remote.attempt(), retry=remote.retry();assertTrue(retry>stale);
        assertFalse(remote.submit(stale,"stale-public-token",(t,r)->fail()));
        relay.next=ack(200,"published","public-board","public-benchmark-result",ID);
        assertTrue(remote.submit(retry,"second-public-token",(t,r)->{}));worker.drain();
        assertEquals(PlaytestReportSubmission.State.PUBLISHED,remote.state());
        assertEquals(frozen(relay.requests.get(0)),frozen(relay.requests.get(1)));
        assertEquals(2,relay.publicOpens);assertEquals(0,relay.privateOpens);assertEquals(-1L,remote.retry());
    }
    @Test public void untrustedSuccessReceiptsNeverBecomeQueuedOrPublished(){
        String good=new String(ack(200,"published","public-board","public-benchmark-result",ID).body,StandardCharsets.UTF_8);
        List<PlaytestReportSubmission.Response> receipts=new ArrayList<>();
        receipts.add(ack(200,"sent","public-board","public-benchmark-result",ID));
        receipts.add(ack(202,"published","public-board","public-benchmark-result",ID));
        receipts.add(ack(200,"published","developer-private","public-benchmark-result",ID));
        receipts.add(ack(200,"published","public-board","benchmark-summary",ID));
        receipts.add(ack(200,"published","public-board","public-benchmark-result",RUN));
        receipts.add(response(200,good.replace("{","{\"extra\":true,")));
        receipts.add(response(200,good.replace("{","{\"id\":\""+ID+"\",")));
        receipts.add(response(200,good.replace("{","{\"\\u0069d\":\""+ID+"\",")));
        receipts.add(response(200,good.replace(",\"destination\":\"public-board\",\"reportKind\":\"public-benchmark-result\"","")));
        receipts.add(response(200,"{\"ok\":true"));
        receipts.add(new PlaytestReportSubmission.Response(200,new byte[]{(byte)0xff}));
        for(PlaytestReportSubmission.Response receipt:receipts){
            Worker worker=new Worker();PublicRelay relay=new PublicRelay();relay.next=receipt;PlaytestReportSubmission remote=owner(worker,relay);
            assertTrue(remote.beginPublicBenchmarkSummary(review(),true));assertTrue(remote.submit(remote.attempt(),"fresh-public-token",(t,r)->{}));worker.drain();
            assertEquals(PlaytestReportSubmission.State.RETRYABLE,remote.state());assertNotNull(remote.statisticsJsonForReview());
        }
    }
    @Test public void cancellationInterruptionAndFailureKeepRetriesExplicit(){
        Worker worker=new Worker();PublicRelay relay=new PublicRelay();PlaytestReportSubmission remote=owner(worker,relay);
        assertTrue(remote.beginPublicBenchmarkSummary(review(),true));assertTrue(remote.submit(remote.attempt(),"public-token",(t,r)->fail()));
        remote.cancel();worker.drain();assertEquals(0,relay.publicOpens);assertEquals(PlaytestReportSubmission.State.CANCELLED,remote.state());
        for(int code:new int[]{403,429,503}){
            Worker w=new Worker();PublicRelay r=new PublicRelay();r.next=response(code,"");PlaytestReportSubmission o=owner(w,r);
            assertTrue(o.beginPublicBenchmarkSummary(review(),true));assertTrue(o.submit(o.attempt(),"public-fresh",(t,v)->{}));w.drain();
            assertEquals(PlaytestReportSubmission.State.RETRYABLE,o.state());assertEquals(1,r.publicOpens);
        }
        Worker w=new Worker();PublicRelay r=new PublicRelay();PlaytestReportSubmission o=owner(w,r);
        assertTrue(o.beginPublicBenchmarkSummary(review(),true));byte[] frozen=o.statisticsJsonForReview();
        assertTrue(o.submit(o.attempt(),"public-first",(t,v)->fail()));assertTrue(o.interruptInFlight());w.drain();
        assertEquals(0,r.publicOpens);assertArrayEquals(frozen,o.statisticsJsonForReview());
        long retry=o.retry();assertTrue(o.submit(retry,"public-after-resume",(t,v)->{}));w.drain();assertEquals(PlaytestReportSubmission.State.QUEUED,o.state());
    }
    @Test public void nonAsciiUnicodeEscapeDigitsCannotForgeAReceiptKey(){
        String good=new String(ack(200,"published","public-board","public-benchmark-result",ID).body,StandardCharsets.UTF_8);
        String malformed=good.replace("\"id\":","\"\\u００６９d\":"); // Invalid JSON: escape hex must be ASCII.
        Worker worker=new Worker();PublicRelay relay=new PublicRelay();relay.next=response(200,malformed);
        PlaytestReportSubmission remote=owner(worker,relay);
        assertTrue(remote.beginPublicBenchmarkSummary(review(),true));
        assertTrue(remote.submit(remote.attempt(),"public-fresh-token",(t,r)->{}));worker.drain();
        assertEquals(PlaytestReportSubmission.State.RETRYABLE,remote.state());
    }
    @Test public void productionAndPrivateQueuedCannotUsePublicReceiptRecheck(){
        Worker worker=new Worker();PlaytestReportSubmission production=owner(worker,new PlaytestReportSubmission.HttpsTransport());
        assertTrue(production.beginPublicBenchmarkSummary(review(),true));assertFalse(production.statisticsSendAvailable());
        assertFalse(production.submit(production.attempt(),"must-not-open-network",(t,r)->fail()));assertTrue(worker.jobs.isEmpty());
        DeveloperReportFlowTest.LocalRelay privateRelay=new DeveloperReportFlowTest.LocalRelay();privateRelay.loseFirstReceipt=false;
        PlaytestReportSubmission privateOwner=owner(worker,privateRelay);
        assertTrue(privateOwner.beginPrivateCompatibility(DeveloperReportFlowTest.review("Example model"),true));
        assertTrue(privateOwner.submit(privateOwner.attempt(),"private-fresh",(t,r)->{}));worker.drain();
        assertEquals(PlaytestReportSubmission.State.QUEUED,privateOwner.state());assertEquals(-1L,privateOwner.retry());
    }
    @Test public void localReviewCannotSmugglePriorRemoteConsentOrAttemptToken(){
        for(String field:new String[]{"consentToPublish","consentToSubmit","delivery","turnstileToken"}){
            BenchmarkSummaryReview local=new BenchmarkSummaryReview(RUN,true);
            assertFalse(local.prepare(true,false,0,ID,UTC,(r,id,u,c,h,k)->{
                byte[] source=BenchmarkSummaryReviewTest.publicPrepared(id,h);
                String text=new String(source,1,source.length-1,StandardCharsets.UTF_8);
                return ("\0"+text.substring(0,text.length()-1)+",\""+field+"\":true}").getBytes(StandardCharsets.UTF_8);
            }));
        }
    }
}
