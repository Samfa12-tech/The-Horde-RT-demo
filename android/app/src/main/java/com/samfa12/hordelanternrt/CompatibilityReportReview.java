package com.samfa12.hordelanternrt;

import java.nio.ByteBuffer;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import org.json.JSONObject;

/** One startup attempt's native-approved, renderer-independent local review. */
final class CompatibilityReportReview {
    static final int MAX_JSON_BYTES = 8 * 1024;
    interface Preparer { byte[] prepare(long attempt, String id, String utc, boolean localConsent); }
    private final long startupAttempt;
    private byte[] envelope;
    private String id, json;
    private PlaytestReportExport export;
    private boolean closed;
    CompatibilityReportReview(long attempt) {
        if (attempt <= 0) throw new IllegalArgumentException("startup attempt required");
        startupAttempt = attempt;
    }
    boolean isPrepared() { return !closed && envelope != null; }
    String reportId() { return id; }
    String json() { return json; }
    PlaytestReportExport exportOwner() { return export; }
    boolean prepare(long currentAttempt, boolean localConsent, String reportId, String utc, Preparer nativeOwner) {
        if (closed || isPrepared() || currentAttempt != startupAttempt || !localConsent ||
                !BenchmarkSummaryReview.validUuid(reportId) || nativeOwner == null) return false;
        try {
            byte[] prepared = nativeOwner.prepare(startupAttempt, reportId, utc, true);
            if (prepared == null || prepared.length < 3 || prepared[0] != 0 || prepared.length > MAX_JSON_BYTES + 1) return false;
            String text = StandardCharsets.UTF_8.newDecoder().onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(prepared, 1, prepared.length - 1)).toString();
            JSONObject root = new JSONObject(text), report = root.getJSONObject("report");
            // Native builder owns the allowlist/evidence. Never reserialize it.
            if (root.getInt("schemaVersion") != 2 || !"horde-lantern-rt".equals(root.getString("product")) ||
                    !"compatibility".equals(root.getString("reportKind")) || !root.getBoolean("consentToPrepare") ||
                    root.has("consentToSubmit") || root.has("delivery") || root.has("turnstileToken") ||
                    report.getInt("schemaVersion") != 1 || !reportId.equals(report.getString("reportId")) ||
                    !utc.equals(report.getString("capturedAtUtc"))) return false;
            envelope = prepared.clone(); id = reportId; json = text;
            return true;
        } catch (Exception | LinkageError unavailable) { return false; }
    }
    byte[] exactJsonBytes() { return isPrepared() ? Arrays.copyOfRange(envelope, 1, envelope.length) : null; }
    boolean beginSave() {
        if (!isPrepared()) return false;
        if (export == null) { export = new PlaytestReportExport(); return export.begin(envelope, true); }
        return export.state() == PlaytestReportExport.State.RETRYABLE && export.retry();
    }
    void close() {
        closed = true;
        if (export != null) export.cancel();
        if (envelope != null) Arrays.fill(envelope, (byte)0);
        envelope = null; id = json = null;
    }
}
