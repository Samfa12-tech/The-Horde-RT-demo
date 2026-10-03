package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class BenchmarkSummaryReviewTest {
    static final String RUN = "531824c4-e551-42d8-9708-78cfd2b936ea";
    static final String REPORT = "4c417431-ff25-4928-8d5c-35ceaf7af078";
    static final String UTC = "2026-10-03T12:00:00Z";

    static byte[] prepared(String run, String report, boolean hardware, int cooling) {
        // Deliberate spacing ensures Java exports the native bytes without canonicalizing JSON.
        String json = "{\"schemaVersion\":2, \"product\":\"horde-lantern-rt\","
                + "\"reportKind\":\"benchmark-summary\",\"consentToPrepare\":true,"
                + "\"includeBasicHardware\":" + hardware + ",\"report\":{\"schemaVersion\":1,"
                + "\"reportId\":\"" + report + "\",\"runId\":\"" + run + "\","
                + "\"thermal\":{\"availability\":\"not-collected\",\"temperatureC\":null,\"status\":\"unknown\"},"
                + "\"declaredCooling\":\"" + new String[]{"unknown","none-declared","external-declared"}[cooling] + "\""
                + (hardware ? ",\"basicHardware\":{\"rawModel\":\"example\",\"gpu\":\"example\",\"vulkanApi\":\"1.3\"}" : "")
                + "}}";
        byte[] bytes = json.getBytes(StandardCharsets.UTF_8);
        byte[] envelope = new byte[bytes.length+1];
        System.arraycopy(bytes,0,envelope,1,bytes.length);
        return envelope;
    }

    @Test public void initialConsentAndCoolingAdmissionDoNotCallNative() {
        BenchmarkSummaryReview owner = new BenchmarkSummaryReview(RUN);
        final int[] calls = {0};
        BenchmarkSummaryReview.Preparer nativeCall = (run,id,time,consent,hardware,cooling) -> {
            ++calls[0]; return prepared(run,id,hardware,cooling);
        };
        assertFalse(owner.prepare(false,false,0,REPORT,UTC,nativeCall));
        assertFalse(owner.prepare(true,false,3,REPORT,UTC,nativeCall));
        assertFalse(owner.prepare(true,false,0,"not-a-uuid",UTC,nativeCall));
        for (String invalid : new String[]{
                "531824c4-e551-12d8-9708-78cfd2b936ea", // Version 1.
                "00000000-0000-0000-0000-000000000000", // Nil.
                "531824c4-e551-42d8-1708-78cfd2b936ea", // Non-IETF variant.
                RUN.toUpperCase(java.util.Locale.ROOT)}) {
            assertFalse(BenchmarkSummaryReview.validUuid(invalid));
            assertFalse(owner.prepare(true,false,0,invalid,UTC,nativeCall));
            try {
                new BenchmarkSummaryReview(invalid);
                fail("Invalid run identity admitted");
            } catch (IllegalArgumentException expected) { }
        }
        assertTrue(BenchmarkSummaryReview.validUuid(RUN));
        assertEquals(0,calls[0]);
        assertNull(owner.exactJsonBytes());
        assertFalse(BenchmarkSummaryReview.validUuid(""));
    }

    @Test public void exactBytesAndIdentitiesSurviveFailedSaveAndExplicitRetry() {
        BenchmarkSummaryReview owner = new BenchmarkSummaryReview(RUN);
        byte[] source = prepared(RUN,REPORT,false,0);
        final int[] calls = {0};
        assertTrue(owner.prepare(true,false,0,REPORT,UTC,(run,id,time,consent,hardware,cooling) -> {
            ++calls[0]; return source;
        }));
        byte[] expected = Arrays.copyOfRange(source,1,source.length);
        source[1] = 'x'; // Caller mutation cannot alter reviewed bytes.
        byte[] copy = owner.exactJsonBytes(); copy[0] = 'x';
        assertArrayEquals(expected,owner.exactJsonBytes());
        assertFalse(owner.json().contains("basicHardware"));
        assertTrue(owner.json().contains("\"status\":\"unknown\""));
        assertTrue(owner.beginSave());
        PlaytestReportExport export = owner.exportOwner();
        long first = export.token();
        assertArrayEquals(expected,export.startWrite(first));
        assertTrue(export.complete(first,false));
        assertTrue(owner.beginSave());
        long second = export.token();
        assertTrue(second>first);
        byte[] bytes = export.startWrite(second);
        ByteArrayOutputStream destination = new ByteArrayOutputStream();
        assertTrue(PlaytestReportExport.writeApproved(export,second,bytes,() -> destination));
        assertArrayEquals(expected,destination.toByteArray());
        assertEquals(REPORT,owner.reportId());
        assertEquals(RUN,owner.runId());
        assertEquals(1,calls[0]);
    }

    @Test public void rejectsStaleIdentityHardwareMismatchSchemaAndInvalidUtf8() {
        byte[][] invalid = {
            prepared(REPORT,REPORT,false,0), prepared(RUN,RUN,false,0), prepared(RUN,REPORT,true,0),
            new byte[]{0,'{',(byte)0xc3,(byte)0x28,'}'}, new byte[]{2,'{','}'},
            new byte[PlaytestReportExport.MAX_JSON_BYTES+2],
            ("\0{\"schemaVersion\":1,\"product\":\"horde-lantern-rt\"}").getBytes(StandardCharsets.UTF_8)
        };
        for (byte[] payload : invalid) {
            BenchmarkSummaryReview owner = new BenchmarkSummaryReview(RUN);
            assertFalse(owner.prepare(true,false,0,REPORT,UTC,(run,id,time,consent,hardware,cooling) -> payload));
            assertFalse(owner.beginSave());
        }
    }

    @Test public void hardwareAndCoolingAreIndependentExplicitApprovalFields() {
        BenchmarkSummaryReview owner = new BenchmarkSummaryReview(RUN);
        assertTrue(owner.prepare(true,true,2,REPORT,UTC,(run,id,time,consent,hardware,cooling) -> {
            assertTrue(consent); assertTrue(hardware); assertEquals(2,cooling);
            assertEquals(RUN,run); assertEquals(REPORT,id); assertEquals(UTC,time);
            return prepared(run,id,hardware,cooling);
        }));
        assertTrue(owner.json().contains("basicHardware"));
        assertTrue(owner.json().contains("external-declared"));
        assertTrue(owner.json().contains("\"temperatureC\":null"));
        assertFalse(owner.prepare(true,false,0,RUN,UTC,(run,id,time,c,h,k) -> prepared(run,id,h,k)));
    }

    @Test public void closeCancelsUnstartedDestinationAndClearsPrivatePayload() {
        BenchmarkSummaryReview owner = new BenchmarkSummaryReview(RUN);
        assertTrue(owner.prepare(true,false,0,REPORT,UTC,(run,id,time,c,h,k) -> prepared(run,id,h,k)));
        assertTrue(owner.beginSave());
        PlaytestReportExport export = owner.exportOwner();
        long token = export.token();
        byte[] bytes = export.startWrite(token);
        owner.close();
        final boolean[] opened = {false};
        assertFalse(PlaytestReportExport.writeApproved(export,token,bytes,() -> {
            opened[0] = true; return new ByteArrayOutputStream();
        }));
        assertFalse(opened[0]);
        assertNull(owner.json()); assertNull(owner.exactJsonBytes());
        assertFalse(owner.beginSave());
    }
}
