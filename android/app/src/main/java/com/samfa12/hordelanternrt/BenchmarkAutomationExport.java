package com.samfa12.hordelanternrt;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.regex.Pattern;

/** Copies one native benchmark report into an immutable, run-id-owned export. */
public final class BenchmarkAutomationExport {
    private static final String JSON_NAME = "HordeLanternRT-benchmark-latest.json";
    private static final String TEXT_NAME = "HordeLanternRT-benchmark-latest.txt";
    private static final int MAX_JSON_BYTES = 16 * 1024 * 1024;
    private static final int MAX_TEXT_BYTES = 1 * 1024 * 1024;
    private static final Pattern RUN_ID_PATTERN = Pattern.compile("[A-Za-z0-9_-]{1,64}");

    private BenchmarkAutomationExport() {
    }

    public static final class Result {
        public final File directory;
        public final boolean successful;
        public final String detail;

        private Result(final File directory, final boolean successful, final String detail) {
            this.directory = directory;
            this.successful = successful;
            this.detail = detail;
        }
    }

    public static Result export(final File privateReports,
                                final File externalFilesRoot,
                                final String runId,
                                final int nativeStatus) throws IOException {
        validateRunId(runId);
        if (privateReports == null || externalFilesRoot == null) {
            throw new IllegalArgumentException("report and external roots are required");
        }

        final File benchmarks = new File(externalFilesRoot, "benchmarks");
        ensureDirectory(externalFilesRoot, "external files root");
        ensureDirectory(benchmarks, "benchmark export root");
        final File destination = new File(benchmarks, runId);
        if (destination.exists()) {
            throw new IOException("benchmark run directory already exists: " + runId);
        }
        if (!destination.mkdir()) {
            throw new IOException("could not create benchmark run directory: " + runId);
        }

        try {
            final File jsonFile = new File(privateReports, JSON_NAME);
            final File textFile = new File(privateReports, TEXT_NAME);
            final byte[] jsonBytes = readBounded(jsonFile, MAX_JSON_BYTES);
            final byte[] textBytes = readBounded(textFile, MAX_TEXT_BYTES);
            final String jsonText = decodeUtf8(jsonBytes, JSON_NAME);
            final String reportText = decodeUtf8(textBytes, TEXT_NAME);
            validateReport(jsonText, reportText, runId, nativeStatus);

            writeBytes(new File(destination, "benchmark.json"), jsonBytes);
            writeBytes(new File(destination, "benchmark.txt"), textBytes);
            writeResult(destination, runId, "complete", "benchmark export complete");
            return new Result(destination, true, "benchmark export complete");
        } catch (final InvalidReportException invalid) {
            writeResult(destination, runId, "invalid", invalid.getMessage());
            return new Result(destination, false, invalid.getMessage());
        } catch (final JSONException malformed) {
            final String detail = "malformed benchmark JSON: " + safeMessage(malformed);
            writeResult(destination, runId, "invalid", detail);
            return new Result(destination, false, detail);
        }
    }

    private static void validateRunId(final String runId) {
        if (runId == null || !RUN_ID_PATTERN.matcher(runId).matches()) {
            throw new IllegalArgumentException("runId must match [A-Za-z0-9_-]{1,64}");
        }
    }

    private static void ensureDirectory(final File directory, final String label)
            throws IOException {
        if (directory.exists()) {
            if (!directory.isDirectory()) {
                throw new IOException(label + " is not a directory");
            }
            return;
        }
        if (!directory.mkdirs() && !directory.isDirectory()) {
            throw new IOException("could not create " + label);
        }
    }

    private static byte[] readBounded(final File file, final int maximumBytes)
            throws IOException, InvalidReportException {
        if (!file.isFile()) {
            throw new InvalidReportException("missing benchmark report: " + file.getName());
        }
        try (FileInputStream input = new FileInputStream(file)) {
            final byte[] buffer = new byte[8192];
            byte[] result = new byte[Math.min(8192, maximumBytes + 1)];
            int length = 0;
            while (true) {
                final int read = input.read(buffer);
                if (read < 0) break;
                if (length + read > maximumBytes) {
                    throw new InvalidReportException("benchmark report exceeds size limit: " +
                            file.getName());
                }
                if (length + read > result.length) {
                    final int expanded = Math.min(maximumBytes,
                            Math.max(length + read, result.length * 2));
                    final byte[] replacement = new byte[expanded];
                    System.arraycopy(result, 0, replacement, 0, length);
                    result = replacement;
                }
                System.arraycopy(buffer, 0, result, length, read);
                length += read;
            }
            final byte[] exact = new byte[length];
            System.arraycopy(result, 0, exact, 0, length);
            return exact;
        } catch (final FileNotFoundException missing) {
            throw new InvalidReportException("missing benchmark report: " + file.getName());
        }
    }

