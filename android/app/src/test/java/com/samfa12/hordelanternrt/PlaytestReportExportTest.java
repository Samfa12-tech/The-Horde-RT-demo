package com.samfa12.hordelanternrt;

import org.junit.Test;
import java.nio.charset.StandardCharsets;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import static org.junit.Assert.*;

public final class PlaytestReportExportTest {
    private static byte[] prepared() {
        final byte[] text = "{\"reportId\":\"opaque-123\",\"note\":\"龍 🔥\"}".getBytes(StandardCharsets.UTF_8);
        final byte[] result = new byte[text.length + 1];
        System.arraycopy(text, 0, result, 1, text.length);
        return result;
    }

    @Test public void explicitConsentAndEnvelopeRequired() {
        final PlaytestReportExport export = new PlaytestReportExport();
        assertFalse(export.begin(prepared(), false));
        assertFalse(export.begin(null, true));
        assertFalse(export.begin(new byte[] {1, 123, 125}, true));
        assertFalse(export.begin(new byte[] {0, 123, (byte)0xff, 125}, true));
        assertFalse(export.begin(new byte[PlaytestReportExport.MAX_JSON_BYTES + 2], true));
        assertEquals(PlaytestReportExport.State.DRAFT, export.state());
        assertNull(export.startWrite(export.token()));
    }

    @Test public void retryOwnsExactImmutableBytesAndPreventsDuplicate() {
        final PlaytestReportExport export = new PlaytestReportExport();
        final byte[] input = prepared();
        assertTrue(export.begin(input, true));
        assertFalse(export.begin(input, true));
        final long first = export.token();
        final byte[] bytes = export.startWrite(first);
        input[5] = 'x';
        assertNotNull(bytes);
        assertNull(export.startWrite(first));
        assertTrue(export.complete(first, false));
        assertTrue(export.retry());
        final long second = export.token();
        assertNotEquals(first, second);
        assertFalse(export.complete(first, true));
        assertArrayEquals(bytes, export.startWrite(second));
        assertTrue(export.complete(second, true));
        assertEquals(PlaytestReportExport.State.SAVED, export.state());
        assertFalse(export.retry());
    }

    @Test public void pickerCancellationRetainsOnlyAnExplicitRetry() {
        final PlaytestReportExport export = new PlaytestReportExport();
        assertTrue(export.begin(prepared(), true));
        final long first = export.token();
        assertTrue(export.pickerCancelled(first));
        assertNull(export.startWrite(first));
        assertTrue(export.retry());
        assertFalse(export.pickerCancelled(first));
        assertNotNull(export.startWrite(export.token()));
    }

    @Test public void destroyedOrCancelledOwnerRejectsLateCallbacks() {
        final PlaytestReportExport export = new PlaytestReportExport();
        assertTrue(export.begin(prepared(), true));
        final long attempt = export.token();
        assertNotNull(export.startWrite(attempt));
        export.cancel();
        assertFalse(export.canWrite(attempt));
        assertFalse(export.complete(attempt, true));
        assertNull(export.startWrite(attempt));
        assertFalse(export.retry());
    }

    @Test public void writesOnlyApprovedExactUtf8AndClosesStream() {
        final PlaytestReportExport export = new PlaytestReportExport();
        assertTrue(export.begin(prepared(), true));
        final long attempt = export.token();
        final byte[] bytes = export.startWrite(attempt);
        final boolean[] closed = {false};
        final ByteArrayOutputStream destination = new ByteArrayOutputStream() {
            @Override public void close() { closed[0] = true; }
        };
        assertTrue(PlaytestReportExport.writeApproved(export, attempt, bytes, () -> destination));
        assertArrayEquals(bytes, destination.toByteArray());
        assertTrue(closed[0]);
        export.cancel();
        assertFalse(PlaytestReportExport.writeApproved(export, attempt, bytes,
                () -> { fail("cancelled owner must not open a destination"); return destination; }));
    }

    @Test public void providerFailureAndCloseFailureNeverReportSuccessOrAutoRetry() {
        final PlaytestReportExport export = new PlaytestReportExport();
        assertTrue(export.begin(prepared(), true));
        final long attempt = export.token();
        final byte[] bytes = export.startWrite(attempt);
        final int[] opened = {0};
        assertFalse(PlaytestReportExport.writeApproved(export, attempt, bytes,
                () -> { ++opened[0]; throw new IOException("offline"); }));
        assertEquals(1, opened[0]);
        assertFalse(PlaytestReportExport.writeApproved(export, attempt, bytes, () -> null));
        assertFalse(PlaytestReportExport.writeApproved(export, attempt, bytes,
                () -> new ByteArrayOutputStream() {
                    @Override public void close() throws IOException { throw new IOException("close failed"); }
                }));
        assertTrue(export.complete(attempt, false));
        assertEquals(PlaytestReportExport.State.RETRYABLE, export.state());
        assertEquals(1, opened[0]);
    }
}
