package com.samfa12.hordelanternrt;

import java.nio.ByteBuffer;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.UUID;
import org.json.JSONObject;

/** Memory-only owner of one immutable run and its explicitly prepared local report. */
final class BenchmarkSummaryReview {
    interface Preparer {
        byte[] prepare(String runId, String reportId, String utc, boolean consent,
                boolean hardware, int cooling);
    }

    private final String runId;
    private byte[] envelope;
    private String json, reportId;
    private PlaytestReportExport export;
    private boolean closed;

    BenchmarkSummaryReview(String runId) {
        if (!validUuid(runId)) throw new IllegalArgumentException("Invalid benchmark identity");
        this.runId = runId;
    }

    static boolean validUuid(String value) {
        if (value == null || value.length() != 36) return false;
        try {
            final UUID parsed = UUID.fromString(value);
            return parsed.toString().equals(value) && parsed.version() == 4 && parsed.variant() == 2;
        }
        catch (IllegalArgumentException invalid) { return false; }
    }

    String runId() { return runId; }
    String reportId() { return reportId; }
    String json() { return json; }
    boolean isPrepared() { return !closed && envelope != null; }
    PlaytestReportExport exportOwner() { return export; }

    boolean prepare(boolean consent, boolean hardware, int cooling, String id, String utc,
            Preparer preparer) {
        if (closed || isPrepared() || !consent || cooling < 0 || cooling > 2 || !validUuid(id)) return false;
        try {
            final byte[] prepared = preparer.prepare(runId, id, utc, true, hardware, cooling);
            if (prepared == null || prepared.length < 3 || prepared[0] != 0 ||
                    prepared.length > PlaytestReportExport.MAX_JSON_BYTES + 1) return false;
            final String text = StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(prepared, 1, prepared.length - 1)).toString();
            // Validate bridge identity only. Native typed capture/builder owns populations and scopes.
            // Parsing never replaces the exact native bytes reviewed, copied and exported below.
            final JSONObject root = new JSONObject(text);
            final JSONObject report = root.getJSONObject("report");
            if (root.getInt("schemaVersion") != 2 ||
                    !"horde-lantern-rt".equals(root.getString("product")) ||
                    !"benchmark-summary".equals(root.getString("reportKind")) ||
                    !root.getBoolean("consentToPrepare") ||
                    root.getBoolean("includeBasicHardware") != hardware ||
                    report.getInt("schemaVersion") != 1 ||
                    !id.equals(report.getString("reportId")) ||
                    !runId.equals(report.getString("runId")) ||
                    (!hardware && report.has("basicHardware"))) return false;
            envelope = prepared.clone();
            json = text;
            reportId = id;
            return true;
        } catch (Exception | LinkageError invalid) { return false; }
    }

    boolean beginSave() {
        if (!isPrepared()) return false;
        if (export == null) {
            export = new PlaytestReportExport();
            return export.begin(envelope, true);
        }
        return export.state() == PlaytestReportExport.State.RETRYABLE && export.retry();
    }

    byte[] exactJsonBytes() {
        return isPrepared() ? Arrays.copyOfRange(envelope, 1, envelope.length) : null;
    }

    void close() {
        closed = true;
        if (export != null) export.cancel();
        envelope = null;
        json = null;
        reportId = null;
    }
}