    private static void validateReport(final String jsonText,
                                       final String reportText,
                                       final String runId,
                                       final int nativeStatus)
            throws InvalidReportException, JSONException {
        if (nativeStatus != 2) {
            throw new InvalidReportException("native benchmark status is not complete");
        }
        if (jsonText.length() == 0 || reportText.length() == 0) {
            throw new InvalidReportException("benchmark report is empty");
        }
        final JSONObject root = new JSONObject(jsonText);
        if (requiredInteger(root, "schema") != 2) {
            throw new InvalidReportException("benchmark JSON schema is not 2");
        }
        if (!runId.equals(root.optString("runId", ""))) {
            throw new InvalidReportException("benchmark JSON runId does not match requested run");
        }
        if (!"complete".equals(root.optString("result", ""))) {
            throw new InvalidReportException("benchmark JSON result is not complete");
        }
        final String marker = "Run ID: " + runId + "\n";
        if (!reportText.contains(marker)) {
            throw new InvalidReportException("benchmark text Run ID marker does not match requested run");
        }

        final JSONObject evidence = root.optJSONObject("completedFrameEvidence");
        if (evidence == null || !"complete".equals(evidence.optString("status", ""))) {
            throw new InvalidReportException("completed-frame evidence is not complete");
        }
        final JSONObject counts = evidence.optJSONObject("counts");
        final JSONArray rows = evidence.optJSONArray("rows");
        if (counts == null || rows == null) {
            throw new InvalidReportException("completed-frame evidence counts or rows are missing");
        }
        final int expected = requiredNonnegative(counts, "expected");
        final int measuredFrames = requiredNonnegative(root, "measuredFrames");
        final int completed = requiredNonnegative(counts, "completed");
        final int cpuAccepted = requiredNonnegative(counts, "cpuAccepted");
        if (expected <= 0 || expected != measuredFrames || expected != completed ||
                expected != cpuAccepted || expected != rows.length()) {
            throw new InvalidReportException("benchmark count mismatch");
        }
        if (requiredNonnegative(counts, "rejected") != 0 ||
                requiredNonnegative(counts, "cancelled") != 0 ||
                requiredNonnegative(counts, "outstanding") != 0 ||
                requiredNonnegative(counts, "cpuRejected") != 0) {
            throw new InvalidReportException("benchmark evidence contains rejected, cancelled, outstanding, or CPU-rejected rows");
        }
    }

    private static String decodeUtf8(final byte[] bytes, final String name)
            throws InvalidReportException {
        try {
            return StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(bytes))
                    .toString();
        } catch (final CharacterCodingException malformed) {
            throw new InvalidReportException("benchmark report is not valid UTF-8: " + name);
        }
    }

    private static int requiredNonnegative(final JSONObject object, final String key)
            throws InvalidReportException {
        final long value = requiredInteger(object, key);
        if (value < 0) {
            throw new InvalidReportException("benchmark field is negative: " + key);
        }
        return (int) value;
    }

    private static long requiredInteger(final JSONObject object, final String key)
            throws InvalidReportException {
        if (!object.has(key) || object.isNull(key)) {
            throw new InvalidReportException("benchmark field is missing: " + key);
        }
        final Object raw = object.opt(key);
        if (!(raw instanceof Integer) && !(raw instanceof Long)) {
            throw new InvalidReportException("benchmark field is not an integer: " + key);
        }
        final long value = ((Number) raw).longValue();
        if (value < 0 || value > Integer.MAX_VALUE) {
            throw new InvalidReportException("benchmark field is outside supported range: " + key);
        }
        return value;
    }

    private static void writeBytes(final File destination, final byte[] bytes) throws IOException {
        try (FileOutputStream output = new FileOutputStream(destination)) {
            output.write(bytes);
            output.getFD().sync();
        }
    }

    private static void writeResult(final File directory,
                                    final String runId,
                                    final String status,
                                    final String detail) throws IOException {
        final JSONObject result;
        try {
            result = new JSONObject()
                    .put("schema", 1)
                    .put("runId", runId)
                    .put("status", status)
                    .put("detail", detail);
        } catch (final JSONException impossible) {
            throw new IOException("could not construct benchmark result marker", impossible);
        }
        final File temporary = new File(directory, "result.json.tmp");
        writeBytes(temporary, result.toString().getBytes(StandardCharsets.UTF_8));
        final File target = new File(directory, "result.json");
        if (!temporary.renameTo(target)) {
            throw new IOException("could not commit benchmark result marker");
        }
    }

    private static String safeMessage(final Exception exception) {
        final String message = exception.getMessage();
        return message == null || message.isEmpty() ? "invalid JSON" : message;
    }

    private static final class InvalidReportException extends Exception {
        InvalidReportException(final String message) {
            super(message);
        }
    }
}
